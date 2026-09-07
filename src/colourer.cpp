#include "colourer.hpp"

#include <stdexcept>

#include "offline.hpp"

namespace horizon {

std::vector<Interval> lookahead_buffer(const Instance& inst, std::size_t t, int k,
                                       Lookahead model) {
    std::vector<Interval> buf;
    if (k <= 0) return buf;
    const auto& xs = inst.arrival_order;
    if (model == Lookahead::Weak) {
        for (std::size_t j = t + 1; j < xs.size() && buf.size() < static_cast<std::size_t>(k); ++j)
            buf.push_back(xs[j]);
        return buf;
    }
    // ponytail: linear scan forward, O(n) per step. Fine to n ~ 1e4; an
    // interval tree over the suffix would be the upgrade if n gets large.
    for (std::size_t j = t + 1; j < xs.size() && buf.size() < static_cast<std::size_t>(k); ++j)
        if (overlaps(xs[t], xs[j])) buf.push_back(xs[j]);
    return buf;
}

RunResult run(const Instance& inst, OnlineColourer& alg, int k, Lookahead model) {
    for (std::size_t t = 0; t < inst.size(); ++t)
        if (inst.arrival_order[t].id != static_cast<int>(t))
            throw std::logic_error("run(): Interval::id must equal the arrival position");

    alg.reset();
    RunResult r;
    r.colouring.resize(inst.size());
    for (std::size_t t = 0; t < inst.size(); ++t) {
        std::vector<Interval> buf = lookahead_buffer(inst, t, k, model);
        const int c = alg.colour(inst.arrival_order[t], buf);
        if (c < 0) throw std::logic_error(alg.name() + ": negative colour");
        r.colouring[t] = c;
    }
    if (!is_valid_colouring(inst, r.colouring))
        throw std::logic_error(alg.name() + ": produced an invalid colouring");

    r.colours_used = colours_used(r.colouring);
    r.omega = exact_chromatic_number(inst);
    r.ratio = r.omega > 0 ? static_cast<double>(r.colours_used) / r.omega : 0.0;
    return r;
}

}  // namespace horizon
