// Cost of the payoff call: a virtual call through `const Payoff &` against the same payoff
// resolved at compile time through the EuropeanPayoff concept.
//
//   1. Payoff only: payoff(spot) over 20M pre-generated spot prices, written to an output
//      array. Nothing else is in the loop, so this isolates the call.
//   2. Whole pricer: MonteCarloPricer::price() for 5M paths, same seed. The random number
//      and exp() per path are included.
//
// Each figure is the median of 5 runs. Usage: bench_dispatch

#include "MonteCarlo.hpp"
#include "Payoff.hpp"
#include "Random.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <functional>
#include <vector>

namespace {

volatile bool g_pickPut = false; // chosen at run time, so the compiler can't tell which payoff it is

template <typename Fn>
double medianNs(Fn&& fn, int reps = 5) {
  std::vector<double> times;
  for (int r = 0; r < reps; ++r) {
    const auto t0 = std::chrono::steady_clock::now();
    fn();
    times.push_back(std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - t0).count());
  }
  std::sort(times.begin(), times.end());
  return times[times.size() / 2];
}

// out[i] = payoff(spots[i]). No reduction, so the compiler is free to vectorize when it can
// see the payoff's body.
template <typename P>
void applyPayoff(const P& payoff, const std::vector<double>& spots, std::vector<double>& out) {
  for (size_t i = 0; i < spots.size(); ++i) out[i] = payoff(spots[i]);
}

double checksum(const std::vector<double>& v) {
  double s = 0.0;
  for (double x : v) s += x;
  return s;
}

} // namespace

int main() {
  constexpr double kStrike = 100.0;
  PayoffCall call(kStrike);
  PayoffPut put(kStrike);
  const Payoff& base = g_pickPut ? static_cast<const Payoff&>(put) : static_cast<const Payoff&>(call);
  const auto lambda = [kStrike](double spot) { return std::max(spot - kStrike, 0.0); };

  // ---- 1. payoff only ----
  constexpr size_t kSpots = 20'000'000;
  std::vector<double> spots(kSpots);
  Random::init(7);
  for (double& s : spots) s = 100.0 * std::exp(0.2 * Random::getNormal());
  std::vector<double> out(kSpots);

  const double virtualNs = medianNs([&] { applyPayoff(base, spots, out); }) / kSpots;
  const double virtualSum = checksum(out);
  const double finalNs = medianNs([&] { applyPayoff(call, spots, out); }) / kSpots;
  const double finalSum = checksum(out);
  const double lambdaNs = medianNs([&] { applyPayoff(lambda, spots, out); }) / kSpots;
  const double lambdaSum = checksum(out);

  std::printf("Payoff only: out[i] = payoff(spot[i]) over %zu spots, median of 5\n", kSpots);
  std::printf("  %-40s %6.2f ns per call\n", "virtual, through const Payoff&", virtualNs);
  std::printf("  %-40s %6.2f ns per call  (%.1fx)\n", "template, PayoffCall (final)", finalNs, virtualNs / finalNs);
  std::printf("  %-40s %6.2f ns per call  (%.1fx)\n", "template, lambda", lambdaNs, virtualNs / lambdaNs);

  // ---- 2. whole pricer ----
  constexpr size_t kPaths = 5'000'000;
  MonteCarloPricer pricer(100.0, 0.2, 0.05, 1.0);
  MonteCarloResult rVirtual{}, rFinal{}, rLambda{};
  const double pVirtual = medianNs([&] { Random::init(42); rVirtual = pricer.price(base, kPaths); }) / kPaths;
  const double pFinal = medianNs([&] { Random::init(42); rFinal = pricer.price(call, kPaths); }) / kPaths;
  const double pLambda = medianNs([&] { Random::init(42); rLambda = pricer.price(lambda, kPaths); }) / kPaths;

  std::printf("\nWhole pricer: MonteCarloPricer::price, %zu paths, same seed, median of 5\n", kPaths);
  std::printf("  %-40s %6.2f ns per path  price %.6f\n", "virtual, through const Payoff&", pVirtual, rVirtual.price);
  std::printf("  %-40s %6.2f ns per path  price %.6f  (%.2fx)\n", "template, PayoffCall (final)", pFinal, rFinal.price,
              pVirtual / pFinal);
  std::printf("  %-40s %6.2f ns per path  price %.6f  (%.2fx)\n", "template, lambda", pLambda, rLambda.price,
              pVirtual / pLambda);

  // Same inputs must give the same answers, whichever way the payoff is called.
  const bool same = virtualSum == finalSum && finalSum == lambdaSum && rVirtual.price == rFinal.price &&
                    rFinal.price == rLambda.price;
  if (!same) {
    std::printf("\nMISMATCH between virtual and template results\n");
    return 1;
  }
  return 0;
}
