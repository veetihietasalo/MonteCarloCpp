#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
#ifdef MONTECARLO_API_EXPORTS
#define MONTECARLO_API __declspec(dllexport)
#else
#define MONTECARLO_API __declspec(dllimport)
#endif
#else
#define MONTECARLO_API
#endif

  typedef struct PricingResult
  {
    double price;
    double std_error;
    double variance;
  } PricingResult;

  // Configure RNG seed for deterministic runs
  MONTECARLO_API void monte_carlo_set_seed(uint64_t seed);

  // European vanilla option pricing
  MONTECARLO_API int price_european_call(double spot, double strike, double vol,
                                         double rate, double maturity,
                                         uint64_t num_paths,
                                         PricingResult *out_result);

  MONTECARLO_API int price_european_put(double spot, double strike, double vol,
                                        double rate, double maturity,
                                        uint64_t num_paths,
                                        PricingResult *out_result);

  // Path-dependent payoffs
  MONTECARLO_API int price_asian_call(double spot, double strike, double vol,
                                      double rate, double maturity,
                                      uint64_t num_paths, uint64_t num_steps,
                                      PricingResult *out_result);

  MONTECARLO_API int price_lookback_call(double spot, double vol, double rate,
                                         double maturity, uint64_t num_paths,
                                         uint64_t num_steps,
                                         PricingResult *out_result);

#ifdef __cplusplus
}
#endif
