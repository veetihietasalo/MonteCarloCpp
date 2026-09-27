#pragma once
#include <random>

class Random {
public:
    // Initialize with seed
    static void init(uint64_t seed);

    // Get standard normal distribution sample N(0,1)
    static double getNormal();

    // Get uniform distribution sample U(0,1)
    static double getUniform();

private:
    static thread_local std::mt19937_64 s_engine;
    static thread_local std::normal_distribution<double> s_normalDist;
    static thread_local std::uniform_real_distribution<double> s_uniformDist;
};
