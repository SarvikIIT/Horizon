#include "offline.hpp"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace horizon {
namespace {

// (coordinate, delta). Starts sort before ends at an equal coordinate because
// intervals are closed: [0,1] and [1,2] overlap at the point 1.
struct Event {
    double x;
    int delta;
    bool operator<(const Event& o) const {
        if (x != o.x) return x < o.x;
        return delta > o.delta;  // +1 before -1
    }
};

}  // namespace

int exact_chromatic_number(const Instance& inst) {
    if (inst.arrival_order.empty()) return 0;
    std::vector<Event> ev;
    ev.reserve(inst.arrival_order.size() * 2);
    for (const Interval& iv : inst.arrival_order) {
        ev.push_back({iv.left, +1});
        ev.push_back({iv.right, -1});
    }
    std::sort(ev.begin(), ev.end());
    int cur = 0, best = 0;
    for (const Event& e : ev) {
        cur += e.delta;
        best = std::max(best, cur);
    }
    return best;
}

int brute_force_chromatic_number(const Instance& inst) {
    const int n = static_cast<int>(inst.arrival_order.size());
    if (n == 0) return 0;

    std::vector<std::vector<char>> adj(n, std::vector<char>(n, 0));
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            adj[i][j] = (i != j && overlaps(inst.arrival_order[i], inst.arrival_order[j]));

    std::vector<int> col(n, -1);
    // Try k = 1, 2, ... and backtrack. Symmetry break: vertex i may only open a
    // colour index one higher than the highest used so far.
    for (int k = 1; k <= n; ++k) {
        auto solve = [&](auto&& self, int v, int used) -> bool {
            if (v == n) return true;
            const int limit = std::min(used + 1, k);
            for (int c = 0; c < limit; ++c) {
                bool ok = true;
                for (int u = 0; u < v && ok; ++u)
                    if (adj[v][u] && col[u] == c) ok = false;
                if (!ok) continue;
                col[v] = c;
                if (self(self, v + 1, std::max(used, c + 1))) return true;
                col[v] = -1;
            }
            return false;
        };
        if (solve(solve, 0, 0)) return k;
    }
    return n;
}

bool is_valid_colouring(const Instance& inst, const Colouring& col) {
    const auto& xs = inst.arrival_order;
    if (col.size() != xs.size()) return false;
    for (int c : col)
        if (c < 0) return false;

    struct Ev {
        double x;
        int delta;
        int colour;
        bool operator<(const Ev& o) const {
            if (x != o.x) return x < o.x;
            return delta > o.delta;
        }
    };
    std::vector<Ev> ev;
    ev.reserve(xs.size() * 2);
    for (std::size_t i = 0; i < xs.size(); ++i) {
        ev.push_back({xs[i].left, +1, col[i]});
        ev.push_back({xs[i].right, -1, col[i]});
    }
    std::sort(ev.begin(), ev.end());

    std::unordered_map<int, int> active;
    for (const Ev& e : ev) {
        if (e.delta == +1) {
            if (++active[e.colour] > 1) return false;  // two live intervals share a colour
        } else {
            if (--active[e.colour] == 0) active.erase(e.colour);
        }
    }
    return true;
}

int colours_used(const Colouring& col) {
    std::unordered_set<int> s(col.begin(), col.end());
    return static_cast<int>(s.size());
}

}  // namespace horizon
