#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "first_fit.hpp"
#include "generators.hpp"
#include "kierstead_trotter.hpp"
#include "lookahead.hpp"
#include "offline.hpp"

using namespace horizon;

namespace {

std::unique_ptr<OnlineColourer> make_algo(const std::string& name) {
    if (name == "first_fit") return std::make_unique<FirstFit>();
    if (name == "kierstead_trotter") return std::make_unique<KiersteadTrotter>();
    if (name == "lookahead") return std::make_unique<LookaheadColourer>();
    return nullptr;
}

void usage() {
    std::cerr
        << "usage: bench [options]\n"
           "  --algo=first_fit|kierstead_trotter|lookahead   (default first_fit)\n"
           "  --gen=random_uniform|random_clustered|adversary|ff_worst\n"
           "                        (default random_uniform)\n"
           "  --model=strong|weak   lookahead model (default strong)\n"
           "  --pad=none|weak|strong\n"
           "                        apply the lower-bound padding transform (default none)\n"
           "  --n=INT               intervals per trial; search budget for the\n"
           "                        adversary (default 1000)\n"
           "  --k=INT               lookahead size (default 0)\n"
           "  --trials=INT          (default 100)\n"
           "  --seed=INT            (default 42)\n"
           "  --clusters=INT        random_clustered only (default 8)\n"
           "  --omega=INT           adversary clique cap (default 4)\n"
           "  --beam=INT            adversary beam width (default 6)\n"
           "  --out=PATH            write the CSV here as well as to stdout\n";
}

}  // namespace

int main(int argc, char** argv) {
    std::map<std::string, std::string> opt = {
        {"algo", "first_fit"}, {"gen", "random_uniform"}, {"model", "strong"},
        {"pad", "none"},       {"n", "1000"},             {"k", "0"},
        {"trials", "100"},     {"seed", "42"},            {"clusters", "8"},
        {"omega", "4"},        {"beam", "6"},             {"out", ""},
    };
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        const auto eq = a.find('=');
        if (a.rfind("--", 0) != 0 || eq == std::string::npos) {
            usage();
            return 2;
        }
        const std::string key = a.substr(2, eq - 2);
        if (!opt.count(key)) {
            std::cerr << "unknown option: " << key << "\n";
            usage();
            return 2;
        }
        opt[key] = a.substr(eq + 1);
    }

    auto alg = make_algo(opt["algo"]);
    if (!alg) {
        std::cerr << "unknown algo: " << opt["algo"] << "\n";
        usage();
        return 2;
    }

    const Lookahead model = opt["model"] == "weak" ? Lookahead::Weak : Lookahead::Strong;
    const int n = std::atoi(opt["n"].c_str());
    const int k = std::atoi(opt["k"].c_str());
    const int trials = std::atoi(opt["trials"].c_str());
    const auto seed = std::strtoull(opt["seed"].c_str(), nullptr, 10);
    const int clusters = std::atoi(opt["clusters"].c_str());
    const int target_omega = std::atoi(opt["omega"].c_str());
    const int beam = std::atoi(opt["beam"].c_str());
    const std::string& gen = opt["gen"];
    const std::string& pad = opt["pad"];

    std::ofstream fout;
    if (!opt["out"].empty()) {
        fout.open(opt["out"]);
        if (!fout) {
            std::cerr << "cannot write " << opt["out"] << "\n";
            return 1;
        }
    }
    // colours_original / omega_base are the quantities the lower-bound proof
    // talks about: what the algorithm spends on the intervals of the underlying
    // hard instance, against that instance's own optimum. They are what must
    // stay flat as k grows if the padding transform does its job.
    const std::string header =
        "algo,gen,pad,model,k,trial,n,omega,colours_used,ratio,omega_base,colours_original,leaks";
    std::cout << header << "\n";
    if (fout) fout << header << "\n";

    double worst = 0.0, sum = 0.0;
    int done = 0, total_leaks = 0;
    for (int t = 0; t < trials; ++t) {
        Instance inst;
        if (gen == "random_uniform") {
            inst = random_uniform(n, seed + t);
        } else if (gen == "random_clustered") {
            inst = random_clustered(n, clusters, seed + t);
        } else if (gen == "adversary") {
            inst = adversary(*alg, target_omega, n, seed + t, beam);
        } else if (gen == "ff_worst") {
            inst = first_fit_worst_case_omega2();
        } else {
            std::cerr << "unknown gen: " << gen << "\n";
            usage();
            return 2;
        }

        int leaks = 0;
        const int omega_base = exact_chromatic_number(inst);
        std::vector<char> is_padding(inst.size(), 0);
        if (pad == "weak") {
            Padded p = pad_weak(inst, k);
            inst = std::move(p.instance);
            is_padding = std::move(p.is_padding);
            leaks = p.leaks;
        } else if (pad == "strong") {
            Padded p = pad_strong(inst, k);
            inst = std::move(p.instance);
            is_padding = std::move(p.is_padding);
            leaks = p.leaks;
        } else if (pad != "none") {
            std::cerr << "unknown pad: " << pad << "\n";
            usage();
            return 2;
        }
        total_leaks += leaks;

        RunResult r;
        try {
            r = run(inst, *alg, k, model);
        } catch (const std::exception& e) {
            std::cerr << "FAILED trial " << t << ": " << e.what() << "\n";
            return 1;
        }

        Colouring originals;
        for (std::size_t i = 0; i < r.colouring.size(); ++i)
            if (!is_padding[i]) originals.push_back(r.colouring[i]);

        const std::string row =
            opt["algo"] + "," + gen + "," + pad + "," + opt["model"] + "," + std::to_string(k) +
            "," + std::to_string(t) + "," + std::to_string(inst.size()) + "," +
            std::to_string(r.omega) + "," + std::to_string(r.colours_used) + "," +
            std::to_string(r.ratio) + "," + std::to_string(omega_base) + "," +
            std::to_string(colours_used(originals)) + "," + std::to_string(leaks);
        std::cout << row << "\n";
        if (fout) fout << row << "\n";

        worst = std::max(worst, r.ratio);
        sum += r.ratio;
        ++done;
        if (gen == "ff_worst") break;  // deterministic, one trial is all there is
    }

    // Competitive analysis is worst case. The mean is printed for context only.
    std::cerr << "max_ratio=" << worst << " mean_ratio=" << (done ? sum / done : 0.0)
              << " trials=" << done << " padding_leaks=" << total_leaks << "\n";
    return 0;
}
