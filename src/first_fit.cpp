#include "first_fit.hpp"

namespace horizon {

void FirstFit::reset() {
    seen_.clear();
    col_.clear();
}

int FirstFit::colour(const Interval& current, std::span<const Interval>) {
    // ponytail: O(n) scan per arrival, O(n^2) per run. The sweep is not
    // available online (arrivals are in adversarial, not left-to-right, order);
    // an interval tree keyed by colour is the upgrade if n gets large.
    std::vector<char> used(col_.size() + 1, 0);
    for (std::size_t i = 0; i < seen_.size(); ++i)
        if (overlaps(seen_[i], current) && col_[i] < static_cast<int>(used.size()))
            used[col_[i]] = 1;

    int c = 0;
    while (c < static_cast<int>(used.size()) && used[c]) ++c;

    seen_.push_back(current);
    col_.push_back(c);
    return c;
}

}  // namespace horizon
