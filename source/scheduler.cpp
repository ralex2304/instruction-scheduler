#include "scheduler.h"

#include <filesystem>
#include <format>

using namespace scheduler;

Scheduler::Scheduler(std::filesystem::path units_config_path,
                     std::filesystem::path instr_config_path,
                     std::filesystem::path input_path,
                     std::filesystem::path output_path)
    try : config_(units_config_path, instr_config_path)
        , data_flow_graph_(input_path, config_, true) {
    } catch (const std::ifstream::failure &e) {
        throw std::runtime_error(std::format("Input file read error: {}", e.what()));
    }

