#include "Random.hpp"
#include <thread>
#include <functional>

#include "Random.hpp"

thread_local std::mt19937_64 Random::s_engine;
thread_local std::normal_distribution<double> Random::s_normalDist(0.0, 1.0);
thread_local std::uniform_real_distribution<double> Random::s_uniformDist(0.0, 1.0);

void Random::init(uint64_t seed) {
    // For thread_local, this only seeds the CURRENT thread.
    // In a real app, we'd seed each thread uniquely (e.g. seed + thread_id)
    s_engine.seed(seed + std::hash<std::thread::id>{}(std::this_thread::get_id()));
}

double Random::getNormal() {
    return s_normalDist(s_engine);
}

double Random::getUniform() {
    return s_uniformDist(s_engine);
}
