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
#include <list>
#include <optional>
#include <ostream>

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

namespace {

void update_ready_from_partial_ready(std::list<DFGNode*>* ready, std::list<DFGNode*>* partial_ready,
                                     ticks_t current_time) {
    for (auto node_it = partial_ready->begin(); node_it != partial_ready->end();) {
        if ((*node_it)->get_early_time() <= current_time) {
            ready->push_back(*node_it);
            node_it = partial_ready->erase(node_it);
        } else {
            node_it++;
        }
    }
}

} // anonymous namespace

void Scheduler::schedule(DataFlowGraph* dfg, std::optional<std::filesystem::path> dump_dir) {
    assert(dfg);

    std::list<DFGNode*> ready;
    std::list<DFGNode*> partial_ready;

    ticks_t current_time = 0;

    dfg->start_node()->schedule(current_time);
    for (const auto& dependency: *dfg->start_node())
        ready.push_back(dependency);

    while (!dfg->end_node()->is_scheduled()) {
        std::map<std::string, UnitConfig> units(config_.units);

        std::set<DFGNode*> ready_dependencies;

        // schedule ready pool
        for (auto node_it = ready.begin(); node_it != ready.end();) {
            bool scheduled = false;
            if ((*node_it)->instr_config.latency == 0) {
                scheduled = true;
            } else {
                for (const auto& unit: (*node_it)->instr_config.units) {
                    if (units[unit].quantity > 0) {
                        units[unit].quantity--;
                        scheduled_instructions_.push_back({
                            .time = current_time,
                            .unit = unit,
                            .line = (*node_it)->line
                        });

                        scheduled = true;
                        break;
                    }
                }
            }

            if (scheduled) {
                (*node_it)->schedule(current_time);
                for (const auto& dependency: **node_it)
                    if (dependency->is_ready())
                        ready_dependencies.insert(dependency);

                node_it = ready.erase(node_it);
            } else {
                node_it++;
            }
        }

        current_time++;

        update_ready_from_partial_ready(&ready, &partial_ready, current_time);

        for (const auto& dependency: ready_dependencies) {
            if (dependency->get_early_time() <= current_time)
                ready.push_back(dependency);
            else
                partial_ready.push_back(dependency);
        }

        dfg->recalc_early_late_time();

        ready.sort([](DFGNode* lhs, DFGNode* rhs) {
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

        constexpr const char* cycle_header = "Cycle";
        const size_t cycle_width = std::max(std::strlen(cycle_header),
                                            std::to_string(std::prev(scheduled_instructions_.end())
                                                           ->time).length());

        constexpr const char* unit_header = "Unit";
        const size_t unit_width = std::max(std::strlen(unit_header),
                                           std::ranges::max_element(config_.units, {},
                                                [](const auto& unit) { return unit.first.length(); })
                                                                                 ->first.length());

        constexpr const char* instr_header = "Instruction";
        const size_t instr_width = std::strlen(instr_header);

        file <<  "| " << std::setw((int)cycle_width) << cycle_header;
        file << " | " << std::setw((int)unit_width) << unit_header;
        file << " | " << instr_header << " |" << std::endl;

        file << "|" << std::string(cycle_width + 2, '-') << "|" << std::string(unit_width + 2, '-');
        file << "|" << std::string(instr_width + 2, '-') << "|" << std::endl;

        ticks_t last_time = ~0ul;
        for (const auto& instr: scheduled_instructions_) {
            file << "| ";
            if (last_time != instr.time) {
                file << std::setw((int)cycle_width) << instr.time;
                last_time = instr.time;
            } else {
                file << std::string(cycle_width, ' ');
            }

            file << " | " << std::setw((int)unit_width) << instr.unit;
            file << " | " << instr.line << std::endl;
        }

        file << "| " << std::setw((int)cycle_width) << end_time_;
        file << " | " << std::string(unit_width, ' ');
        file << " | END" << std::endl;

    } catch (const std::ofstream::failure& e) {
        throw std::runtime_error(std::format("Output file write error: {}", e.what()));
    }
}
