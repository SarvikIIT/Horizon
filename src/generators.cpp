#include "generators.hpp"

#include <algorithm>
#include <memory>
#include <random>
#include <utility>

#include "offline.hpp"

namespace horizon {
namespace {

Instance shuffled(std::vector<Interval> xs, std::mt19937_64& rng) {
    std::shuffle(xs.begin(), xs.end(), rng);
    for (std::size_t i = 0; i < xs.size(); ++i) xs[i].id = static_cast<int>(i);
    return Instance{std::move(xs)};
}

// A padding interval leaks a future request only if some later arrival
// overlaps it.
int count_leaks(const Instance& inst, const std::vector<char>& is_padding) {
    int leaks = 0;
    for (std::size_t t = 0; t < inst.size(); ++t) {
        if (!is_padding[t]) continue;
        for (std::size_t j = t + 1; j < inst.size(); ++j) {
            if (overlaps(inst.arrival_order[t], inst.arrival_order[j])) {
                ++leaks;
                break;
            }
        }
    }
    return leaks;
}

// The widest sub-interval of I_t that no later arrival covers. Returns an
// empty range (second < first) when I_t is entirely swallowed by its own
// future. A stub placed strictly inside this range is overlapped by nothing
// that comes after it.
std::pair<double, double> largest_free_gap(const Instance& inst, std::size_t t) {
    const auto& xs = inst.arrival_order;
    std::vector<std::pair<double, double>> blocked;
    for (std::size_t s = t + 1; s < xs.size(); ++s) {
        if (!overlaps(xs[t], xs[s])) continue;
        blocked.push_back({std::max(xs[t].left, xs[s].left), std::min(xs[t].right, xs[s].right)});
    }
    std::sort(blocked.begin(), blocked.end());

    std::pair<double, double> best{0.0, -1.0};
    double cur = xs[t].left;
    auto consider = [&](double a, double b) {
        if (b - a > best.second - best.first) best = {a, b};
    };
    for (const auto& ab : blocked) {
        if (ab.first > cur) consider(cur, ab.first);
        cur = std::max(cur, ab.second);
    }
    if (cur < xs[t].right) consider(cur, xs[t].right);
    return best;
}

// Every interval keeps a point its own future never covers -- the hypothesis
// the strong-lookahead padding transform needs (docs/proof_k1.md, Lemma 3).
bool stub_friendly(const Instance& inst) {
    for (std::size_t t = 0; t < inst.size(); ++t) {
        const auto g = largest_free_gap(inst, t);
        if (g.second <= g.first) return false;
    }
    return true;
}

}  // namespace

Instance random_uniform(int n, std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> u(0.0, 1.0);
    std::vector<Interval> xs;
    xs.reserve(n);
    for (int i = 0; i < n; ++i) {
        double a = u(rng), b = u(rng);
        if (a > b) std::swap(a, b);
        xs.push_back({a, b, i});
    }
    return shuffled(std::move(xs), rng);
}

Instance random_clustered(int n, int clusters, std::uint64_t seed) {
    if (clusters < 1) clusters = 1;
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<int> pick(0, clusters - 1);
    std::uniform_real_distribution<double> u(0.0, 1.0);
    // Cluster c owns the window [c, c + 1); intervals inside it are short, so
    // they pile up and omega grows with n / clusters.
    const double span = 0.05;
    std::vector<Interval> xs;
    xs.reserve(n);
    for (int i = 0; i < n; ++i) {
        const double base = pick(rng);
        double a = base + u(rng) * (1.0 - span);
        double b = a + u(rng) * span;
        xs.push_back({a, b, i});
    }
    return shuffled(std::move(xs), rng);
}

Instance first_fit_worst_case_omega2() {
    // X -> 0, Y -> 0, Z -> 1 (meets Y only), W -> 2 (meets X and Z at disjoint
    // points). No point is covered three times, so omega = 2 and First-Fit
    // spends 2*omega - 1 = 3 colours.
    std::vector<Interval> xs = {
        {0.0, 1.0, 0},
        {2.0, 3.0, 1},
        {1.5, 2.2, 2},
        {0.5, 1.7, 3},
    };
    return Instance{std::move(xs)};
}

namespace {

// One partial construction under consideration: the intervals presented so far,
// the algorithm's live state, and what it has spent.
struct Node {
    Instance inst;
    std::unique_ptr<OnlineColourer> state;
    std::vector<char> palette;  // palette[c] = the algorithm has used colour c
    int distinct = 0;
    int omega = 0;
    int tie = 0;  // random, to break ranking ties without biasing towards one branch
};

}  // namespace

Instance adversary(OnlineColourer& alg, int target_omega, int n_max, std::uint64_t seed,
                   int beam) {
    if (target_omega < 1) target_omega = 1;
    if (n_max < 1) n_max = 1;
    if (beam < 1) beam = 1;
    std::mt19937_64 rng(seed);

    // Beam search, not hill climbing. Forcing a colour beyond omega needs a run
    // of locally neutral preparatory moves -- intervals that buy nothing on
    // their own and only pay off several steps later. Greedy descent cannot hold
    // those, so it plateaus; a beam can carry several such lines at once.
    std::vector<Node> live;
    {
        Node root;
        alg.reset();
        root.state = alg.clone();
        live.push_back(std::move(root));
    }

    Instance best;
    double best_ratio = 0.0;

    const int kCandidates = 16;
    for (int step = 0; step < n_max; ++step) {
        std::vector<Node> children;
        children.reserve(live.size() * kCandidates);

        for (const Node& node : live) {
            std::vector<double> pts;
            pts.reserve(node.inst.size() * 2 + 1);
            double hi = 0.0;
            for (const Interval& iv : node.inst.arrival_order) {
                pts.push_back(iv.left);
                pts.push_back(iv.right);
                hi = std::max(hi, iv.right);
            }
            if (pts.empty()) pts.push_back(0.0);
            std::uniform_int_distribution<std::size_t> pick(0, pts.size() - 1);
            std::uniform_real_distribution<double> jitter(-0.37, 0.37);

            for (int c = 0; c < kCandidates; ++c) {
                double a, b;
                if (c == 0 || node.inst.size() == 0) {
                    a = hi + 1.0;  // a fresh slot, disjoint from everything so far
                    b = a + 1.0;
                } else {
                    a = pts[pick(rng)] + jitter(rng);
                    b = pts[pick(rng)] + jitter(rng);
                    if (a > b) std::swap(a, b);
                    if (a == b) b += 0.5;
                }

                Instance trial = node.inst;
                trial.arrival_order.push_back({a, b, static_cast<int>(trial.size())});
                const int omega = exact_chromatic_number(trial);
                if (omega > target_omega) continue;
                // Keep the construction inside the class the padding proof covers.
                if (!stub_friendly(trial)) continue;

                Node child;
                child.state = node.state->clone();
                const int colour = child.state->colour(trial.arrival_order.back(), {});
                child.palette = node.palette;
                if (colour >= static_cast<int>(child.palette.size()))
                    child.palette.resize(colour + 1, 0);
                child.distinct = node.distinct + (child.palette[colour] ? 0 : 1);
                child.palette[colour] = 1;
                child.omega = omega;
                child.tie = static_cast<int>(rng() & 0xffff);
                child.inst = std::move(trial);
                children.push_back(std::move(child));
            }
        }
        if (children.empty()) break;

        // Rank by colours spent, then by keeping omega down: a colour bought by
        // widening the maximum clique is not a colour worth buying.
        std::sort(children.begin(), children.end(), [](const Node& x, const Node& y) {
            if (x.distinct != y.distinct) return x.distinct > y.distinct;
            if (x.omega != y.omega) return x.omega < y.omega;
            return x.tie > y.tie;
        });
        if (static_cast<int>(children.size()) > beam) children.resize(beam);
        live = std::move(children);

        for (const Node& node : live) {
            const double ratio = static_cast<double>(node.distinct) / node.omega;
            if (ratio > best_ratio) {
                best_ratio = ratio;
                best = node.inst;
            }
        }
    }
    for (std::size_t i = 0; i < best.size(); ++i) best.arrival_order[i].id = static_cast<int>(i);
    return best;
}

Padded pad_weak(const Instance& inst, int k) {
    Padded out;
    double hi = 0.0;
    for (const Interval& iv : inst.arrival_order) hi = std::max(hi, iv.right);

    double park = hi + 2.0;
    for (const Interval& iv : inst.arrival_order) {
        out.instance.arrival_order.push_back(iv);
        out.is_padding.push_back(0);
        for (int j = 0; j < k; ++j) {
            out.instance.arrival_order.push_back({park, park + 1.0, 0});
            out.is_padding.push_back(1);
            park += 2.0;  // pairwise disjoint, so they never raise omega
        }
    }
    for (std::size_t i = 0; i < out.instance.size(); ++i)
        out.instance.arrival_order[i].id = static_cast<int>(i);
    out.leaks = count_leaks(out.instance, out.is_padding);
    return out;
}

Padded pad_strong(const Instance& inst, int k) {
    Padded out;
    const auto& xs = inst.arrival_order;

    for (std::size_t t = 0; t < xs.size(); ++t) {
        out.instance.arrival_order.push_back(xs[t]);
        out.is_padding.push_back(0);
        if (k <= 0) continue;

        const auto gap = largest_free_gap(inst, t);
        for (int j = 0; j < k; ++j) {
            double centre, half;
            if (gap.second > gap.first) {
                const double w = (gap.second - gap.first) / (k + 1);
                centre = gap.first + w * (j + 1);
                half = w / 4.0;  // width w/2 at spacing w, so stubs stay disjoint
            } else {
                // I_t is swallowed by its own future, so no leak-free stub
                // exists. Fall back to its midpoint; count_leaks records it.
                centre = 0.5 * (xs[t].left + xs[t].right);
                half = 0.0;
            }
            out.instance.arrival_order.push_back({centre - half, centre + half, 0});
            out.is_padding.push_back(1);
        }
    }
    for (std::size_t i = 0; i < out.instance.size(); ++i)
        out.instance.arrival_order[i].id = static_cast<int>(i);
    out.leaks = count_leaks(out.instance, out.is_padding);
    return out;
}

}  // namespace horizon
