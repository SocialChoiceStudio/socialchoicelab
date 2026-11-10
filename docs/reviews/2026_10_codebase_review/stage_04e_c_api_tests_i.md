# Stage 04e — C API tests I
Model / effort: Sonnet 5.5 / high   Commit: 2771655   Files reviewed: 1 (`tests/unit/test_c_api.cpp` lines 1–1910, 1,910 lines, all 103 tests read in full)
Not read: nothing in the slice. Code under test was read only to settle specific questions (`scs_api.cpp` 905–1000, 218–262).

Scratch material is in `SCR4E=/private/tmp/claude-501/-Users-robiragan-Dropbox-SocialChoiceStudioDev/2402f97e-221e-494b-88cb-fc63c41efc50/scratchpad/stage4e/`:
- `mutant.cpp` / `libmutant.dylib`: an interposing library (`DYLD_INSERT_LIBRARIES`) that replaces selected `scs_*` functions with wrong behaviour, chosen by env `SCS_MUTANT`. Run against the unmodified Release `test_c_api` from `stage4/build_release`, filtered to the 103 tests of this slice (`my_tests.txt`). Results in `mutants_run.txt`. Every mutant was also checked to be active (`probe*.cpp`, `probe*_real.txt`, `probe*_mut*.txt`).
- `probe.cpp`, `probe2.cpp`, `probe3.cpp`, `probe4.cpp`, `probe5.cpp`, `cyc.cpp`: direct calls against `libscs_api.dylib` (Release). Output in `probe_real.txt`, `probe2_real.txt`, `probe4_out.txt`.
- `cov.py`, `cov_out.txt`, `mine.profdata`, `f_all.txt`, `f_mine.txt`: coverage restricted to this slice. The slice-only run used `build_cov/test_c_api` with a fresh `LLVM_PROFILE_FILE` and `llvm-cov` on `libscs_api.dylib` only. A full `test_c_api` run into `profall/` reproduces the shared report's `scs_api.cpp` line (3,884 lines, 60.81%), so the `scs_api.cpp` numbers below are reliable. No header coverage is cited.

## Summary
- **0 P0, 6 P1, 12 P2, 3 P3** (21 findings). All 103 tests pass on Release (`ctest` `CApiTest`, 0.19 s). None is a benchmark, none uses time, none depends on test order. `CApiTest` is not labelled `benchmark`, so CI runs it (`ctest -LE benchmark`, Release, no sanitizers or leak checks).
- **None of the 13 earlier P0/P1 findings in this area (S03a-001..006, S03c-001..005, S03h-001, SCP-003) has a test that could catch it:** 7 have no test, 6 have a test that exists but cannot catch it. No test asserts wrong behaviour for any of them. Three of the overflow/null-pointer cases crash the process (exit 139 on Release, `probe4_out.txt`), so a plain `EXPECT_*` test would also kill the whole binary until the fix lands.
- **Most assertions are weak.** 14 of 17 wrong-behaviour mutants were not noticed by any of the 103 tests (convex hull vertices shifted or reversed, Voronoi cells replaced by the whole box, `contains_point` always 0, Minkowski computed as Chebyshev, boolean operations replaced by trivial results, weights ignored, RNG draws constant, polygon vertices garbage, and others). Details in S04e-007..011.
- **Owner decisions:** two tests lock in behaviour that Q7 changes (competition runs with `mgr = NULL`) or that Q6 puts in doubt (threshold 0.5 as "simple majority", with a comment that records the known distinction). Q2 (p ≥ 100), Q5 (near-Plott), Q6 (near-quota), Q7 (coincident Voronoi sites) and Q10 (bounds) have no test at the C API; Q2 and Q7 would fail today (demonstrated).
- **Never called by any test (whole file):** `scs_winset_symmetric_difference`, `scs_competition_trace_final_positions`, `scs_competition_trace_round_seat_shares`, `scs_competition_experiment_mean_final_seat_shares` in this slice's area; `scs_centroid_2d`, `scs_marginal_median_2d`, `scs_geometric_mean_2d` outside it (no slice owns their tests). Gaussian and threshold loss are never executed through the C API.

## Contracts this layer provides
What the 103 tests actually establish. Later stages should rely only on this, not on the test names.
- **Values checked against hand calculations:** `scs_calculate_distance` (3-4-5 Euclidean, Manhattan 6, Chebyshev 5); linear and quadratic `scs_distance_to_utility` and `scs_normalize_utility` (−6, 0, 0.5); `scs_level_set_1d` quadratic points 2 and 4; Euclidean circle radius and ellipse semi-axes r/w (weights 1, 2); IC polygon radius 5; competition vote totals (2, 1), vote shares 2/3 and 1/3, seat totals (1, 0) for a static two-sticker run; 1D winset interval (−2, 2) and "median seat is empty"; pairwise values for a 2-alternative Condorcet-winner case.
- **Structure only (counts, types, signs, `rc`):** convex hull (vertex count), Voronoi (sizes, `cell_start` monotone), Manhattan/Chebyshev polygon (4 vertices), winset boundary (positive sizes, size-query), winset booleans (`is_empty` only), weighted and veto winsets (`is_empty` only), RNG (range, finite, same seed same value).
- **Consistency only (two paths of the same code agree):** `scs_ic_interval_1d`, `scs_ic_polygon_2d`, `scs_level_set_polygon_2d` versus the multi-call paths; `scs_winset_2d_export_boundary` versus the handle path; Voronoi heap versus fill.
- **Error paths checked:** null pointers, zero counts, dimension and weight-count mismatch, short buffers for `to_polygon`, `ic_polygon_2d`, `pairwise_matrix_2d`, `dist_config`, unknown stream under an allowlist, invalid `uniform_real` range, invalid weighted-majority threshold, negative weight. Messages are checked only for `strlen > 0`.
- **Not checked anywhere in this slice:** non-finite input, hostile counts, unknown enum values, extreme magnitudes, Gaussian/threshold loss, distances with non-unit weights, `p ≥ 100`, Hunter/Aggregator/Predator, explicit bounds, cross-platform draw values, thread use.
- **Test hygiene facts:** fixed seeds throughout; no timing; helpers use function-local `static double w[2]` that is never written; `SCS_DistanceConfig` pointers are valid for the call only and the tests respect that.

## Findings

### S04e-001 [P1, confirmed] No test can catch the count-overflow bugs (S03a-004, S03a-005, SCP-003, S03c-004), and each would crash the binary
- **Where:** tests/unit/test_c_api.cpp:1245 (`Error_ZeroPoints`), :1387 (`Error_BufferTooSmall`), :1739 (`InvalidNSites`); nothing in :951–1063 for `n_competitors * n_dims`
- **What:** The existing tests use ordinary bad counts only: `n_points = 0`, a 3-alternative matrix with a 4-cell buffer, `n_sites = 0`. None passes a count whose `* 2` or `n * n` overflows `int`. The Stage 3 findings say these checks are skipped or wrap.
- **Failure scenario:** `$SCR4E/probe4.cpp` on the Release library: `scs_convex_hull_2d(…, n_points = -1073741825)`, `scs_voronoi_cells_2d_size(…, n_sites = -1073741825)` and `scs_pairwise_matrix_2d(…, n_alts = 65536, out_len = 0)` each end with **exit 139 (SIGSEGV)** (`probe4_out.txt`). The suite is green, and a test with such an input would kill `test_c_api` and every test after it, not fail cleanly.
- **Suggested fix:** After the Stage 11 fixes, add tests expecting `SCS_ERROR_INVALID_ARGUMENT` (or `BUFFER_TOO_SMALL` for the matrix) for counts of −1073741825, 1073741825 and 65536 on `convex_hull_2d`, `pairwise_matrix_2d`, all three Voronoi entry points, `competition_run` and `competition_run_experiment`. Run them under a sanitizer build too (S04e-017).
- **Also affects:** 4f for the other `n * 2` sites (S03a-009, S03b).

### S04e-002 [P1, confirmed (logic)] No test exercises the failing-factory leak path (S03a-001)
- **Where:** tests/unit/test_c_api.cpp:1864 (`Error_NullVoters`), :1874, :1890; `.github/workflows/ci.yml:88–96`
- **What:** The leak happens only when the core throws after `new SCS_WinsetImpl`. The only failing-factory test passes null voters, which is rejected before any allocation. No test in the range passes an input the core rejects (for example `k = 99` with 3 voters, shown to throw by `probe5.cpp`: `majority: k must be in [1, n]`). CI runs plain `ctest` on Release, with no `leaks`, ASan or LSan.
- **Failure scenario:** `probe5`: `scs_winset_2d(…, k = 99, …)` returns NULL with the core's message; every such call leaks 880 bytes (S03a-001, measured in 3a). 103 tests pass and `leaks` on the slice shows 0 leaks, as 3a also found.
- **Suggested fix:** Add a loop test with core-rejected inputs for the three factories, and run `test_c_api` under LSan/ASan in CI, or add a macOS `leaks` step.
- **Also affects:** CI configuration.

### S04e-003 [P1, confirmed] No test passes an unknown enum value (S03a-002, S03c-003, S03h-002)
- **Where:** whole range; the enum arguments in the tests are always valid constants
- **What:** The C API maps unknown loss, distance, seat-rule, boundary-policy and objective integers to defaults without error. Nothing in the slice sets an enum field to an invalid value.
- **Failure scenario:** `probe4`: `loss_type = 99` returns `rc = 0, u = -4` (quadratic); `seat_rule = 99, boundary_policy = 99, objective_kind = 99` returns a handle with an empty error string. A test for either would fail today.
- **Suggested fix:** After the fix, test each mapping with an out-of-range value expecting `SCS_ERROR_INVALID_ARGUMENT` and a message naming the value (`scs_distance_to_utility`, `scs_normalize_utility`, `scs_level_set_1d/2d`, `scs_ic_polygon_2d`, `scs_competition_run`).
- **Also affects:** Under UBSan the out-of-range read is itself undefined (S03h-002); run these tests there.

### S04e-004 [P1, confirmed] No test covers non-finite or hostile values in the competition and 1D-winset entry points (S03c-001, S03c-002, S03a-006)
- **Where:** tests/unit/test_c_api.cpp:951 (`RunExperimentAndExportSummary`, finite values only), :787, :892; :1762–1798 (1D winset: finite voters only). The 4f `NonFiniteInput` group has no competition or 1D-winset case (no `scs_competition*` call after line 1910).
- **What:** `scs_competition_run_experiment` skips the finiteness checks that `scs_competition_run` makes; step and termination doubles are unchecked in both; `scs_winset_interval_1d` accepts NaN/Inf. No test passes any of these.
- **Failure scenario:** `probe4 exp_nan`: `scs_competition_run_experiment` with a NaN position and `max_rounds = 0` returns a handle with an empty message, while `scs_competition_run` rejects the same input ("competitor_positions: non-finite value at index 0"). `probe4 w1d_nan`: voters (NaN, 2, 3), seat 1 returns `rc = 0, [1, 3]`. A test comparing the two competition entry points, or a NaN in `xs`, would fail today.
- **Suggested fix:** Add a table-driven test: every double argument of `competition_run`, `competition_run_experiment` (positions, headings, voters, weights, every step and termination field) set to NaN and Inf in turn must return `INVALID_ARGUMENT`; the same for `winset_interval_1d`. Add a test that `fixed_step_size = Inf` with `reflect` returns an error instead of hanging (S03c-002; use a timeout).
- **Also affects:** R and Python tests have the same gap (Stage 5/6).

### S04e-005 [P1, confirmed] `scs_level_set_to_polygon` and `scs_competition_trace_strategy_kinds` have no test for the inputs that break them (S03a-003, S03c-005)
- **Where:** tests/unit/test_c_api.cpp:436–456 (`PolygonPassthroughVertices`, valid 4 vertices), :881–887 (`strategy_kinds` called once with a correct buffer)
- **What:** Both functions are exercised with valid input only. The header says POLYGON has exactly 4 vertices; the function accepts any count. `strategy_kinds` is the only trace accessor with no null check and the wrong code for a short buffer.
- **Failure scenario:** `probe4 poly6`: `n_vertices = 6` returns `rc = 0, n = 6` with uninitialised vertices copied out. `probe4 kinds_null`: `out_len = 1` returns `rc = 1` (expected 3 = `BUFFER_TOO_SMALL`), then a NULL buffer gives **exit 139**.
- **Suggested fix:** Tests for `n_vertices` ∈ {−1, 0, 3, 5, 6, INT_MAX} and `exponent_p` ∈ {0, −2, NaN} expecting `INVALID_ARGUMENT`; for `strategy_kinds`, a loop over all trace accessors with NULL and short buffers expecting 1 and 3 respectively.
- **Also affects:** R `level_set_to_polygon` (hand-built list path).

### S04e-006 [P1, confirmed] The weighted-majority tests use threshold 0.5 as "simple" and cannot catch S03h-001 (Q6)
- **Where:** tests/unit/test_c_api.cpp:1400–1432 (`HighWeightVoterDominates`, `UniformWeightsMatchesSimpleMajority`); comment at :1419–1420. Decision: Q6
- **What:** Both tests use odd voter counts and results far from the quota (100 of 102; 2 of 3). The second test's comment says it ignores "the ⌊n/2⌋+1 vs ≥ half distinction for odd n", which records the defect and avoids it. `scs_weighted_majority_prefers_2d` returns 1 for both directions at exactly half the weight. The same default (0.5) is what Q6 replaces with a "more than half" sentinel, so these tests will need rewriting, not just rerunning.
- **Failure scenario:** The mutant `weighted_strict` (">" instead of "≥" at 0.5, i.e. the Q6 behaviour) leaves all 103 tests passing (`mutants_run.txt`); the real library gives `A>B = 1, B>A = 1` for two equal voters at (0,0) and (1,0) (`probe_real.txt`), and no test notices that either. A second mutant that makes both `scs_majority_prefers_2d` and `scs_weighted_majority_prefers_2d` fail with `rc = 2` fails 10 tests but **not** `UniformWeightsMatchesSimpleMajority`, because that test ignores both return codes and compares two untouched `-1` values.
- **Suggested fix:** For Stage 11: an even equal-weight split must give neither side a win (and match `scs_majority_prefers_2d`); a near-quota case (1, 1, 2.004) must not let the two weight-1 voters win; assert the return codes in `UniformWeightsMatchesSimpleMajority`. Use the new sentinel in place of 0.5.
- **Also affects:** `WeightedWinset_NonEmpty` (:1874), which passes an explicit 0.5; 4f has the same default at :3551.

### S04e-007 [P2, confirmed] Convex-hull tests check vertex counts, not vertices
- **Where:** tests/unit/test_c_api.cpp:1200–1251
- **What:** `Triangle_ThreeVertices` and `InteriorPointExcluded` assert only `n`. `FiorinaPlott1978S1` asserts `3 ≤ n ≤ 5` and coordinate bounds loose enough for an error of 0.1. The output coordinates and winding order are never compared with an expected hull. No collinear, duplicate or single-point case.
- **Failure scenario:** Mutants `hull_shift` (every vertex +0.01) and `hull_cw` (reversed order): 103/103 pass. `probe_mut`: triangle hull becomes (0.01, 0.01) (1.01, 0.01) (0.01, 1.01) with `n = 3`.
- **Suggested fix:** Assert exact vertices (and counter-clockwise order, if the header promises it) for the triangle and the square-with-interior-point; add collinear, duplicate and n = 1, 2 cases.

### S04e-008 [P2, confirmed] Voronoi tests check sizes, not geometry; coincident sites (Q7) untested and would fail today
- **Where:** tests/unit/test_c_api.cpp:1678–1756. Decision: Q7
- **What:** For sites (0, 0) and (2, 0) in a box, the cells are two rectangles split at x = 1. The tests assert only positive vertex counts and `cell_start` bookkeeping; the heap test compares the heap with the fill path (consistency only). No test places two sites at the same point.
- **Failure scenario:** Mutant `voronoi_bbox` (both cells replaced by the whole box, the bisector ignored) passes 103/103 once applied to both the fill and heap paths. For coincident sites (1, 1), (1, 1) the real output is two identical whole-box cells (`probe_real.txt`: `cell0 = cell1 = [(-2,-2)(4,-2)(4,4)(-2,4)]`, `rc = 0`), which Q7 declares wrong; a "shared cell" test would fail today.
- **Suggested fix:** Assert the vertices of the two-site cells (x = 1 bisector); add the Q7 shared-cell test, a three-site case and a site on the box edge.

### S04e-009 [P2, confirmed] Winset query tests cannot detect wrong containment, bounding box or hole flags
- **Where:** tests/unit/test_c_api.cpp:1521–1531 (`ContainsPoint_OutsideIsZero`), :1533–1550 (`BoundingBox_NonEmpty`), :1583–1607 (`BoundarySampleFill`)
- **What:** `contains_point` is tested only at (100, 100), never at a point inside the winset (4f has one at :3516, for another purpose). The bounding box is only required to be non-degenerate and inside x ∈ [−0.5, 2.5]; the real box is x ∈ [−0.064, 2.064], y ∈ [−0.667, 1.202], and y is not checked at all. `BoundarySampleFill` requires `holes[0] >= 0`, which any value written satisfies (the buffer was pre-filled with −1).
- **Failure scenario:** Mutants `contains_never`, `bbox_pad` (box widened by 0.2 each side) and `hole_flag_wrong` (outer ring marked as a hole): 103/103 pass. With `contains_never`, `probe2` shows (1, 0.9), which really is in the winset, reported as outside.
- **Suggested fix:** Test points known to be inside and outside; compare the bounding box with the box computed independently from the three circles (to the sampling tolerance); assert `holes[0] == 0` for the outer ring and compare ring areas or orientation.

### S04e-010 [P2, confirmed] Boolean, weighted and veto winset tests assert only `is_empty`
- **Where:** tests/unit/test_c_api.cpp:1800–1905
- **What:** `Clone`, union, self-intersection, self-difference, weighted and veto tests each check one `is_empty` value. Union of two non-empty sets is non-empty for any implementation that returns one of them; self-intersection non-empty and self-difference empty are satisfied by trivial implementations. The weighted test uses equal weights, so it cannot tell whether weights are used. The veto test covers only the case where the veto player sits at the status quo (result empty by construction); no case where a veto player changes a non-empty winset. `scs_winset_symmetric_difference` is never called by any test.
- **Failure scenario:** Mutants `boolops_trivial` (union := a, intersection := a, difference := empty), `weights_ignored` (weighted winset computed as plain majority) and `veto_always_empty`: 103/103 pass for each. `probe2` confirms each is active: with weights ignored, a threshold of 0.9 (unanimity) gives a non-empty set instead of an empty one.
- **Suggested fix:** For two overlapping winsets compare bounding boxes and `contains_point` at points in A only, B only, both and neither, for union, intersection, difference and symmetric difference; add unequal-weight, non-0.5 threshold and non-binding-veto cases.

### S04e-011 [P2, confirmed] Distance and loss tests do not pin the metric, the weights or the loss families
- **Where:** tests/unit/test_c_api.cpp:49–174
- **What:**
  - `MinkowskiP3` measures the point (1, 0) from the origin, where every Lp norm equals 1.
  - All distance tests use weights (1, 1) (Convention B placement is untested; 4f's weight tests go through level sets and majority, not `scs_calculate_distance`).
  - Loss tests cover only linear and quadratic; Gaussian and threshold loss are never executed (coverage of `to_cpp_loss`, `scs_api.cpp:231–236`).
  - `NormalizeUtilityRoundTrip` stays inside [0, 1], so the unclamped behaviour in S03h-012 is not exposed.
  - `LevelSet1d` covers two-point output only, with weight 1.
- **Failure scenario:** Mutants `minkowski_as_chebyshev` and `distance_ignores_weights`: 103/103 pass. `probe3`: weights (2, 1) on (3, 4) gives 5 instead of 7.2111 under the mutant.
- **Suggested fix:** Use (1, 1) → 2^(1/3) = 1.2599 for p = 3; add non-unit weights ((3, 4) with (2, 1) → √52); add Gaussian and threshold values (Q19/Q21 are unanswered, so record only the formula chosen); add a utility below the minimum for `normalize_utility`; add a one-point 1D case (`n = 1`).

### S04e-012 [P2, confirmed] Level-set and polygon tests check shape and size, not vertices or centre (S03a-011); ellipse and superellipse sampling never run
- **Where:** tests/unit/test_c_api.cpp:289–317, :538–556, :666–696; `scs_api.cpp:922–929, 960–977` never executed by any test
- **What:** The Manhattan and Chebyshev tests assert `type == POLYGON` and `n_vertices == 4`; the comment "diamond with semi-axes 1" is never checked. The centre of a POLYGON level set is never read, so S03a-011 (centre reported as (0, 0)) cannot be caught. `scs_level_set_to_polygon` is only called with a circle or a hand-built polygon; the ELLIPSE and SUPERELLIPSE branches (and non-unit weights through `ic_polygon_2d`) have no test at this layer. The "matches multi-call path" tests (`IcInterval1d.MatchesThreeCallPath`, `IcPolygon2d.ResultMatchesFourCallPath`, `LevelSetPolygon2d.*`) compare code with itself; `MatchesThreeCallPath` would also pass if both paths returned no points.
- **Failure scenario:** Mutant `polygon_garbage` (Manhattan vertices replaced by 123…130 with `n_vertices = 4`), applied consistently to `level_set_2d`, `level_set_polygon_2d` and `ic_polygon_2d`: 103/103 pass. Real: Manhattan level set about ideal (3, 4) reports `center = (0, 0)`, `v0 = (4, 4)` (`probe_real.txt`).
- **Suggested fix:** Assert the four Manhattan and Chebyshev vertices and the centre; assert the independent values for `ic_interval_1d` (ideal 2, reference 5, linear loss: points −1 and 5, which `probe5` confirms) and `n == 2`; add ellipse and superellipse `to_polygon` tests with a radius check per vertex.

### S04e-013 [P2, confirmed] RNG tests ignore return codes and assert only ranges; no golden values (Q4)
- **Where:** tests/unit/test_c_api.cpp:734–747, :771–785, :1065–1168; `NormalDraw` :1086; `UniformIntInRange` :1112. Decision: Q4
- **What:** `ReproducibleUniformReal`, `ResetAllRestoresSeed`, `ResetStreamRestoresSeed` and `SkipAdvancesState` do not check the return code of `scs_uniform_real`; if the call fails, both sides are 0.0 and the test passes. `NormalDraw` asserts `isfinite` and `|v| < 10`; `UniformIntInRange` 20 draws in [1, 10] (not even that both ends occur); `BernoulliDraw` accepts 0 or 1 (probability 0.5 only, no 0 or 1 edge). No test fixes any drawn value, so nothing detects a change of algorithm (Q4 requires versioned, portable draws with golden values on macOS and Linux). Invalid-argument paths for `normal`, `bernoulli`, `uniform_int`, `uniform_choice` (`n ≤ 0`), `reset_all`, `reset_stream`, `skip`, and `register_streams` with null names are never run (coverage: `scs_api.cpp:3599–3615, 3632–3668, 3708–3774`).
- **Failure scenario:** Mutant `uniform_real_fails` (returns `INTERNAL`, writes nothing): the four reproducibility tests still pass; only the two tests that check `rc` fail. Mutant `draws_const` (normal always the mean, uniform_int always `min`, bernoulli always 0): 103/103 pass.
- **Suggested fix:** Check every return code; add golden-value assertions (exact for uniform, integer, bernoulli, discrete and shuffle; last-bit tolerance for normal) as Q4 specifies, once the algorithms are replaced (this regenerates the expected values once); test `uniform_choice(0)`, `uniform_int(min > max)`, `bernoulli` at 0, 1, −0.1; a statistical sanity check (mean of 1,000 normals) is a cheap extra.

### S04e-014 [P2, confirmed] Competition tests run with `mgr = NULL`, contradicting Q7
- **Where:** tests/unit/test_c_api.cpp:811–812 (`scs_competition_run(…, nullptr, …)`), :914–916. Decision: Q7
- **What:** Q7 says `scs_competition_run` must require a `StreamManager`; a run without one is a defect the reviewers report. Both single-run tests pass `nullptr` and expect success. The experiment test owns its manager and is not affected.
- **Failure scenario:** After Q7 is implemented these two tests fail with the missing-manager error, so the fix must change them. No test asserts the new error today, nor that the stream names used are on the allowlist.
- **Suggested fix:** Create a `StreamManager` in both tests; add the Q7 missing-manager error test.

### S04e-015 [P2, confirmed] Competition coverage is one static scenario; accessors, error paths and Q10 behaviour untested
- **Where:** tests/unit/test_c_api.cpp:787–1063; `scs_api.cpp:347–356, 366–369, 381–389, 405–410, 420–423, 434–440, 452–461, 4140–4164, 4217–4307, 4514–4732`. Decisions: Q7, Q8, Q10
- **What:** Every run uses Sticker strategies, `FIXED` step, `PLURALITY_TOP_K`, `PROJECT_TO_BOX`, `VOTE_SHARE`, one seat, voters that never change the outcome, and nobody moves. Consequences:
  - The Hunter/Aggregator/Predator conversion, Hare, random step, `STAY_PUT`, seat-share objective and cycle termination branches (the Hunter, `HARE`, `RANDOM_UNIFORM`, `STAY_PUT`, `SEAT_SHARE` cases in `scs_api.cpp`) are never executed.
  - Because positions never change, the final shares equal the round-0 shares in any definition, so S02d-008 ("final shares are the last evaluated round's") cannot be caught; the inferred-bounds code (Q10) has no effect on the result.
  - Only round 0 is read from a 2-round trace; out-of-range `round_index`, null and short buffers on every accessor, and the `Euclidean`/`Chebyshev` branches of `trace_dist_config` have no test.
  - `retain_traces` is only passed as 0 (S03c-009), and `final_positions`, `round_seat_shares` and `mean_final_seat_shares` are never called.
  - `RunExperimentAndExportSummary` has three identical runs, so seeds and per-run arrays are not distinguished.
  - No test sets or reads explicit bounds (Q10), or checks scale invariance.
- **Failure scenario:** Any bug in motion, boundary handling, seat allocation, objective choice or accessor row layout passes the suite. Layout example: `run_final_positions` is `(run * n_competitors + c) * n_dims + d` (S03h-005); the test uses n_dims = 1 and identical runs, so a transposed layout would give the same numbers.
- **Suggested fix:** Add a moving scenario (Hunter or fixed-step Aggregator on a small hand-computed example), a 2D case with n_dims = 2 and two runs with different seeds, a Hare case, accessor error loops, and the Q10 tests (explicit bounds, relative-margin default, zero-range dimension, scale invariance, bounds read-back).

### S04e-016 [P2, confirmed] No test for `scs_winset_2d_export_boundary` errors, invalid `k`, or the Q5 near-Plott cases
- **Where:** tests/unit/test_c_api.cpp:1623–1672, :1495–1519. Decision: Q5
- **What:** (a) `export_boundary` is tested only on success (S03a-008, S03h-004: bad input returns 2). (b) `scs_winset_2d` is never called with `k` outside [1, n] or negative (S03a-012: `k = -7` is accepted); `probe5`: `k = 99` gives rc 2 from the export and NULL from the factory; `k = -7` gives a valid handle. (c) The two emptiness tests use a single voter at the status quo (empty by construction) and a centred triangle; `IsEmpty_AtCondorcetWinner` is named for the Condorcet winner but one voter makes the case trivial. Q5 requires near-Plott δ = 0.03, 0.01, 0.003 cases (non-empty) and exact symmetry (empty) at C API level; none exists, and the header's "empty means no policy beats the SQ" (S03h-006, S03a-015) is tested only where it is trivially true.
- **Failure scenario:** `probe5` shows rc 2 for a user error; a test asserting rc 1 would fail today. For Q5, see S03h-006 (`plott.c`): `is_empty = 1` at 64 samples for δ = 0.03.
- **Suggested fix:** Add the export-boundary error tests, `k` ∈ {0, −7, n+1} tests, and the Q5 near-Plott tests when the exact test lands.

### S04e-017 [P2, confirmed] CI runs `test_c_api` on Release only, with no sanitizer or leak check
- **Where:** .github/workflows/ci.yml:88–96; CMakeLists.txt:338–341
- **What:** `CApiTest` is built `-DCMAKE_BUILD_TYPE=Release` and run with `ctest -LE benchmark`. S03a-001 (leaks), S03h-002 (UB on enums), S03a-004/-005, S03c-004 (overflow) and S03a-007 (Release-only SIGFPE) are all invisible to that run.
- **Failure scenario:** An ASan/UBSan or `leaks` run of the existing 103 tests reports 0 issues (3a), so it would not catch these either today; the missing piece is both the sanitizer run and the hostile-input tests of S04e-001..005.
- **Suggested fix:** Add one Debug+ASan/UBSan job (or macOS `leaks`) for `test_c_api` and add the hostile-input tests; the ThreadSanitizer test required by Q9 can use the same job.

### S04e-018 [P2, confirmed] Extreme magnitudes untested: SCP-001, SCP-002, S03a-007, S03a-009, S03c-007
- **Where:** whole range (largest coordinate used is 100)
- **What:** No test uses coordinates near 1e308, weights near 1e-13, or a range whose width overflows.
- **Failure scenario:** `probe4 scp002`: one voter at (0, 0), weights (1e-13, 1e-12), utility −1e-22 returns a circle of radius 100 (type 0, `param1 = 0`) where an ellipse with semi-axes (100, 10) is expected. `probe4 uniform_overflow`: `scs_uniform_real(−1.7e308, 1.7e308)` returns `rc = 0, x = inf`. SCP-001/S03a-007 (SIGFPE on Release) were demonstrated in 3a (`rel.out`) and are not repeated here. Tests for the first two would fail today.
- **Suggested fix:** Add the SCP-002 weights case, the SCP-001/S03a-007 Manhattan 1.5e308 case (on Release; under a death test until fixed) and the `uniform_real` overflow case.

### S04e-019 [P3, confirmed] Q2 (exact Minkowski at p ≥ 100) has no test at the C API and would fail today
- **Where:** tests/unit/test_c_api.cpp:97–111, :319–333 (only p = 1, 2, 3 anywhere in the file). Decision: Q2
- **What:** No call uses p ≥ 100 for `scs_calculate_distance`, `scs_level_set_2d` or any winset. The C++ tests that assert the old behaviour belong to slice 4a. Rated P3 here because 4a owns the main fix; the C API check is a one-assertion addition.
- **Failure scenario:** `probe_real.txt`: (0, 0) to (1, 1), Minkowski: p = 99 gives 1.0070260543835, p = 100 gives exactly 1, p = 101 gives 1, p = 1000 gives 1. The exact values are 1.00695555, 1.00688645, 1.00069339.
- **Suggested fix:** Add C API assertions at p = 50, 99, 100, 1000 with relative tolerance 1e-12, as the Q2 "Implies" list specifies.

### S04e-020 [P3, confirmed] Misleading names, comments and dead code
- **Where:** tests/unit/test_c_api.cpp:157 (`NormalizeUtilityRoundTrip`, not a round trip); :518 (`EuclideanCircleMatchesLevelSetPath`, does not compare with the level-set path); :1170 (`ErrorOnNullStream`, nulls the manager, and creates a manager it never uses); :1344–1367 (`CondorcetCycle_AntiSymmetry`); :1065–1084 (`ResetStreamRestoresSeed`, unused `v1` and `stream_seed` with `(void)` casts, effectively a duplicate of `ResetAllRestoresSeed`); :1506 (`IsEmpty_AtCondorcetWinner`, single voter); :1142 (`SkipAdvancesState` silently depends on `uniform_real` consuming exactly one raw output, which Q4's replacement algorithms may change).
- **What and scenario (the cycle test):** The test comment says "3-voter Condorcet cycle", but the matrix is `0 0 1 / 0 0 1 / −1 −1 0` (`probe_real.txt`): A and B tie, both beat C, no cycle. Anti-symmetry is checked on non-trivial entries, so the test is useful; only the name and comment are wrong, and no C API test contains a real cycle. `$SCR4E/cyc.cpp` found one: alternatives (3, 2), (2.5, 3.5), (0, 3.5) with voters (4, 3.5), (0.5, 2.5), (0.5, 0) give the matrix `0 −1 1 / 1 0 −1 / −1 1 0`: A loses to B, B loses to C, C loses to A.
- **Suggested fix:** Rename and re-comment; use the cyc.cpp fixture for a real cycle; delete the dead variables.

### S04e-021 [P3, confirmed] Duplicated tests and helpers, scattered groups, message checks too weak
- **Where:** tests/unit/test_c_api.cpp:375–384 vs :386–392 (`AllDistinct` implied by `SentinelValues`); :404–408 vs :398–402; :508 `make_euclidean_2d()` vs :1258 `make_euclidean_dist(...)` (same job; 4f also uses the second); `CApi_StreamManager` tests are split around the `CApi_Competition` tests (lines 723–1195), and `CApi_Winset` tests resume after 1800 and again in 4f; all error-message checks are `EXPECT_GT(strlen(err), 0u)` (e.g. :126, :173, :501)
- **What:** Constant-value tests are tautologies against the header. Messages are never checked for the function name, the value entered or the fix, so S03a-014, S03c-011 and the end-user rule cannot be enforced by these tests.
- **Failure scenario:** The raw "vector" message from `scs_register_streams(n_names = -1)` (`probe4`: `rc = 2, err = 'vector'`) would pass any current message check.
- **Suggested fix:** Drop the redundant tests, share one helper, group tests by suite, and assert that key messages contain the argument name and the entered value.

## Findings map
Every P0/P1 from the earlier reports for this area, the SCP items in the area, and the P2s briefly. There are no P0s. "Wrong-behaviour test" means a test that asserts the buggy behaviour; none was found.

| Earlier finding | Severity | Status | Test name or note |
|---|---|---|---|
| S03a-001 winset factories leak on throw | P1 | no test | No core-throwing input in range; S04e-002 |
| S03a-002 unknown loss/distance enum accepted | P1 | no test | S04e-003; `probe4 enum_loss` |
| S03a-003 `level_set_to_polygon` unvalidated polygon | P1 | cannot catch | `ToPolygon.PolygonPassthroughVertices` uses valid 4 vertices; S04e-005 |
| S03a-004 `pairwise_matrix` `n*n` overflow | P1 | cannot catch | `PairwiseMatrix.Error_BufferTooSmall` (n = 3); S04e-001 |
| S03a-005 `convex_hull` `n*2` before check | P1 | cannot catch | `ConvexHull.Error_ZeroPoints` (n = 0); S04e-001 |
| S03a-006 `winset_interval_1d` accepts NaN/Inf | P1 | no test | `WinsetInterval1d.*` finite only; S04e-004 |
| S03c-001 experiment skips finiteness checks | P1 | no test | S04e-004; `probe4 exp_nan` |
| S03c-002 step/termination doubles unchecked | P1 | no test | S04e-004 |
| S03c-003 unknown seat/boundary/objective enum | P1 | no test | S04e-003; `probe4 enum_seat` |
| S03c-004 `n * dims` overflow | P1 | no test | S04e-001 |
| S03c-005 `strategy_kinds` null/short buffer | P1 | cannot catch | `Competition.RunTraceAndExportRoundData` calls it once, validly; S04e-005 |
| S03h-001 0.5 "simple" weighted majority, both sides win | P1 | cannot catch | `WeightedMajority.HighWeightVoterDominates`, `UniformWeightsMatchesSimpleMajority`; S04e-006 (mutant) |
| SCP-003 Voronoi `n_sites * 2` overflow | P1 | cannot catch | `Voronoi2d.InvalidNSites` (n = 0); S04e-001 |
| SCP-001 finite semi-axes, Inf vertices | P2 | no test | S04e-018 |
| SCP-002 near-equal weights treated as equal | P2 | no test | S04e-018 (`probe4 scp002`: circle instead of ellipse) |
| S03a-007 extreme coordinates reach CGAL | P2 | no test | S04e-018 |
| S03a-008 `export_boundary` returns 2 for input errors | P2 | no test | `Winset.ExportBoundaryMatchesHandlePath` success path only; S04e-016 |
| S03a-009 other `n * 2` overflows | P2 | no test | S04e-001 |
| S03a-010 `bad_alloc` outside `try` | P2 | no test | Needs fault injection; not practical in this suite |
| S03a-011 POLYGON centre reported (0, 0) | P2 | cannot catch | `LevelSet2d.ManhattanIsPolygon4Vertices` checks type and count; S04e-012 |
| S03c-006 `signature_resolution` ≤ 0 replaced | P2 | no test | S04e-004/015 |
| S03c-007 `uniform_real` overflow returns Inf | P2 | no test | S04e-018 |
| S03c-008 `register_streams` negative count | P2 | no test | S04e-013; `probe4 neg_names` |
| S03c-009 `retain_traces` unreadable | P2 | no test | S04e-015 (only 0 passed) |
| S03c-010 `bad_alloc` outside `try` | P2 | no test | Not practical |
| S03h-002 enums UB | P2 | no test | S04e-003 |
| S03h-003 short buffers give code 1 or 3 | P2 | no test | Only `strategy_kinds` is in range; S04e-005 (4f for the others) |
| S03h-004 export reports input error as INTERNAL | P2 | no test | S04e-016 |
| S03h-005 competition API undocumented | P2 | n/a (docs) | Tests are the only layout evidence and use n_dims = 1; S04e-015 |
| S03h-006 header overclaims emptiness | P2 | no test | S04e-016 |
| S03h-007 header silent on thread safety | P2 | no test | No concurrency test; Q9's TSan test belongs in S04e-017 |
| S03h-008 size-query only for NULL in four functions | P2 | no test | `SizeQueryNullBuffer` tests pass NULL; capacity 0 with non-NULL never tested |
| S03a-012, -013, -014, -015, S03c-011, S03h-009..015 | P3 | no test / n/a | S04e-021 (messages); Heart: no tests in this range (they are in 4f); S03h-015 heap failure state untested |
| PR #1 items (Q18 zero weights, finite Voronoi) | n/a | fixed and tested, in 4f | `CApi_SalienceWeights`, `CApi_NonFiniteInput`; not in this range |

Count of earlier P0/P1 findings with no test that could catch them: **13 of 13** (7 no test, 6 test exists but cannot catch, 0 wrong-behaviour tests, 0 fixed and tested).

## Tool leads checked
- Whole-suite uncovered `scs_api.cpp` lines in this slice's areas (188 groups, 795 lines) → mostly catch blocks (`e.what()`, not reachable without fault injection) and argument checks; the ones that matter are S04e-004, -005, -013, -015, -016. The 7 functions never called anywhere: four in this slice's area (S04e-010, -015) and `scs_centroid_2d`, `scs_marginal_median_2d`, `scs_geometric_mean_2d` (`scs_api.cpp:4030–4113`), which no slice owns: out-of-slice note below.
- `to_cpp_loss` Gaussian/threshold (`scs_api.cpp:231–236`) never run → S04e-011.
- `scs_level_set_to_polygon` ELLIPSE/SUPERELLIPSE branches (`:922–929`, `:960–977`) never run → S04e-012.
- Voronoi malloc-failure branches (`:1938–1951`) → not a defect in the tests; needs fault injection.
- Slice-only coverage (103 tests): 41.7% of regions, 38.3% of lines of `scs_api.cpp`; the other tests (4f) bring it to 69.8% / 60.8%. The 53 line groups covered only by 4f include the Manhattan branch of `to_dist_config` and most of the finiteness checks, i.e. this slice does not test PR #1.
- Determinism and timing: fixed seeds, no clocks, no benchmark label; total 0.19 s. No finding.
- `CApi_Version.ReturnsCompiledVersion` (:354) compares the library with the header macros; it stays valid under Q11 but does not pin 0.3.0, which `check-all.sh` must do. Not a defect.
- `CApi_ErrorCodes.SentinelValues` pins the numeric codes; Q12 allows later layout changes but not code changes. No finding.
- Q1 Heart: no Heart tests in lines 1–1910 (they are in 4f). Q8: no `hunter_sticker` use in range. Q3, Q4 beyond S04e-013, Q9, Q10 beyond S04e-015: no further instance.
- Suspected expected values pasted from output: none found. Hand values in the range are derivable (3-4-5, r/w, 2/3, 1/3, −2/2). The `0.667` in `make_triangle_winset` is an approximate centroid, not an output.

## Open questions
1. Under the Q6 sentinel, should `scs_weighted_majority_prefers_2d` with an explicit `threshold = 0.5` mean "at least half" (both directions can win) or "more than half"? S04e-006 asks Stage 11 to test the chosen reading.
2. For S04e-001 and S04e-005: do you want the hostile-input tests in the plain GoogleTest binary (a fixed bug then fails cleanly) or as `EXPECT_EXIT` death tests until the fixes land? Today they would kill the whole binary.

## Out-of-slice notes
- `scs_centroid_2d`, `scs_marginal_median_2d`, `scs_geometric_mean_2d` (`scs_api.cpp:4030–4113`) are never called by any test, and no stage-4 slice lists them; assign them (probably 4f, with Stage 3b's range).
- Stage 4f's `CApi_Winset` tests (e.g. :3551) also use the default threshold 0.5 and will need the same Q6 update.
- The coverage columns the coordinator flagged (header files) are not used here; the `scs_api.cpp` totals were re-derived from a fresh `test_c_api` run and match the shared report.
