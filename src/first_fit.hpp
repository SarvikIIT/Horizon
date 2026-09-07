#pragma once
#include "colourer.hpp"

namespace horizon {

// Lowest colour index not used by an already-coloured overlapping interval.
// Ignores the buffer. Baseline.
class FirstFit : public OnlineColourer {
public:
    void reset() override;
    int colour(const Interval& current, std::span<const Interval> buffer) override;
    std::string name() const override { return "first_fit"; }
    std::unique_ptr<OnlineColourer> clone() const override {
        return std::make_unique<FirstFit>(*this);
    }

private:
    std::vector<Interval> seen_;
    std::vector<int> col_;
};

}  // namespace horizon
