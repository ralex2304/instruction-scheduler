#include "data_flow_graph.h"
#include "types.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <memory>
#include <variant>

using namespace scheduler;

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template<class> inline constexpr bool always_false_v = false;

ticks_t DFGNode::recalc_early_late_time(ticks_t parent_finish_time) {
    // TODO: possible optimization: traverse further only when all parents visited this node
    return std::visit(overloaded {
        [this, parent_finish_time](UnscheduledDFGNode& val) {
            val.early_time = std::max(val.early_time, parent_finish_time);

            for (auto dep: dependencies_) {
                auto dep_early_time = dep->recalc_early_late_time(val.early_time +
                                                                  instruction_.get_latency());

                val.late_time = std::max(val.late_time, dep_early_time);
            }

            if (dependencies_.size() == 0)
                val.late_time = val.early_time;

            return val.early_time;
        },
        [this](ScheduledDFGNode& val) {
            for (auto dep: dependencies_)
                dep->recalc_early_late_time(val.time + instruction_.get_latency());
            return val.time;
        },
        [](auto&& val) {
            static_assert(always_false_v<decltype(val)>, "Unhandled DFGNode type");
        }
    }, data_);
}

void DFGNode::dump_node_label(std::ofstream& file) const {
    std::visit(overloaded {
        [this, &file](const UnscheduledDFGNode& val) {
            file << "Unscheduled node:\\r";
            file << instruction_.get_line() << "\\r";
            file << "Early time = " << val.early_time << "\\r";
            file << "Late time = " << val.late_time << "\\r";
        },
        [this, &file](const ScheduledDFGNode& val) {
            file << "Sceduled node:\\r";
            file << instruction_.get_line() << "\\r";
            file << "Time = " << val.time << "\\r";
        },
        [](const auto&& unhandled) {
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

    nodes_.push_back(std::make_unique<DFGNode>(Instruction::create(std::string("END"), config)));
    nodes_.push_back(std::make_unique<DFGNode>(Instruction::create(std::string("START"), config),
                                               std::prev(nodes_.end())->get()));

    for (std::string line; std::getline(input_file_contents, line);) {
        if (line.size() == 0)
            continue;

        Instruction instruction = Instruction::create(std::move(line), config);

        nodes_.push_back(std::make_unique<DFGNode>(instruction, nodes_.begin()->get()));
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

        if (instruction.is_memory()) {
            if (last_mem_oper) {
                last_mem_oper->add_dependency(cur_node);
                depends = true;
            }

            last_mem_oper = cur_node;
        }

        if (!depends)
            nodes_[1]->add_dependency(cur_node);
    }

    nodes_[1]->recalc_early_late_time(0);
}

