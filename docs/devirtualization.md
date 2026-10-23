# Devirtualizing the payoff call

The pricer used to call the payoff through `const Payoff &`: one virtual call per simulated path. This change adds a concept-constrained template, so a concrete payoff is resolved at compile time. It then measures what that buys, and reads the generated code to see why.

## The change

- **The concept.** [`MonteCarlo.hpp`](../include/MonteCarlo.hpp) adds `EuropeanPayoff`: anything callable as `payoff(spot) -> double`, whether a `Payoff` subclass, a plain struct or a lambda. `static_assert`s in [`test_dispatch.cpp`](../src/test_dispatch.cpp) check that it accepts `PayoffCall` and lambdas and rejects path payoffs and `int`, all at compile time.
- **One loop for both.** `MonteCarloPricer::price` is now a template over `EuropeanPayoff`. The old `price(const Payoff &)` stays for callers that only hold the base class, and forwards to `price<Payoff>`. So both paths run the same loop, and the tests check that they give bit-identical prices from the same seed.
- **`final`.** `PayoffCall` and `PayoffPut` are now `final`, which tells the compiler nothing can override their `operator()`.

## Results

Ryzen 9 9900X3D. MSVC 19.51 (Visual Studio 2026, Release `/O2`), GCC 13.3 (WSL2, Release `-O3`) and GCC 16.2 (Fedora 44 on bare metal, Release `-O3`). Each figure is the median of 5 runs. [`bench_dispatch`](../src/bench_dispatch.cpp) measures three things:
- **Payoff only:** `out[i] = payoff(spot[i])` over 20M prices already in memory. This isolates the call, until the call gets cheaper than reading the data (point 6).
- **Payoff only, in L1 cache:** the same loop over 2048 prices, repeated 20,000 times. It was added after the GCC 16 run, so its Ryzen figures aren't in the table yet.
- **Whole pricer:** `price()` over 5M paths, each with its random draw and `exp()`.

| | Virtual (`const Payoff &`) | Template, `PayoffCall` (`final`) | Template, lambda |
|---|---|---|---|
| Payoff only, MSVC | 1.11 ns | 1.13 ns (1.0×) | **0.43 ns (2.6×)** |
| Payoff only, GCC 13 (WSL2) | 5.43 ns | **0.42 ns (12.8×)** | **0.38 ns (14.2×)** |
| Payoff only, GCC 16 (bare metal) | 0.44 ns | 0.42 ns (1.1×) | 0.42 ns (1.0×) |
| Whole pricer, MSVC | 14.89 ns/path | 14.84 (1.00×) | 14.27 (1.04×) |
| Whole pricer, GCC 13 (WSL2) | 18.42 ns/path | 17.91 (1.03×) | 18.00 (1.02×) |
| Whole pricer, GCC 16 (bare metal) | 14.75 ns/path | 14.67 (1.01×) | 14.58 (1.01×) |

Two things changed between the GCC rows: the compiler and WSL2. WSL2 doesn't slow down a loop that makes no system calls, and the generated code below accounts for the difference. Pinned to one core (`taskset -c 4`), GCC 16's payoff-only figures drop to 0.34–0.36 ns and the pricer rises to about 15.2 ns/path; `perf stat` measured that core at an average of 5.3 GHz.

## What the compilers actually generated

Each payoff-only loop was compiled as its own function (`/O2 /FAs`, `-O3 -S`) and read. For GCC 16, the `bench_dispatch` binary was disassembled with `objdump -d`, and only the virtual loop and the payoff function were read:

| Loop | MSVC 19.51 | GCC 13 | GCC 16 |
|------|------------|--------|--------|
| virtual | indirect `call [rax+8]` per element | indirect `call *16(%rax)` per element | **checks the target** against `PayoffCall::operator()` and `PayoffPut::operator()` and runs an inlined copy of the match; the indirect `call *%rbp` is only the fallback |
| `PayoffCall` (`final`) | **the same indirect call**: not devirtualized | inlined **and vectorized**, no call | not read |
| lambda | inlined, unrolled 4×, branch-free `maxsd` (scalar) | inlined and vectorized | not read |
| `PayoffCall::operator()` itself, the target of the virtual call | `subsd` + `maxsd`: branch-free | `subsd` + `comisd` + **`ja`: a branch** | `subsd` + `cmpnltsd` + `andpd`: branch-free |

## What that means

1. **The call isn't what's expensive.** On MSVC a virtual call to a branch-free payoff costs 1.1 ns. GCC 13's 5.4 ns comes from inside the called function: it compiled `max(spot - strike, 0)` as a compare and branch. With random prices that branch goes either way about half the time, so the CPU mispredicts it constantly. Same C++, different code, a 5× difference. GCC 16 compiles the same line without a branch: `cmpnltsd` turns `x >= 0` into a mask of all ones or all zeros, and `andpd` keeps `x` or makes it 0.
2. **Inlining matters for what it lets the optimizer do next.** With the payoff inlined, GCC vectorized the loop and MSVC unrolled it without branches. That's where the 2.6× and 12.8–14.2× come from. A call in the loop body blocks both.
3. **`final` is a hint, not a guarantee.** GCC devirtualized through it; MSVC 19.51 didn't, even for a call through a `const PayoffCall &` it can prove is exact. A template over a concrete non-virtual type (a lambda or a plain struct) is the portable way to get inlining. GCC 16 goes further and inlines the virtual call without proof: it compares the function pointer loaded from the vtable with the two `operator()`s in the program, runs an inlined copy on a match, and keeps the real call for any other subclass. This is speculative devirtualization, and it's why GCC 16's virtual loop runs as fast as the templates here.
4. **Measure the whole loop before optimizing a piece of it.** In the real pricer the payoff is about 1 ns of a 15–18 ns path, so devirtualizing saves only 1–4%. The rest is the random normal (an out-of-line call into `std::mt19937_64` and `std::normal_distribution`) and `exp()`. `perf record` on the GCC 16 build, pinned to one core, puts numbers on it:
   - **Generating the normals** takes about 68% of the cycles (`Random::getNormal` 51.6%, `log` 13.1%, the Mersenne Twister 3.3%). It also causes about 74% of the 78M branch misses (`getNormal` 53.3%, `log` 21.0%). libstdc++ makes normals with the polar method, which throws away about one candidate pair in five and calls `log()`, and both cost branch misses.
   - **`exp()`** takes about 14% of the cycles.
   - **Everything else**, all the payoff loops included, takes about 17% of the cycles and 4% of the branch misses.

   That's where the next gains are (roadmap W16): a branch-free normal transform such as Box–Muller, on a counter-based generator.
5. **Same seed, different prices: the seed wasn't the same.** `Random::init` added a hash of the thread id to the seed. On Linux that hash is a memory address that changes every run, so one compiler on one machine printed different prices: three GCC 16 runs gave 10.4568, 10.4493 and 10.4617. The MSVC-vs-GCC difference recorded earlier (10.4516 vs 10.4458) therefore says nothing about the standard libraries. `Random::init` now uses the seed as given and clears the normal distribution's cached value, and `priceParallel` gives thread `i` the seed `42 + i`. With that, GCC 13.3 prints 10.448002 on every run. The cross-compiler comparison needs rerunning: `std::mt19937_64`'s output is fixed by the C++ standard, but `std::normal_distribution`'s algorithm isn't, so MSVC and GCC can still differ.
6. **Past a size, the payoff-only loop measures memory.** `spots` and `out` hold 20M doubles each: 320 MB, against a 96 MB L3. At 0.35–0.44 ns per element that's roughly 45–70 GB/s, close to what one core can get from memory, so every variant hits the same floor and any difference below it is hidden. The in-cache section (2048 prices, 16 KB in and 16 KB out, inside the 48 KB L1 data cache) measures the call instead.

## Also fixed

- **Tests that checked nothing in Release builds.** `test_payoffs` used `assert`, which `NDEBUG` removes, so a Release build checked nothing. The test files now `#undef NDEBUG` first.
- **A test that never checked.** `test_barrier` printed its expected values but never compared them. It now does.
- **CI.** [`.github/workflows/build.yml`](../.github/workflows/build.yml) builds and runs the tests on Ubuntu (GCC) and Windows (MSVC) on every push.
- **Reproducible seeds.** See point 5. [`test_random`](../src/test_random.cpp) checks that a seed gives the same draws again, after an odd number of draws, and on another thread.

## Reproduce

```bash
cmake -S . -B build-rel -DBUILD_CUDA=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel --config Release
ctest --test-dir build-rel -C Release
./build-rel/bench_dispatch        # build-rel/Release/bench_dispatch.exe on Windows
```

On Linux, the counters, the profile and the generated code:

```bash
perf stat -e cycles,instructions,branches,branch-misses taskset -c 4 ./build-rel/bench_dispatch
perf record -e cycles:u,branch-misses:u -o bench.data taskset -c 4 ./build-rel/bench_dispatch
perf report -i bench.data
objdump -d -C --no-show-raw-insn build-rel/bench_dispatch | awk '/<PayoffCall::operator\(\)\(double\) const>:/,/^$/'
```
