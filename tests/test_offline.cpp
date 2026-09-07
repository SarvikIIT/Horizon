#include <gtest/gtest.h>

#include <random>

#include "generators.hpp"
#include "offline.hpp"

using namespace horizon;

namespace {

Instance make(std::vector<std::pair<double, double>> ps) {
    Instance inst;
    int id = 0;
    for (auto [a, b] : ps) inst.arrival_order.push_back({a, b, id++});
    return inst;
}

}  // namespace

TEST(Offline, Empty) { EXPECT_EQ(exact_chromatic_number(make({})), 0); }

TEST(Offline, DisjointIntervalsNeedOneColour) {
    auto inst = make({{0, 1}, {2, 3}, {4, 5}, {6, 7}, {8, 9}});
    EXPECT_EQ(exact_chromatic_number(inst), 1);
}

TEST(Offline, MutuallyOverlappingNeedN) {
    for (int n = 1; n <= 12; ++n) {
        std::vector<std::pair<double, double>> ps;
        for (int i = 0; i < n; ++i) ps.push_back({-i, i});  // all contain 0
        EXPECT_EQ(exact_chromatic_number(make(ps)), n) << "n=" << n;
    }
}

TEST(Offline, NestedChainNeedsN) {
    for (int n = 1; n <= 12; ++n) {
        std::vector<std::pair<double, double>> ps;
        for (int i = 0; i < n; ++i) ps.push_back({i, 100.0 - i});  // strictly nested
        EXPECT_EQ(exact_chromatic_number(make(ps)), n) << "n=" << n;
    }
}

TEST(Offline, TouchingEndpointsOverlap) {
    // Closed intervals: [0,1] and [1,2] share the point 1.
    EXPECT_EQ(exact_chromatic_number(make({{0, 1}, {1, 2}})), 2);
    EXPECT_EQ(exact_chromatic_number(make({{0, 1}, {1.0001, 2}})), 1);
}

// Exhaustive over every interval system with n <= 3 on the integer grid [0,4].
TEST(Offline, ExhaustiveTinyMatchesBruteForce) {
    std::vector<std::pair<double, double>> grid;
    for (int a = 0; a <= 4; ++a)
        for (int b = a; b <= 4; ++b) grid.push_back({double(a), double(b)});

    for (std::size_t i = 0; i < grid.size(); ++i) {
        for (std::size_t j = 0; j < grid.size(); ++j) {
            for (std::size_t l = 0; l < grid.size(); ++l) {
                auto inst = make({grid[i], grid[j], grid[l]});
                ASSERT_EQ(exact_chromatic_number(inst), brute_force_chromatic_number(inst))
                    << i << " " << j << " " << l;
            }
        }
    }
}

// Random systems up to n = 8 against the backtracking chromatic number.
TEST(Offline, RandomSmallMatchesBruteForce) {
    std::mt19937_64 rng(20260824);
    std::uniform_int_distribution<int> pt(0, 9);
    for (int trial = 0; trial < 4000; ++trial) {
        const int n = 1 + int(rng() % 8);
        std::vector<std::pair<double, double>> ps;
        for (int i = 0; i < n; ++i) {
            int a = pt(rng), b = pt(rng);
            if (a > b) std::swap(a, b);
            ps.push_back({double(a), double(b)});
        }
        auto inst = make(ps);
        ASSERT_EQ(exact_chromatic_number(inst), brute_force_chromatic_number(inst))
            << "trial " << trial;
    }
}

TEST(Offline, ValidityChecker) {
    auto inst = make({{0, 2}, {1, 3}, {4, 5}});
    EXPECT_TRUE(is_valid_colouring(inst, {0, 1, 0}));
    EXPECT_FALSE(is_valid_colouring(inst, {0, 0, 0}));  // first two overlap
    EXPECT_FALSE(is_valid_colouring(inst, {0, 1}));     // wrong length
    EXPECT_FALSE(is_valid_colouring(inst, {0, -1, 0})); // negative colour

    auto touching = make({{0, 1}, {1, 2}});
    EXPECT_FALSE(is_valid_colouring(touching, {0, 0}));
    EXPECT_TRUE(is_valid_colouring(touching, {0, 1}));
}

TEST(Offline, ColoursUsedCountsDistinct) {
    EXPECT_EQ(colours_used({0, 0, 3, 3, 1}), 3);
    EXPECT_EQ(colours_used({}), 0);
}
