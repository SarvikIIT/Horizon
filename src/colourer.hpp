#pragma once
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "interval.hpp"

namespace horizon {

// Which future intervals land in the buffer. See docs/model.md.
//   Weak   : the literal next k arrivals.
//   Strong : the next k arrivals that intersect the current interval, and only
//            those (the irrelevant arrivals between them are not shown).
enum class Lookahead { Weak, Strong };

class OnlineColourer {
public:
    virtual ~OnlineColourer() = default;

    // Forget all state. Called before every run.
    virtual void reset() = 0;

    // Assign an irrevocable colour (>= 0) to `current`. `buffer` holds the
    // lookahead intervals, in arrival order. The implementation must record
    // `current` itself; the harness does not do it.
    virtual int colour(const Interval& current, std::span<const Interval> buffer) = 0;

    virtual std::string name() const = 0;

    // A deep copy, state included. The adversary search needs to try many
    // continuations from one prefix; without this it would have to replay the
    // whole prefix per candidate, which is the difference between O(n) and
    // O(n^2) work per step.
    virtual std::unique_ptr<OnlineColourer> clone() const = 0;
};

struct RunResult {
    Colouring colouring;
    int colours_used = 0;
    int omega = 0;
    double ratio = 0.0;
};

// Build the buffer for arrival position t under the given model.
std::vector<Interval> lookahead_buffer(const Instance& inst, std::size_t t, int k,
                                       Lookahead model);

// Feed the instance to the colourer one interval at a time and validate the
// result. Requires Interval::id to equal the arrival position -- colourers that
// track intervals across buffers rely on it -- and throws if it does not. Throws std::logic_error on an invalid colouring -- in release builds
// too, deliberately: a silently invalid colouring produces a beautiful and
// completely wrong competitive ratio.
RunResult run(const Instance& inst, OnlineColourer& alg, int k, Lookahead model);

}  // namespace horizon
