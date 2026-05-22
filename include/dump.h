#pragma once

#include "graph_traversal.h"

#include <filesystem>
#include <fstream>

namespace scheduler {

using NodeIdx = size_t;

class DumpableNode: public TraversableNode {
public:
    void dump_subtree(std::ofstream& file, DumpableNode* parent, size_t traversal_counter);

    virtual ~DumpableNode() = default;
protected:
    DumpableNode() {
        static NodeIdx node_idx = 0;
        index_ = node_idx++;
    }

    virtual void dump_subtree_traversal_(std::ofstream& file, size_t traversal_counter) = 0;
    virtual void dump_node_label(std::ofstream& file) const = 0;

    NodeIdx index_;
};

class DumpableGraph: protected TraversableGraph {
public:
    DumpableGraph(bool generate_dot_images) : generate_dot_images_(generate_dot_images) {}

    void dump(std::filesystem::path path);

    static void generate_dot_image(std::filesystem::path dot_path);

    virtual ~DumpableGraph() = default;
protected:
    const bool generate_dot_images_;

    virtual void dump_traversal_entry_(std::ofstream& file) = 0;
};

} // namespace scheduler

