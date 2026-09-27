#pragma once
#include "PathPayoff.hpp"
#include <numeric>
#include <algorithm>

// Asian Call Option (Arithmetic Average)
class AsianOption : public PathPayoff {
public:
    AsianOption(double strike) : m_strike(strike) {}

    double operator()(const std::vector<double>& path) const override {
        if (path.empty()) return 0.0;

        // Calculate arithmetic mean of the path
        double sum = std::accumulate(path.begin(), path.end(), 0.0);
        double average = sum / path.size();

        // Payoff = max(Average - Strike, 0)
        return std::max(average - m_strike, 0.0);
    }

    std::string name() const override { return "Asian Call (Arithmetic)"; }

private:
    double m_strike;
};
