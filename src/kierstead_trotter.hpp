#pragma once
#include "colourer.hpp"

namespace horizon {

// Kierstead-Trotter (1981). Optimal for the zero-lookahead model: uses at most
// 3*omega - 2 colours, and no online algorithm does better in the worst case.
// Ignores the buffer. Second baseline.
//
//   level(v) = min { j >= 1 : omega(G_{1,j}, v) <= j }
// where omega(G_{1,j}, v) is the largest clique containing v among v plus the
// already-arrived intervals of level <= j. Within its level, v is coloured by
// First-Fit over a palette private to that level: 1 colour for level 1, and 3
// colours for every level j >= 2.
class KiersteadTrotter : public OnlineColourer {
public:
    void reset() override;
    int colour(const Interval& current, std::span<const Interval> buffer) override;
    std::string name() const override { return "kierstead_trotter"; }
    std::unique_ptr<OnlineColourer> clone() const override {
        return std::make_unique<KiersteadTrotter>(*this);
    }

private:
    // First colour index of level j's private palette.
    static int palette_offset(int level) { return level <= 1 ? 0 : 1 + 3 * (level - 2); }

    int level_of(const Interval& v) const;

    std::vector<Interval> seen_;
    std::vector<int> lvl_;
    std::vector<int> col_;
    int max_level_ = 0;
};

}  // namespace horizon
