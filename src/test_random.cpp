#undef NDEBUG // keep assert() active in every build type, Release included
#include "Random.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

// Seeds the calling thread, then draws n standard normals.
static std::vector<double> draw(uint64_t seed, int n)
{
  Random::init(seed);
  std::vector<double> v(n);
  for (double &x : v)
    x = Random::getNormal();
  return v;
}

int main()
{
  // The same seed gives the same draws. An odd count leaves the second value of a pair
  // cached in the normal distribution; seeding again must not return that leftover.
  const std::vector<double> first = draw(42, 5);
  assert(draw(42, 5) == first);

  // The same seed gives the same draws on another thread too. Seeding used to mix in a hash
  // of the thread id, which also changes from run to run.
  std::vector<double> onOtherThread;
  std::thread([&] { onOtherThread = draw(42, 5); }).join();
  assert(onOtherThread == first);

  // A different seed gives different draws.
  assert(draw(43, 5) != first);

  std::cout << "All random tests passed." << std::endl;
  return 0;
}
