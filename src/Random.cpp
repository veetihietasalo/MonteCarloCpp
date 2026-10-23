#include "Random.hpp"

thread_local std::mt19937_64 Random::s_engine;
thread_local std::normal_distribution<double> Random::s_normalDist(0.0, 1.0);
thread_local std::uniform_real_distribution<double> Random::s_uniformDist(0.0, 1.0);

void Random::init(uint64_t seed) {
    // Seeds only the CURRENT thread (the engine is thread_local). The seed is used as given,
    // so it gives the same draws on every run and every thread; threads that need different
    // streams pass different seeds (see MonteCarloPricer::priceParallel).
    s_engine.seed(seed);
    // normal_distribution keeps the second value of each pair it generates. Drop it, so the
    // first draw after init() comes from the new seed.
    s_normalDist.reset();
    s_uniformDist.reset();
}

double Random::getNormal() {
    return s_normalDist(s_engine);
}

double Random::getUniform() {
    return s_uniformDist(s_engine);
}
