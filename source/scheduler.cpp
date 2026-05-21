#include "scheduler.h"
#include "configs_parser.h"
#include "data_flow_graph.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <optional>
#include <ostream>
#include <set>
#include <string_view>

using namespace scheduler;

Scheduler::Scheduler(std::filesystem::path units_config_path,
                     std::filesystem::path instr_config_path,
                     std::filesystem::path input_path,
                     std::optional<std::filesystem::path> dump_dir)
    try : config_(units_config_path, instr_config_path) {

        DataFlowGraph data_flow_graph(input_path, config_, true);

        schedule(&data_flow_graph, dump_dir);

    } catch (const std::ifstream::failure& e) {
        throw std::runtime_error(std::format("Input file read error: {}", e.what()));
    }

void Scheduler::schedule(DataFlowGraph* dfg, std::optional<std::filesystem::path> dump_dir) {
    assert(dfg);

    std::vector<DFGNode*> ready;
    std::vector<DFGNode*> partial_ready;

    ticks_t current_time = 0;

    dfg->start_node()->schedule(current_time);
    for (const auto& dependency: *dfg->start_node())
        ready.push_back(dependency);

    while (!dfg->end_node()->is_scheduled()) {
        std::vector<UnitConfig> units(config_.get_units());

        std::set<DFGNode*> ready_dependencies;

        std::erase_if(ready, [this, &units, &ready_dependencies, current_time](DFGNode* node) {
            bool scheduled = false;
            if (node->instr_config.latency == 0) {
                scheduled = true;
            } else {
                for (const auto& unit_index: node->instr_config.units_indexes) {
                    if (units[unit_index].quantity > 0) {
                        units[unit_index].quantity--;
                        scheduled_instructions_.push_back({
                            .time = current_time,
                            .unit_index = unit_index,
                            .line = node->line
                        });

                        scheduled = true;
                        break;
                    }
                }
            }

            if (scheduled) {
                node->schedule(current_time);
                for (const auto& dependency: *node) {
                    if (dependency->is_ready())
                        ready_dependencies.insert(dependency);
                }
                return true;
            }
            return false;
        });

        current_time++;

        auto partial_ready_to_ready_move_condition = [current_time](DFGNode* node) {
                return node->get_early_time() <= current_time;
            };
        std::copy_if(std::make_move_iterator(partial_ready.begin()),
                     std::make_move_iterator(partial_ready.end()),
                     std::back_inserter(ready),
                     partial_ready_to_ready_move_condition);
        std::erase_if(partial_ready, partial_ready_to_ready_move_condition);

        for (const auto& dependency: ready_dependencies) {
            if (dependency->get_early_time() <= current_time)
                ready.push_back(dependency);
            else
                partial_ready.push_back(dependency);
        }

        dfg->recalc_early_late_time();

        std::sort(ready.begin(), ready.end(), [](DFGNode* lhs, DFGNode* rhs) {
                        return lhs->get_late_time() < rhs->get_late_time();
                   });

        if (dump_dir)
            dfg->dump(*dump_dir / std::format("schedule_{}", current_time));
    }
    assert(ready.empty() && partial_ready.empty() && "End node is scheduled, but lists are not empty");

    end_time_ = dfg->end_node()->get_time();
}

void Scheduler::write_scheduled_instructions(std::filesystem::path path) {
    try {
        std::ofstream file;
        file.exceptions(std::ifstream::badbit | std::ifstream::failbit);
        file.open(path);

        if (scheduled_instructions_.size() == 0) {
            file << "No instructions were scheduled";
            return;
        }

        constexpr std::string_view cycle_header = "Cycle";
        const size_t cycle_width = std::max(cycle_header.length(),
                                            std::to_string(std::prev(scheduled_instructions_.end())
                                                           ->time).length());

        constexpr std::string_view unit_header = "Unit";
        const size_t unit_width = std::max(unit_header.length(),
                                           std::ranges::max_element(config_.get_units(), {},
                                                [](const auto& unit) { return unit.name.length(); })
                                                                                 ->name.length());

        constexpr std::string_view instr_header = "Instruction";
        const size_t instr_width = instr_header.length();

        file <<  "| " << std::setw((int)cycle_width) << cycle_header;
        file << " | " << std::setw((int)unit_width) << unit_header;
        file << " | " << instr_header << " |\n";

        file << "|" << std::string(cycle_width + 2, '-') << "|" << std::string(unit_width + 2, '-');
        file << "|" << std::string(instr_width + 2, '-') << "|\n";

        ticks_t last_time = ~0ul;
        for (const auto& instr: scheduled_instructions_) {
            file << "| ";
            if (last_time != instr.time) {
                file << std::setw((int)cycle_width) << instr.time;
                last_time = instr.time;
            } else {
                file << std::string(cycle_width, ' ');
            }

            file << " | " << std::setw((int)unit_width) << config_.get_units()[instr.unit_index].name;
            file << " | " << instr.line << "\n";
        }

        file << "| " << std::setw((int)cycle_width) << end_time_;
        file << " | " << std::string(unit_width, ' ');
        file << " | END\n";

    } catch (const std::ofstream::failure& e) {
        throw std::runtime_error(std::format("Output file write error: {}", e.what()));
    }
}
