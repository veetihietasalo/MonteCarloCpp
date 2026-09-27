# MonteCarloCpp

This module contains a simple Monte Carlo pricer and several payoff implementations (European, Asian, Barrier, Lookback).

New: Barrier options support both Call and Put types, and an optional rebate for knock-out scenarios.

## Build and test

```bash
cmake -S . -B build-rel -DBUILD_CUDA=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel --config Release
ctest --test-dir build-rel -C Release --output-on-failure
```

CI runs the same steps on Ubuntu (GCC) and Windows (MSVC) on every push. On Windows, `build.bat` still works too (it needs Visual Studio and CMake).

## Programs

- `MonteCarloEngine`: prices European, Asian, barrier and lookback options and times single-threaded vs multi-threaded runs.
- `bench_dispatch`: a virtual payoff call against a template one ([results and generated code](docs/devirtualization.md)).
- `test_payoffs`, `test_barrier`, `test_dispatch`: run by `ctest`.

## Pricing any payoff

`MonteCarloPricer::price` accepts anything that satisfies the `EuropeanPayoff` concept (callable as `payoff(spot) -> double`), including a lambda:

```cpp
MonteCarloPricer pricer(100.0, 0.2, 0.05, 1.0);
auto result = pricer.price([](double s) { return std::max(s - 100.0, 0.0); }, 1'000'000);
```

A concrete payoff type is resolved at compile time, so the call can be inlined. A `const Payoff &` still works, through a virtual call.

The Monte Carlo code is minimal and primarily educational; it includes single-threaded, multi-threaded, and a CUDA stub for GPU runs.
