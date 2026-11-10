# Stage 04a — C++ tests: core, RNG, distance, loss, level set
Model / effort: Sonnet / high   Commit: 2771655   Files reviewed: 8, 2,098 lines (all read in full)
Not read: none. Test files: `tests/unit/test_{core_types,distance_functions,distance_comparison,distance_speed,performance_comparison,loss_functions,level_set,prng}.cpp`. Code under test was read only to settle specific questions (`prng.h`, `stream_manager.h`, `distance_functions.h`, `loss_functions.h`, `level_set.h` after PR #1).

Scratch area: `SCR4A=/private/tmp/claude-501/-Users-robiragan-Dropbox-SocialChoiceStudioDev/2402f97e-221e-494b-88cb-fc63c41efc50/scratchpad/stage4a/`.
- `build_test.sh`: compiles one repo test file against an include tree copy and the Stage 4 gtest libraries.
- `base/`: unmodified copy of `include/`.
- `m_<name>/`: copies of `include/` with one deliberate bug each (a "mutant"); the repo was not touched.
- `probe1.cpp`/`.out`, `probe2.cpp`/`.out`, `probe3.cpp`/`.out`: programs that reproduce the earlier findings and the Q2 numbers.
- `cov/`, `cov2/`: coverage of this slice's test binaries on their own (`cov2/` uses the `build_cov` binaries with fresh profiles; the shared coverage view understates header-only files, see Tool leads).

## Summary
- **0 P0, 5 P1, 8 P2, 4 P3.** The review stage's `S02a-001` (P0, wrong circle radius for equal non-unit weights) is **fixed and tested**: reverting the fix makes two tests fail (`m_rev001`). The Q18 zero-weight rule is also tested and a revert is caught (`m_rev018`).
- **Four of the five earlier P1s have no test that could catch them** (`S02a-002` seed collisions, `S02a-003` Chebyshev square for p >= 100, `S02a-004` Gaussian IC throws, `S02a-005` `level_set_1d` swallows errors). Each reproduces today (`probe1.out`). The suite does not exercise them: no test uses Gaussian or Threshold loss through a level set, none uses p >= 100 in the level-set code, none varies the master seed in `reset_for_run`.
- **Two tests assert behaviour that Q2 declares wrong** (`S04a-001`). `MinkowskiP100LargeCoordsNoOverflow` expects Chebyshev (100.0) where exact Minkowski gives 100.6956 (7 per mille off). It fails under a correct fix (`m_q2`). `MinkowskiAtChebyshevCutoffBoundary` cannot tell Chebyshev from Minkowski, and stops compiling when the cutoff constant is removed. The failing `S00-001` test is an accuracy test with a reference that copies the cutoff policy (and compares at `>` while the library uses `>=`), plus a 1e-15 tolerance. **It is invisible to CI**, because it lives in a binary labelled `benchmark` and CI runs `ctest -LE benchmark` (`S04a-007`).
- **PRNG tests are range checks only.** Five deliberate bugs all survive all 43 tests in `test_prng.cpp`: `skip()` doing nothing, `uniform_real` always returning `min`, `normal` always returning `mean`, `uniform_int` never returning `max`, `discrete_choice` ignoring weights (`S04a-008`). The suite pins no seeded value anywhere, so the Q4 golden-value test is a pure addition; nothing needs regenerating in this slice.
- Level-set tests leave `level_set_1d` weights, vertex orientation (documented CCW), Gaussian/Threshold inversion, p ~ 1 and p >= 100, and `SCP-001`/`SCP-002` unchecked. All of those mutants survive (`S04a-009`, `S04a-010`).

## Contracts this layer provides
What Stage 11 (fix plan) and later reviewers may rely on about this slice's tests. "Today" means commit 2771655.

**CI behaviour**
- `ctest -LE benchmark` (CI, `scripts/check-all.sh`) skips `DistanceSpeedTest` and `PerformanceComparisonTest`. Everything else in this slice runs in CI. No test in this slice has a timing assertion; the `benchmark` binaries only print. Release `ctest` here: 35/36 pass; the one failure is `DistanceSpeedTest.AccuracyCheck` (15 assertions, order 100 only).
- Tests are deterministic on one platform: fixed seeds, no clocks in assertions. They are not platform-stable in their inputs (see `S04a-015`, `S04a-016`).

**Tests that must change when an owner decision is implemented**

| Decision | Tests to rewrite or delete | New tests required |
|---|---|---|
| Q2 (exact Minkowski, no cutoff) | `DistanceFunctionsTest.MinkowskiP100LargeCoordsNoOverflow` (wrong result asserted), `MinkowskiAtChebyshevCutoffBoundary` (uses `k_minkowski_chebyshev_cutoff`, will not compile), `MonotonicityProperty` comment and line 74 label ("Chebyshev approximation"), reference lambdas in `test_distance_comparison.cpp:54–64` and `test_distance_speed.cpp:60–84`, `AccuracyCheck` tolerance | p = 50, 99, 100, 1000 with differences about 1e4 and 1e-4 (Q2 list); level-set p >= 100 returns a superellipse |
| Q4 (portable draws) | None assert seeded values. 7 tests use `EXPECT_NE` on small-range seeded draws (`S04a-015`). `Skip` uses `engine()` directly | golden values: raw `mt19937_64` outputs, uniform int/real, bernoulli, discrete, shuffle exact; normal/gamma/beta last-bit tolerance; stream-seed golden values (FNV-1a + SplitMix64 are pure integer arithmetic and can be pinned today) |
| `S02a-006` (remove global manager) | `StreamManagerTest.GlobalStreamManager`, `SetGlobalStreamManagerSeedSecondCallProducesDifferentSequences` | none |
| `S02a-012` (remove exponential/gamma/beta/discrete_choice) | 13 tests in `test_prng.cpp` (listed in `S04a-014`) | none |
| `S02a-013` (private `engine()`, no copy) | `Skip`, `CopyProducesIdenticalSequenceFromCopyPoint`, `StateStringReflectsDrawCount` | `skip` equivalence through the public API |

## Findings

### S04a-001 [P1, confirmed] Two distance tests lock in the Chebyshev substitution at p >= 100 (Decision: Q2)
- **Where:** tests/unit/test_distance_functions.cpp:289–300 (`MinkowskiP100LargeCoordsNoOverflow`), :276–285 (`MinkowskiAtChebyshevCutoffBoundary`), :74 (`MonotonicityProperty`).
- **What:** Q2 says exact Minkowski at every finite p >= 1.
  - `MinkowskiP100LargeCoordsNoOverflow` computes Minkowski p = 100 for the difference (100, 100) and asserts it equals `chebyshev_distance` (100.0) with `EXPECT_DOUBLE_EQ`. The exact value is 100 · 2^(1/100) = 100.69555500567189. The test only passes because of the substitution Q2 removes.
  - `MinkowskiAtChebyshevCutoffBoundary` uses points (0,0) and (1000,1). There the exact Minkowski value at p = 100 and p = 100.1 equals 1000 to double precision, so the test cannot distinguish Minkowski from Chebyshev and would pass with or without the cutoff. It also names the constant `k_minkowski_chebyshev_cutoff`, which Q2 deletes, so it stops compiling.
  - `MonotonicityProperty` line 74 labels p = 1000 "(Chebyshev approximation)". The value 1000.0 is also the exact answer for these points, so it keeps passing, but the label records the wrong contract.
- **Failure scenario:** apply the Q2 fix (mutant `m_q2`: scaled Minkowski, cutoff removed). `DistanceFunctionsTest.MinkowskiP100LargeCoordsNoOverflow` fails: `d_mink = 100.69555500567189`, `d_cheb = 100`. `MinkowskiAtChebyshevCutoffBoundary` still passes (so it would silently test nothing). The correct fix is blocked by a test that asserts the old answer.
- **Suggested fix:** replace both with exact-value tests at p = 50, 99, 100, 1000 and differences 1e4 and 1e-4, expected value `m · (Σ(wd/m)^p)^(1/p)` computed in a different way (a hand value such as 10000 · 2^(1/99) = 10070.2605438349…), relative tolerance about 1e-12. Update the label at :74.
- **Also affects:** `test_winset*.cpp` and `test_majority.cpp` grep-match `100.0`; other slices should check whether they assume Chebyshev at p >= 100.

### S04a-002 [P1, confirmed] No test can catch `reset_for_run` seed collisions (S02a-002)
- **Where:** tests/unit/test_prng.cpp:420–458 (`ResetForRunReproducibility`, `ResetForRunDifferentIndicesDifferentSequences`).
- **What:** both tests use a single master seed (42) and run indices 0, 1, 2. They check that equal inputs give equal output and that three indices give different first draws. Neither varies the master seed. The collision (`combine_seed(m, r)` equals `combine_seed(m', r')` when m ^ r = m' ^ r') needs two different masters.
- **Failure scenario:** `probe1.out`: `run_seed(1,0) == run_seed(0,1)` (both first draws 0.71079655983611456); masters 1 vs 2, 100 runs: 100/100 run seeds shared. `PRNGTest`/`StreamManagerTest` pass on this code today (ctest 35/36).
- **Suggested fix:** add a test that masters m, m+1, m^d (for several m, d) with N = 100 and 1000 runs give pairwise disjoint run-seed sets, using `master_seed()` after `reset_for_run`. Add a golden value for `combine_seed` once the new mixer is chosen.
- **Also affects:** `competition/experiment_runner.h` tests (slice 4d) should assert distinct `run_seed` values across masters.

### S04a-003 [P1, confirmed] The p >= 100 → Chebyshev-square branch of `level_set_2d` has no test (S02a-003; Decision: Q2)
- **Where:** tests/unit/test_level_set.cpp (no Minkowski test with p >= 100 or p ~ 1); level_set.h:262–270 (uncovered in the slice-binary coverage recheck, `cov2/`).
- **What:** the only Minkowski level-set test uses p = 3 (`MinkowskiMiddlePReturnsSuperellipse`; `IcPolygonVerticesAtWeightedDistanceForAllMetrics` also uses p = 3). Nothing runs p >= 100 or p <= 1 + 1e-9.
- **Failure scenario:** mutant `m_lsp100` (branch deleted): all 14 tests pass. `probe1.out`: p = 150, linear loss, level −1 returns a `Polygon2d` square with corner (1,1), whose exact distance is 1.004632. After the Q2 fix the shape must be a `Superellipse2d`; no test says so.
- **Suggested fix:** add tests at p in {1 + 1e-10, 50, 99, 100, 150, 1000}: result type, and every vertex of `ic_polygon_2d` at weighted distance r under `calculate_distance` with relative tolerance 1e-12. These are the same tests Q2 asks for in the distance layer.
- **Also affects:** `test_c_api.cpp` (slice 4e): `scs_level_set_2d` output type changes from POLYGON to SUPERELLIPSE for p >= 100.

### S04a-004 [P1, confirmed] No test of a level set or IC with Gaussian or Threshold loss (S02a-004, S02a-007, S02a-019)
- **Where:** tests/unit/test_level_set.cpp (all 14 tests use QUADRATIC or LINEAR); level_set.h:119–157 (Gaussian and Threshold inversion). No other C++ test file uses Gaussian or Threshold loss together with `level_set_*`/`ic_polygon_2d`/`scs_ic_*` (grep of `SCS_LOSS_`/`LossFunctionType::` outside `test_loss_functions` and `test_level_set`: no matches).
- **What:** this slice's binaries leave lines 119–159 (both inversions), 91–92 (non-finite level) and the error branches 96–102, 112–114 uncovered.
- **Failure scenario:** `probe1.out`: Gaussian, defaults, `ic_polygon_2d` with sq (7,0) throws "utility level at or below minimum for Gaussian"; sq (6,0) gives radius 6.00363668 (true 6). Also `gaussian_loss(1e-9)` returns 0 (true 1e-18). The suite passes with all of this.
- **Suggested fix:** for each loss type, test `ic_polygon_2d` vertices against `calculate_distance` for βd² in {1e-18-scale, 1, 30, 40}; test `level_set_2d` round trip `distance_to_utility(level_set radius) == level` for Gaussian and Threshold; test Gaussian `max_loss <= 0` message order (S02a-019). The Threshold-inside-zone case depends on Q19 (unanswered): add the test after the owner answers.
- **Also affects:** `test_c_api.cpp` (4e) has the same gap through `scs_ic_polygon_2d` and `scs_ic_interval_1d`.

### S04a-005 [P1, confirmed] No test that `level_set_1d` rejects an invalid loss configuration (S02a-005)
- **Where:** tests/unit/test_level_set.cpp:63–73 (`EmptyWhenLevelUnreachable`), :34–61.
- **What:** the only empty-set test uses a valid configuration with an unreachable level, which is the correct case for `{}`. No test passes a negative sensitivity, a NaN level, or an unknown loss type.
- **Failure scenario:** `probe1.out`: `level_set_1d` with sensitivity −1 returns 0 points, no throw; same for a NaN level and `loss_type = 42`. While `level_set_2d` throws for each (the `InvalidDimensionThrows` test only covers sizes). The existing test cannot tell "unreachable" from "invalid".
- **Suggested fix:** keep `EmptyWhenLevelUnreachable`, and add `EXPECT_THROW` cases for each invalid config in 1D mirroring 2D; also test the w <= 0 path at level_set.h:306–309 (currently uncovered).
- **Also affects:** `test_c_api.cpp` (4e) for `scs_level_set_1d`.

### S04a-006 [P2, confirmed] `AccuracyCheck` compares against a reference that copies the library's cutoff policy, at the wrong boundary, with a sub-ulp tolerance (S00-001; Decision: Q2)
- **Where:** tests/unit/test_distance_speed.cpp:62–64, :76–78 (references), :170, :178 (assertions); test_distance_comparison.cpp:54–64, :45, :89.
- **What:**
  - The references return Chebyshev for `order > 100`; the library switches at `order >= 100`. So at exactly 100 the reference is exact Minkowski and the library is Chebyshev: this is the whole S00-001 failure (15 assertions, only order 100, up to 3.3e-9 absolute). At 1000 both are Chebyshev, so the test cannot see the substitution there.
  - `EXPECT_NEAR(..., 1e-15)` is below one double spacing at values near 100 (1.4e-14). After the Q2 fix the failure moves: with the scaled formula (`m_q2`) `AccuracyCheck` fails for orders **3, 10 and 100** (9, 9 and 6 equal-weight failures; 9, 9 and 8 weighted): the scaled result differs from the unscaled `pow` reference by a few ulp.
  - The reference for orders 3 and 10 is the same `pow`-sum the library uses, so it is not independent. It is acceptable for p in 1..10 at 1e-12, but not as proof at p = 100 or 1000.
  - `test_distance_comparison.cpp` also hard-codes "order > 100 → Chebyshev" in its reference and the comment at :45 ("now safe with socialchoicelab fix"). It passes after Q2 only by luck: for the first 40 seeded pairs the largest gap between exact and Chebyshev at p = 1000 is 0 (`probe2.out`), but a pair with |dx| = |dy| differs by 6.9e-4, and one pair at p = 100 differs by 3.6e-4.
- **Failure scenario:** see `m_q2` above. Q2 states the intended fix: compare against exact Minkowski with relative tolerance about 1e-12.
- **Suggested fix:** compute the reference in scaled form (or with `long double`/Python decimals for golden values), relative tolerance 1e-12; include pairs with |dx| = |dy|; drop the policy copy.
- **Also affects:** none.

### S04a-007 [P2, confirmed] A correctness test sits in a binary labelled `benchmark`, so CI never runs it
- **Where:** CMakeLists.txt:124–127 (`LABELS "benchmark"` on `DistanceSpeedTest`, `PerformanceComparisonTest`); .github/workflows/ci.yml:96, scripts/check-all.sh:63 (`ctest -LE benchmark`); tests/unit/test_distance_speed.cpp:162–182.
- **What:** the CMake comment says benchmark tests "report timing only". `DistanceSpeedTest.AccuracyCheck` is a pure accuracy assertion and is the only test that compares weighted general-p Minkowski against an independent computation (`test_distance_functions` has no weighted p = 3 or p = 10 test). It is excluded from every CI and pre-push run.
- **Failure scenario:**
  - S00-001 was found only by a full local `ctest`; CI-equivalent `ctest -LE benchmark` is green.
  - Mutant `m_w` (general-p path ignores the salience weights): `test_distance_functions` and `test_distance_comparison` (both CI) pass; only `test_level_set` fails by accident and `DistanceSpeedTest.AccuracyCheck` (not CI) fails. A weighted p-norm bug would be caught only through the level-set test's incidental p = 3 case.
- **Suggested fix:** move the accuracy test into `test_distance_functions.cpp` (or a new unlabelled suite) and leave only timing output in the benchmark binary. Add weighted general-p value tests (hand-computed).
- **Also affects:** none.

### S04a-008 [P2, confirmed] PRNG tests check ranges and averages only; five deliberate bugs survive
- **Where:** tests/unit/test_prng.cpp:58–98, :140–160, :278–302.
- **What:** `UniformInt` checks values in [10, 20] and `uniform_int(5,5) == 5`; `UniformReal` checks [1, 2); `Normal` checks the sample mean of normal(0,1) is within 0.1; `DiscreteChoice`/`UniformChoice` check only that the index is below the size; `SkipViaPublicAPI` compares two PRNGs that both skipped 10.
- **Failure scenario:** each mutant below leaves all 43 tests passing (`SCR4A/m_*`, `t_prng_<name>`):

  | Mutant | Why the tests miss it |
  |---|---|
  | `skip()` does nothing | both generators skip identically (no comparison with an un-skipped or engine-discarded one) |
  | `uniform_real` returns `min` always | range [min, max) is satisfied; no spread or mean check; `normal`/`uniform_real` are never otherwise exercised |
  | `normal` returns `mean` always | only normal(0,1) is used, and the mean check passes; no variance check, no sd != 1, no mean != 0 |
  | `uniform_int` never returns `max` | range check is inclusive and does not require hitting the endpoints |
  | `discrete_choice` picks uniformly among positive weights | only the single-positive case checks the result; no frequency test |

  Also `Skip` (:278–291) tests `std::mt19937_64::discard` through `engine()`, not `PRNG::skip`.
- **Suggested fix:** for each draw method: seeded golden values (Q4; exact for integer/uniform/bernoulli/discrete/shuffle, last-bit tolerance for normal/gamma/beta); chi-square or binomial frequency checks with fixed seed; endpoint coverage for `uniform_int`; sample variance for `normal(mean != 0, sd != 1)`; `skip(n)` compared with n raw `engine()` outputs (until `engine()` becomes private, then with a reference engine in the test). Pin the raw `mt19937_64` 10000th output (9981545732273789042, fixed by [rand.predef]) to prove the engine.
- **Also affects:** none.

### S04a-009 [P2, confirmed] Level-set tests miss 1D weights, polygon orientation and many branches
- **Where:** tests/unit/test_level_set.cpp:34–61 (1D tests use weight 1), :107–140 (Manhattan checks vertices 0–1, Chebyshev only vertex 0).
- **What:** gaps, all checked by deliberate bugs and by the slice-binary coverage recheck (`cov2/`):
  - 1D weight w != 1: mutant `m_ls1d` (`half = r` instead of `r / w`) passes all tests.
  - Vertex order of the 4-vertex polygons ("CCW", stage 2a contract): mutant `m_lsorient` (Manhattan vertices 2 and 3 swapped into a bow-tie, Chebyshev reversed to clockwise) passes all tests. Winset code that builds polygons from these shapes depends on orientation.
  - Not covered at all by `test_level_set`: Gaussian/Threshold (`S04a-004`), Minkowski invalid p (level_set.h:248–252), p <= 1 + 1e-9 (:253–261), p >= 100 (`S04a-003`), 1D weights size and w <= 0 (:296–298, :307–309), `to_polygon` default/zero-sample cases (S02a-009), `ic_polygon_2d` size != 2 and `num_samples < 3` errors (:425–430), r = 0 in 2D.
  - `to_polygon` is tested only for the circle (`CirclePointsOnCurve`) and, indirectly, the superellipse at p = 3; Ellipse sampling is exercised only through unequal-weight `ic_polygon_2d` (in `IcPolygonVerticesAtWeightedDistanceForAllMetrics`); extreme p (S02a-009: 4 of 64 vertices infinite at p = 2200, `probe1.out`) has no test.
- **Failure scenario:** the `m_ls1d` and `m_lsorient` mutants above.
- **Suggested fix:** assert all four vertices of each polygon with expected signed area > 0; 1D tests with w = 2 and 0.5; table-driven tests over distance type x loss type x weights; `to_polygon` argument tests once S02a-009 validation exists.
- **Also affects:** none.

### S04a-010 [P2, confirmed] No tests for SCP-001 and SCP-002
- **Where:** tests/unit/test_level_set.cpp:285–299 (`SemiAxisOverflowThrows` covers only the semi-axis check), :200–212 (equal weights use only 2 and 0.5).
- **What:** SCP-001 (finite semi-axes, infinite vertices) and SCP-002 (equal-weights tolerance too loose for tiny weights) are not covered.
- **Failure scenario:**
  - SCP-001: `probe1.out`: ideal (1e308, 0), sq (0,0), weights (1e-155, 1e-155): `ic_polygon_2d` returns vertex 0 at x = inf, no throw. The existing overflow test cannot see it because it checks only `r / w`.
  - SCP-002: weights (1e-13, 1e-12) give a `Circle2d` (variant index 0). The mutant `m_eqw` (relative tolerance 1e-3, far looser) passes all tests, so there is no test of the equal-weights boundary at all.
- **Suggested fix:** add (a) a test that `ic_polygon_2d`/`to_polygon` either throw or return finite vertices for the SCP-001 input; (b) tests that weights (1e-13, 1e-12), (1, 1 + 1e-6) and (1, 1 + 1e-12) select Ellipse, Ellipse and Circle respectively.
- **Also affects:** winset tests (4b) using `pts_polygon` for the same inputs.

### S04a-011 [P2, confirmed] Loss tests: `normalize_utility` has no value test; Gaussian expected values reuse the code's formula
- **Where:** tests/unit/test_loss_functions.cpp:93, :108–115, :229 (Gaussian expected values), :303–336, :357–373 (normalization).
- **What:**
  - `UtilityNormalization` asserts only 0 <= x <= 1 and x = 1 at d = 0. `UtilityNormalizerMatchesNormalizeUtility` compares the class with the function. Mutant `m_norm` (normalised value at max distance becomes 0.5 instead of 0, still inside [0, 1]) passes all 13 tests. No test asserts that x = 0 at `max_distance` or any interior value.
  - The Gaussian expected values are `max_loss * (1.0 - std::exp(-steepness * d * d))`: the same expression the code uses. That is a copy, not an independent value, and carries the same cancellation as S02a-007: the test cannot catch an `expm1` fix being wrong or the d = 1e-9 collapse to 0 (`probe1.out`). Linear, quadratic and threshold use literal values or trivial formulas (fine).
  - Uncovered throws (coverage recheck, `cov2/`): non-finite sensitivity/max_loss/steepness/threshold/distance inputs (loss_functions.h:50, :80, :113, :115, :146, :148, :150), `normalize_utility` non-finite max_distance (:257–258), `UtilityNormalizer` negative `max_distance` and non-finite utility (:320–321, :336–337).
- **Failure scenario:** `m_norm` above.
- **Suggested fix:** hand-computed literals, e.g. Gaussian d = 2, α = 5, β = 1 gives 5(1 − e^−4) = 4.90842179…; d = 1e-5 and 1e-9 with expected values from the Taylor series; normalise tests at d = 0, mid, `max_distance` with literal results; one throw test per uncovered branch.
- **Also affects:** none.

### S04a-012 [P2, confirmed] StreamManager contracts without tests
- **Where:** tests/unit/test_prng.cpp:304–530; stream_manager.h contracts listed in stage 2a.
- **What:** the stage 2a contracts with no test:
  - a stream's seed depends only on (master, name), not on creation order;
  - `reset_all` keeps the allowlist;
  - `reset_for_run` sets `master_seed()` to the run seed;
  - `has_stream`, `remove_stream` and `stream_names` ignore the allowlist;
  - the stream-seed function (FNV-1a over the name, then SplitMix64) has no golden value, although it is pure integer arithmetic and can be pinned today (Q4).
  `BasicOperations` (:309–324) and `ResetAll` (:326–340) assert `EXPECT_NE` on two integers in 0..100, which is a 1% accident on any seed set (see S04a-015).
- **Failure scenario:** a refactor that makes stream seeds depend on creation order (a documented-forbidden change) passes all current tests. Likewise a change to the FNV or SplitMix constants goes unnoticed; Q4's reproducibility promise rests on this function.
- **Suggested fix:** add order-independence, allowlist-survival and `master_seed()` tests, and pin the first 64-bit output for 3 names x 2 masters (generate independently with a short Python implementation of FNV-1a/SplitMix64).
- **Also affects:** none.

### S04a-013 [P2, confirmed] Distance edge-case gaps, including the tests Q2 asks for (Decision: Q2)
- **Where:** tests/unit/test_distance_functions.cpp (whole file); tests/unit/test_distance_comparison.cpp.
- **What:** missing, per rubric edge-case list and Q2's Implies:
  - p = 50, 99 and 1000 with differences about 1e4 and 1e-4. `probe3.out`: today p = 99 gives `inf` (1e4) and 0 (1e-4); exact values are 10070.26 and 1.007e-4. This is the unscaled-pow overflow Q2 names. Also p = 100 and 1000 give 10000 where exact is 10069.5555 and 10006.9339.
  - N = 0 (empty points with empty weights; the contract says it throws), all-zero weights, zero weight at general p and Chebyshev, equal points at p = 3.
  - Fast-path boundaries |p − 1| < 1e-9 and |p − 2| < 1e-9 (p = 1 + 1e-10 should equal Manhattan; p = 1 + 1e-8 should not).
  - Extreme magnitudes at p = 2 (S02a-015: 1e154 overflows).
  - `calculate_distance` dispatch for every enum value and the contract that `order_p` is ignored (even NaN) for non-Minkowski types.
  - Error messages: `NonFiniteAndInvalidInputsThrow` checks only the exception type.
- **Failure scenario:** the Q2 fix could regress p = 99 overflow and no test would notice.
- **Suggested fix:** add the list above with hand-computed expected values; weighted general-p test (see S04a-007).
- **Also affects:** none.

### S04a-014 [P3, confirmed] Tests that exercise code scheduled for removal
- **Where:** tests/unit/test_prng.cpp:100–121, :173–220, :140–171, :222–246, :460–484, :533–573, :265–291.
- **What:**
  - Singleton (S02a-006): `GlobalStreamManager`, `SetGlobalStreamManagerSeedSecondCallProducesDifferentSequences` (2 tests).
  - Unused distributions (S02a-012): `Exponential`, `ExponentialMean`, `DiscreteChoice`, `BetaInvalidParametersThrow`, `BetaHappyPath`, `GammaHappyPath`, `InvalidDistributionParametersThrow` (the exponential/gamma lines), `DiscreteChoiceEmptyWeightsThrows`, `DiscreteChoiceAllZeroWeightsThrows`, `DiscreteChoiceNegativeWeightThrows`, `DiscreteChoiceNaNWeightThrows`, `DiscreteChoiceInfWeightThrows`, `DiscreteChoiceSinglePositiveAmongZerosReturnsItsIndex`, `DiscreteChoiceValidWeightsWorks`.
  - Direct engine use and copyability (S02a-013): `Skip`, `CopyProducesIdenticalSequenceFromCopyPoint`; `StateStringReflectsDrawCount` pins the unused `draw_count_` string.
- **Failure scenario:** none today. When the owner-approved removals go in, these tests fail to compile or become meaningless, and need deleting in the same change. If the owner decides to keep `discrete_choice` (the Q4 text lists "discrete" among the portable draws), the discrete tests are the only ones for it and need strengthening (S04a-008).
- **Suggested fix:** delete with the code; keep a discrete-choice test only if `discrete_choice` stays.
- **Also affects:** none. No Heart tests live in this slice.

### S04a-015 [P3, confirmed] Seed-dependent `EXPECT_NE` on small-range draws
- **Where:** tests/unit/test_prng.cpp:40, :317, :339, :380, :468, :482–483.
- **What:** each asserts that two seeded draws differ, from ranges of 101 values (`Construction`, `BasicOperations`, `ResetAll`, `GlobalStreamManager`: 1 in 101 chance of equality for a given seed pair), 1,001 (`ResetStream`) or 1,000,001. They pass today on libc++ and on CI's libstdc++ by luck of the seeds.
- **Failure scenario:** Q4 replaces the draw algorithms, so these draws change once. The chance that at least one of the four 0..100 tests then fails by coincidence is about 4%, and the failure would look like a seed-derivation bug.
- **Suggested fix:** compare 10 or more draws (or whole sequences) instead of one integer, or compare the streams' first raw `engine()` outputs.
- **Also affects:** none.

### S04a-016 [P3, confirmed] Hygiene: misleading names, tests of third-party code, unused results, duplicated tests
- **Where:** as listed.
- **What:**
  - `PerformanceComparisonTest.EigenVectorOperations` (test_performance_comparison.cpp:68–89) tests `Eigen::norm`, not project code; `DistanceCalculationSpeed` asserts only `sum > 0`. `PerformanceConsistency` and the two `*Performance` tests in `test_distance_speed.cpp` assert nothing (by design, labelled benchmark); they still count as passing tests.
  - `LossFunctionsTest.TemplateInstantiation` (test_loss_functions.cpp:339–353) computes `f_gaussian` and `f_threshold` and asserts nothing about them (clang-tidy DeadStores, Stage 0 lead); it asserts only linear and quadratic float results.
  - `CoreTypes` (test_core_types.cpp) only checks that Eigen vectors work with the distance API; `core/kernels.h`, `linalg.h`, `numeric_constants.h` have no test of their own. `LinalgIndexIsUsable` asserts `p.size() == 2`.
  - `DistanceComparisonTest.SmallTest` and `MediumTest` are the same loop with different counts (test_distance_comparison.cpp:68–123).
  - Comments cite "Item 15", "Item 24", "Consensus plan 4 test gap" with no way to resolve them.
  - Test inputs come from `std::mt19937` + `std::uniform_real_distribution` (test_distance_comparison/speed/performance), which Q4 calls implementation-defined, so the 10 point pairs differ by platform. The assertions are tolerance-based and mostly unaffected, but `AccuracyCheck` failure counts depend on them. test_prng.cpp uses `std::find` without `<algorithm>`; several `EXPECT_EQ` compare signed with unsigned (`names.size(), 2`, `manager.size(), 0`, `EXPECT_LT(choice, 5)`, `EXPECT_EQ(eigen_vec.size(), size)`).
- **Failure scenario:** none; clarity and maintenance.
- **Suggested fix:** delete or rename the Eigen test; assert on `f_gaussian`/`f_threshold` or remove them; merge duplicates; drop "Item N" comments; use fixed literal points for accuracy tests.
- **Also affects:** none.

### S04a-017 [P3, confirmed] Error-message content is never asserted (S02a-016)
- **Where:** tests/unit/test_distance_functions.cpp:104–192, test_loss_functions.cpp:264–288, test_prng.cpp:222–246, test_level_set.cpp:179–192.
- **What:** all `EXPECT_THROW` cases check the exception type only. Only `NonPositiveOrNonFiniteWeightThrowsWithMessage` (PR #1) and `SemiAxisOverflowThrows` check message text. The project rule says messages must state what failed, what was entered and how to fix it.
- **Failure scenario:** S02a-016 (messages that omit the value entered) cannot be caught or driven by tests.
- **Suggested fix:** when the messages are rewritten, assert on the substrings (parameter name and received value) for each validation branch.
- **Also affects:** none.

## Findings map
Statuses: **wrong-behaviour test** (the fix must change a test), **cannot catch** (a test exists but would pass with the bug), **no test** (gap), **fixed and tested** (PR #1).

| Earlier finding | Severity | Status | Test name or note |
|---|---|---|---|
| S02a-001 equal non-unit weights circle radius | P0 | fixed and tested | `LevelSet2d.EqualNonUnitWeightsCircleRadiusIsROverW`, `IcPolygonVerticesAtWeightedDistanceForAllMetrics`. Revert mutant `m_rev001` fails both. |
| S02a-002 `reset_for_run` seed collisions | P1 | cannot catch | `StreamManagerTest.ResetForRunDifferentIndicesDifferentSequences` (one master only). `S04a-002`. |
| S02a-003 level set Chebyshev square at p >= 100 | P1 | no test | none. Mutant `m_lsp100` passes. `S04a-003`. |
| S02a-004 Gaussian IC round trip throws / empty | P1 | no test | no Gaussian or Threshold level-set test anywhere. `S04a-004`. |
| S02a-005 `level_set_1d` swallows invalid config | P1 | no test | `LevelSet1d.EmptyWhenLevelUnreachable` covers only the valid case. `S04a-005`. |
| Q2 Chebyshev substitution in distance (S00-001) | decision | wrong-behaviour test | `DistanceFunctionsTest.MinkowskiP100LargeCoordsNoOverflow` (and `MinkowskiAtChebyshevCutoffBoundary`, `AccuracyCheck` references). `S04a-001`, `S04a-006`. |
| S00-001 `AccuracyCheck` fails at p = 100 | P2 | wrong-behaviour test (test bug per Q2) | `DistanceSpeedTest.AccuracyCheck`; also hidden from CI. `S04a-006`, `S04a-007`. |
| S02a-006 global StreamManager singleton | P2 | wrong-behaviour test (tests exist for code to be removed) | `GlobalStreamManager`, `SetGlobalStreamManagerSeedSecondCallProducesDifferentSequences`. `S04a-014`. |
| S02a-007 Gaussian precision near 0 | P2 | cannot catch | `GaussianLoss` expected values use the same formula; no d < 1e-5 case. `S04a-011`. |
| S02a-008 `uniform_real` overflow to inf | P2 | no test | none (`UniformReal` uses [1, 2)). Reproduces: `probe1.out`. |
| S02a-009 superellipse inf vertices; `to_polygon` unchecked | P2 | no test | none for p >= 1000 or bad arguments. `S04a-009`. |
| S02a-010 `level_set.h` branch coverage | P2 | partly fixed (PR #1 added 4 tests, 14 now) | remaining gaps: `S04a-003`, `-004`, `-005`, `-009`, `-010`. |
| SCP-001 finite semi-axes, infinite vertices | P2 | no test | `SemiAxisOverflowThrows` checks only `r / w`. `S04a-010`. |
| SCP-002 equal-weights tolerance too loose | P2 | no test | `EqualNonUnitWeightsCircleRadiusIsROverW` uses weights 2 and 0.5 only. Mutant `m_eqw` passes. `S04a-010`. |
| Q18 zero weight in 2D shapes | decision | fixed and tested | `LevelSet2d.NonPositiveOrNonFiniteWeightThrowsWithMessage` (level set and IC; revert mutant `m_rev018` fails). 1D weight check at level_set.h:306–309 untested. |
| S02a-011 `uniform_real` can return max | P3 | cannot catch | `UniformReal` uses a wide range. Owner decision Q20 open. |
| S02a-012 unused distributions have edge-case defects | P3 | no test of the defects; tests exist for the happy paths | 14 tests listed in `S04a-014`. |
| S02a-013 public `engine()`, draw_count, copy | P3 | wrong-behaviour test (locks current design) | `Skip`, `CopyProducesIdentical…`, `StateStringReflectsDrawCount`. `S04a-014`. |
| S02a-014 `skip` discards raw outputs | P3 | cannot catch | `SkipViaPublicAPI` passes with a no-op `skip` (`m_skip`). `S04a-008`. |
| S02a-015 p = 2 path unscaled | P3 | no test | none. `S04a-013`. |
| S02a-016 error messages omit value | P3 | cannot catch | exception type only. `S04a-017`. |
| S02a-017 loss comments / integer branches | P3 | no test needed (comment); `TemplateInstantiation` documents float only | `S04a-016`. |
| S02a-018 swappable loss parameters | P3 | no test | n/a (design). |
| S02a-019 level-set validation order, docs | P3 | no test | none for Gaussian `max_loss <= 0` message. `S04a-004`. |
| SCP-003, SCP-004 | — | not this slice | SCP-003 is slice 4e/4f, SCP-004 is 4d. |

**Count of earlier P0/P1 findings with no test that could catch them: 4** (S02a-002, S02a-003, S02a-004, S02a-005). **Fixed and tested: 1** (S02a-001). **Tests asserting wrong behaviour: 2** (the two Q2 distance tests).

## Tool leads checked
- Stage 0 `DistanceSpeedTest.AccuracyCheck` fails at p = 100 (S00-001) → `S04a-006`/`-007`: test bug per Q2 (reference uses `>`, library `>=`; 1e-15 below double spacing); after Q2 the failing orders change to 3, 10 and 100.
- Coverage recheck (coordinator warning: the shared report understates header-only files). I ran this slice's six `build_cov` test binaries (`test_prng`, `test_distance_functions`, `test_distance_comparison`, `test_loss_functions`, `test_level_set`, `test_core_types`) with fresh profiles in `SCR4A/cov2/` and ran `llvm-cov` on only those. Results: `level_set.h` 71.3% lines (shared report: 64.3%), `loss_functions.h` 90.5% (87.6%), `distance_functions.h` 97.5% (same), `prng.h` 95.6% (same), `stream_manager.h` 89.2% (90.0%), `stream_manager.cpp` 90.5% (same). The uncovered line lists below are from this recheck; they match my earlier per-binary builds in `SCR4A/cov/`. The shared `coverage_show` file prints 0 for all of `level_set.h` lines 87–161 although `level_set_2d` (line 333) shows 46 calls, so I did not use it.
- `level_set.h` → `S04a-003`/`-004`/`-005`/`-009`. Uncovered by this slice's binaries: lines 91–92, 96–102, 112–114, 119–159 (Gaussian and Threshold inversion), 180–183, 248–270, 274–276, 296–298, 307–309, 394, 425–430, 451–452. No other test file uses Gaussian/Threshold loss in a level set (grep).
- Coverage `prng.h` 95.6% (recheck: same) → uncovered `master_seed()` (:42), `gamma` non-finite (:157–158), `beta` domain_error (:197–199), const `engine()` (:303): unimportant except as removal candidates (S02a-012/013); the `beta` throw is a real defect path (S02a-012) → `S04a-014`.
- Coverage `stream_manager.h` 90% / 81.8% functions → uncovered lines 320–331 are the global `*_rng()` helpers → `S04a-014` (to be removed).
- Coverage `loss_functions.h` 90.5% → uncovered lines 50, 80, 113, 115, 146, 148, 150, 257–258, 320–321, 336–337 (non-finite and range throws) → `S04a-011`.
- Coverage `distance_functions.h` 97.5% → no uncovered line in the `double` instantiation; the 3 missed lines are in secondary template instantiations → not a defect; dispatch and edge cases are `S04a-013`.
- Stage 0 `-Wunused-result` in `test_prng`, `test_distance_functions`, `test_loss_functions` → not a defect (`[[nodiscard]]` inside `EXPECT_THROW`).
- `EnumCastOutOfRange` at `test_distance_functions.cpp:155`, `test_loss_functions.cpp:259` → not a defect (intentional invalid-enum tests, and they do assert the throw).
- `DeadStores`/`-Wunused-variable` `test_loss_functions.cpp:344–345` → `S04a-016` (test claims instantiation, asserts nothing on these two).
- `-Wimplicit-int-float-conversion` `test_distance_speed.cpp:30, :204` → not a defect (timing code, `count()` to double).
- `bugprone-random-generator-seed` `test_distance_comparison.cpp:17`, `test_distance_speed.cpp:34`, `test_performance_comparison.cpp:20` → not a defect (all seeded with constant 12345 in `SetUp` or the constructor); the use of `std::` distributions in test inputs is `S04a-016`.
- Heart tests → none in this slice.

## Open questions
1. **Q19 and Q20 are unanswered**, and the tests for them cannot be written until they are: Threshold IC through a point inside the "don't care" zone (`S04a-004`), and whether `uniform_real` may return `max` (`S04a-008`, S02a-011).
2. **Q21 (source for the Gaussian and Threshold formulas).** The loss tests should use independent expected values (`S04a-011`). Gaussian formula text could not be verified against Conevska & Mutlu (2025) (Stage 2a), so hand values would be checked against the same formula as the code until the owner names a source.
3. **Should the benchmark binaries keep their `benchmark` label once the accuracy test moves out?** (Recommended: yes, for timing only; this is a project decision, not an owner-question item.)
4. **Q4 and `discrete_choice`.** Q4 lists "discrete" among bit-identical draws, while S02a-012 proposes deleting `discrete_choice` (no callers). If it is kept, its tests need frequency checks (`S04a-008`); if removed, they go (`S04a-014`). Which?

## Out-of-slice notes
- `tests/unit/test_winset*.cpp`, `test_majority.cpp`, `test_voting_rules.cpp` and `test_c_api.cpp` mention `100.0` or large orders. Stage 4b/4d/4e should check whether any asserts Chebyshev behaviour at p >= 100 (Q2).
- `test_c_api.cpp` has no Gaussian or Threshold loss use (grep), so the `S04a-004` gap repeats at the C API (4e).
- Release builds here use `-O3`, not `-ffp-contract=off` (Q4); the `EXPECT_DOUBLE_EQ` equality tests in this slice (e.g. `SymmetryProperty`) pass today, but Q4's symmetric-tie regression test has no home in this slice's files.
