# Devirtualizing the payoff call

The pricer used to call the payoff through `const Payoff &`: one virtual call per simulated path. This change adds a concept-constrained template, so a concrete payoff is resolved at compile time. It then measures what that buys, and reads the generated code to see why.

## The change

- **The concept.** [`MonteCarlo.hpp`](../include/MonteCarlo.hpp) adds `EuropeanPayoff`: anything callable as `payoff(spot) -> double`, whether a `Payoff` subclass, a plain struct or a lambda. `static_assert`s in [`test_dispatch.cpp`](../src/test_dispatch.cpp) check that it accepts `PayoffCall` and lambdas and rejects path payoffs and `int`, all at compile time.
- **One loop for both.** `MonteCarloPricer::price` is now a template over `EuropeanPayoff`. The old `price(const Payoff &)` stays for callers that only hold the base class, and forwards to `price<Payoff>`. So both paths run the same loop, and the tests check that they give bit-identical prices from the same seed.
- **`final`.** `PayoffCall` and `PayoffPut` are now `final`, which tells the compiler nothing can override their `operator()`.

## Results

Ryzen 9 9900X3D. MSVC 19.51 (Visual Studio 2026, Release `/O2`) and GCC 13.3 (WSL2, Release `-O3`). Each figure is the median of 5 runs. [`bench_dispatch`](../src/bench_dispatch.cpp) measures two things:
- **Payoff only:** `out[i] = payoff(spot[i])` over 20M prices already in memory. This isolates the call.
- **Whole pricer:** `price()` over 5M paths, each with its random draw and `exp()`.

| | Virtual (`const Payoff &`) | Template, `PayoffCall` (`final`) | Template, lambda |
|---|---|---|---|
| Payoff only, MSVC | 1.11 ns | 1.13 ns (1.0×) | **0.43 ns (2.6×)** |
| Payoff only, GCC | 5.43 ns | **0.42 ns (12.8×)** | **0.38 ns (14.2×)** |
| Whole pricer, MSVC | 14.89 ns/path | 14.84 (1.00×) | 14.27 (1.04×) |
| Whole pricer, GCC | 18.42 ns/path | 17.91 (1.03×) | 18.00 (1.02×) |

## What the compilers actually generated

Each payoff-only loop was compiled as its own function (`/O2 /FAs`, `-O3 -S`) and read:

| Loop | MSVC 19.51 | GCC 13 |
|------|------------|--------|
| virtual | indirect `call [rax+8]` per element | indirect `call *16(%rax)` per element |
| `PayoffCall` (`final`) | **the same indirect call**: not devirtualized | inlined **and vectorized**, no call |
| lambda | inlined, unrolled 4×, branch-free `maxsd` (scalar) | inlined and vectorized |
| `PayoffCall::operator()` itself, the target of the virtual call | `subsd` + `maxsd`: branch-free | `subsd` + `comisd` + **`ja`: a branch** |

## What that means

1. **The call isn't what's expensive.** On MSVC a virtual call to a branch-free payoff costs 1.1 ns. GCC's 5.4 ns comes from inside the called function: it compiled `max(spot - strike, 0)` as a compare and branch. With random prices that branch goes either way about half the time, so the CPU mispredicts it constantly. Same C++, different code, a 5× difference.
2. **Inlining matters for what it lets the optimizer do next.** With the payoff inlined, GCC vectorized the loop and MSVC unrolled it without branches. That's where the 2.6× and 12.8–14.2× come from. A call in the loop body blocks both.
3. **`final` is a hint, not a guarantee.** GCC devirtualized through it; MSVC 19.51 didn't, even for a call through a `const PayoffCall &` it can prove is exact. A template over a concrete non-virtual type (a lambda or a plain struct) is the portable way to get inlining.
4. **Measure the whole loop before optimizing a piece of it.** In the real pricer the payoff is about 1 ns of a 15–18 ns path, so devirtualizing saves only 2–4%. The rest is the random normal (an out-of-line call into `std::mt19937_64` and `std::normal_distribution`) and `exp()`. That's where the next gains are (roadmap W16).
5. **Same seed, different prices.** The two compilers print different prices from the same seed: 10.4516 (MSVC) vs 10.4458 (GCC). `std::normal_distribution`'s algorithm isn't fixed by the C++ standard, so each standard library produces its own sequence. That's why W16 replaces it with a counter-based generator and our own normal transform.

## Also fixed

- **Tests that checked nothing in Release builds.** `test_payoffs` used `assert`, which `NDEBUG` removes, so a Release build checked nothing. The test files now `#undef NDEBUG` first.
- **A test that never checked.** `test_barrier` printed its expected values but never compared them. It now does.
- **CI.** [`.github/workflows/build.yml`](../.github/workflows/build.yml) builds and runs the tests on Ubuntu (GCC) and Windows (MSVC) on every push.

## Reproduce

```bash
cmake -S . -B build-rel -DBUILD_CUDA=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel --config Release
ctest --test-dir build-rel -C Release
./build-rel/bench_dispatch        # build-rel/Release/bench_dispatch.exe on Windows
```
