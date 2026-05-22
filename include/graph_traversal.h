#pragma once

#include <cassert>
#include <cstddef>

namespace scheduler {

class TraversableNode {
public:
    enum TraversalStatus {
        UNVISITED = 0,
        VISITING  = 1,
        VISITED   = 2,
        INCORRECT = 3,
    };

protected:
    TraversalStatus traversal_status_(size_t traversal_counter) {
        assert(traversal_counter_ >= traversal_counter);
        assert(traversal_counter_ - traversal_counter < INCORRECT);

        return static_cast<TraversalStatus>(traversal_counter_ - traversal_counter);
    }

    size_t traversal_counter_ = UNVISITED;
};

class TraversableGraph {
protected:
    size_t traversal_counter_ = TraversableNode::UNVISITED;
};

} // namespace scheduler

