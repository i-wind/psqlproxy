/**
 * sqlproxy.cpp
 */
#include "server.hpp"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/info_parser.hpp>


namespace pt = boost::property_tree;
namespace fs = boost::filesystem;
namespace po = boost::program_options;
namespace ip = boost::asio::ip;

using boost::asio::ip::tcp;

namespace sqlproxy {

/// Initialization function
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
}  // void init_all(int argc, char **argv)
}  // namespace sqlproxy


/// Entry point
int main(int argc, char **argv) {
    try {
        sqlproxy::init_all(argc, argv);
    } catch (po::required_option &err) {
        std::cerr << "Exception: " << err.what() << "\n";
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
