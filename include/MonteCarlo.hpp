#pragma once
#include "PathPayoff.hpp"
#include "Payoff.hpp"
#include "Statistics.hpp"
#include <memory>

struct MonteCarloResult {
  double price;
  double stdError;
  double variance;
};

class MonteCarloPricer {
public:
  MonteCarloPricer(double spot, double vol, double rate, double time);

  // Run simulation for a given payoff
  MonteCarloResult price(const Payoff &payoff, size_t numPaths);

  // Run simulation for path-dependent payoff
  MonteCarloResult pricePathDependent(const PathPayoff &payoff, size_t numPaths,
                                      size_t numSteps);

  // Run parallel simulation for a given payoff
  MonteCarloResult priceParallel(const Payoff &payoff, size_t numPaths);

  // Run GPU simulation for a given payoff
  MonteCarloResult priceCuda(const Payoff &payoff, size_t numPaths);

private:
  double m_spot;
  double m_vol;
  double m_rate;
  double m_time;
};
