#pragma once
#include "interval.hpp"

namespace horizon {

// Chromatic number of the interval graph. Interval graphs are perfect, so this
// equals the maximum clique, which equals the maximum number of intervals
// covering a common point. Sweep line, O(n log n).
int exact_chromatic_number(const Instance& inst);

// Exact chromatic number by backtracking on the graph, ignoring interval
// structure. Exponential; for tests on tiny instances only.
int brute_force_chromatic_number(const Instance& inst);

// True iff no two overlapping intervals share a colour. O(n log n) sweep.
bool is_valid_colouring(const Instance& inst, const Colouring& col);

// Number of distinct colours used.
int colours_used(const Colouring& col);

}  // namespace horizon
