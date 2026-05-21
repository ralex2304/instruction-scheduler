#pragma once

#include "configs_parser.h"
#include "dump.h"
#include "types.h"

#include <cassert>
#include <fstream>
#include <set>
#include <variant>

namespace scheduler {

struct UnscheduledDFGNode {
    ticks_t early_time;
    ticks_t late_time;

    size_t unscheduled_parents;
};

struct ScheduledDFGNode {
    const ticks_t time;
};

class DFGNode : public DumpableNode {
public:
    DFGNode(const InstructionConfig& instr_config, std::string line)
        : instr_config(instr_config)
        , line(line)
        , data_(UnscheduledDFGNode(0, 0, 0)) {}

    const InstructionConfig& instr_config;
    const std::string line;

    void add_dependency(DFGNode* node) {
        if (dependencies_.insert(node).second)
            std::get_if<UnscheduledDFGNode>(&node->data_)->unscheduled_parents++;
    }

    void tie_end_node(DFGNode* end_node) {
        if (this == end_node) return;

        for (auto& dep: dependencies_)
            dep->tie_end_node(end_node);

        if (dependencies_.size() == 0)
            add_dependency(end_node);
    }

    ticks_t get_time() const;
    ticks_t get_early_time() const;
    ticks_t get_late_time() const;

    ticks_t recalc_early_late_time(ticks_t parent_finish_time);

    bool is_scheduled() const { return std::holds_alternative<ScheduledDFGNode>(data_); }

    bool is_ready() const;

    void schedule(ticks_t current_time);

    virtual void dump_node_label(std::ofstream& file) const override;

    virtual void dump_subtree_traversal_(std::ofstream& file, size_t traversal_counter) override {
        for (auto node: dependencies_) {
            node->dump_subtree(file, this, traversal_counter);
        }
    }

private:
    DFGNode(const DFGNode&) = delete;
    DFGNode& operator=(const DFGNode&) = delete;
    DFGNode(DFGNode&&) noexcept = default;
    DFGNode& operator=(DFGNode&&) noexcept = delete;


    std::variant<UnscheduledDFGNode, ScheduledDFGNode> data_;

    std::set<DFGNode*> dependencies_;

public:
    const decltype(dependencies_)::const_iterator begin() { return dependencies_.begin(); }
    const decltype(dependencies_)::const_iterator end()   { return dependencies_.end(); }
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

    void recalc_early_late_time() { start_node()->recalc_early_late_time(0); }

private:
    std::vector<std::unique_ptr<DFGNode>> nodes_;

public:
    decltype(nodes_)::const_iterator begin() const noexcept { return nodes_.begin(); };
    decltype(nodes_)::const_iterator end()   const noexcept { return nodes_.end(); };
};

} //< namespace scheduler

