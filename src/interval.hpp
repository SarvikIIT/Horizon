#pragma once
#include <vector>

namespace horizon {

// Closed interval [left, right] on the real line. Plain value, no vtable.
struct Interval {
    double left = 0.0;
    double right = 0.0;
    int id = -1;
};

// Closed-interval adjacency: touching endpoints count as overlapping.
inline bool overlaps(const Interval& a, const Interval& b) {
    return a.left <= b.right && b.left <= a.right;
}

struct Instance {
    std::vector<Interval> arrival_order;

    std::size_t size() const { return arrival_order.size(); }
};

using Colouring = std::vector<int>;  // colour_of_interval[i] for arrival position i

}  // namespace horizon
