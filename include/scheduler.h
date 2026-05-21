#pragma once

#include "configs_parser.h"
#include "data_flow_graph.h"
#include "types.h"

#include <filesystem>
#include <optional>

namespace scheduler {

class Scheduler {
public:
    Scheduler(std::filesystem::path units_config_path,
              std::filesystem::path instr_config_path,
              std::filesystem::path input_path,
              std::optional<std::filesystem::path> dump_dir);

    void write_scheduled_instructions(std::filesystem::path path);

private:
    struct ScheduledInstruction {
        ticks_t time;
        const std::string& unit;
        std::string line;
    };

    void schedule(DataFlowGraph* dfg, std::optional<std::filesystem::path> dump_dir);

    Config config_;
    std::vector<ScheduledInstruction> scheduled_instructions_;
    ticks_t end_time_;
};

} //< namespace scheduler
