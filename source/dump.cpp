#include "dump.h"

#include <cassert>
#include <fstream>
#include <iostream>

using namespace scheduler;

void DumpableNode::dump_subtree(std::ofstream& file, DumpableNode* parent, size_t traversal_counter) {
    if (parent != nullptr)
        file << "node_" << parent->index_ << "->node_" << index_ << "[color=white]\n";

    if (traversal_status_(traversal_counter) != UNVISITED) {
        assert(traversal_status_(traversal_counter) == VISITED);
        return;
    }
    traversal_counter_ = traversal_counter + VISITED;

    file << "\nnode_" << index_ << " [label=\"";

    dump_node_label(file);

    file << "\"]\n";

    dump_subtree_traversal_(file, traversal_counter);

    file << "\n";
};

void DumpableGraph::dump(std::filesystem::path path) {
    std::ofstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file.open(path.replace_extension("dot"));

    file << "digraph List{\n"
            "    graph [bgcolor=\"#1f1f1f\"];\n"
            "    node[shape=rect, color=white, fontcolor=\"#000000\", fontsize=14, "
                "fontname=\"verdana\", style=\"filled\", fillcolor=\"#6e7681\"];\n\n";

    dump_traversal_entry_(file);
    traversal_counter_ += DumpableNode::VISITED;

    file << "\n}\n";

    file.close();

    if (generate_dot_images_)
        generate_dot_image(path);
}

void DumpableGraph::generate_dot_image(std::filesystem::path dot_path) {
    namespace fs = std::filesystem;

    fs::path image_path = dot_path;
    image_path.replace_extension("svg");

    std::system(("dot -Tsvg " + dot_path.generic_string() + " > " +
                 image_path.generic_string()).c_str());
}

