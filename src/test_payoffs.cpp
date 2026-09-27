#undef NDEBUG // keep assert() active in every build type, Release included
#include "Payoff.hpp"
#include "AsianOption.hpp"
#include "LookbackOption.hpp"
#include "BarrierOption.hpp"
#include <cassert>
#include <iostream>
#include <vector>

int main()
{
  // Payoff Call/Put
  PayoffCall call(100.0);
  PayoffPut put(100.0);
  assert(call(110.0) == 10.0);
  assert(call(90.0) == 0.0);
  assert(put(90.0) == 10.0);
  assert(put(110.0) == 0.0);

  // Asian option
  AsianOption asian(100.0);
  std::vector<double> path = {100.0, 110.0, 90.0, 100.0}; // avg = 100 -> payoff 0
  assert(asian(path) == 0.0);
  path = {110.0, 110.0, 110.0}; // avg =110 -> payoff=10
  assert(asian(path) == 10.0);

  // Lookback option
  LookbackOption lookback;
  path = {100.0, 80.0, 120.0}; // final 120 - min 80 = 40
  assert(lookback(path) == 40.0);

  // Barrier options: Down-and-Out with barrier 90
  BarrierOption downOut(100.0, 90.0, BarrierType::DownAndOut, OptionType::Call);
  path = {100.0, 95.0, 85.0, 87.0}; // barrier hit
  assert(downOut(path) == 0.0);
  // Down-and-Out with rebate
  BarrierOption downOutReb(100.0, 90.0, BarrierType::DownAndOut, OptionType::Call, 5.0);
  assert(downOutReb(path) == 5.0);

  std::cout << "All payoff tests passed." << std::endl;
  return 0;
}
