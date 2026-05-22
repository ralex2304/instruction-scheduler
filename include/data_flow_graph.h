#pragma once

#include "configs_parser.h"
#include "dump.h"
#include "graph_traversal.h"
#include "types.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <variant>

namespace scheduler {

struct UnscheduledDFGNode {
    ticks_t early_time;
    ticks_t late_time;
};

struct ScheduledDFGNode {
    const ticks_t time;
};

class DFGNode : public DumpableNode {
public:
    DFGNode(const InstructionConfig& instr_config, std::string line)
        : instr_config(instr_config)
        , line(line)
        , data_(UnscheduledDFGNode(0, 0)) {}

    const InstructionConfig& instr_config;
    const std::string line;

    void add_dependency(DFGNode* node) {
        if (std::ranges::find(dependencies_, node) == dependencies_.end())
            dependencies_.push_back(node);

        if (std::ranges::find(node->parents_, this) == node->parents_.end())
            node->parents_.push_back(this);
    }

    void tie_end_node(DFGNode* end_node) {
        if (this == end_node) return;

        for (auto& dep: dependencies_)
            dep->tie_end_node(end_node);

        if (dependencies_.empty())
            add_dependency(end_node);
    }

    ticks_t get_time() const;
    ticks_t get_early_time() const;
    ticks_t get_late_time() const;

    bool is_scheduled() const { return std::holds_alternative<ScheduledDFGNode>(data_); }

    bool is_ready() const;

    void schedule(ticks_t current_time);

    virtual void dump_node_label(std::ofstream& file) const override;

    virtual void dump_subtree_traversal_(std::ofstream& file, size_t traversal_counter) override {
        for (auto node: dependencies_) {
            node->dump_subtree(file, this, traversal_counter);
        }
    }

    auto begin() const noexcept { return dependencies_.begin(); }
    auto end()   const noexcept { return dependencies_.end(); }

private:
    DFGNode(const DFGNode&) = delete;
    DFGNode& operator=(const DFGNode&) = delete;
    DFGNode(DFGNode&&) noexcept = default;
    DFGNode& operator=(DFGNode&&) noexcept = delete;

    friend class DataFlowGraph;

    void recalc_early_time(ticks_t parent_finish_time, size_t traversal_counter);
    void recalc_late_time(ticks_t dependency_late_time, size_t traversal_counter);

    std::variant<UnscheduledDFGNode, ScheduledDFGNode> data_;

    std::vector<DFGNode*> dependencies_;
    std::vector<DFGNode*> parents_;
};

class DataFlowGraph : public DumpableGraph {
public:
    DataFlowGraph(std::filesystem::path input_path, const Config& config, bool generate_dot_images);

    DFGNode* start_node() const {
        assert(nodes_.size() >= 1 && "DataFlowGraph isn't properly initialized");
        return nodes_[0].get();
    }

    DFGNode* end_node() const {
        assert(nodes_.size() >= 2 && "DataFlowGraph isn't properly initialized");
        return nodes_[1].get();
    }

    virtual void dump_traversal_entry_(std::ofstream& file) override {
        start_node()->dump_subtree(file, nullptr, traversal_counter_);
    }

    void recalc_early_late_time() {
        start_node()->recalc_early_time(0, traversal_counter_);
        traversal_counter_ += TraversableNode::VISITED;
        end_node()->recalc_late_time(end_node()->is_scheduled() ? end_node()->get_time()
                                                                : end_node()->get_early_time(),
                                     traversal_counter_);
        traversal_counter_ += TraversableNode::VISITED;
    }

    auto begin() const noexcept { return nodes_.begin(); };
    auto end()   const noexcept { return nodes_.end(); };

private:
    std::vector<std::unique_ptr<DFGNode>> nodes_;
};

} //< namespace scheduler

