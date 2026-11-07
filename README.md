# MonteCarloCpp

A Monte Carlo option pricer in C++20: European, Asian, barrier and lookback payoffs, run single-threaded or across all cores, with a CUDA kernel for the vanilla case.

[![Build and Test](https://github.com/veetihietasalo/MonteCarloCpp/actions/workflows/build.yml/badge.svg)](https://github.com/veetihietasalo/MonteCarloCpp/actions/workflows/build.yml)

The pricer is the vehicle. The question is what a virtual call per simulated path really costs, and what the compiler does with it. [docs/devirtualization.md](docs/devirtualization.md) has the measurements and the generated code on MSVC, GCC 13 and GCC 16.

Built with coding agents: all four non-merge commits have Claude as author (2) or co-author (2): the import, the devirtualization with tests and CI, the reproducible seeds, and the in-cache benchmark. The pricer was imported as it stood before version control, so the history doesn't show who first wrote it.

## What is in it

- `MonteCarloPricer`: `price` for a payoff of the terminal price, `pricePathDependent` for payoffs of a whole path, `priceParallel` (one `std::async` task per hardware thread, merged statistics), `priceCuda`.
- Payoffs: European call and put, Asian arithmetic-average call, barrier (down/up, in/out, call/put, optional rebate), floating-strike lookback call.
- `EuropeanPayoff` concept: `price` accepts anything callable as `payoff(spot) -> double`, a lambda included. A concrete type is resolved at compile time. A `const Payoff &` still works, through a virtual call.
- `Statistics` (running sum and sum of squares, standard error, `merge`) and a seedable per-thread `Random`.
- A C interface (`MonteCarloAPI.h`) in a shared library: `price_european_call`, `price_european_put`, `price_asian_call`, `price_lookback_call`, `monte_carlo_set_seed`.

## Results

Spot 100, strike 100, vol 20 %, rate 5 %, 1 year. Black-Scholes: call 10.4506, put 5.5735. One run, 1M paths, seed 42, GCC 13.3 on WSL2:

| | Price | Std error |
|---|---|---|
| European call, single-threaded | 10.4723 | 0.0147 |
| European call, multi-threaded (24 logical CPUs) | 10.4414 | 0.0147 |
| European put | 5.5849 | 0.0087 |

The multi-threaded run was 4.0x faster (0.0166 s against 0.0042 s). Its threads use seeds 42 + i, so the price depends on the thread count.

Cost of the payoff call, Ryzen 9 9900X3D, median of 5 ([details](docs/devirtualization.md)):

| | Virtual | Template |
|---|---|---|
| Payoff only, GCC 13, 20M prices in memory | 5.43 ns | 0.42 ns |
| Payoff only, GCC 16, in L1 cache | 0.39 ns | 0.10 ns |
| Whole pricer, 5M paths, MSVC / GCC 13 / GCC 16 | 14.89 / 18.42 / 14.73 ns | 14.84 / 17.91 / 14.65 ns |

## Build and run

```bash
cmake -S . -B build-rel -DBUILD_CUDA=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel --config Release
ctest --test-dir build-rel -C Release --output-on-failure
./build-rel/MonteCarloEngine      # prices every payoff and prints timings
./build-rel/bench_dispatch        # virtual against template
```

On GCC 13.3 (Ubuntu 24.04, WSL2) the 4 tests pass, and `bench_dispatch` prices 5M paths to 10.448002 for all three variants. CI builds and tests on Ubuntu (GCC) and Windows (MSVC).

```cpp
MonteCarloPricer pricer(100.0, 0.2, 0.05, 1.0);
auto r = pricer.price([](double s) { return std::max(s - 100.0, 0.0); }, 1'000'000);
```

## What I learned

- The virtual call is not the expensive part. GCC 13 compiled `max(spot - strike, 0)` as a branch that mispredicts on random prices: 5.4 ns per call, against 1.1 ns on MSVC. GCC 16 compiles it branch-free.
- Measure the whole loop first. The payoff is about 1 ns of a 15-18 ns path, so devirtualizing saves 1-4 %. `perf` puts the random normals at about 68 % of the cycles and `exp()` at 14 %.
- `final` is a hint. GCC devirtualized through it, MSVC 19.51 did not.
- A seed mixed with a thread-id hash is not a seed. Three runs gave 10.4568, 10.4493 and 10.4617. Now the seed is used as given, and GCC 13.3 and 16.2 both print 10.448002.
- Tests can pass while checking nothing. `assert` vanishes under `NDEBUG`, so the Release tests checked nothing, and `test_barrier` never compared its values. Both are fixed.
- A big benchmark can measure memory. 320 MB of arrays against a 96 MB L3 puts every variant on the same floor, so the in-cache run was added.

## Not done

- CUDA is untested. `src/MonteCarlo.cu` is not built in CI, and `-DBUILD_CUDA=ON` fails to build with nvcc 13.4 and Visual Studio 2022 (nvcc rejects the global `/W4 /permissive-` flags). Without CUDA, `priceCuda` runs the multi-threaded CPU path, and `MonteCarloEngine` still prints it as "GPU (CUDA)".
- No variance reduction and no Greeks. The Black-Scholes reference values are typed in, not computed.
- The next gain is the random normal: a branch-free transform such as Box-Muller on a counter-based generator.
