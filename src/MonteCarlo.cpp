#include "MonteCarlo.hpp"
#include "Random.hpp"
#include <cmath>
#include <future>
#include <thread>
#include <vector>

MonteCarloPricer::MonteCarloPricer(double spot, double vol, double rate,
                                   double time)
    : m_spot(spot), m_vol(vol), m_rate(rate), m_time(time) {}

// The same loop as the template in MonteCarlo.hpp, instantiated for the abstract base, so
// every path makes a virtual call.
MonteCarloResult MonteCarloPricer::price(const Payoff &payoff,
                                         size_t numPaths) const
{
  return price<Payoff>(payoff, numPaths);
}

MonteCarloResult MonteCarloPricer::pricePathDependent(const PathPayoff &payoff,
                                                      size_t numPaths,
                                                      size_t numSteps)
{
  Statistics stats;

  double dt = m_time / numSteps;
  double drift = (m_rate - 0.5 * m_vol * m_vol) * dt;
  double diffusion = m_vol * std::sqrt(dt);
  double discount = std::exp(-m_rate * m_time);

  std::vector<double> path(numSteps);

  for (size_t i = 0; i < numPaths; ++i)
  {
    double currentSpot = m_spot;

    for (size_t j = 0; j < numSteps; ++j)
    {
      double z = Random::getNormal();
      currentSpot *= std::exp(drift + diffusion * z);
      path[j] = currentSpot;
    }

    double payoffVal = payoff(path);
    stats.add(payoffVal);
  }

  double price = discount * stats.mean();
  double stdError = discount * stats.stdError();
  double variance = stats.variance();

  return {price, stdError, variance};
}

MonteCarloResult MonteCarloPricer::priceParallel(const Payoff &payoff,
                                                 size_t numPaths)
{
  size_t numThreads = std::thread::hardware_concurrency();
  if (numThreads == 0)
    numThreads = 1;

  size_t pathsPerThread = numPaths / numThreads;
  std::vector<std::future<Statistics>> futures;

  for (size_t i = 0; i < numThreads; ++i)
  {
    size_t paths = (i == numThreads - 1) ? (numPaths - i * pathsPerThread)
                                         : pathsPerThread;

    futures.push_back(std::async(std::launch::async, [this, &payoff, paths]()
                                 {
      Statistics localStats;
      // Re-seed RNG for each thread to ensure independence
      Random::init(42 +
                   std::hash<std::thread::id>{}(std::this_thread::get_id()));

      double drift = (m_rate - 0.5 * m_vol * m_vol) * m_time;
      double diffusion = m_vol * std::sqrt(m_time);

      for (size_t j = 0; j < paths; ++j) {
        double z = Random::getNormal();
        double spotT = m_spot * std::exp(drift + diffusion * z);
        double payoffVal = payoff(spotT);
        localStats.add(payoffVal);
      }
      return localStats; }));
  }

  Statistics globalStats;
  for (auto &f : futures)
  {
    globalStats.merge(f.get());
  }

  double discount = std::exp(-m_rate * m_time);
  double price = discount * globalStats.mean();
  double stdError = discount * globalStats.stdError();
  double variance = globalStats.variance();

  return {price, stdError, variance};
}

// CUDA wrapper stub: if CUDA is available, implementation is in MonteCarlo.cu.
// When compiling without CUDA, provide a CPU fallback (parallel) implementation.
MonteCarloResult MonteCarloPricer::priceCuda(const Payoff &payoff, size_t numPaths)
{
  // Fallback: use parallel CPU implementation when CUDA isn't available.
  return priceParallel(payoff, numPaths);
}
