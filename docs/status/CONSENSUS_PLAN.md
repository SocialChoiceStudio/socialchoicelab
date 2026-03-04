# Consensus Plan: SocialChoiceLab Stabilization and Improvement

**Date:** February 14, 2026
**Sources:** Independent reviews by Claude Opus 4.6 and GPT 5.5 Codex (source docs consolidated into this plan during Feb 14 cleanup).
**Status:** All disputes resolved. This is the final agreed plan.

---

## How to Use This Plan

This is a prioritized backlog for any Cursor agent (or human) working on the SocialChoiceLab codebase. Work through phases in order. Within each phase, tackle Critical items before High, High before Medium, Medium before Low. Each item is tagged with its origin: **(Both)** means both agents independently identified it, **(Claude Only)** or **(ChatGPT Only)** means one agent found it and the other accepted it during review.

---

## Dispute Resolutions

Before finalizing the plan, the two agents debated six points. All were resolved:

| Dispute | Resolution |
|---------|------------|
| `.clang-format` missing? | **ChatGPT correct.** File exists at `socialchoicelab/.clang-format`. Removed from plan. |
| Missing includes = Critical? | **ChatGPT correct on severity.** Code compiles today via transitive includes. Reclassified to Medium (portability risk). |
| CMake modernization = Critical? | **ChatGPT correct on severity.** Build works for current scope. Moved to backlog as technical debt. |
| Loss sign convention = bug or semantics? | **Both agree: correctness bug.** `distance_to_utility(5.0, LINEAR)` returns `+5.0` — utility increases with distance due to double negation. The code's own Doxygen cites Singh (2013) `u = -alpha|d|` which decreases with distance. Fix immediately. |
| `std::hash` = broken now or future risk? | **Compromise.** Deterministic on a single machine/compiler today; will break across toolchains (GCC vs Clang produce different hashes). High priority for a reproducibility-focused library. |
| `lint.sh` — remove `\|\| true` entirely? | **ChatGPT's approach adopted.** Strict mode for CI, permissive default for local development. |

---

## Phase 0 / Week 1: Immediate Stabilization (Correctness + Compile Trust)

| # | Item | Source | Severity | Status |
|---|------|--------|----------|--------|
| 1 | Fix loss function sign convention: `linear_loss`, `quadratic_loss`, and `threshold_loss` return negative values, then `distance_to_utility` negates again, making utility increase with distance. Adopt consistent convention: all loss functions return positive loss magnitude, `distance_to_utility` returns `-loss`. | **(Both)** | Critical | ✅ Done (Feb 14) |
| 2 | Fix `normalize_utility` to accept all loss parameters (`max_loss`, `steepness`, `threshold`) — currently uses defaults internally, producing wrong normalization for GAUSSIAN and THRESHOLD types. | **(Both)** | Critical | ✅ Done (Feb 14) |
| 3 | Rewrite or delete `utility_functions.h` — references deleted `PointND` type, wrong enum name (`loss::LossType` instead of `LossFunctionType`), non-existent `loss::get_loss_function<T>()`, and fabricated `EXPONENTIAL` enum value. Cannot compile. | **(Both)** | Critical | ✅ Done (deleted Feb 14) |
| 4 | Delete or rewrite both example files (`basic_usage.cpp`, `distance_example.cpp`) — reference deleted types, call distance functions with wrong signatures (missing mandatory `salience_weights`). Cannot compile. | **(Both)** | Critical | ✅ Done (deleted Feb 14) |
| 5 | Add `test_loss_functions` target to root `CMakeLists.txt` — this test file exists but is never built or run, which is why the sign convention and normalization bugs were never caught. | **(Both)** | Critical | ✅ Done (Feb 14) |
| 6 | Delete dead `tests/CMakeLists.txt` and `examples/CMakeLists.txt` — never included by root build, use incompatible GTest strategy, contain wrong paths. | **(Both)** | High | ✅ Done (deleted Feb 14) |
| 7 | Fix Eigen include directory ordering bug in CMake — line 20 references `${eigen_SOURCE_DIR}` before `FetchContent_MakeAvailable(eigen)` at line 39. Variable is empty at that point. Remove the line (Eigen includes are already provided via the `Eigen3::Eigen` target). | **(Claude Only)** | High | ✅ Done |
| 8 | Add PRNG edge-case guards: `uniform_choice(size==0)` causes unsigned underflow to `SIZE_MAX`; `discrete_choice(weights.empty())` is UB. Also guard `beta()` against division by zero when both gamma draws return 0.0. Throw `std::invalid_argument` with clear messages. | **(Both)** | High | ✅ Done |
| 9 | Validate `order_p >= 1` in Minkowski distance — `p < 1` is not a valid metric, `p == 0` causes division by zero. | **(Both)** | High | ✅ Done |
| 10 | Fix `lint.sh`: fix `find` expression precedence bug (wrap `-name` conditions in parentheses); add `--strict` mode for CI that fails on lint errors (keep permissive default for local development). | **(Both)** | Medium | ✅ Done |
| 11 | Align `make format` / `make lint` doc references with actual CMake targets — docs mention these but they don't exist. Add the targets or remove the docs. | **(Both)** | Medium | ✅ Done |

---

## Phase 1 / Week 2: Reproducibility, Safety, and Test Quality

| # | Item | Source | Severity | Status |
|---|------|--------|----------|--------|
| 12 | Replace `std::hash<std::string>` in `generate_stream_seed()` with a deterministic hash (e.g., FNV-1a) — `std::hash` is implementation-defined and produces different values across GCC/Clang/MSVC, breaking cross-platform reproducibility. | **(Both)** | High | ✅ Done |
| 13 | Fix SIOF: convert file-scope statics in `stream_manager.cpp` to Meyers' singleton (function-local static). Latent — not yet triggered in current code, but becomes UB if any global constructor calls `get_global_stream_manager()`. Trivial 3-line fix. | **(Both)** | High (latent) | ✅ Done |
| 14 | Fix non-deterministic test seeds — 4 of 5 test files use `std::random_device`, making failures irreproducible. Replace with fixed seeds. | **(Both)** | High | ✅ Done |
| 15 | Add error-condition tests (`EXPECT_THROW`) for invalid inputs across distance, loss, and PRNG functions — currently zero error-condition tests in the entire suite. | **(Both)** | Medium | ✅ Done |
| 16 | Add missing includes for portability: `<stdexcept>` in `distance_functions.h` and `loss_functions.h`; `<type_traits>` in `loss_functions.h`; `<vector>` in `prng.h` and `stream_manager.h`; `<numeric>` in `test_distance_speed.cpp`. | **(Both)** | Medium | ✅ Done |
| 17 | Harden enum handling: replace silent `default` fallbacks in `calculate_distance` and `distance_to_utility` switch statements with `throw std::invalid_argument("Unknown type")` — current defaults silently use Euclidean/Linear for unrecognized enum values. | **(Both)** | Medium | ✅ Done |
| 18 | Either remove thread-safety claims from `StreamManager` or fix with proper lock management. Current issues: `get_stream()` returns references that outlive the lock; `master_seed()` reads without lock while `reset_all()` writes under lock (data race); `const` overload of `get_stream()` silently creates streams via `mutable` members. If single-threaded use is intended, document that and remove the mutex overhead. | **(Both)** | Medium | ✅ Done — single-owner policy adopted. See `docs/architecture/StreamManager_Design.md` for full decision and rationale. |
| 19 | Fix or remove tautological/empty tests: `EXPECT_TRUE(true)` in Skip test; `EXPECT_GE(size_t, 0)` (always true for unsigned); `MemoryUsage` test that doesn't measure memory; tautological `utility == -loss` assertions. | **(Claude Only)** | Medium | ✅ Done |
| 20 | Rename `beta()` parameter to avoid self-shadowing — `beta(T alpha, T beta)` shadows the function name. | **(Claude Only)** | Low | ✅ Done (already `beta_param` in prng.h) |
| 21 | Separate performance benchmarks from unit tests — `test_distance_speed.cpp` and `test_performance_comparison.cpp` have arbitrary timing thresholds that will randomly fail on different machines. Use CTest labels or Google Benchmark. | **(Claude Only)** | Low | ✅ Done — timing assertions removed (report only); CTest label `benchmark` added; doc in DEVELOPMENT.md. |

---

## Phase 2 / Week 3: Documentation Truthfulness

| # | Item | Source | Severity | Status |
|---|------|--------|----------|--------|
| 22 | Update README: separate "Implemented Now" (distance functions, loss functions, PRNG/StreamManager) from "Planned" (CGAL, voting rules, outcome concepts, R/Python bindings, GUI). | **(Both)** | High | ✅ Done |
| 23 | Fix stale PointND references in docs — `DEVELOPMENT.md`, `docs/references/foundation/README.md`, and others still reference the deleted `PointND` type and `core::types` namespace. | **(Both)** | High | ✅ Done |
| 24 | Fix PROJECT_LOG: restore chronological order (Sept 10 entry appears after Sept 14); resolve GTest contradiction (log says "abandoned" but GTest is in use); note that Eigen migration is complete (utility_functions.h was deleted Feb 14). | **(Both)** | Medium | ✅ Done |
| 25 | Repair reference index structure: remove phantom subdirectories from `docs/references/README.md` and `REFERENCE_INDEX.md` that were never created; consolidate the implementation priority list that appears identically in three files. | **(Both)** | Medium | ✅ Done |
| 26 | Fix DEVELOPMENT.md: style guide says functions use `PascalCase` but actual code uses `snake_case`; remove instruction to install Google Test via Homebrew (project uses FetchContent). | **(Claude Only)** | Medium | ✅ Done (verified; already correct) |
| 27 | Fix design doc: change "Rcpp" to "cpp11" in licensing section — the R binding layer uses cpp11, not Rcpp. | **(Claude Only)** | Low | ✅ Done |

See `docs/Phase2Changes.md` for a step-by-step record of Phase 2 edits.

---

## Phase 3 / Week 4: Developer Experience and Process

| # | Item | Source | Severity |
|---|------|--------|----------|
| 28 | Add CI workflow (build + test on push/PR across major OS targets; include format/lint checks). | **(Both)** | High | ✅ Done |
| 29 | Create `docs/status/ROADMAP.md` with near-term (1-2 months), mid-term (3-6 months), and long-term (6+ months) plans. Link to design doc and implementation priority instead of duplicating. | **(Both)** | Medium | ✅ Done |
| 30 | Define milestone gates with "done" criteria: required features, tests, docs, and API stability expectations for each milestone. | **(Both)** | Medium | ✅ Done |
| 31 | Add `CONTRIBUTING.md`, `SECURITY.md`, `CHANGELOG.md` — standard open-source project hygiene. | **(Both)** | Medium | ✅ Done |
| 32 | Add `.clang-tidy` config and pre-commit hooks for automated quality enforcement. | **(Both)** | Medium |
| 33 | Add dependency-aware sequencing to roadmap (e.g., `c_api` before language bindings, geometry primitives before advanced electoral methods). | **(ChatGPT Only)** | Low |

---

## Backlog: Quality and Modernization (No Rush)

| # | Item | Source | Severity |
|---|------|--------|----------|
| 34 | Modernize CMakeLists.txt: use `target_include_directories()` instead of global `include_directories()`; use `target_compile_options()` with generator expressions instead of global `set()`; add `install()` targets; remove unused `C` language; remove redundant `gtest` linking. | **(Claude Only)** | Low |
| 35 | Unify namespace style to C++17 nested form (`namespace socialchoicelab::core::rng {`) — currently mixed with C++11 style. | **(Claude Only)** | Low |
| 36 | Extract magic number `12345` (default seed) to a named constant — appears in `prng.h`, `stream_manager.h`, and `stream_manager.cpp`. | **(Claude Only)** | Low |
| 37 | Special-case `p=1` and `p=2` in `minkowski_distance` to avoid `std::pow` in tight loop — use `std::abs` for p=1 and `x*x` for p=2. | **(Claude Only)** | Low |
| 38 | DRY: have `minkowski_distance` call `chebyshev_distance()` for high-p case instead of duplicating the Chebyshev logic. | **(Claude Only)** | Low |
| 39 | Fix broken indentation in `minkowski_distance` — main calculation block uses 12-space indent while rest of file uses 4; closing brace at column 0. | **(Claude Only)** | Low |
| 40 | Remove commented-out dead code in `distance_functions.h` (lines 197-200: unused type aliases with unresolved "Note:"). | **(Claude Only)** | Low |
| 41 | Add `noexcept` to simple accessors (`master_seed()`, `state_string()`, etc.). | **(Claude Only)** | Low |
| 42 | Remove unused `<algorithm>` include from `loss_functions.h`. | **(Claude Only)** | Low |
| 43 | Fix Doxygen `@tparam N` on distance wrapper functions that don't have a template parameter `N`. | **(Claude Only)** | Low |
| 44 | Add weighted and higher-dimensional distance tests — current tests almost exclusively use equal weights `{1.0, 1.0}` on 2D points. | **(Claude Only)** | Low |
| 45 | Add triangle inequality tests for distance metrics (`d(a,c) <= d(a,b) + d(b,c)`). | **(Claude Only)** | Low |
| 46 | Evaluate GPL v3 vs LGPL v3 for library adoption — GPL requires downstream programs to also be GPL, which may discourage academic use. | **(Claude Only)** | Low |

---

## Resolution Method for Future Disputes

- **If a claim is about current breakage:** prove with a reproducible failing build/test case.
- **If a claim is about portability/design risk:** label as risk tier; defer after Phase 0 unless it blocks correctness. Note: some reproducibility bugs (e.g., `std::hash` cross-toolchain) are inherently cross-environment and cannot be demonstrated on a single machine.

---

## Summary

| Category | Count |
|----------|-------|
| **Both agents identified** | 26 items |
| **ChatGPT only** | 1 item |
| **Claude only** | 19 items |
| **Total** | **46 actionable items** |

**Priority order:** Correctness and reproducibility (Phases 0-1) before documentation and process (Phases 2-3) before backlog modernization.

**Next step:** Work through Phase 0 items in order. Each item is self-contained and can be tackled independently unless noted otherwise.
