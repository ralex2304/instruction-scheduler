#pragma once

#include "dump.h"
#include "instruction.h"
#include "types.h"

#include <cassert>
#include <fstream>

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
    DFGNode(Instruction instr, DFGNode* end_node)
        : instruction_(instr)
        , data_(UnscheduledDFGNode(0))
        , dependencies_({end_node}) {}

    DFGNode(Instruction instr)
        : instruction_(instr)
        , data_(UnscheduledDFGNode(0)) {}

    void add_dependency(DFGNode* node) {
        if (!has_dependencies) {
            has_dependencies = true;
            dependencies_[0] = node;
        } else {
            dependencies_.push_back(node);
        }
    }

    ticks_t get_early_time() const;
    ticks_t get_late_time() const;

    ticks_t recalc_early_late_time(ticks_t parent_finish_time);

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

    const Instruction instruction_;

    std::variant<UnscheduledDFGNode, ScheduledDFGNode> data_;

    std::vector<DFGNode*> dependencies_;
    bool has_dependencies = false;
};

class DataFlowGraph : public DumpableGraph {
public:
    DataFlowGraph(std::filesystem::path input_path, const Config& config, bool generate_dot_images);

    virtual void dump_traversal_entry_(std::ofstream& file) override {
        assert(nodes_.size() >= 2);

        nodes_[1]->dump_subtree(file, nullptr, traversal_counter_);
    }

private:
    std::vector<std::unique_ptr<DFGNode>> nodes_;
};

} //< namespace scheduler

