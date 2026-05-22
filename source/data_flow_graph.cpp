#include "data_flow_graph.h"
#include "instruction.h"
#include "types.h"
#include "utils.h"

#include <cassert>
#include <format>
#include <fstream>
#include <iostream>
#include <variant>

using namespace scheduler;

ticks_t DFGNode::get_time() const {
    auto data = std::get_if<ScheduledDFGNode>(&data_);
    assert(data && "Trying to get time for Unscheduled node");
    return data->time;
}

ticks_t DFGNode::get_early_time() const {
    auto data = std::get_if<UnscheduledDFGNode>(&data_);
    assert(data && "Trying to get early time for scheduled node");
    return data->early_time;
}

ticks_t DFGNode::get_late_time() const {
    auto data = std::get_if<UnscheduledDFGNode>(&data_);
    assert(data && "Trying to get late time for scheduled node");
    return data->late_time;
}

void DFGNode::recalc_early_time(ticks_t parent_finish_time) {
    // TODO: possible optimization: traverse further only when all parents visited this node
    return std::visit(overloaded {
        [this, parent_finish_time](UnscheduledDFGNode& val) {
            val.late_time = std::numeric_limits<decltype(val.late_time)>::max();
            val.early_time = std::max(val.early_time, parent_finish_time);

            for (auto dep: dependencies_)
                dep->recalc_early_time(val.early_time + instr_config.latency);
        },
        [this](ScheduledDFGNode& val) {
            for (auto dep: dependencies_)
                dep->recalc_early_time(val.time + instr_config.latency);
        },
        [](auto&& val) {
            static_assert(always_false_v<decltype(val)>, "Unhandled DFGNode type");
        }
    }, data_);
}

void DFGNode::recalc_late_time(ticks_t child_late_time) {
    // TODO: possible optimization: traverse further only when all parents visited this node
    return std::visit(overloaded {
        [this, child_late_time](UnscheduledDFGNode& val) {
            val.late_time = std::min(val.late_time, child_late_time - instr_config.latency);
            for (auto parent: parents_)
                parent->recalc_late_time(val.late_time);
        },
        [](ScheduledDFGNode&) {
            // do nothing
        },
        [](auto&& val) {
            static_assert(always_false_v<decltype(val)>, "Unhandled DFGNode type");
        }
    }, data_);
}

bool DFGNode::is_ready() const {
    return std::visit(overloaded {
                [this](const UnscheduledDFGNode&) {
                    for (auto parent: parents_)
                        if (!parent->is_scheduled())
                            return false;
                    return true;
                },
                [](const ScheduledDFGNode&) { return true; },
                [](auto&& val) {
                    static_assert(always_false_v<decltype(val)>, "Unhandled DFGNode type");
                }},
            data_);
}

void DFGNode::schedule(ticks_t current_time) {
    UnscheduledDFGNode* data = std::get_if<UnscheduledDFGNode>(&data_);
    assert(data && "Trying to schedule instruction that has already been scheduled");
    assert(current_time >= data->early_time && "Instruction isn't ready for scheduling");

    data_.emplace<ScheduledDFGNode>(current_time);
}

void DFGNode::dump_node_label(std::ofstream& file) const {
    std::visit(overloaded {
        [this, &file](const UnscheduledDFGNode& val) {
            file << "Unscheduled node:\\r";
            file << line << "\\r";
            file << "Early time = " << val.early_time << "\\r";
            file << "Late time = " << val.late_time << "\\r";
        },
        [this, &file](const ScheduledDFGNode& val) {
            file << "Scheduled node:\\r";
            file << line << "\\r";
            file << "Time = " << val.time << "\\r";
        },
        [](auto&& unhandled) {
            static_assert(always_false_v<decltype(unhandled)>, "Unhandled DFGNode type");
        }
    }, data_);
}

DataFlowGraph::DataFlowGraph(std::filesystem::path input_path, const Config& config,
                             bool genetate_dot_images)
    : DumpableGraph(genetate_dot_images) {

    std::ifstream input_file;
    input_file.exceptions(std::ifstream::badbit | std::ifstream::failbit);
    input_file.open(input_path);

    std::stringstream input_file_contents;
    input_file_contents << input_file.rdbuf();
    input_file.close();

    std::map<reg_t, DFGNode*> defs;
    DFGNode* last_mem_oper = nullptr;

    nodes_.push_back(std::make_unique<DFGNode>(config.start_instruction, std::string("START")));
    nodes_.push_back(std::make_unique<DFGNode>(config.end_instruction, std::string("END")));

    for (std::string line; std::getline(input_file_contents, line);) {
        if (line.size() == 0)
            continue;

        Instruction instruction = Instruction::create(line, config);

        nodes_.push_back(std::make_unique<DFGNode>(instruction.get_config(), line));
        DFGNode* cur_node = std::prev(nodes_.end())->get();

        auto dst_reg = instruction.get_dst_reg();
        if (dst_reg) {
            bool inserted = defs.try_emplace(*dst_reg, cur_node).second;
            if (!inserted)
                throw std::runtime_error(std::format("Register R{} is redefined", *dst_reg));
        }

        bool depends = false;

        auto src_regs = instruction.get_src_regs();
        for (auto reg: src_regs) {
            auto it = defs.find(reg);
            if (it == defs.end())
                throw std::runtime_error(std::format("Register R{} is used before its definition",
                                                     reg));
            it->second->add_dependency(cur_node);
            depends = true;
        }

        if (instruction.get_config().memory_order == MemoryOrder::STRICT) {
            if (last_mem_oper) {
                last_mem_oper->add_dependency(cur_node);
                depends = true;
            }

            last_mem_oper = cur_node;
        }

        if (!depends)
            start_node()->add_dependency(cur_node);
    }

    start_node()->tie_end_node(end_node());

    recalc_early_late_time();
}

