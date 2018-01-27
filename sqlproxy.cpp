#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>


namespace {
    namespace fs = boost::filesystem;
    namespace po = boost::program_options;

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
    }
}


int main(int argc, char **argv) {
    try {
        init_all(argc, argv);
    } catch (po::required_option &err) {
        std::cerr << "  error: " << err.what() << "\n";
    } catch (std::exception &e) {
        std::cerr << "  error:" << e.what() << "\n";
    }
}
