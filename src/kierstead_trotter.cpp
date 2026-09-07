#include "kierstead_trotter.hpp"

#include <algorithm>
#include <stdexcept>

namespace horizon {
namespace {

struct Ev {
    double x;
    int delta;
    int level;
    bool operator<(const Ev& o) const {
        if (x != o.x) return x < o.x;
        return delta > o.delta;  // closed intervals: opens before closes
    }
};

}  // namespace

void KiersteadTrotter::reset() {
    seen_.clear();
    lvl_.clear();
    col_.clear();
    max_level_ = 0;
}

// best[j] = max over points p in v of the number of already-arrived intervals
// of level <= j covering p. Returns min { j : best[j] + 1 <= j }.
int KiersteadTrotter::level_of(const Interval& v) const {
    const int levels = max_level_ + 1;  // index 1..levels
    std::vector<Ev> ev;
    ev.reserve(seen_.size() * 2);
    for (std::size_t i = 0; i < seen_.size(); ++i) {
        if (!overlaps(seen_[i], v)) continue;
        const double l = std::max(v.left, seen_[i].left);
        const double r = std::min(v.right, seen_[i].right);
        ev.push_back({l, +1, lvl_[i]});
        ev.push_back({r, -1, lvl_[i]});
    }
    std::sort(ev.begin(), ev.end());

    std::vector<int> cnt(levels + 2, 0);   // cnt[j] = live intervals of level exactly j
    std::vector<int> best(levels + 2, 0);  // best[j] over prefix levels 1..j
    for (std::size_t i = 0; i < ev.size();) {
        const double x = ev[i].x;
        while (i < ev.size() && ev[i].x == x && ev[i].delta == +1) cnt[ev[i++].level] += 1;
        int prefix = 0;
        for (int j = 1; j <= levels; ++j) {
            prefix += cnt[j];
            best[j] = std::max(best[j], prefix);
        }
        while (i < ev.size() && ev[i].x == x) cnt[ev[i++].level] -= 1;
    }

    for (int j = 1; j <= levels; ++j)
        if (best[j] + 1 <= j) return j;
    // best[levels] + 1 <= levels + 1 always holds, since every counted interval
    // has level <= max_level_ and they all pairwise overlap at the witness point.
    return levels + 1;
}

int KiersteadTrotter::colour(const Interval& current, std::span<const Interval>) {
    const int level = level_of(current);
    max_level_ = std::max(max_level_, level);

    const int width = (level == 1) ? 1 : 3;
    std::vector<char> used(static_cast<std::size_t>(width) + 1, 0);
    for (std::size_t i = 0; i < seen_.size(); ++i) {
        if (lvl_[i] != level || !overlaps(seen_[i], current)) continue;
        const int local = col_[i] - palette_offset(level);
        if (local >= 0 && local < static_cast<int>(used.size())) used[local] = 1;
    }

    int local = 0;
    while (local < static_cast<int>(used.size()) && used[local]) ++local;
    if (local >= width) {
        // The 3*omega-2 bound rests on exactly this never happening. If it
        // fires, the implementation or the understood algorithm is wrong.
        throw std::logic_error("kierstead_trotter: level " + std::to_string(level) +
                               " needed more than " + std::to_string(width) + " colours");
    }

    const int c = palette_offset(level) + local;
    seen_.push_back(current);
    lvl_.push_back(level);
    col_.push_back(c);
    return c;
}

}  // namespace horizon
