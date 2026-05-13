#include "scheduler.h"

#include <cxxopts.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

using namespace scheduler;

int main(int argc, const char* argv[]) {
    cxxopts::Options options("instruction-scheduler",
        "Data-dependecies, latency and resource aware instructions scheduler");

    options.add_options()
        ("i,input", "Input file with instructions", cxxopts::value<std::filesystem::path>())
        ("o,output", "Output file with scheduled instructions", cxxopts::value<std::filesystem::path>())
        ("u,units", "Config file with execution units description",
                    cxxopts::value<std::filesystem::path>()->default_value("assets/units.toml"))
        ("x,instructions", "Config file with instructions",
                    cxxopts::value<std::filesystem::path>()->default_value("assets/instructions.toml"))
        ("d,dump_dir", "Directory for graphs dumps",
                    cxxopts::value<std::filesystem::path>()->default_value("dumps/"))
        ("h,help", "Print help")
    ;

    options.positional_help("<output>");

    options.parse_positional("output");
    options.show_positional_help();
    auto opt_result = options.parse(argc, argv);

    bool invalid_args = false;

    if (opt_result.count("input") == 0 || opt_result.count("output") == 0) {
        std::cout << "Not enough arguments. Check help:" << std::endl;
        invalid_args = true;
    }

    if (opt_result.count("help") || invalid_args) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    const auto& dump_dir = opt_result["dump_dir"].as<std::filesystem::path>();
    std::filesystem::create_directory(dump_dir);

    try {
        Scheduler scheduler(opt_result["units"].as<std::filesystem::path>(),
                            opt_result["instructions"].as<std::filesystem::path>(),
                            opt_result["input"].as<std::filesystem::path>(),
                            opt_result["output"].as<std::filesystem::path>());

    } catch (const std::ifstream::failure &e) {
        std::cerr << "Input file read error: " << e.what() << std::endl;
        return 1;
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;
}

