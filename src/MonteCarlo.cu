#include "MonteCarlo.hpp"
#include <cuda_runtime.h>
#include <curand_kernel.h>
#include <iostream>
#include <vector>

// CUDA error checking macro
#define cudaCheckError(ans)               \
  {                                       \
    gpuAssert((ans), __FILE__, __LINE__); \
  }
inline void gpuAssert(cudaError_t code, const char *file, int line,
                      bool abort = true)
{
  if (code != cudaSuccess)
  {
    fprintf(stderr, "GPUassert: %s %s %d\n", cudaGetErrorString(code), file,
            line);
    if (abort)
      exit(code);
  }
}

__global__ void setup_kernel(curandState *state, unsigned long long seed,
                             int n)
{
  int id = threadIdx.x + blockIdx.x * blockDim.x;
  if (id < n)
  {
    curand_init(seed, id, 0, &state[id]);
  }
}

__global__ void monteCarloKernel(curandState *state, double spot, double vol,
                                 double rate, double time, double strike,
                                 bool isCall, double *results, int n)
{
  int id = threadIdx.x + blockIdx.x * blockDim.x;
  if (id < n)
  {
    curandState localState = state[id];
    double z = curand_normal_double(&localState);
    state[id] = localState; // Save state back if needed for subsequent calls

    double price =
        spot * exp((rate - 0.5 * vol * vol) * time + vol * sqrt(time) * z);
    double payoff = 0.0;
    if (isCall)
    {
      payoff = price - strike;
    }
    else
    {
      payoff = strike - price;
    }
    if (payoff < 0.0)
      payoff = 0.0;

    results[id] = payoff * exp(-rate * time);
  }
}

MonteCarloResult MonteCarloPricer::priceCuda(const Payoff &payoff,
                                             size_t numPaths)
{
  double *d_results;
  curandState *d_states;
  double *h_results = new double[numPaths];

  // Determine strike and type from payoff (simplified for now, assuming
  // Vanilla) In a real scenario, we'd need a way to pass payoff details to the
  // kernel For this MVP, let's assume VanillaOption is passed and we cast it or
  // extract params But Payoff is an interface. We might need to dynamic_cast or
  // change the design to pass params. For now, let's just assume it's a
  // VanillaOption with strike and call/put. We'll need to extend Payoff or
  // check its type. Let's assume we can get strike and type. Actually, the
  // Payoff class has operator(). We can't pass the functor to the device easily
  // unless it's a device functor. For this step, I'll hardcode it to work with
  // the parameters, but I should probably refactor to pass parameters.

  // Try to extract strike and type (call/put) from the Payoff. For common
  // vanilla payoffs, PayoffCall/PayoffPut override strike() to provide value.
  double strike = 100.0; // fallback
  bool isCall = true;    // fallback
  if (payoff.strike().has_value())
  {
    strike = payoff.strike().value();
    // Determine whether it is call or put by a dynamic cast to the known types
    if (dynamic_cast<const PayoffCall *>(&payoff) != nullptr)
    {
      isCall = true;
    }
    else if (dynamic_cast<const PayoffPut *>(&payoff) != nullptr)
    {
      isCall = false;
    }
  }

  // Allocate memory on GPU
  cudaCheckError(cudaMalloc((void **)&d_results, numPaths * sizeof(double)));
  cudaCheckError(
      cudaMalloc((void **)&d_states, numPaths * sizeof(curandState)));

  // Setup RNG
  int blockSize = 256;
  int numBlocks = (numPaths + blockSize - 1) / blockSize;
  setup_kernel<<<numBlocks, blockSize>>>(d_states, 1234, numPaths);
  cudaCheckError(cudaPeekAtLastError());

  // Run simulation
  monteCarloKernel<<<numBlocks, blockSize>>>(d_states, m_spot, m_vol, m_rate,
                                             m_time, strike, isCall, d_results,
                                             numPaths);
  cudaCheckError(cudaPeekAtLastError());

  // Copy results back
  cudaCheckError(cudaMemcpy(h_results, d_results, numPaths * sizeof(double),
                            cudaMemcpyDeviceToHost));

  // Compute mean and variance on CPU (for now)
  // We could do reduction on GPU for better performance
  double sum = 0.0;
  double sumSq = 0.0;
  for (size_t i = 0; i < numPaths; ++i)
  {
    sum += h_results[i];
    sumSq += h_results[i] * h_results[i];
  }

  double mean = sum / numPaths;
  double variance = (sumSq / numPaths) - (mean * mean);
  double stdError = sqrt(variance / numPaths);

  // Cleanup
  cudaFree(d_results);
  cudaFree(d_states);
  delete[] h_results;

  return {mean, stdError, variance};
}
