// Exhaustive solver for the online interval-colouring game.
//
// The adversary presents intervals on a grid of `cells` elementary cells; the
// algorithm colours them from a palette of `colours`, keeping every cell covered
// at most `omega` times. The adversary wins if it can drive the algorithm into a
// position where no colour in the palette is legal, which proves that *every*
// online algorithm needs at least colours+1 on that grid.
//
// Two things make this tractable. First, the whole future depends on the
// presented intervals only through, for each cell, the set of colours living
// there -- so the state is one bitmask per cell, not a list of intervals.
// Second, the game is symmetric under renaming colours, so states are
// canonicalised by first appearance before memoising.
//
// Restricting the adversary to a finite grid only ever makes it weaker, so a
// win here is a genuine lower bound on the unrestricted game.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

struct Params {
    int cells = 6;
    int omega = 2;
    int colours = 3;
    int lookahead = 0;  // weak model: the algorithm sees the next k arrivals
    int moves = 8;
    long long budget = 40'000'000;
};

struct Solver {
    Params p;
    long long nodes = 0;
    bool exhausted = false;
    std::unordered_map<std::string, char> memo;

    std::vector<std::uint32_t> mask;               // cell -> set of colours present
    std::vector<std::pair<int, int>> pending;      // revealed, not yet coloured

    explicit Solver(Params par) : p(par), mask(par.cells, 0) {}

    int pending_cover(int cell) const {
        int n = 0;
        for (const auto& iv : pending)
            if (iv.first <= cell && cell <= iv.second) ++n;
        return n;
    }

    bool may_present(int a, int b) const {
        for (int c = a; c <= b; ++c)
            if (__builtin_popcount(mask[c]) + pending_cover(c) + 1 > p.omega) return false;
        return true;
    }

    // Colours renamed by order of first appearance, so that two states that
    // differ only in the names of colours hash to the same key.
    std::string key(int moves_left) const {
        int rename[32];
        std::fill(rename, rename + 32, -1);
        int next = 0;
        std::string s;
        s.reserve(mask.size() + pending.size() * 2 + 2);
        for (std::uint32_t m : mask) {
            std::uint32_t out = 0;
            for (int bit = 0; bit < p.colours; ++bit) {
                if (!(m >> bit & 1)) continue;
                if (rename[bit] < 0) rename[bit] = next++;
                out |= 1u << rename[bit];
            }
            s.push_back(static_cast<char>(33 + out));
        }
        s.push_back('|');
        for (const auto& iv : pending) {
            s.push_back(static_cast<char>(33 + iv.first));
            s.push_back(static_cast<char>(33 + iv.second));
        }
        s.push_back('|');
        s.push_back(static_cast<char>(33 + moves_left));
        return s;
    }

    // True iff the adversary, to move, can force the algorithm to run out of
    // colours within `moves_left` further reveals.
    bool adversary_wins(int moves_left) {
        if (++nodes > p.budget) {
            exhausted = true;
            return false;
        }
        const std::string k = key(moves_left);
        if (const auto it = memo.find(k); it != memo.end()) return it->second;

        bool result = false;
        const bool buffer_full = static_cast<int>(pending.size()) >= p.lookahead + 1;

        if (!buffer_full && moves_left > 0) {
            // The adversary must keep the buffer topped up before the algorithm
            // is asked to commit.
            for (int a = 0; a < p.cells && !result; ++a) {
                for (int b = a; b < p.cells && !result; ++b) {
                    if (!may_present(a, b)) continue;
                    pending.push_back({a, b});
                    result = adversary_wins(moves_left - 1);
                    pending.pop_back();
                }
            }
        } else if (!pending.empty()) {
            const auto iv = pending.front();
            std::uint32_t blocked = 0;
            for (int c = iv.first; c <= iv.second; ++c) blocked |= mask[c];

            const std::uint32_t all = (p.colours >= 32) ? ~0u : ((1u << p.colours) - 1);
            const std::uint32_t legal = all & ~blocked;
            if (legal == 0) {
                result = true;  // the algorithm is stuck: it needs colour p.colours + 1
            } else {
                result = true;  // adversary wins only if every legal reply still loses
                for (int c = 0; c < p.colours && result; ++c) {
                    if (!(legal >> c & 1)) continue;
                    pending.erase(pending.begin());
                    for (int x = iv.first; x <= iv.second; ++x) mask[x] |= 1u << c;

                    const bool sub = adversary_wins(moves_left);

                    for (int x = iv.first; x <= iv.second; ++x) mask[x] &= ~(1u << c);
                    pending.insert(pending.begin(), iv);
                    if (!sub) result = false;
                }
            }
        }

        memo.emplace(k, static_cast<char>(result));
        return result;
    }
};

void usage() {
    std::fprintf(stderr,
                 "usage: solve [options]\n"
                 "  --cells=INT      grid resolution (default 6)\n"
                 "  --omega=INT      clique cap (default 2)\n"
                 "  --colours=INT    palette the algorithm is allowed (default 3)\n"
                 "  --k=INT          weak lookahead size (default 0)\n"
                 "  --moves=INT      intervals the adversary may present (default 8)\n"
                 "  --budget=INT     node cap before giving up (default 40000000)\n"
                 "\n"
                 "Prints whether the adversary can force an algorithm restricted to\n"
                 "`colours` colours to fail, i.e. whether every online algorithm needs\n"
                 "at least colours+1 on this grid.\n");
}

}  // namespace

int main(int argc, char** argv) {
    Params p;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        const auto eq = a.find('=');
        if (a.rfind("--", 0) != 0 || eq == std::string::npos) {
            usage();
            return 2;
        }
        const std::string key = a.substr(2, eq - 2);
        const long long v = std::atoll(a.c_str() + eq + 1);
        if (key == "cells") p.cells = static_cast<int>(v);
        else if (key == "omega") p.omega = static_cast<int>(v);
        else if (key == "colours") p.colours = static_cast<int>(v);
        else if (key == "k") p.lookahead = static_cast<int>(v);
        else if (key == "moves") p.moves = static_cast<int>(v);
        else if (key == "budget") p.budget = v;
        else {
            usage();
            return 2;
        }
    }
    if (p.colours > 30 || p.cells > 60) {
        std::fprintf(stderr, "parameters out of range\n");
        return 2;
    }

    Solver s(p);
    const bool win = s.adversary_wins(p.moves);

    std::printf("cells=%d omega=%d palette=%d k=%d moves=%d -> ", p.cells, p.omega, p.colours,
                p.lookahead, p.moves);
    if (win)
        std::printf("FORCED: every online algorithm needs >= %d colours\n", p.colours + 1);
    else if (s.exhausted)
        std::printf("INCONCLUSIVE (node budget exhausted)\n");
    else
        std::printf("survivable: some algorithm holds out with %d colours\n", p.colours);
    std::printf("  nodes=%lld states=%zu\n", s.nodes, s.memo.size());
    return win ? 0 : 1;
}
