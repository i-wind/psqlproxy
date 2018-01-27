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

using boost::asio::ip::tcp;

namespace sqlproxy {

    class session
        : public std::enable_shared_from_this<session>
    {
    public:
        session(tcp::socket socket)
            : socket_(std::move(socket))
        {}

        void start() {
            do_read();
        }

    private:
        void do_read() {
            auto self(shared_from_this());
            socket_.async_read_some(
                boost::asio::buffer(data_, max_length),
                [this, self](boost::system::error_code ec, std::size_t length) {
                    if (!ec) {
                        std::cout << data_ << "\n";
                        do_write(length);
                    }
                });
        }

        void do_write(std::size_t length) {
            auto self(shared_from_this());
            boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
                [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                    if (!ec) {
                        do_read();
                    }
                });
        }

        tcp::socket socket_;
        enum { max_length = 1024 };
        char data_[max_length];
    };

    class server
    {
    public:
        server(boost::asio::io_service& io_service, short port)
            : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
              socket_(io_service)
        {
            do_accept();
        }

    private:
        void do_accept() {
            acceptor_.async_accept(socket_,
                [this](boost::system::error_code ec) {
                    if (!ec) {
                        std::make_shared<session>(std::move(socket_))->start();
                    }

                    do_accept();
                });
        }

        tcp::acceptor acceptor_;
        tcp::socket socket_;
    };

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

        server s(io_service, local_port);
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
