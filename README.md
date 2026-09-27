# MonteCarloCpp

This module contains a simple Monte Carlo pricer and several payoff implementations (European, Asian, Barrier, Lookback).

New: Barrier options support both Call and Put types, and an optional rebate for knock-out scenarios.

Quick usage:

- Build with `build.bat` (Windows, requires Visual Studio and CMake).
- Run the example: `MonteCarloEngine_v2.exe` (or build and run `main.exe`).
- Run the deterministic Barrier test: compile and run `src/test_barrier.cpp` to verify payoff behavior.

The Monte Carlo code is minimal and primarily educational; it includes single-threaded, multi-threaded, and a CUDA stub for GPU runs.
