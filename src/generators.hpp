#pragma once
#include <cstdint>

#include "colourer.hpp"

namespace horizon {

// Endpoints uniform on [0, 1], arrival order a uniform random permutation.
Instance random_uniform(int n, std::uint64_t seed);

// Intervals concentrated into `clusters` narrow windows, so omega is large
// relative to n. Arrival order a uniform random permutation.
Instance random_clustered(int n, int clusters, std::uint64_t seed);

// The standard bad construction for First-Fit at omega = 2: First-Fit is forced
// to 3 = 2*omega - 1 colours. Fixed, no seed.
Instance first_fit_worst_case_omega2();

// Adaptive adversary. Beam search over partial constructions: each step
// proposes candidate intervals from every surviving prefix, feeds each to a
// clone of the algorithm, and keeps the `beam` best by colours spent subject to
// omega <= target_omega. Returns the prefix with the worst observed ratio.
//
// A beam rather than greedy descent because forcing a colour beyond omega needs
// a run of locally neutral preparatory moves that hill climbing cannot hold.
//
// Adaptive by construction and deterministic given all five arguments. It is a
// search, not a closed-form construction: see docs/proof_k1.md.
Instance adversary(OnlineColourer& alg, int target_omega, int n_max, std::uint64_t seed,
                   int beam = 6);

// The padding transforms from the lower-bound proof. Each takes an instance
// that is hard at zero lookahead and returns one that is hard at lookahead k,
// because every buffer it produces is a fixed function of what the algorithm
// has already been shown.
struct Padded {
    Instance instance;
    std::vector<char> is_padding;  // parallel to instance.arrival_order
    // Padding intervals that some later arrival overlaps, i.e. that do leak a
    // future request. The transform is information-free exactly when this is 0.
    int leaks = 0;
};

// Weak model: after each original interval, k dummies parked past the right end
// of everything, pairwise disjoint. omega is unchanged.
Padded pad_weak(const Instance& inst, int k);

// Strong model: after each original interval I_t, k tiny stubs nested inside
// I_t, placed in the part of I_t that no later arrival covers. omega grows by
// at most 1.
Padded pad_strong(const Instance& inst, int k);

}  // namespace horizon
