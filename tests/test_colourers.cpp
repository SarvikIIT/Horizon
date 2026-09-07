#include <gtest/gtest.h>

#include <memory>
#include <random>

#include "first_fit.hpp"
#include "generators.hpp"
#include "kierstead_trotter.hpp"
#include "lookahead.hpp"
#include "offline.hpp"

using namespace horizon;

namespace {

Instance random_small(std::mt19937_64& rng) {
    const int n = 1 + int(rng() % 40);
    std::uniform_real_distribution<double> u(0.0, 1.0);
    Instance inst;
    for (int i = 0; i < n; ++i) {
        double a = u(rng), b = u(rng);
        if (a > b) std::swap(a, b);
        inst.arrival_order.push_back({a, b, i});
    }
    return inst;
}

}  // namespace

// 10,000 random instances per colourer, every colouring validated by run().
TEST(Colourers, ValidOnTenThousandRandomInstances) {
    std::mt19937_64 rng(1234567);
    FirstFit ff;
    KiersteadTrotter kt;
    LookaheadColourer la;
    OnlineColourer* algs[] = {&ff, &kt, &la};

    for (int trial = 0; trial < 10000; ++trial) {
        Instance inst = random_small(rng);
        const int k = trial % 4;
        for (OnlineColourer* a : algs) {
            // run() throws if the colouring is invalid, so reaching the
            // assertions below already proves validity.
            RunResult r = run(inst, *a, k, Lookahead::Strong);
            ASSERT_TRUE(is_valid_colouring(inst, r.colouring)) << a->name() << " trial " << trial;
            ASSERT_GE(r.colours_used, r.omega) << a->name() << " trial " << trial;
        }
    }
}

TEST(Colourers, KiersteadTrotterRespectsThreeOmegaMinusTwo) {
    std::mt19937_64 rng(777);
    KiersteadTrotter kt;
    for (int trial = 0; trial < 3000; ++trial) {
        Instance inst = random_small(rng);
        RunResult r = run(inst, kt, 0, Lookahead::Strong);
        ASSERT_LE(r.colours_used, 3 * r.omega - 2) << "trial " << trial;
    }
}

TEST(Colourers, KiersteadTrotterOnClusteredInstances) {
    KiersteadTrotter kt;
    for (int s = 0; s < 40; ++s) {
        Instance inst = random_clustered(300, 4, 9000 + s);
        RunResult r = run(inst, kt, 0, Lookahead::Strong);
        ASSERT_LE(r.colours_used, 3 * r.omega - 2) << "seed " << s;
    }
}

TEST(Colourers, FirstFitHitsKnownWorstCaseAtOmegaTwo) {
    Instance inst = first_fit_worst_case_omega2();
    EXPECT_EQ(exact_chromatic_number(inst), 2);
    FirstFit ff;
    RunResult r = run(inst, ff, 0, Lookahead::Strong);
    EXPECT_EQ(r.colours_used, 3);  // 2*omega - 1
    EXPECT_EQ(r.colouring, (Colouring{0, 0, 1, 2}));
}

TEST(Colourers, LookaheadWithEmptyBufferIsExactlyFirstFit) {
    std::mt19937_64 rng(24680);
    FirstFit ff;
    LookaheadColourer la;
    for (int trial = 0; trial < 2000; ++trial) {
        Instance inst = random_small(rng);
        EXPECT_EQ(run(inst, ff, 0, Lookahead::Strong).colouring,
                  run(inst, la, 0, Lookahead::Strong).colouring)
            << "trial " << trial;
    }
}

TEST(Colourers, LookaheadNeverBeatsOptimum) {
    LookaheadColourer la;
    for (int s = 0; s < 200; ++s) {
        Instance inst = random_uniform(120, s);
        for (int k : {0, 1, 2, 4}) {
            RunResult r = run(inst, la, k, Lookahead::Strong);
            ASSERT_GE(r.colours_used, r.omega) << "k=" << k << " seed=" << s;
        }
    }
}

// The positive control. An algorithm that cannot turn a full buffer into an
// optimal colouring cannot be trusted to report a flat curve either: a broken
// instrument produces flat lines too.
TEST(Colourers, LookaheadIsOfflineOptimalGivenTheWholeFuture) {
    LookaheadColourer la;
    for (int s = 0; s < 40; ++s) {
        Instance a = random_uniform(80, s);
        Instance b = random_clustered(80, 4, s);
        for (const Instance* inst : {&a, &b}) {
            RunResult r = run(*inst, la, static_cast<int>(inst->size()), Lookahead::Weak);
            ASSERT_EQ(r.colours_used, r.omega) << "seed " << s;
            ASSERT_DOUBLE_EQ(r.ratio, 1.0) << "seed " << s;
        }
    }
}

TEST(Colourers, LookaheadImprovesAsTheBufferGrows) {
    FirstFit ff;
    Instance inst = adversary(ff, 5, 100, 11);
    LookaheadColourer la;
    const int small = run(inst, la, 1, Lookahead::Strong).colours_used;
    const int large = run(inst, la, static_cast<int>(inst.size()), Lookahead::Weak).colours_used;
    EXPECT_LT(large, small) << "the buffer bought nothing";
    EXPECT_EQ(large, exact_chromatic_number(inst));
}

// The other half of the control: the padding transform must destroy exactly
// that gain, which is the claim Lemma 1 and Lemma 3 make.
TEST(Colourers, PaddingNeutralisesTheBuffer) {
    FirstFit ff;
    Instance base = adversary(ff, 5, 100, 11);
    LookaheadColourer la;
    const int plain = run(base, la, 0, Lookahead::Strong).colours_used;

    for (int k : {8, 32, 64}) {
        Padded s = pad_strong(base, k);
        RunResult rs = run(s.instance, la, k, Lookahead::Strong);
        Colouring on_originals;
        for (std::size_t i = 0; i < rs.colouring.size(); ++i)
            if (!s.is_padding[i]) on_originals.push_back(rs.colouring[i]);
        EXPECT_EQ(colours_used(on_originals), plain) << "strong padding leaked, k=" << k;

        Padded w = pad_weak(base, k);
        RunResult rw = run(w.instance, la, k, Lookahead::Weak);
        Colouring w_originals;
        for (std::size_t i = 0; i < rw.colouring.size(); ++i)
            if (!w.is_padding[i]) w_originals.push_back(rw.colouring[i]);
        EXPECT_EQ(colours_used(w_originals), plain) << "weak padding leaked, k=" << k;
    }
}

TEST(Colourers, ResetClearsState) {
    Instance a = random_uniform(50, 1);
    FirstFit ff;
    RunResult r1 = run(a, ff, 0, Lookahead::Strong);
    RunResult r2 = run(a, ff, 0, Lookahead::Strong);
    EXPECT_EQ(r1.colouring, r2.colouring);
}
