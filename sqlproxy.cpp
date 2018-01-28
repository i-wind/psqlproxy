#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>
#include <cstdlib>
#include <string>
#include <iostream>
#include <memory>
#include <utility>


namespace pt = boost::property_tree;
namespace fs = boost::filesystem;
namespace po = boost::program_options;
namespace ip = boost::asio::ip;

using boost::asio::ip::tcp;


namespace sqlproxy {

    class session
        : public boost::enable_shared_from_this<session>
    {
    public:
        typedef boost::shared_ptr<session> ptr_type;

        session(boost::asio::io_service& ios)
            : client_socket_(ios),
              server_socket_(ios)
        {}

        tcp::socket& client_socket() {
            return client_socket_;
        }

        void start(const std::string& server_host,
                   uint16_t server_port) {
            // Attempt to connect to PostgreSQL server
            server_socket_.async_connect(
                tcp::endpoint(
                    ip::address::from_string(server_host),
                    server_port),
                boost::bind(
                    &session::on_server_connect,
                    shared_from_this(),
                    boost::asio::placeholders::error));
        }

    private:
        // Server connected
        void on_server_connect(
                const boost::system::error_code& ec) {
            if (!ec) {
                auto self(shared_from_this());
                // Async read from PostgreSQL server
                server_socket_.async_read_some(
                    boost::asio::buffer(server_data_, max_length),
                    boost::bind(
                        &session::on_server_read,
                        self,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));

                // Async read from client
                client_socket_.async_read_some(
                    boost::asio::buffer(client_data_, max_length),
                    boost::bind(
                        &session::on_client_read,
                        self,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
            }
            else {
                close();
            }
        }

        // Send data from PostgreSQL server to client
        void on_server_read(const boost::system::error_code& ec,
                          const size_t& bytes_transferred) {
            if (!ec) {
                async_write(
                    client_socket_,
                    boost::asio::buffer(server_data_, bytes_transferred),
                    boost::bind(&session::on_client_write,
                                shared_from_this(),
                                boost::asio::placeholders::error));
            }
            else {
                close();
            }
        }

        // Async read from PostgreSQL server
        void on_client_write(const boost::system::error_code& error) {
            if (!error) {
                server_socket_.async_read_some(
                    boost::asio::buffer(server_data_, max_length),
                    boost::bind(
                        &session::on_server_read,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
            }
            else {
                close();
            }
        }

        // Send data to PostgreSQL server
        void on_client_read(const boost::system::error_code& ec,
                            const size_t& bytes_transferred) {
            if (!ec) {
                if (client_data_[0] == 'Q') {
                    std::cout << client_data_[0] << " ";
                    uint64_t p = (client_data_[1] << 24) +
                                 (client_data_[2] << 16) +
                                 (client_data_[3] << 8) +
                                 client_data_[4];
                    std::cout << p << " ";
                    std::string command(
                        // const_cast<const char*>(&client_data_[5]),
                        &client_data_[5],
                        bytes_transferred - 5);
                    std::cout << command << "\n";
                }

                async_write(
                    server_socket_,
                    boost::asio::buffer(client_data_, bytes_transferred),
                    boost::bind(&session::on_server_write,
                                shared_from_this(),
                                boost::asio::placeholders::error));
            }
            else {
                close();
            }
        }

        // Async read from client
        void on_server_write(const boost::system::error_code& ec) {
            if (!ec) {
                client_socket_.async_read_some(
                    boost::asio::buffer(client_data_, max_length),
                    boost::bind(
                        &session::on_client_read,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
            }
            else {
                close();
            }
        }

        // Close client and server sockets
        void close() {
            boost::mutex::scoped_lock lock(mutex_);

            if (client_socket_.is_open()) {
                client_socket_.close();
            }

            if (server_socket_.is_open()) {
                server_socket_.close();
            }
        }

        tcp::socket client_socket_;
        tcp::socket server_socket_;

        enum { max_length = 4096 };
        std::array<char, max_length> client_data_;
        std::array<char, max_length> server_data_;

        boost::mutex mutex_;
    };  // class session

    class server
    {
    public:
        server(boost::asio::io_service& io_service,
               const std::string& local_host, uint16_t local_port,
               const std::string& server_host, uint16_t server_port)
            : io_service_(io_service),
              localhost_address_(
                  boost::asio::ip::address_v4::from_string(local_host)),
              acceptor_(io_service_,
                        tcp::endpoint(localhost_address_, local_port)),
              server_port_(server_port),
              server_host_(server_host)
        {}

        bool accept_connections() {
            try {
                session_ = boost::make_shared<session>(io_service_);

                acceptor_.async_accept(
                    session_->client_socket(),
                    boost::bind(&server::on_accept,
                                this,
                                boost::asio::placeholders::error));
            }
            catch(std::exception& e) {
                std::cerr << "Server exception: " << e.what() << "\n";
                return false;
            }

            return true;
        }

    private:
        // On accept connection
        void on_accept(const boost::system::error_code& ec) {
            if (!ec) {
                session_->start(server_host_, server_port_);

                if (!accept_connections()) {
                    std::cerr << "Failure during call to accept.\n";
                }
            }
            else {
                std::cerr << "Error: " << ec.message() << "\n";
            }
        }

        boost::asio::io_service& io_service_;
        ip::address_v4 localhost_address_;
        tcp::acceptor acceptor_;
        session::ptr_type session_;
        uint16_t server_port_;
        std::string server_host_;
    };  // class server

    /**
     * Initialization
     */
    void init_all(int argc, char **argv) {
        po::options_description desc("SQLProxy command line options");
        std::string config;
        desc.add_options()
            ("help,h", "show help message")
            ("config,c", po::value<std::string>(&config)->required(), "path to configuration")
        ;

        auto params = po::command_line_parser(argc, argv).options(desc).run();
        po::variables_map vm;
        po::store(params, vm);

        if (vm.count("help")) {
            std::cerr << desc << "\n";
            exit(1);
        }

        po::notify(vm);

        // std::cout << "configuration: '" << config << "'\n";
        if (!fs::exists(config)) {
            throw std::runtime_error(config + " does not exist...");
        }

        pt::ptree proxy_config;
        std::ifstream in(config);
        pt::info_parser::read_info(in, proxy_config);
        in.close();

        std::string local_ip = proxy_config.get<std::string>("SQLProxy.proxy_ip");
        uint16_t local_port = proxy_config.get<int>("SQLProxy.proxy_port");
        std::string server_ip = proxy_config.get<std::string>("SQLProxy.postgresql_ip");
        uint16_t server_port = proxy_config.get<int>("SQLProxy.postgresql_port");

        std::cout << "local: " << local_ip << ":" << local_port << ", "
                  << "remote: " <<  server_ip << ":" << server_port << "\n";

        boost::asio::io_service io_service;
        sqlproxy::server server(
            io_service,
            local_ip, local_port,
            server_ip, server_port);

        server.accept_connections();
        io_service.run();
    }
}  // namespace sqlproxy


int main(int argc, char **argv) {
    try {
        sqlproxy::init_all(argc, argv);
    } catch (po::required_option &err) {
        std::cerr << "Exception: " << err.what() << "\n";
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
