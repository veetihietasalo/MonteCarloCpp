#include "AsianOption.hpp"
#include "BarrierOption.hpp"
#include "LookbackOption.hpp"
#include "MonteCarlo.hpp"
#include "Random.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>

int main()
{
  // Initialize RNG
  Random::init(42);

  // Parameters
  double spot = 100.0;
  double strike = 100.0;
  double vol = 0.20;
  double rate = 0.05;
  double time = 1.0;
  size_t paths = 1'000'000;

  std::cout << "Parameters:" << std::endl;
  std::cout << "  Spot:  " << spot << std::endl;
  std::cout << "  Strike:" << strike << std::endl;
  std::cout << "  Vol:   " << vol * 100 << "%" << std::endl;
  std::cout << "  Rate:  " << rate * 100 << "%" << std::endl;
  std::cout << "  Time:  " << time << " years" << std::endl;
  std::cout << "  Paths: " << paths << std::endl;
  std::cout << "-----------------------------------------" << std::endl;

  // Create Pricer
  MonteCarloPricer pricer(spot, vol, rate, time);

  // --- Benchmarking European Call ---
  std::cout << "Benchmarking European Call (Paths: " << paths << ")..."
            << std::endl;

  // Single-threaded
  PayoffCall callPayoff(strike);
  auto start = std::chrono::high_resolution_clock::now();
  auto callResult = pricer.price(callPayoff, paths);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end - start;

  std::cout << "Single-threaded: " << diff.count() << " s" << std::endl;
  std::cout << "Price: " << callResult.price << " +/- " << callResult.stdError
            << std::endl;

  // Multi-threaded
  start = std::chrono::high_resolution_clock::now();
  auto parallelResult = pricer.priceParallel(callPayoff, paths);
  end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diffParallel = end - start;

  std::cout << "Multi-threaded:  " << diffParallel.count() << " s" << std::endl;
  std::cout << "Price: " << parallelResult.price << " +/- "
            << parallelResult.stdError << std::endl;

  if (diffParallel.count() > 0)
  {
    std::cout << "Speedup: " << diff.count() / diffParallel.count() << "x"
              << std::endl;
  }
  std::cout << "-----------------------------------------" << std::endl;

  // GPU (CUDA)
  start = std::chrono::high_resolution_clock::now();
  auto cudaResult = pricer.priceCuda(callPayoff, paths);
  end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diffCuda = end - start;

  std::cout << "GPU (CUDA):      " << diffCuda.count() << " s" << std::endl;
  std::cout << "Price: " << cudaResult.price << " +/- " << cudaResult.stdError
            << std::endl;

  if (diffCuda.count() > 0)
  {
    std::cout << "Speedup (vs CPU): " << diff.count() / diffCuda.count() << "x"
              << std::endl;
  }
  std::cout << "-----------------------------------------" << std::endl;

  // Price Put
  PayoffPut putPayoff(strike);
  auto putResult = pricer.price(putPayoff, paths);

  std::cout << "Put Price:  " << putResult.price << " +/- "
            << putResult.stdError << std::endl;

  // Black-Scholes Reference (Approximate)
  std::cout << "-----------------------------------------" << std::endl;
  std::cout << "BS Ref (Call): 10.4506" << std::endl;
  std::cout << "BS Ref (Put):   5.5735" << std::endl;
  std::cout << "-----------------------------------------" << std::endl;

  // Price Asian Option
  AsianOption asianPayoff(strike);
  size_t steps = 252; // Daily monitoring for 1 year

  std::cout << "Pricing Asian Call (252 steps)..." << std::endl;
  auto asianResult = pricer.pricePathDependent(asianPayoff, paths, steps);

  std::cout << "Asian Price: " << asianResult.price << " +/- "
            << asianResult.stdError << std::endl;
  std::cout << "-----------------------------------------" << std::endl;

  // Price Barrier Option (Down-and-Out Call)
  double barrierLevel = 90.0;
  BarrierOption barrierPayoff(strike, barrierLevel, BarrierType::DownAndOut);

  std::cout << "Pricing Down-and-Out Call (Barrier " << barrierLevel << ")..."
            << std::endl;
  auto barrierResult = pricer.pricePathDependent(barrierPayoff, paths, steps);

  std::cout << "Barrier Price: " << barrierResult.price << " +/- "
            << barrierResult.stdError << std::endl;
  std::cout << "-----------------------------------------" << std::endl;

  // Price a Down-and-In Put with Rebate example
  double rebate = 2.0;
  BarrierOption downInPut(strike, barrierLevel, BarrierType::DownAndIn, OptionType::Put, rebate);
  std::cout << "Pricing Down-and-In Put (Barrier " << barrierLevel << ", rebate " << rebate << ")..." << std::endl;
  auto downInPutResult = pricer.pricePathDependent(downInPut, paths, steps);
  std::cout << "Down-and-In Put Price: " << downInPutResult.price << " +/- " << downInPutResult.stdError << std::endl;
  // Price Lookback Option (Floating Strike)
  LookbackOption lookbackPayoff;

  std::cout << "Pricing Lookback Call..." << std::endl;
  auto lookbackResult = pricer.pricePathDependent(lookbackPayoff, paths, steps);

  std::cout << "Lookback Price: " << lookbackResult.price << " +/- "
            << lookbackResult.stdError << std::endl;

  return 0;
}
