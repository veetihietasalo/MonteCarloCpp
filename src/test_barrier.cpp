// Simple deterministic tests for BarrierOption behavior
#undef NDEBUG // keep assert() active in every build type, Release included
#include "BarrierOption.hpp"
#include <cassert>
#include <iostream>
#include <vector>

int main_test_barrier()
{
  std::vector<double> path = {100.0, 95.0, 85.0, 87.0}; // barrier at 90 is hit (down)
  double strike = 100.0;
  double barrier = 90.0;

  BarrierOption downOutCall(strike, barrier, BarrierType::DownAndOut, OptionType::Call);
  BarrierOption downInPut(strike, barrier, BarrierType::DownAndIn, OptionType::Put);
  BarrierOption downOutCallWithRebate(strike, barrier, BarrierType::DownAndOut, OptionType::Call, 5.0);

  std::cout << "Path: ";
  for (double p : path)
    std::cout << p << " ";
  std::cout << std::endl;

  std::cout << "downOutCall payoff (expected 0): " << downOutCall(path) << std::endl;
  std::cout << "downOutCallWithRebate payoff (expected 5): " << downOutCallWithRebate(path) << std::endl;
  std::cout << "downInPut payoff (expected 13): " << downInPut(path) << std::endl;

  assert(downOutCall(path) == 0.0);
  assert(downOutCallWithRebate(path) == 5.0);
  assert(downInPut(path) == 13.0);
  std::cout << "All barrier tests passed." << std::endl;
  return 0;
}

// Quick main to allow running this standalone test
int main()
{
  return main_test_barrier();
}
