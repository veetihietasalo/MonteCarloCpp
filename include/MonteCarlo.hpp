#pragma once
#include "PathPayoff.hpp"
#include "Payoff.hpp"
#include "Random.hpp"
#include "Statistics.hpp"
#include <cmath>
#include <concepts>
#include <memory>

struct MonteCarloResult {
  double price;
  double stdError;
  double variance;
};

// Anything that turns the spot price at expiry into a payoff: a Payoff subclass, any
// struct with a matching operator(), or a lambda.
template <typename P>
concept EuropeanPayoff = requires(const P &payoff, double spot) {
  { payoff(spot) } -> std::convertible_to<double>;
};

class MonteCarloPricer {
public:
  MonteCarloPricer(double spot, double vol, double rate, double time);

  // Run simulation for a given payoff.
  //
  // Called with a concrete payoff type (PayoffCall, a lambda, ...), the template overload
  // runs: the payoff call is resolved at compile time and inlined into the path loop.
  // Called through a `const Payoff &`, the non-template overload runs the same loop with
  // a virtual call per path.
  MonteCarloResult price(const Payoff &payoff, size_t numPaths) const;

  template <EuropeanPayoff P>
  MonteCarloResult price(const P &payoff, size_t numPaths) const;

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

template <EuropeanPayoff P>
MonteCarloResult MonteCarloPricer::price(const P &payoff, size_t numPaths) const
{
  Statistics stats;

  // Pre-calculate constants
  const double drift = (m_rate - 0.5 * m_vol * m_vol) * m_time;
  const double diffusion = m_vol * std::sqrt(m_time);

  for (size_t i = 0; i < numPaths; ++i)
  {
    const double z = Random::getNormal();
    const double spotT = m_spot * std::exp(drift + diffusion * z);
    stats.add(payoff(spotT));
  }

  const double discount = std::exp(-m_rate * m_time);
  return {discount * stats.mean(), discount * stats.stdError(), stats.variance()};
}
