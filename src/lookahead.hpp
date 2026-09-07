#pragma once
#include <unordered_map>

#include "colourer.hpp"

namespace horizon {

// The research algorithm: plan once, then honour the plan.
//
// Offline, First-Fit in left-endpoint order is exactly optimal on an interval
// graph. The buffer is a window onto that order, so the algorithm sorts
// {current} + buffer by left endpoint, greedily colours the whole window, and
// *reserves* the result. An interval that already carries a reservation when it
// arrives simply takes it; it is never re-planned.
//
// Re-planning is what a naive lookahead algorithm gets wrong. Recomputing a
// fresh window sweep at every arrival optimises each step for an ordering that
// never happened, and fights the colours already committed -- measurably worse
// than plain First-Fit. Persisting the plan is what makes the lookahead pay.
//
// Two properties fall out, both pinned by tests:
//   - empty buffer  => exactly First-Fit;
//   - weak model with k >= n => exactly omega colours, i.e. offline optimal,
//     because the first window is the whole instance and every later arrival is
//     already reserved.
class LookaheadColourer : public OnlineColourer {
public:
    void reset() override;
    int colour(const Interval& current, std::span<const Interval> buffer) override;
    std::string name() const override { return "lookahead"; }
    std::unique_ptr<OnlineColourer> clone() const override {
        return std::make_unique<LookaheadColourer>(*this);
    }

private:
    // Smallest colour free for `iv` given everything already assigned, whether
    // committed or merely reserved.
    int free_colour_for(const Interval& iv) const;
    void assign(const Interval& iv, int colour);

    std::vector<Interval> assigned_iv_;
    std::vector<int> assigned_col_;
    std::unordered_map<int, int> colour_of_id_;
};

}  // namespace horizon
