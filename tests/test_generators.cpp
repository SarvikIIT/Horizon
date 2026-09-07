#include <gtest/gtest.h>

#include "first_fit.hpp"
#include "generators.hpp"
#include "kierstead_trotter.hpp"
#include "lookahead.hpp"
#include "offline.hpp"

using namespace horizon;

namespace {

std::vector<std::pair<double, double>> shape(const Instance& i) {
    std::vector<std::pair<double, double>> v;
    for (const Interval& iv : i.arrival_order) v.push_back({iv.left, iv.right});
    return v;
}

}  // namespace

TEST(Generators, UniformIsReproducibleAndWellFormed) {
    Instance a = random_uniform(500, 7);
    Instance b = random_uniform(500, 7);
    Instance c = random_uniform(500, 8);
    EXPECT_EQ(shape(a), shape(b));
    EXPECT_NE(shape(a), shape(c));
    EXPECT_EQ(a.size(), 500u);
    for (std::size_t i = 0; i < a.size(); ++i) {
        EXPECT_LE(a.arrival_order[i].left, a.arrival_order[i].right);
        EXPECT_EQ(a.arrival_order[i].id, static_cast<int>(i));
    }
}

TEST(Generators, ClusteredIsReproducibleAndConcentratesOmega) {
    EXPECT_EQ(shape(random_clustered(400, 5, 11)), shape(random_clustered(400, 5, 11)));
    // Same n, fewer clusters => more crowding => larger omega.
    const int few = exact_chromatic_number(random_clustered(600, 2, 3));
    const int many = exact_chromatic_number(random_clustered(600, 40, 3));
    EXPECT_GT(few, many);
}

TEST(Generators, FirstFitWorstCaseIsFixed) {
    EXPECT_EQ(shape(first_fit_worst_case_omega2()), shape(first_fit_worst_case_omega2()));
    EXPECT_EQ(exact_chromatic_number(first_fit_worst_case_omega2()), 2);
}

TEST(Generators, AdversaryIsDeterministicGivenAlgorithmAndSeed) {
    FirstFit a, b;
    EXPECT_EQ(shape(adversary(a, 4, 60, 99)), shape(adversary(b, 4, 60, 99)));

    // A different algorithm colours differently, so the search diverges.
    KiersteadTrotter kt;
    FirstFit ff;
    EXPECT_NE(shape(adversary(kt, 4, 60, 99)), shape(adversary(ff, 4, 60, 99)));
}

TEST(Generators, AdversaryRespectsTheCliqueCap) {
    for (int w = 1; w <= 4; ++w) {
        FirstFit ff;
        Instance inst = adversary(ff, w, 40, 5);
        EXPECT_LE(exact_chromatic_number(inst), w) << "omega cap " << w;
    }
}

TEST(Generators, AdversaryBeatsTheOptimumOnEveryColourer) {
    FirstFit ff;
    KiersteadTrotter kt;
    LookaheadColourer la;
    OnlineColourer* algs[] = {&ff, &kt, &la};
    for (OnlineColourer* a : algs) {
        Instance inst = adversary(*a, 4, 80, 5);
        RunResult r = run(inst, *a, 0, Lookahead::Strong);
        EXPECT_GT(r.colours_used, r.omega) << a->name();
    }
}

TEST(Padding, WeakPaddingKeepsOmegaAndFillsTheBuffer) {
    FirstFit ff;
    Instance base = adversary(ff, 4, 60, 5);
    const int omega = exact_chromatic_number(base);

    for (int k : {1, 2, 3}) {
        Padded p = pad_weak(base, k);
        EXPECT_EQ(exact_chromatic_number(p.instance), omega) << "k=" << k;
        EXPECT_EQ(p.instance.size(), base.size() * (k + 1)) << "k=" << k;

        // Every original interval sees exactly k dummies, all of them parked
        // past the right end of the construction: zero information.
        double hi = 0.0;
        for (const Interval& iv : base.arrival_order) hi = std::max(hi, iv.right);
        for (std::size_t t = 0; t < p.instance.size(); ++t) {
            if (p.is_padding[t]) continue;
            auto buf = lookahead_buffer(p.instance, t, k, Lookahead::Weak);
            if (t + 1 + k > p.instance.size()) continue;  // tail of the sequence
            ASSERT_EQ(buf.size(), static_cast<std::size_t>(k));
            for (const Interval& b : buf) EXPECT_GT(b.left, hi);
        }
    }
}

TEST(Padding, StrongPaddingIsInformationFreeAndCostsAtMostOneClique) {
    FirstFit ff;
    Instance base = adversary(ff, 4, 60, 5);
    const int omega = exact_chromatic_number(base);

    for (int k : {1, 2, 3}) {
        Padded p = pad_strong(base, k);
        EXPECT_EQ(p.leaks, 0) << "k=" << k;  // no stub is overlapped by its own future
        EXPECT_LE(exact_chromatic_number(p.instance), omega + 1) << "k=" << k;

        for (std::size_t t = 0; t < p.instance.size(); ++t) {
            auto buf = lookahead_buffer(p.instance, t, k, Lookahead::Strong);
            if (p.is_padding[t]) {
                EXPECT_TRUE(buf.empty()) << "a stub saw the future, k=" << k;
            } else {
                // Every interval the original interval sees is one of its own
                // stubs, i.e. nested inside it.
                ASSERT_EQ(buf.size(), static_cast<std::size_t>(k));
                for (const Interval& b : buf) {
                    EXPECT_GE(b.left, p.instance.arrival_order[t].left);
                    EXPECT_LE(b.right, p.instance.arrival_order[t].right);
                }
            }
        }
    }
}

TEST(Padding, PaddedInstancesStayHard) {
    // The point of the transforms: the hardness survives the lookahead.
    FirstFit ff;
    Instance base = adversary(ff, 4, 80, 5);
    const RunResult plain = run(base, ff, 0, Lookahead::Strong);

    for (int k : {1, 2, 4}) {
        Padded w = pad_weak(base, k);
        EXPECT_GE(run(w.instance, ff, k, Lookahead::Weak).colours_used, plain.colours_used);

        Padded s = pad_strong(base, k);
        EXPECT_GE(run(s.instance, ff, k, Lookahead::Strong).colours_used, plain.colours_used);
    }
}

TEST(Buffers, WeakLookaheadIsTheLiteralNextK) {
    Instance inst;
    inst.arrival_order = {{0, 1, 0}, {10, 11, 1}, {0.5, 2, 2}, {20, 21, 3}};
    auto b = lookahead_buffer(inst, 0, 2, Lookahead::Weak);
    ASSERT_EQ(b.size(), 2u);
    EXPECT_EQ(b[0].id, 1);
    EXPECT_EQ(b[1].id, 2);
}

TEST(Buffers, StrongLookaheadSkipsIrrelevantArrivals) {
    Instance inst;
    inst.arrival_order = {{0, 1, 0}, {10, 11, 1}, {0.5, 2, 2}, {20, 21, 3}, {0.9, 3, 4}};
    auto b = lookahead_buffer(inst, 0, 2, Lookahead::Strong);
    ASSERT_EQ(b.size(), 2u);
    EXPECT_EQ(b[0].id, 2);
    EXPECT_EQ(b[1].id, 4);
}

TEST(Buffers, RunsShortAtTheEndOfTheSequence) {
    Instance inst;
    inst.arrival_order = {{0, 1, 0}, {10, 11, 1}};
    EXPECT_TRUE(lookahead_buffer(inst, 0, 3, Lookahead::Strong).empty());
    EXPECT_EQ(lookahead_buffer(inst, 0, 3, Lookahead::Weak).size(), 1u);
    EXPECT_TRUE(lookahead_buffer(inst, 1, 3, Lookahead::Weak).empty());
}
