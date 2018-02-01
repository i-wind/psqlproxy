/**
 * server.hpp
 */

#pragma once

#include "logger.hpp"
#include <boost/any.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/asio.hpp>
#include <cstdlib>
#include <string>
#include <iostream>
#include <memory>
#include <utility>


namespace ip = boost::asio::ip;

using boost::asio::ip::tcp;


namespace sqlproxy {

/// Class to represent client <-> server session
class session
    : public std::enable_shared_from_this<session>
{
public:
    /// Pointer type to session class
    typedef std::shared_ptr<session> ptr_type;

    /// Session constructor
    session(boost::asio::io_service& ios, const std::string& log_name)
        : client_socket_(ios)
        , server_socket_(ios)
        , logger_(ios, "SQL")
    {
        // Set the name of the file that all logger instances will use.
        logger_.use_file(log_name);
    }

    /// Client socket property
    tcp::socket& client_socket() {
        return client_socket_;
    }

    void start(const std::string& server_host,
               uint16_t server_port) {
        auto self = shared_from_this();
        // Connect to PostgreSQL server
        server_socket_.async_connect(
            tcp::endpoint(
                ip::address::from_string(server_host),
                server_port),
            [this, self](const boost::system::error_code& ec) {
                if (!ec) {
                    // std::cerr << "Connection to PostgreSQL\n";
                    on_server_connect();
                }
                else {
                    on_error(ec, "Connect");
                }
            });
    }

private:
    /// PostgreSQL server connected
    void on_server_connect() {
        auto self(shared_from_this());
        // Async read from PostgreSQL server
        server_socket_.async_read_some(
            boost::asio::buffer(server_data_, max_length),
            [this, self](const boost::system::error_code& ec, std::size_t length) {
                if (!ec) {
                    on_server_read(length);
                }
                else {
                    on_error(ec, "Server read");
                }
            });

        // Async read from client
        client_socket_.async_read_some(
            boost::asio::buffer(client_data_, max_length),
            [this, self](const boost::system::error_code& ec, std::size_t length) {
                if (!ec) {
                    on_client_read(length);
                }
                else {
                    on_error(ec, "Client read");
                }
            });
    }

    /// Send data from PostgreSQL server to client
    void on_server_read(std::size_t length) {
        auto self(shared_from_this());
        boost::asio::async_write(
            client_socket_,
            boost::asio::buffer(server_data_, length),
            [this, self](const boost::system::error_code& ec, std::size_t /*length*/) {
                if (!ec) {
                    on_client_write();
                }
                else {
                    on_error(ec, "Client write");
                }
            });
    }

    /// Send data to PostgreSQL server
    void on_client_read(const std::size_t& length) {
        if (client_data_[0] == 'Q') {
            // Log sql request
            // uint64_t p = (client_data_[1] << 24) +
            //              (client_data_[2] << 16) +
            //              (client_data_[3] << 8) +
            //              client_data_[4];
            // std::cout << p << "] ";
            std::string command(&client_data_[5], length - 6);
            logger_.log(command);
        }
        // else {
        //     // Log connection user and database
        //     std::string value(&client_data_[0], length);
        //     std::size_t pos = value.find("user");
        //     if (pos != std::string::npos) {
        //         std::size_t pos1 = value.find("database");
        //         if (pos1 != std::string::npos) {
        //             std::string str = format::fmt(
        //                 "Connection user: %s, database: %s",
        //                 value.substr(pos + 5, pos1 - pos - 6),
        //                 value.substr(pos1 + 9, value.size() - pos1 - 11));
        //             logger_.log(str);
        //         }
        //     }
        // }

        auto self(shared_from_this());
        boost::asio::async_write(
            server_socket_,
            boost::asio::buffer(client_data_, length),
            [this, self](const boost::system::error_code& ec, std::size_t /*length*/) {
                if (!ec) {
                    on_server_write();
                }
                else {
                    on_error(ec, "Server write");
                }
            });
    }

    /// Async read from PostgreSQL server
    void on_client_write() {
        auto self(shared_from_this());
        server_socket_.async_read_some(
            boost::asio::buffer(server_data_, max_length),
            [this, self](const boost::system::error_code& ec, std::size_t length) {
                if (!ec) {
                    on_server_read(length);
                }
                else {
                    on_error(ec, "Server read");
                }
            });
    }

    /// Async read from client
    void on_server_write() {
        auto self(shared_from_this());
        client_socket_.async_read_some(
            boost::asio::buffer(client_data_, max_length),
            [this, self](const boost::system::error_code& ec, std::size_t length) {
                if (!ec) {
                    on_client_read(length);
                }
                else {
                    on_error(ec, "Client read");
                }
            });
    }

    /// Close client and server sockets
    void close() {
        boost::mutex::scoped_lock lock(mutex_);

        if (client_socket_.is_open()) {
            client_socket_.close();
        }

        if (server_socket_.is_open()) {
            server_socket_.close();
        }
    }

    /// Log error and close sockets
    void on_error(const boost::system::error_code& ec,
                  const std::string& prefix) {
        auto code = boost::any_cast<int>(ec.value());
        if (!((code == 2) || (code == 125))) {
            logger_.log(format::fmt(
                "%s error: [%d] %s", prefix, code, ec.message()));
        }
        close();
    }

    tcp::socket client_socket_;
    tcp::socket server_socket_;
    services::logger logger_;

    enum { max_length = 4096 };
    std::array<char, max_length> client_data_;
    std::array<char, max_length> server_data_;

    boost::mutex mutex_;
};  // class session

/// Class to accept connections
class server
{
public:
    server(boost::asio::io_service& io_service,
           const std::string& local_host, uint16_t local_port,
           const std::string& server_host, uint16_t server_port,
           const std::string& log_name)
        : io_service_(io_service)
        , localhost_address_(
              boost::asio::ip::address_v4::from_string(local_host))
        , acceptor_(io_service_,
                    tcp::endpoint(localhost_address_, local_port))
        , server_port_(server_port)
        , server_host_(server_host)
        , log_name_(log_name)
    {}

    /// Accept client connections
    bool accept_connections() {
        try {
            // Create session with client/server sockets
            session_ = std::make_shared<session>(io_service_, log_name_);

            acceptor_.async_accept(
                session_->client_socket(),
                boost::bind(&server::on_accept,
                            this,
                            boost::asio::placeholders::error));
        }
        catch (std::exception& e) {
            std::cerr << "Server exception: " << e.what() << "\n";
            return false;
        }

        return true;
    }

private:
    /// On accept connection
    void on_accept(const boost::system::error_code& ec) {
        if (!ec) {
            // Start session
            session_->start(server_host_, server_port_);

            if (!accept_connections()) {
                std::cerr << "Error on call to accept connections.\n";
            }
        }
        else {
            std::cerr << "Error: [" << ec.value() << "] " << ec.message() << "\n";
        }
    }

    boost::asio::io_service& io_service_;
    ip::address_v4 localhost_address_;
    tcp::acceptor acceptor_;
    session::ptr_type session_;
    uint16_t server_port_;
    std::string server_host_;
    std::string log_name_;
};  // class server

}  // namespace sqlproxy
