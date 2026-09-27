#pragma once
#include "PathPayoff.hpp"
#include <algorithm>

// Lookback Option (Floating Strike Call)
// Payoff = S_T - S_min
class LookbackOption : public PathPayoff {
public:
    double operator()(const std::vector<double>& path) const override {
        if (path.empty()) return 0.0;

        double spotT = path.back();
        double minSpot = *std::min_element(path.begin(), path.end());

        // Floating Strike Call: Payoff is Spot at maturity minus minimum spot during life
        // This allows buying at the absolute lowest point
        return std::max(spotT - minSpot, 0.0);
    }

    std::string name() const override { return "Lookback Call (Floating Strike)"; }
};
