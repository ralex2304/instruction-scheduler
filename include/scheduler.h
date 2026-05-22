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

    const auto& get_scheduled_instructions() const { return scheduled_instructions_; }
    ticks_t get_end_time() const { return end_time_; }

private:
    struct ScheduledInstruction {
        ticks_t time;
        size_t unit_index;
        std::string line;
    };

    void schedule(DataFlowGraph* dfg, std::optional<std::filesystem::path> dump_dir);

    Config config_;
    std::vector<ScheduledInstruction> scheduled_instructions_;
    ticks_t end_time_;
};

} //< namespace scheduler
