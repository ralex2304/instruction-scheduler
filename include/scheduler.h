#pragma once

#include "configs_parser.h"
#include "data_flow_graph.h"

#include <filesystem>

namespace scheduler {

class Scheduler {
public:
    Scheduler(std::filesystem::path units_config_path,
              std::filesystem::path instr_config_path,
              std::filesystem::path input_path,
              std::filesystem::path output_path);
private:
    Config config_;
    DataFlowGraph data_flow_graph_;
};

} //< namespace scheduler
