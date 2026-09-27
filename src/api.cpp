#include "MonteCarloAPI.h"
#include "AsianOption.hpp"
#include "LookbackOption.hpp"
#include "MonteCarlo.hpp"
#include "Payoff.hpp"
#include "Random.hpp"
#include <atomic>

namespace
{
  std::atomic<uint64_t> g_seed{42};

  PricingResult toPricingResult(const MonteCarloResult &result)
  {
    return {result.price, result.stdError, result.variance};
  }

  void initRandom()
  {
    Random::init(g_seed.load(std::memory_order_relaxed));
  }
}

extern "C"
{

  MONTECARLO_API void monte_carlo_set_seed(uint64_t seed)
  {
    g_seed.store(seed, std::memory_order_relaxed);
  }

  MONTECARLO_API int price_european_call(double spot, double strike, double vol,
                                         double rate, double maturity,
                                         uint64_t num_paths,
                                         PricingResult *out_result)
  {
    if (!out_result)
      return -1;

    initRandom();
    PayoffCall payoff(strike);
    MonteCarloPricer pricer(spot, vol, rate, maturity);
    const auto result = pricer.price(payoff, static_cast<size_t>(num_paths));
    *out_result = toPricingResult(result);
    return 0;
  }

  MONTECARLO_API int price_european_put(double spot, double strike, double vol,
                                        double rate, double maturity,
                                        uint64_t num_paths,
                                        PricingResult *out_result)
  {
    if (!out_result)
      return -1;

    initRandom();
    PayoffPut payoff(strike);
    MonteCarloPricer pricer(spot, vol, rate, maturity);
    const auto result = pricer.price(payoff, static_cast<size_t>(num_paths));
    *out_result = toPricingResult(result);
    return 0;
  }

  MONTECARLO_API int price_asian_call(double spot, double strike, double vol,
                                      double rate, double maturity,
                                      uint64_t num_paths, uint64_t num_steps,
                                      PricingResult *out_result)
  {
    if (!out_result)
      return -1;

    initRandom();
    AsianOption payoff(strike);
    MonteCarloPricer pricer(spot, vol, rate, maturity);
    const auto result = pricer.pricePathDependent(
        payoff, static_cast<size_t>(num_paths), static_cast<size_t>(num_steps));
    *out_result = toPricingResult(result);
    return 0;
  }

  MONTECARLO_API int price_lookback_call(double spot, double vol, double rate,
                                         double maturity, uint64_t num_paths,
                                         uint64_t num_steps,
                                         PricingResult *out_result)
  {
    if (!out_result)
      return -1;

    initRandom();
    LookbackOption payoff;
    MonteCarloPricer pricer(spot, vol, rate, maturity);
    const auto result = pricer.pricePathDependent(
        payoff, static_cast<size_t>(num_paths), static_cast<size_t>(num_steps));
    *out_result = toPricingResult(result);
    return 0;
  }
}
