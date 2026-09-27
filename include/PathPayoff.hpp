#pragma once
#include <vector>
#include <string>

// Abstract base class for Path-Dependent Option Payoff
class PathPayoff {
public:
    virtual ~PathPayoff() = default;
    // Takes a vector of spot prices representing the path
    virtual double operator()(const std::vector<double>& path) const = 0;
    virtual std::string name() const = 0;
};
