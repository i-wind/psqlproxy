#include <iostream>
#include <boost/program_options.hpp>

namespace {
    namespace po = boost::program_options;

    void init_all(int argc, char **argv) {
        po::options_description desc("SQLProxy command line options");
        desc.add_options()
            ("help,h", "show help message")
            // ("config,c", po::value<std::string>(&config)->required(), "path to configuration")
            ("config,c", "path to configuration")
        ;

        auto options = po::command_line_parser(argc, argv).options(desc).run();
        po::variables_map vm;
        po::store(options, vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cerr << desc << "\n";
            exit(1);
        }

        if (!vm.count("config")) {
            std::cerr << "  error: the option '--config' is required but missing\n";
            exit(1);
        }

        std::string config = vm["config"].as<std::string>();
        std::cout << "configuration: '" << config << "'\n";
    }
}

int main(int argc, char **argv) {
    init_all(argc, argv);
}
