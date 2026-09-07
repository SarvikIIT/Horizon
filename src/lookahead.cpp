#include "lookahead.hpp"

#include <algorithm>

namespace horizon {

void LookaheadColourer::reset() {
    assigned_iv_.clear();
    assigned_col_.clear();
    colour_of_id_.clear();
}

int LookaheadColourer::free_colour_for(const Interval& iv) const {
    std::vector<char> used(assigned_col_.size() + 2, 0);
    for (std::size_t i = 0; i < assigned_iv_.size(); ++i)
        if (overlaps(assigned_iv_[i], iv) && assigned_col_[i] < static_cast<int>(used.size()))
            used[assigned_col_[i]] = 1;
    int c = 0;
    while (c < static_cast<int>(used.size()) && used[c]) ++c;
    return c;
}

void LookaheadColourer::assign(const Interval& iv, int colour) {
    assigned_iv_.push_back(iv);
    assigned_col_.push_back(colour);
    colour_of_id_[iv.id] = colour;
}

int LookaheadColourer::colour(const Interval& current, std::span<const Interval> buffer) {
    // Already planned in an earlier window: honour it. The reservation is still
    // legal because every assignment made since avoided it.
    if (const auto it = colour_of_id_.find(current.id); it != colour_of_id_.end())
        return it->second;

    std::vector<Interval> window;
    window.reserve(buffer.size() + 1);
    window.push_back(current);
    for (const Interval& j : buffer)
        if (!colour_of_id_.count(j.id)) window.push_back(j);

    // Left-endpoint order. Ties on both endpoints fall back to arrival order so
    // the comparator is a strict weak ordering.
    std::sort(window.begin(), window.end(), [](const Interval& a, const Interval& b) {
        if (a.left != b.left) return a.left < b.left;
        if (a.right != b.right) return a.right < b.right;
        return a.id < b.id;
    });

    // ponytail: O(window * assigned) per plan. Fine to n ~ a few thousand; an
    // interval tree over `assigned_iv_` is the upgrade if n gets large.
    for (const Interval& iv : window) assign(iv, free_colour_for(iv));

    return colour_of_id_[current.id];
}

}  // namespace horizon
