#undef NDEBUG // keep assert() active in every build type, Release included
#include "AsianOption.hpp"
#include "MonteCarlo.hpp"
#include "Payoff.hpp"
#include "Random.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>

// The concept accepts anything that maps a spot price to a payoff...
static_assert(EuropeanPayoff<PayoffCall>);
static_assert(EuropeanPayoff<Payoff>);
static_assert(EuropeanPayoff<decltype([](double s) { return std::max(s - 100.0, 0.0); })>);
// ...and rejects everything else, at compile time.
static_assert(!EuropeanPayoff<AsianOption>); // needs a whole path, not one spot
static_assert(!EuropeanPayoff<int>);

int main()
{
  MonteCarloPricer pricer(100.0, 0.2, 0.05, 1.0);
  PayoffCall call(100.0);
  PayoffPut put(100.0);
  const Payoff& callBase = call;
  const Payoff& putBase = put;
  const auto callLambda = [](double s) { return std::max(s - 100.0, 0.0); };

  // Same seed, same loop: the virtual and the inlined call must give bit-identical results.
  Random::init(42);
  const MonteCarloResult viaVirtual = pricer.price(callBase, 200'000);
  Random::init(42);
  const MonteCarloResult viaTemplate = pricer.price(call, 200'000);
  Random::init(42);
  const MonteCarloResult viaLambda = pricer.price(callLambda, 200'000);
  assert(viaVirtual.price == viaTemplate.price && viaVirtual.stdError == viaTemplate.stdError);
  assert(viaTemplate.price == viaLambda.price);

  Random::init(42);
  const MonteCarloResult putVirtual = pricer.price(putBase, 200'000);
  Random::init(42);
  const MonteCarloResult putTemplate = pricer.price(put, 200'000);
  assert(putVirtual.price == putTemplate.price);

  // Sanity: close to Black-Scholes (call 10.4506, put 5.5735 for these inputs).
  assert(std::abs(viaTemplate.price - 10.4506) < 4 * viaTemplate.stdError);
  assert(std::abs(putTemplate.price - 5.5735) < 4 * putTemplate.stdError);

  std::cout << "All dispatch tests passed." << std::endl;
  return 0;
}
