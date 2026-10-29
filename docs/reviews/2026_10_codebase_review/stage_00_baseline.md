# Stage 00 — Baseline
Model / effort: Sonnet / medium   Commit: 3f19579   Date: 2026-10-12

All raw logs are in `SCR/logs/` where
`SCR=/private/tmp/claude-501/-Users-robiragan-Dropbox-SocialChoiceStudioDev/2402f97e-221e-494b-88cb-fc63c41efc50/scratchpad/stage0`.
Nothing was written in the repo except this file.

## Summary

- **Builds:** Release, Debug+warnings, ASan/UBSan and coverage builds all configure and compile cleanly (0 errors).
- **C++ tests (Release):** 35/36 pass. The one failure is `DistanceSpeedTest.AccuracyCheck` (15 assertion failures, all Minkowski order p=100). Same failure in ASan and coverage builds. See S00-001. Longest test is 0.45 s; none exceeds 10 s.
- **Sanitizers (ASan + UBSan, Debug):** 0 sanitizer errors in 36 tests (same single test failure as above). `UncoveredSetTest` takes 63.9 s under ASan (0.45 s in Release); everything else is under 4 s. LeakSanitizer was turned off (`detect_leaks=0`; not supported by Apple clang on macOS).
- **Python:** 302 passed, 0 failed, 0 skipped. Line coverage 91% overall; lowest module `_loader.py` 60%.
- **R:** testthat 858 pass, 0 fail, 0 warn, 0 skip. `R CMD check`: 0 errors, 2 warnings, 0 notes (both warnings are about build setup, see section 7). covr total 88.88%; lowest `R/stream_manager.R` 48.72%, `src/stream_manager.c` 50.96%, `R/winset.R` 64.00%, `R/profile.R` 68.18%.
- **C++ coverage (ctest only):** 72.15% lines, 94.55% functions, 76.81% regions overall. Files under 70% lines: `geometry/geom2d.h` 33%, `aggregation/voter_sampler.h` 51%, `geometry/winset.h` 57%, `src/c_api/scs_api.cpp` 60%, `competition/termination.h` 60%, `competition/competition_types.h` 64%, `indifference/level_set.h` 65%. The C API file (3,815 lines) is the biggest gap by absolute missed lines (1,531).
- **Warnings:** no `-Wshadow`, `-Wconversion` (non-sign) or `-Wpedantic` warnings in project code. 137 unique `-Wsign-conversion` sites in project files (26 in `scs_api.cpp`, rest in headers and tests), 148 `-Wunused-result` and 6 other warnings, all in tests.
- **Lint:** cpplint 0 errors; clang-format 0 unformatted files; ruff 512 diagnostics (131 magic-value, 127 line-length; 9 F401, 4 F841, 1 F811, 13 B017, 2 B007, 1 B905); lintr 329 (mostly line length and indentation).
- **clang-tidy (two passes):** the broad pass found 2 `clang-analyzer` paths worth a look (a possible out-of-bounds write at `scs_api.cpp:1894`, a possible leak at `yolk.h:229`), 23 `bugprone-unchecked-optional-access`, 25 `bugprone-implicit-widening-of-multiplication-result` (15 in include/src), 1 `bugprone-exception-escape` (`strategy_registry.h:54`). These are tool reports, not confirmed bugs.
- **Repo:** no tracked build artifacts or `.pyc`/`.DS_Store`. Zero TODO/FIXME/XXX/HACK in project code (only vendored JS and one docs line). `build/` and `build17/` are about 230 MB each (not tracked).
- **Alarming:** nothing crashes or corrupts memory under sanitizers. The p=100 accuracy failure (S00-001) is the only red test.

### S00-001 [P2, confirmed] DistanceSpeedTest.AccuracyCheck fails at Minkowski order 100
- **Where:** tests/unit/test_distance_speed.cpp:170 and :178 (assertions); library side not investigated.
- **What:** In every build type tried (Release, Debug+ASan/UBSan, Debug+coverage), 15 `EXPECT_NEAR(..., 1e-15)` assertions fail, all with the message "failed for order 100": 7 equal-weight, 8 weighted. Largest difference between the library and the test's reference computation is 3.26e-9 (equal weights: 170.0790433560357 vs 170.07904335929086). Other differences are 1.4e-14 to 5.7e-14. gtest itself notes that 1e-15 is below the spacing of doubles at this magnitude.
- **Failure scenario:** Library distance with p=100 vs. a raw `pow`-based reference → mismatch of up to 3e-9 absolute (about 2e-11 relative) in the equal-weight case. Either the tolerance is unrealistic (most failures are 1 ulp or a few ulp) or the library uses a different formula at large p. The 3.26e-9 case is far more than a few ulp, so it needs a look.
- **Suggested fix:** Stage owner for distance functions should decide whether the 3e-9 difference is an accuracy loss in the library (large-p path) or in the reference, and switch the test to a relative tolerance.
- **Also affects:** none known (the test is labelled `benchmark` in ctest).

No sanitizer errors were found, so no sanitizer findings.

## 1. Environment

| Item | Value |
|---|---|
| Commit / branch | 3f19579 / main |
| `git status --porcelain` at start | ` M docs/status/follow_up_queue.md` (3 added lines, pre-existing), `?? docs/reviews/` (review folder, pre-existing) |
| cmake | 4.2.3 |
| Compiler | Apple clang 21.0.0 (clang-2100.3.34.2); SDK MacOSX27.0 |
| clang-tidy / run-clang-tidy | Homebrew LLVM 23.1.3 (`/opt/homebrew/opt/llvm/bin`) |
| Python | 3.11.9; pytest 9.0.2; ruff 0.17.0 |
| R | 4.5.2 (aarch64-apple-darwin20) |
| cpplint | 2.0.2 |
| CPU | 12 cores |

Deviations from the prompt (none change results):
- `run-clang-tidy` given explicit file paths matched 0 files; I passed a regex (`.*/socialchoicelab/(src|tests/unit)/.*\.cpp$`) instead. It then analysed all 38 translation units (2 in `src/`, 36 in `tests/unit/`).
- In `$SCR/r_copy` I deleted the untracked `.Renviron`, `.RData`, `.Rhistory` copied from `r/`. The `.Renviron` sets `SCS_LIB_PATH`, `SCS_INCLUDE_PATH`, `DYLD_LIBRARY_PATH` and would have overridden my env vars with the developer's paths. I did not read its values.
- I added `DYLD_LIBRARY_PATH=$SCR/build_release` for the R steps (same reason).
- Warnings build was re-run with `-j1` after a `-j6` run produced interleaved log lines; counts below are from the `-j1` log, de-duplicated by `file:line`.

## 2. C++ build and tests

Release build (`$SCR/build_release`): configured and built with 0 errors in 18.6 s wall.
`ctest`: 35/36 passed, total real time 2.45 s.

| Test | Result | Time (s) |
|---|---|---|
| DistanceFunctionsTest | Passed | 0.08 |
| PRNGTest | Passed | 0.05 |
| DistanceComparisonTest | Passed | 0.04 |
| **DistanceSpeedTest** | **Failed** | 0.09 |
| PerformanceComparisonTest | Passed | 0.04 |
| LossFunctionsTest | Passed | 0.04 |
| CoreTypesTest | Passed | 0.04 |
| LevelSetTest | Passed | 0.04 |
| Geom2dTest | Passed | 0.05 |
| ConvexHullTest | Passed | 0.05 |
| MajorityTest | Passed | 0.04 |
| WinsetTest | Passed | 0.08 |
| YolkTest | Passed | 0.22 |
| UncoveredSetTest | Passed | 0.45 |
| WinsetOpsTest | Passed | 0.10 |
| CoreTest | Passed | 0.08 |
| CopelandTest | Passed | 0.05 |
| HeartTest | Passed | 0.05 |
| CentralityTest | Passed | 0.04 |
| GeometryIntegrationTest | Passed | 0.12 |
| Voronoi2dTest | Passed | 0.04 |
| WinSet1dTest | Passed | 0.04 |
| ProfileTest | Passed | 0.04 |
| VoterSamplerTest | Passed | 0.04 |
| VotingRulesTest | Passed | 0.04 |
| AggregationPropertiesTest | Passed | 0.04 |
| AggregationIntegrationTest | Passed | 0.04 |
| CompetitorInitializationTest | Passed | 0.04 |
| CompetitionStrategiesTest | Passed | 0.03 |
| CompetitionMotionTest | Passed | 0.06 |
| SeatAllocationTest | Passed | 0.03 |
| CompetitionElectionFeedbackTest | Passed | 0.03 |
| CompetitionEngineTest | Passed | 0.03 |
| CompetitionTerminationTest | Passed | 0.04 |
| CompetitionExperimentRunnerTest | Passed | 0.04 |
| CApiTest | Passed | 0.13 |

Tests over 10 s in Release: none. The failure is S00-001. Failure excerpt:

```
test_distance_speed.cpp:170: Failure
The difference between our_equal and raw_equal is 3.2551668027736014e-09, where
our_equal evaluates to 170.0790433560357,
raw_equal evaluates to 170.07904335929086.
The abs_error parameter 1e-15 ... smaller than the minimum distance between doubles ...
Equal weights accuracy failed for order 100
```

Also visible in the same test's output: for Minkowski order 1000 the library takes 70 ns vs. 4 ns for the raw reference (ratio about 17x), and for order 3 and 10 it is about 1.2x slower than raw. Reported only; the test does not assert on it.

## 3. Compiler warnings

Debug build with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion`: 0 errors. Raw log shows 1356 warning lines; 245 are in Eigen (244 `-Wsign-conversion`, 1 `-Wcharacter-conversion`) and are ignored. The project-file count below is unique `file:line` sites.

By type (project files only):

| Warning | Unique sites | Where |
|---|---|---|
| `-Wsign-conversion` | 137 | 26 `src/c_api/scs_api.cpp`; 98 in `include/` (see below); 13 in tests |
| `-Wunused-result` | 148 | tests only |
| `-Wimplicit-int-float-conversion` | 2 | tests |
| `-Wunused-variable` | 2 | tests |
| `-Wunused-function` | 1 | tests |
| `-Wzero-length-array` | 1 | tests |
| `-Wshadow`, `-Wpedantic`, `-Wextra`, other `-Wconversion` | 0 | |

`-Wsign-conversion` by file: `scs_api.cpp` 26; `aggregation/seat_allocation.h` 14; `competition/election_feedback.h` 12; `aggregation/profile.h` 12; `competition/engine.h` 10; `aggregation/borda.h` 7; `aggregation/approval.h` 7; `distance/distance_functions.h` 6; `aggregation/social_ranking.h` 6; `aggregation/scoring_rule.h` 6; `tests/unit/test_aggregation_integration.cpp` 5; `tests/unit/test_voting_rules.cpp` 4; `aggregation/profile_generators.h` 4; `competition/experiment_runner.h` 3; `aggregation/plurality.h` 3; `aggregation/antiplurality.h` 3; 2 each in `geometry/winset.h`, `competition/strategy_registry.h`, `aggregation/condorcet_consistency.h`; 1 each in `geometry/majority.h`, `core/rng/stream_manager.h`, `aggregation/pareto.h`.

`-Wunused-result` by file (all in tests, all calls to `[[nodiscard]]` functions whose result is dropped, presumably inside `EXPECT_THROW`): `test_prng.cpp` 29, `test_distance_functions.cpp` 22, `test_loss_functions.cpp` 14, `test_centrality.cpp` 12, `test_yolk.cpp` 9, `test_voter_sampler.cpp` 8, `test_profile.cpp` 8, `test_majority.cpp` 7, `test_voting_rules.cpp` 6, `test_copeland.cpp` 6, `test_aggregation_properties.cpp` 5, `test_winset_ops.cpp` 4, `test_winset.cpp` 4, `test_heart.cpp` 4, `test_aggregation_integration.cpp` 4, `test_voronoi_2d.cpp` 3, `test_core.cpp` 2, `test_uncovered_set.cpp` 1.

Warnings outside the conversion families, individually:
- tests/unit/test_distance_speed.cpp:30 `-Wimplicit-int-float-conversion`
- tests/unit/test_distance_speed.cpp:204 `-Wimplicit-int-float-conversion`
- tests/unit/test_uncovered_set.cpp:80 `-Wunused-function`
- tests/unit/test_loss_functions.cpp:344 `-Wunused-variable`
- tests/unit/test_loss_functions.cpp:345 `-Wunused-variable`
- tests/unit/test_c_api.cpp:2097 `-Wzero-length-array`
- (linker) `ld: warning: ignoring duplicate libraries: 'libsocialchoicelab_core.a'`

## 4. Sanitizers

Debug build with `-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined` (compiler and linker flags). `ctest --timeout 300`, run with `ASAN_OPTIONS=detect_leaks=0`.

- Result: 35/36 passed; `DistanceSpeedTest` failed with the same 15 accuracy assertions as in Release (no sanitizer output). Total 82 s.
- Sanitizer reports (AddressSanitizer / runtime error / UndefinedBehaviorSanitizer): **0**.
- Slow: `UncoveredSetTest` 63.87 s; other slow ones are `CApiTest` 2.68 s and two others under 3.5 s. No timeouts.
- Leak detection was not run.

## 5. C++ coverage

Debug build with `-fprofile-instr-generate -fcoverage-mapping`; `ctest` 35/36 pass (same failure). 37 `.profraw` files merged; `llvm-cov report` over all 36 test executables plus `libscs_api.dylib` (needed so `src/c_api/scs_api.cpp` is counted), limited to `include/socialchoicelab/` and `src/`. llvm-cov warned "1276 functions have mismatched data" (header-only templates instantiated in many binaries), so per-file numbers for headers may differ slightly between runs.

**Total: 72.15% lines (2162 of 7764 missed), 94.55% functions, 76.81% regions.**

Files below 70% line coverage:

| Lines % | Funcs % | File | Missed/total lines |
|---|---|---|---|
| 33.33 | 50.00 | include/socialchoicelab/geometry/geom2d.h | 12/18 |
| 51.11 | 33.33 | include/socialchoicelab/aggregation/voter_sampler.h | 22/45 |
| 57.04 | 88.89 | include/socialchoicelab/geometry/winset.h | 122/284 |
| 59.87 | 96.13 | src/c_api/scs_api.cpp | 1531/3815 |
| 60.32 | 100.00 | include/socialchoicelab/competition/termination.h | 25/63 |
| 64.29 | 100.00 | include/socialchoicelab/competition/competition_types.h | 30/84 |
| 65.42 | 100.00 | include/socialchoicelab/preference/indifference/level_set.h | 102/295 |

All other files, lowest first:

| Lines % | Funcs % | File |
|---|---|---|
| 71.08 | 100.00 | competition/adaptation.h |
| 73.98 | 90.91 | competition/strategy_registry.h |
| 78.07 | 85.71 | competition/experiment_runner.h |
| 80.22 | 100.00 | aggregation/seat_allocation.h |
| 80.75 | 85.71 | competition/engine.h |
| 80.83 | 100.00 | competition/election_feedback.h |
| 83.33 | 100.00 | competition/state_signature.h |
| 86.79 | 100.00 | aggregation/borda.h |
| 87.50 | 100.00 | competition/step_policy.h |
| 88.24 | 100.00 | aggregation/profile_generators.h |
| 88.24 | 100.00 | aggregation/tie_break.h |
| 89.36 | 100.00 | competition/boundary_policy.h |
| 89.66 | 100.00 | competition/initialization.h |
| 90.00 | 81.82 | core/rng/stream_manager.h |
| 90.48 | 100.00 | src/core/rng/stream_manager.cpp |
| 90.51 | 100.00 | preference/loss/loss_functions.h |
| 92.00 | 100.00 | geometry/winset_1d.h |
| 94.12 | 100.00 | aggregation/pareto.h |
| 94.96 | 100.00 | preference/distance/distance_functions.h |
| 95.00 | 100.00 | aggregation/social_ranking.h |
| 95.57 | 87.50 | core/rng/prng.h |
| 97.12 | 100.00 | geometry/majority.h |
| 97.69 | 100.00 | geometry/heart.h |
| 98.47 | 88.89 | geometry/yolk.h |
| 100 | 100 | aggregation/antiplurality.h, approval.h, condorcet_consistency.h, plurality.h, profile.h, scoring_rule.h; competition/motion.h; geometry/centrality.h, convex_hull.h, copeland.h, core.h, uncovered_set.h, voronoi_2d.h, winset_ops.h |

Note: C API coverage here is from `test_c_api` only. The Python and R suites also exercise `scs_api.cpp` but are not counted.

## 6. Python tests

Command as specified, with `SCS_LIB_PATH=$SCR/build_release`. Result: **302 passed, 0 failed, 0 skipped, 0 xfail** in 1.13 s.

Line coverage by module (lowest first; total 91%, 2284 statements, 213 missed):

| Module | Cover |
|---|---|
| **_loader.py** | **60%** (14 missed: 33, 37, 46-64, 874-875) |
| _winset.py | 80% |
| _stream_manager.py | 84% |
| _competition.py | 86% |
| _profile.py | 91% |
| service.py | 91% |
| _geometry.py | 92% |
| _functions.py | 93% |
| _error.py | 93% |
| plots.py | 93% |
| _voting_rules.py | 95% |
| scenarios.py | 96% |
| _types.py | 97% |
| palette.py | 97% |
| _social.py | 99% |
| `__init__.py`, `data/__init__.py`, `data/scenarios/__init__.py` | 100% |

Modules below 70%: `_loader.py` only. Missing-line lists are in `SCR/logs/py.log`.

## 7. R CMD check

`R CMD build . --no-build-vignettes` succeeded (`socialchoicelab_0.3.0.tar.gz`). `R CMD check --no-manual --no-build-vignettes --no-vignettes`: **Status: 2 WARNINGs, 0 ERRORs, 0 NOTEs.**

WARNING 1 (verbatim):
```
* checking whether package ‘socialchoicelab’ can be installed ... WARNING
Found the following significant warnings:
  /Library/Frameworks/R.framework/Resources/include/R_ext/Boolean.h:62:36: warning: unknown warning group '-Wfixed-enum-extension', ignored [-Wunknown-warning-option]
```
WARNING 2 (verbatim):
```
* checking for GNU extensions in Makefiles ... WARNING
Found the following file(s) containing GNU extensions:
  src/Makevars
Portable Makefiles do not use GNU extensions such as +=, :=, $(shell),
$(wildcard), ifeq ... endif, .NOTPARALLEL See section ‘Writing portable
packages’ in the ‘Writing R Extensions’ manual.
```

testthat summary: `[ FAIL 0 | WARN 0 | SKIP 0 | PASS 858 ]`.

covr package coverage: **88.88%**. Per file, lowest first:

| Cover % | File |
|---|---|
| 48.72 | R/stream_manager.R |
| 50.96 | src/stream_manager.c |
| 64.00 | R/winset.R |
| 68.18 | R/profile.R |
| 76.74 | R/competition.R |
| 78.95 | src/winset_1d.c |
| 84.03 | src/profile.c |
| 84.91 | src/voronoi.c |
| 85.37 | src/winset.c |
| 87.88 | R/scenarios.R |
| 89.67 | src/competition.c |
| 90.64 | src/functions.c |
| 91.53 | R/competition_canvas.R |
| 92.13 | src/voting_rules.c |
| 93.10 | R/voter_sampler.R |
| 93.22 | R/palette.R |
| 95.12 | R/social.R |
| 95.30 | R/spatial_voting_canvas.R |
| 95.77 | R/voting_rules.R |
| 96.99 | R/utils.R |
| 97.59 | src/social.c |
| 100 | R/functions.R, R/geometry.R, src/geometry.c, src/init.c, src/scs_r_helpers.h |

Below 70%: `R/stream_manager.R`, `src/stream_manager.c`, `R/winset.R`, `R/profile.R`.

## 8. Linters

### cpplint (`--root=include` over `include/`, `src/`, `tests/unit/`, 87 files)
0 errors, no categories to report.

### clang-format (`--dry-run --Werror`, same 87 files)
0 files need formatting.

### clang-tidy
Both passes used `-p $SCR/build_release`, `--header-filter='.*/(include/socialchoicelab|src)/.*'`, 38 translation units. Counts are unique diagnostics (file, line, column, check), deduplicated across translation units, restricted to project paths. Raw logs are 84 MB and 30 MB (header diagnostics repeat per TU). Parsed summaries: `SCR/logs/tidy_a_summary.txt`, `tidy_b_summary.txt`.

**(a) Repo `.clang-tidy`** (bugprone-* minus 3, readability-* minus 3, 4 modernize/performance checks). 1146 unique diagnostics (tests 470, src 437, include 239).

| Check | Count |
|---|---|
| readability-implicit-bool-conversion | 300 |
| readability-braces-around-statements | 260 |
| readability-trailing-comma | 182 |
| readability-uppercase-literal-suffix | 148 |
| readability-isolate-declaration | 66 |
| readability-math-missing-parentheses | 65 |
| bugprone-throwing-static-initialization | 33 |
| bugprone-implicit-widening-of-multiplication-result | 25 |
| bugprone-unchecked-optional-access | 23 |
| bugprone-signed-bitwise | 6 |
| readability-container-contains | 5 |
| readability-redundant-lambda-parameter-list | 5 |
| readability-use-std-min-max | 4 |
| performance-unnecessary-copy-initialization | 4 |
| bugprone-misplaced-widening-cast | 3 |
| bugprone-random-generator-seed | 3 |
| readability-convert-member-functions-to-static | 3 |
| readability-else-after-return | 3 |
| readability-qualified-auto | 3 |
| readability-use-anyofallof | 2 |
| bugprone-exception-escape | 1 |
| readability-non-const-parameter | 1 |
| readability-redundant-qualified-alias | 1 |

`bugprone-*` individually, include/ and src/ (all of them):
- include/socialchoicelab/competition/initialization.h:167, :181 unchecked-optional-access
- include/socialchoicelab/competition/strategy_registry.h:54 exception-escape (`find`)
- include/socialchoicelab/core/rng/stream_manager.h:244, :246, :248, :266, :298 (two at 298) signed-bitwise
- include/socialchoicelab/geometry/heart.h:251 misplaced-widening-cast
- include/socialchoicelab/geometry/uncovered_set.h:238 misplaced-widening-cast
- include/socialchoicelab/geometry/yolk.h:369 misplaced-widening-cast
- include/socialchoicelab/geometry/voronoi_2d.h:178 implicit-widening-of-multiplication-result
- src/c_api/scs_api.cpp implicit-widening-of-multiplication-result at lines 263, 865, 967, 998, 1087, 1127, 1188, 1254, 1256, 1324, 1660, 1821, 1889, 2047

`bugprone-*` in tests/unit (summarised; full list in the log):
- unchecked-optional-access: test_core.cpp 11, test_competition_engine.cpp 6, test_competition_election_feedback.cpp 4
- throwing-static-initialization: 33 total, in test_aggregation_integration.cpp (7), test_geometry_integration.cpp (12), test_heart.cpp (5), test_winset_ops.cpp (3), test_copeland/core/majority/profile/voting_rules/winset.cpp (1 each)
- implicit-widening-of-multiplication-result: test_c_api.cpp lines 429, 532, 1229, 1230, 2116, 2132, 2152, 2222, 2238
- random-generator-seed: test_distance_comparison.cpp:17, test_distance_speed.cpp:34, test_performance_comparison.cpp:20

No `clang-analyzer-*` diagnostics in pass (a); that check family is not enabled in the repo config.

**(b) Broad pass** (bugprone, clang-analyzer, performance, concurrency, member-init, narrowing, misc). 1090 unique diagnostics (tests 865, include 119, src 106).

| Check | Count |
|---|---|
| misc-const-correctness | 873 |
| bugprone-easily-swappable-parameters | 33 |
| bugprone-throwing-static-initialization | 33 |
| misc-use-anonymous-namespace | 31 |
| bugprone-implicit-widening-of-multiplication-result | 25 |
| bugprone-unchecked-optional-access | 23 |
| performance-enum-size | 14 |
| misc-unused-using-decls | 11 |
| misc-use-internal-linkage | 8 |
| performance-avoid-endl | 6 |
| bugprone-signed-bitwise | 6 |
| cppcoreguidelines-pro-type-member-init | 5 |
| performance-unnecessary-copy-initialization | 4 |
| bugprone-random-generator-seed | 3 |
| bugprone-misplaced-widening-cast | 3 |
| clang-analyzer-optin.core.EnumCastOutOfRange | 2 |
| clang-analyzer-deadcode.DeadStores | 2 |
| bugprone-narrowing-conversions / cppcoreguidelines-narrowing-conversions | 2 |
| bugprone-exception-escape | 1 |
| performance-inefficient-vector-operation | 1 |
| misc-override-with-different-visibility | 1 |
| clang-analyzer-cplusplus.NewDeleteLeaks | 1 |
| clang-analyzer-security.ArrayBound | 1 |
| performance-use-std-move | 1 |

`clang-analyzer-*` individually:
- **src/c_api/scs_api.cpp:1894** `security.ArrayBound`: "Out of bound access to memory after the end of the heap area" at `cs_buf[n_cells] = xy_pos;`, on a path that assumes `sites_xy` and `out` are non-null (function starting near line 1846, `scs_voronoi_cells_2d`-area). Tool report, not verified.
- **include/socialchoicelab/geometry/yolk.h:229** `cplusplus.NewDeleteLeaks`: "Potential leak of memory pointed to by field 'PTR'" on a path through `limiting_quantile_lines_2d` -> `quantile_lines_2d_annotated` (from test_yolk.cpp:535). Tool report, not verified.
- tests/unit/test_distance_functions.cpp:155 `EnumCastOutOfRange` (value 99 cast to `DistanceType`, intentional invalid-enum test)
- tests/unit/test_loss_functions.cpp:259 `EnumCastOutOfRange` (value 999 to `LossFunctionType`, same pattern)
- tests/unit/test_loss_functions.cpp:344, :345 `DeadStores` (`f_gaussian`, `f_threshold`; same lines as the `-Wunused-variable` warnings)

`bugprone-*` individually that are new in pass (b) (the rest repeat pass (a)):
- narrowing-conversions: tests/unit/test_distance_speed.cpp:30, :204 (same lines as the compiler warnings)
- easily-swappable-parameters (33): include/…/aggregation/condorcet_consistency.h:39; competition/experiment_runner.h:104; core/rng/stream_manager.h:261; geometry/heart.h:218; uncovered_set.h:80, :194; voronoi_2d.h:127, :164; winset.h:260, :321, :513; winset_1d.h:44; yolk.h:336; preference/loss/loss_functions.h:184, :185, :249; src/c_api/scs_api.cpp:314, :487, :488, :1444, :1612, :1613, :1781, :2660, :2717, :3300, :3335, :3383, :3710, :3761, :3877, :4358; tests/unit/test_competition_strategies.cpp:46

### ruff
`ruff check --no-cache --select E,F,W,B,UP,SIM,PL,RUF python/src python/tests app`: **512 diagnostics** (60 auto-fixable).

| Rule | Count | Rule | Count |
|---|---|---|---|
| PLR2004 | 131 | RUF100 | 2 |
| E501 | 127 | RUF043 | 2 |
| RUF059 | 41 | RUF012 | 2 |
| PLC0415 | 36 | PLR0915 | 2 |
| PLR0917 | 21 | B007 | 2 |
| PLR0913 | 21 | UP017 | 1 |
| UP037 | 19 | UP015 | 1 |
| SIM117 | 19 | PLW2901 | 1 |
| B017 | 13 | F811 | 1 |
| RUF005 | 12 | B905 | 1 |
| UP006 | 11 | F401 | 9 |
| RUF022 | 8 | RUF002 | 8 |
| E402 | 7 | F841 | 4 |
| UP035 | 4 | RUF003 | 3 |
| PLR0912 | 3 | | |

Every F and B diagnostic:
- python/src/socialchoicelab/_functions.py:11:36 F401 unused import `SCSError`
- python/src/socialchoicelab/_loader.py:59:13 F841 local `lib` assigned but never used
- python/src/socialchoicelab/_types.py:19:36 F401 `dataclasses.field` unused
- python/src/socialchoicelab/_types.py:24:43 F401 `_loader._lib` unused
- python/src/socialchoicelab/plots.py:24:8 F401 `warnings` unused
- python/src/socialchoicelab/plots.py:39:5 F401 `level_set_to_polygon` unused
- python/src/socialchoicelab/plots.py:42:37 F401 `winset_2d` unused
- python/src/socialchoicelab/plots.py:125:58 B905 `zip()` without `strict=`
- python/src/socialchoicelab/plots.py:559:12 F811 redefinition of unused `warnings` (from line 24)
- python/src/socialchoicelab/plots.py:616:9 F841 `n_frames` unused
- python/src/socialchoicelab/plots.py:689:9 F841 `n_frames_ws` unused
- python/src/socialchoicelab/plots.py:712:24 F401 `math` unused
- python/src/socialchoicelab/plots.py:1132:13 B007 loop variable `frame_i` unused
- python/src/socialchoicelab/service.py:33:37 F401 `typing.Optional` unused
- python/tests/test_b8.py:16:37 F401 `_ffi` unused
- python/tests/test_b8.py B017 `assertRaises/raises(Exception)` at lines 58, 64, 162, 167, 172, 177, 182, 187, 191, 195, 201, 216, 221
- python/tests/test_competition_canvas_ic.py:97:13 B007 loop variable `s_i` unused
- python/tests/test_voter_sampler.py:79:5 F841 `headings` unused

### lintr (`lint_package(".")`, default linters, in `$SCR/r_copy`)
329 lints:

| Linter | Count |
|---|---|
| line_length_linter | 180 |
| indentation_linter | 108 |
| object_name_linter | 14 |
| commented_code_linter | 8 |
| spaces_inside_linter | 6 |
| object_usage_linter | 4 |
| semicolon_linter | 4 |
| object_length_linter | 3 |
| assignment_linter | 1 |
| brace_linter | 1 |

Non-style lints (everything except line length, indentation, quotes, spacing, object naming):
- R/competition_canvas.R:32 object_length (name over 30 chars)
- R/competition_canvas.R:621 object_usage: `ws_competitor_indices` assigned but may not be used
- R/functions.R:309, :373 semicolon
- R/functions.R:470 object_usage: `ideal_u` assigned but may not be used
- R/geometry.R:8, :9 commented_code
- R/palette.R:8, :9, :10, :11, :116 commented_code
- R/utils.R:157 object_length
- tests/testthat/compound_api_interactive_common.R:146 assignment (`<<-`)
- tests/testthat/compound_api_interactive_common.R:211 object_usage: `ang_ok` assigned but may not be used
- tests/testthat/compound_api_interactive_common.R:284 object_length
- tests/testthat/helper_lib.R:5 brace and semicolon
- tests/testthat/test_competition.R:120 object_usage: no visible global function `skip`
- tests/testthat/test_config.R:58 semicolon
- tests/testthat/test_scenarios.R:1 commented_code

## 9. Repo inventory

**Tracked files that look like artifacts:** none match `__pycache__`, `.pyc`, `.DS_Store`, `.Rcheck`, `*.o`, `*.so`, `*.dylib`, `*.a`, tracked `build*/` content, `.Rproj.user`, `.RData`, `.Rhistory`.
**Tracked files over 1 MB (1,000,000 bytes):** one, `docs/site/assets/javascripts/bundle.79ae519e.min.js.map` (1,026,989 B). Next largest: `docs/site/notebooks/spatial_voting/index.html` 713,352 B; `docs/site/assets/javascripts/lunr/wordcut.js` 677,463 B; `r/inst/htmlwidgets/lib/three/three.min.js` and `python/src/socialchoicelab/three.min.js` (identical blob, 608,313 B each).

**Directories:**

| Path | Tracked files | Untracked non-ignored | In .gitignore? | Size |
|---|---|---|---|---|
| build | 0 | 0 | yes (`build/`) | 232 MB |
| build17 | 0 | 0 | no directory entry; contents ignored by file patterns (e.g. `CMakeCache.txt`, `CMakeFiles/`, `.DS_Store`) | 230 MB |
| debug | 0 | 0 | yes (`debug/`) | 0 B (empty) |
| scratch | 0 | 0 | yes (`scratch/`) | 8 KB (holds `dump_heart.cpp`, `dump_winset.cpp`, which appear in the CMake compile database) |
| temp | 0 | 0 | yes (`temp/`) | 24 KB |
| tmp | 1 (`tmp/.gitkeep`) | 0 | yes (`tmp/`) | 992 KB |
| r/tmp | 0 | 0 | yes (`r/tmp/`) | 11 MB |
| papers_and_presentations | 0 | 0 | yes | 17 MB |
| agent_conversations | 0 | 0 | yes | 172 KB |
| python/notebooks | 2 | 0 | no (tracked) | 16 KB |

**Demo scripts tracked:** python/ (11): `canvas_load_demo.py`, `canvas_save_demo.py`, `competition_1d_demo.py`, `competition_2D_demo_2Cand.py`, `_3Cand.py`, `_5Cand.py`, `compound_api_demo.py`, `compound_api_interactive_demo.py`, `devScript_MAIN.py`, `static_scenarios_demo.py`, `utility_surface_demo.py`. r/ (10): the same names as `.R` files, with `devScript_MAIN.R`.
Tracked file count by top-level folder: r 426, docs 110, python 93, include 49, tests 36, .cursor 13, scripts 8, app 5, src 2.

**TODO / FIXME / XXX / HACK:** 0 in `include/`, `src/`, `tests/`, `r/R/`, `r/tests/`, `python/src/`, `python/tests/`, `app/`, `scripts/`, `tools/`. Elsewhere only vendored or generated files: `docs/site/assets/javascripts/lunr/wordcut.js` (6), `docs/site/assets/javascripts/bundle.79ae519e.min.js.map`, `r/docs/deps/jquery-3.6.0/jquery-3.6.0.js` (7, includes `responseXXX`), plus binary/minified files in `r/docs/deps/`; and `docs/status/ROADMAP.md:286` (a sentence that mentions "TODO").

## 10. Repo unchanged check

`git status --porcelain` before:
```
 M docs/status/follow_up_queue.md
?? docs/reviews/
```
After the final run (before writing this file): identical (`diff` reported no difference). `git diff --stat` shows `docs/status/follow_up_queue.md | 3 +++`, which was already modified when I started (I did not touch it). `docs/reviews/` was already untracked and contains `README.md`, `prompts/stage_00_baseline.md`, `review_rubric.md`; this file is the only addition. No other new or modified files; the repo's `build/` directory was not used.

## Tool failures (if any)

No required tool failed to run. Issues encountered:
- `run-clang-tidy` with a list of explicit file paths reported "0 files out of 76"; an equivalent regex worked (see section 1).
- A first `run-clang-tidy` attempt was piped to `head` and hung; I killed it and re-ran to a log file.
- A parallel `-j6` warnings build interleaved log lines; re-ran at `-j1`.
- A stray `saveRDS()` call in one lintr command wrote to `/x` and failed with "Read-only file system"; harmless, the lint output was already printed and was re-collected in a clean run.
- llvm-cov: "1276 functions have mismatched data" warning (see section 5).
- LeakSanitizer not run (unsupported on this platform).
