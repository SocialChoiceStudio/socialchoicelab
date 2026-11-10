# Stage 4c — Geometry II tests
Model / effort: Sonnet / high   Commit: 2771655   Files reviewed: 4 of 5 in full (`test_yolk.cpp` 551, `test_uncovered_set.cpp` 412, `test_centrality.cpp` 262, `test_geometry_integration.cpp` 524 = 1,749 lines); `test_heart.cpp` (306 lines, 17 tests) listed only, per Q1.
Not read (if any): `test_heart.cpp` beyond its first 40 lines and test count (withdrawn code, Q1). Code under test was read only to settle specific questions (`yolk.h` line family and distance helpers, `uncovered_set.h`, `centrality.h`).

`SCR4C=/private/tmp/claude-501/-Users-robiragan-Dropbox-SocialChoiceStudioDev/2402f97e-221e-494b-88cb-fc63c41efc50/scratchpad/stage4c`
Build helper: `$SCR4C/build.sh <file.cpp>` (Apple clang, `-std=c++20 -O2`, repo `include/`, Eigen from the Stage 4 release tree, Homebrew CGAL/GMP/MPFR). Test binaries compiled from the repo's own test files against stub or mutated headers use `$SCR4C/stub_yolk.h` and `$SCR4C/mut_*/`.

## Summary

- **0 P0, 2 P1, 9 P2, 5 P3.**
- **No Yolk test can catch S02b-002 (the Yolk P0).** I replaced `yolk_2d` with a stub that returns the voter centroid and the radius at the centroid, and ran the unchanged test files against it: `test_yolk` 32/32, `test_uncovered_set` 16/16, `test_geometry_integration` 32/32 all pass (`$SCR4C/stub_run.out`). Every Yolk test uses a configuration whose centroid is the Yolk centre (equilateral triangle, collinear voters) or asserts something any point in the cluster satisfies (`radius >= 0`, "within 2.0 of the centroid", "inside the hull"). A hand-checkable known answer that fails today: for any triangle and simple majority the Yolk is the incircle. For voters (0,0), (4,0), (0,3) that is r = 1 at (1,1); `yolk_2d` returns r = 1.333 at (1.333, 1) (`$SCR4C/y3_incircle.out`, S04c-001).
- **No test covers the even-n or supermajority line family (S02b-005, P1).** There is no value test for any even n, and no invariance test. For n = 6 and 8, `yolk_2d(-X)` differs from `yolk_2d(X)` in radius by up to 0.15 (`$SCR4C/y2_props.out`). `Unanimity_ShiftsLineToOutermostVoter` is true for every k, including k = 1 (`$SCR4C/q1_unanimity.out`).
- **The uncovered-set tests only exercise the covering relation in one direction.** Condition (2) of `covers()` is tested only where it fails or is vacuous. A mutant that replaces it with "nothing beats x" passes all 48 tests. A mutant that ignores `DistConfig` passes all of them too, because no test uses a non-default metric or weights (S04c-006, S04c-007; `$SCR4C/mutants_uncovered.out`).
- **Owner decisions.** Q1: Heart tests are to be removed (S04c-012, one P3; includes two Heart theorem tests at lines 494–524 that Q1's Implies list does not name, and the stated relation "Heart ⊆ Uncovered Set" is the reverse of Q1's). Q5, Q6: touched by the integration winset and weighted tests (S04c-010, S04c-016). Q23, Q25 are unanswered; the tests that rely on them are listed under Open questions, not as defects.
- **Coverage is not the problem here.** Line coverage from a fresh run of the slice's own `build_cov` binaries (own profile, `llvm-cov` on those binaries only): `yolk.h` 97.45% (5 of 196 lines missed: a never-called lambda method and the all-collocated early return), `uncovered_set.h` 100%, `centrality.h` 100%. The shared `coverage_report.txt` shows 43% for `yolk.h` because llvm-cov merged mismatched data (see Tool leads).
- **Earlier P0/P1 findings in this slice with no effective test: 2** (S02b-002 cannot be caught; S02b-005 has no test). All tests are deterministic, no RNG, no timing assertions; total runtime about 0.85 s; none carries the `benchmark` label; all five files are registered with `add_test`.

## Contracts this layer provides

What a green run of these five files does and does not tell later stages:

- **Reliable (independent expected values, verified):**
  - `quantile_lines_2d`: right-triangle lines (hand calculation) and the Laing-Olmsted-Bear limiting pairs. I re-derived the six limiting pairs (0,1), (0,3), (1,2), (1,3), (1,4), (2,4) in exact rational arithmetic (`$SCR4C/lob_independent.out`). They match.
  - `covers`/`uncovered_set` on a Condorcet cycle, a Condorcet winner, and a one-dimensional supermajority case (hand calculation; I checked the cycle claims by running `majority_prefers`).
  - `centroid_2d`, `marginal_median_2d`, `geometric_mean_2d` values and error messages.
- **Not guaranteed by any test:** Yolk centre or radius for any non-symmetric configuration; Yolk for even n or k > simple majority; Yolk with duplicate or non-finite voters; `uncovered_set`/`covers`/boundary under any metric or weight other than the default; the boundary's shape beyond "has at least 1 vertex" (3 for one case); `grid_resolution` validation and clamping; the C++ default grid resolution (never computed).
- **Determinism:** no RNG, no clocks, no shared state. `UncoveredSetTest` is the slowest at 0.46 s (Release). Nothing here needs the `benchmark` label.
- **Tolerances:** the Yolk tests use 0.01 (radius) and 0.05 (centre) against exact answers of about 0.29 and 0.5. That is about 3% and 10% of the quantity checked. The header's own accuracy claim is 0.005 × spread.

## Findings

### S04c-001 [P1, confirmed] No Yolk test can catch S02b-002 (solver never leaves the centroid)
- **Where:** `tests/unit/test_yolk.cpp:313–363`, `:371–381`; `tests/unit/test_geometry_integration.cpp:281–310`, `:447–459`
- **What:** Every test that calls `yolk_2d` either uses a configuration where the centroid is the exact Yolk centre (`CollinearMedian_RadiusNearZero`: (0,0),(0.5,0),(1,0); `EquilateralTriangle_AnalyticalResult`; `kCollinear5`), or asserts a property the buggy output also has (`Asymmetric4Voters_NonNegativeRadius`: `radius >= 0`; `YolkRadius_NonNegative`; `YolkCenter_NearVoterCentroid`: within 2.0 of the centroid for voters spread over about 0.8; `Cycle_YolkCenter_InsideVoterHull`: the centroid is always in the hull; `EquilateralTriangle_UnanimityLargerRadius`: an inequality). `Integration_Extended.Core2D_FindsMedianVoterCore` states in a comment that "the Yolk centre for this symmetric configuration is (0,0) exactly", which is the symmetric case that hides the bug. `YolkCenter_NearVoterCentroid` even names the buggy behaviour as the expectation. The only expected values are from symmetric cases; the `uncovered_set` test that uses the Yolk (`EquilateralTriangle_BBoxContainsYolkCenter`) also uses the equilateral triangle.
- **Failure scenario:** `$SCR4C/stub_yolk.h` makes `yolk_2d` return the voter centroid and the radius at the centroid (no optimisation). Compiled with `-include stub_yolk.h` against the unchanged repo test files: `test_yolk` 32/32 passed, `test_uncovered_set` 16/16, `test_geometry_integration` 32/32 (`$SCR4C/stub_run.out`). `$SCR4C/y1_centroid.out`: for every configuration the tests use, `|yolk_2d centre − centroid|` is at most 3.3e-9, and for the asymmetric ones the code's radius is larger than the true minimax of its own line family (asym4 0.425 vs 0.350; `kVoters5` 0.103 vs 0.0705; `kCycleVoters` 0.693 vs 0.584). A hand-checkable known answer fails today (`$SCR4C/y3_incircle.out`): for n = 3 and simple majority the three critical lines are the triangle's sides and every other median line passes through a vertex inside its angle, so the Yolk is the incircle. Triangle (0,0),(4,0),(0,3): expected r = 1, centre (1,1); `yolk_2d` gives r = 1.3333, centre (1.3333, 1.0000). Triangle (0,0),(10,0),(1,1): expected r = 0.4885; `yolk_2d` gives 2.7487 (463% too large). A brute-force minimax over the code's line family (`$SCR4C/y1_centroid.out`) gives the same incircle values, so the derivation is checked independently of the formula.
- **Suggested fix:** Stage 11: add (a) the incircle test for 2–3 non-equilateral triangles (r = Area/s, incentre; this uses only elementary geometry and is exact for any correct solver); (b) an optimality certificate on random odd-n configurations: for the line family from `quantile_lines_2d` plus sampled directions, `yolk.radius` must not exceed the maximum line distance at the centroid or at 100 random probe points, and must be within a stated tolerance of a brute-force minimax; (c) tighten the equilateral tolerance to about 1e-9 once the LP solver replaces the subgradient loop; (d) replace the integration "near centroid" test with the incircle case.
- **Also affects:** `test_c_api.cpp` Yolk tests (4f) probably share the symmetric-only pattern; `test_core.cpp` (4b) for `core_2d`'s first candidate.

### S04c-002 [P1, confirmed] No test covers the even-n or supermajority line family (S02b-005)
- **Where:** `tests/unit/test_yolk.cpp` (whole file); `tests/unit/test_geometry_integration.cpp` (all Yolk calls)
- **What:** Of the `yolk_2d` / `quantile_lines_2d*` tests, only `Asymmetric4Voters_NonNegativeRadius` (assertion `radius >= 0`) and the collinear four-voter line tests use even n; no `yolk_2d` test with n > 3 and a non-default `k` exists. S02b-005 is the missing "other edge of each median band". There is no value test, and no invariance test of the kind that exposes it. Odd n with simple majority is the only case where the code's family is complete, so every test is in the safe region.
- **Failure scenario:** `$SCR4C/y2_props.out` (20 random configurations per n, `yolk_2d(X)` against `yolk_2d(−X)`, where −X is the point reflection of all voters): n = 5 and 7 give identical radii; n = 6 differs by up to 0.1484 and n = 8 by up to 0.0618. The Yolk of a configuration cannot depend on whether it is reflected, so any such test fails today. The centres satisfy c(−X) = −c(X) (centroid), so the test must compare radii.
- **Suggested fix:** Stage 11: add reflection, translation and permutation invariance tests for n = 4, 6, 8 and for k = 4, 5 with n = 7. These do not need Q22's answer. Value tests for even n and k-majority need Q22 first (see Open questions). Keep the exact-line tests (`quantile_lines_2d`) as the oracle for which lines belong to the family: assert that both band edges, k−1 and n−k, are present once S02b-005 is fixed.
- **Also affects:** `scs_yolk_2d` tests (4f); `core_2d` candidate choice (4b).

### S04c-003 [P2, confirmed] `Unanimity_ShiftsLineToOutermostVoter` is true for every k, so it cannot detect a wrongly handled k
- **Where:** `tests/unit/test_yolk.cpp:207–236`
- **What:** The test's k = 3 assertion is "some line in `lines_k3` passes through C = (0,2)". For voters (0,0), (2,0), (0,2), the lines for pairs (A,C) and (B,C) pass through C for every k, so the predicate holds whatever k the code uses. The test name says it checks that the (A,B) line moves to the outermost voter; it does not identify the (A,B) line in the k = 3 set.
- **Failure scenario:** `$SCR4C/q1_unanimity.out`: the number of lines through C is 1 for k = 1, 2 for k = 2, 2 for k = 3. A mutant that handled k = 3 as k = 2 (or k = 1) still passes `found_AB_k3`.
- **Suggested fix:** Use the annotated version (`quantile_lines_2d_annotated`) to pick the (A,B) line by voter indices, then assert its c coefficient: y = 0 for k = 1, 2 and y = 2 for k = 3 (hand calculation in the test's own comment).
- **Also affects:** none.

### S04c-004 [P2, confirmed] Loose tolerances on the Yolk analytic tests; other Yolk assertions are vacuous
- **Where:** `tests/unit/test_yolk.cpp:313–323` (radius ≤ 0.01, centre ±0.05), `:339–352` (radius ±0.01, centre ±0.05), `:358–363`; `tests/unit/test_uncovered_set.cpp:320–352`; `tests/unit/test_geometry_integration.cpp:281–310`
- **What:** The equilateral test's exact answer is √3/6 at the centroid (the incircle). The test accepts radius within 0.01 (3.5%) and centre within 0.05 (17% of the radius). The header's accuracy claim is 0.005 × spread (about 0.005 here), so even the claim is not tested. `EquilateralTriangle_BBoxContainsYolkCenter` accepts the Yolk centre anywhere within 0.1 of the polygon's bounding box, which for the equilateral case is the polygon plus about 35% of its size. The vacuous ones (`radius >= 0`, "within 2.0 of the centroid", "inside the hull") are covered in S04c-001.
- **Failure scenario:** Any centre within 0.05 and any radius within 0.01 of the right answer passes. With the planned exact LP solver, an answer off by 3% would still pass, so the tests cannot guard against a regression in the replacement.
- **Suggested fix:** Compare the equilateral and incircle cases to 1e-9 once the LP replaces the subgradient solver; until then, 1e-6 on the exact quantities that do not depend on the solver (the three side lines). Delete the vacuous assertions or give them a computed expectation.
- **Also affects:** none.

### S04c-005 [P2, plausible] Vacuous and misnamed assertions in the integration tests
- **Where:** `tests/unit/test_geometry_integration.cpp:244–259`, `:261–275`, `:339–364`, `:479–488`, `:180–198`
- **What:**
  - `VetoPlayer_ShrinksWinset` ends with `static_cast<void>(ws_veto); // no throw = pass`. Its comment says the veto winset is a subset of the plain one but checks nothing.
  - `WinsetOps_IntersectionWithSelf_EqualsSelf` and `WinsetOps_UnionWithEmpty_EqualsSelf` assert only non-emptiness, not equality (area or containment of sampled points).
  - `Boundary_ContainedInVoterHullExpansion` allows the polygon to lie 3.0 outside the voter extent, but the grid box is the voter extent plus 30% (0.24 here), so the bound is 12 times looser than what the code guarantees by construction.
  - `WeightedWinset_UniformWeights_NonEmpty` asserts both winsets are non-empty; its comment says it should "match the qualitative result of a standard simple majority winset".
  - `PairwiseMatrix_RowConsistency`: the `EXPECT_FALSE(M(i,j) > 0 && M(j,i) > 0)` line is implied by the preceding `M(i,j)+M(j,i) == 0` check.
- **Failure scenario:** A veto implementation that returns the plain winset, an intersection that returns an unrelated non-empty region, or a boundary that spans the whole grid still passes these tests (the whole-grid mutant passes `Boundary_ContainedInVoterHullExpansion`, `$SCR4C/mutants_uncovered.out`).
- **Suggested fix:** Compare areas or exact containment with `winset_intersection`/`winset_difference` (the veto winset must equal the intersection of the plain winset with the veto's preferred region); equality tests for the set operations; a bound of margin × extent + epsilon for the boundary.
- **Also affects:** `test_winset_ops.cpp` (4b) likely has the exact versions; check for overlap before adding.

### S04c-006 [P2, confirmed] `covers()` condition (2) is only tested where it fails or is vacuous
- **Where:** `tests/unit/test_uncovered_set.cpp:101–186`, `:232–263`; `tests/unit/test_geometry_integration.cpp:316–337`
- **What:** `covers(x, y)` requires that x beats y and that every z beating x also beats y. The cycle test exercises the failing branch (some z beats x but not y). The Condorcet winner and supermajority cases have no z beating x, so condition (2) is vacuous. No test has x covering y while some z beats x and y. There is also no mixed `uncovered_set` result: every test returns all alternatives (cycle), one (winner, k = 2 supermajority case), or both (k = 3).
- **Failure scenario:** `$SCR4C/mutants_uncovered.out`: with condition (2) replaced by "no z beats x" (`mut_c2`), `test_uncovered_set` passes 16/16 and `test_geometry_integration` 32/32. A case that separates them (hand-checkable): voters at x = 0, 1, 2.5 on the x-axis; alternatives 0.4, 1, 3, −1. Alternative 1 (the median voter) beats everything and beats 0.4 by 2:1; 0.4 beats 3 and −1 by 2:1. So 0.4 covers 3 although 1 beats 0.4 and also beats 3. The real code returns true (`$SCR4C/u2_case.out`); the mutant returns false.
- **Suggested fix:** Add that case; add a four-alternative case (a 3-cycle plus one alternative beaten by all three, uncovered set = the cycle); replace `AlwaysNonEmpty_ForNonEmptyAlternatives` (two fixed configurations) with a randomised property test with a fixed seed. I ran 2,196 random (n, m, k) trials, k from 1 to n: no empty result, so the property holds and the test would be stable.
- **Also affects:** none.

### S04c-007 [P2, confirmed] No uncovered-set test uses a non-default `DistConfig`
- **Where:** `tests/unit/test_uncovered_set.cpp` (all `covers`, `uncovered_set`, `uncovered_set_boundary_2d` calls); `tests/unit/test_geometry_integration.cpp` (`kEuc` throughout)
- **What:** Every call uses `DistConfig{}` or `kEuc`, except `ZeroSalienceWeight_Throws`, which checks a throw for the boundary and `EXPECT_NO_THROW` for `uncovered_set` with weights {1, 0} (no value). The config is forwarded to `majority_prefers` in three places in `covers()` and in the grid loop, and nothing verifies that forwarding.
- **Failure scenario:** `$SCR4C/mutants_uncovered.out`, `mut_cfg`: `covers()` ignoring `cfg` (always Euclidean) passes `test_uncovered_set` 16/16 and `test_geometry_integration` 32/32.
- **Suggested fix:** Add one `uncovered_set` and one boundary test each for Manhattan, Chebyshev and weighted Euclidean (positive weights), with a configuration where the metric changes the answer and a hand-calculated expectation (choose alternatives on a diagonal so L1/L2 distances can be computed by hand).
- **Also affects:** `test_core.cpp`, `test_copeland.cpp` (4b) likely share the Euclidean-only pattern.

### S04c-008 [P2, confirmed] Uncovered-set boundary: tests cannot detect a wrong set, degenerate hulls, resolution handling or the default
- **Where:** `tests/unit/test_uncovered_set.cpp:284–393`; `tests/unit/test_geometry_integration.cpp:339–364`. Maps S02b-008, S02b-009 (resolution part), S02b-015.
- **What:** The tests for the uncovered set boundary assert `size() >= 3` for one configuration (equilateral triangle, 20 × 20 grid) and `size() >= 1` elsewhere (`ResolutionParameter_NonEmpty`, `AllVotersCollocated`, and the integration test); a hull of any grid points has at least one vertex. Missing:
  - a case where the hull degenerates (S02b-008: N(0,1) voters at the default resolution give 1 hull vertex; Q25 is unanswered, so this is a gap, not a contradiction);
  - `grid_resolution` below 2 (silently clamped to 2: `res = 1, 0, −3` all return a 3-vertex hull) or very large;
  - the `k` parameter;
  - the C++ default resolution (30), which no test computes. S02b-015's mismatch with the C API default (15) cannot show up in a C++ test, but nothing pins the C++ default either.
- **Failure scenario:** `$SCR4C/mutants_uncovered.out`, `mut_grid` (boundary returns the hull of the whole grid): of 48 tests, only `AllVotersCollocated_BoundaryNearVoter` fails; every equilateral, resolution and integration test still passes. `$SCR4C/u1_misc.out`: N(0,1), n = 51 gives a 1-vertex hull at both res = 15 and res = 30.
- **Suggested fix:** Assert a computed property of the result for the equilateral triangle: the polygon must be inside the grid box and must exclude the grid corners (which are covered by interior points). Add tests for k = 3 (unanimity should give the Pareto-like larger hull), and for `grid_resolution` ≤ 1 once the owner decides the contract (S02b-008 suggests validating with an end-user error).
- **Also affects:** `scs_uncovered_set_boundary_2d` tests (4f).

### S04c-009 [P2, confirmed] Missing edge-case tests for `yolk_2d` and the quantile lines
- **Where:** `tests/unit/test_yolk.cpp`. Maps S02b-009 and S02b-011.
- **What:**
  - Duplicate voters: no test (S02b-011). `$SCR4C/y2_props.out`: voters (0,0),(0,0),(1,0),(0,1),(1,1) give 8 limiting lines, one of them degenerate (a = b = 0).
  - `n_sample`: never passed. Zero and a small negative value are accepted silently (`n_sample = −5` no throw; `0` returns r = 0 at the centroid).
  - Non-finite voters: a NaN voter returns a NaN centre with no error (`$SCR4C/y2_props.out`).
  - `yolk_2d` with `k` outside [1, n]: only `quantile_lines_2d` has a `k` error test (`ThrowsOnKOutOfRange`); `yolk_2d(·, 0)` and `yolk_2d(·, n+1)` throw (the message begins "majority:", not "yolk_2d:"; no test checks the message).
  - All-collocated voters: returns r = 0 at the point (the `spread < 1e-14` branch), untested.
  - Sizes above n = 5, and any scale or translation test (scaling by 3 is stable to 2e-8 in my run).
- **Failure scenario:** Duplicate-voter data (survey and grid data) produces a degenerate line that `limiting_quantile_lines_2d` returns as limiting; nothing alerts the maintainer when that is fixed or regressed.
- **Suggested fix:** One test for each of the bullets, expectations from the decisions made for S02b-009/011 (skip identical pairs; validate `n_sample ≥ 0` with an end-user error); a size-10 or size-20 smoke test with an invariance check.
- **Also affects:** `scs_yolk_2d` tests (4f).

### S04c-010 [P2, confirmed] The weighted-versus-unweighted majority test uses odd n only, so it cannot catch S02b-006 (Decision: Q6)
- **Where:** `tests/unit/test_geometry_integration.cpp:469–477` (and `:479–488`)
- **What:** `WeightedMajority_UniformWeights_MatchesMajority` compares `majority_prefers` with `weighted_majority_prefers(..., threshold 0.5)` for five voters. With five equal weights, exactly half the weight (2.5) is never reachable, so the two always agree. S02b-006 (P1) is the exactly-half case: with an even number of equal weights, both directions hold at once. Q6 says the default must mean "more than half". The test also hard-codes the `0.5` default that Q6 will replace by a sentinel.
- **Failure scenario:** `$SCR4C/u1_misc.out`: two voters at 0 and 2, x = 0.5, y = 1.5: `majority_prefers(x,y)` = 0 and `(y,x)` = 0, but `weighted_majority_prefers(x,y,w={1,1},0.5)` = 1 and `(y,x)` = 1.
- **Suggested fix:** Run the agreement test for n = 2, 4, 6 as well as odd n; expect agreement with the unweighted rule (Q6). Severity depends on 4b: this is P1 if no test in `test_majority.cpp` catches S02b-006; I rate it P2 here because that is where the primary test belongs.
- **Also affects:** `test_majority.cpp` (4b).

### S04c-011 [P2, plausible] `uncovered_set` and `covers` edge cases are untested
- **Where:** `tests/unit/test_uncovered_set.cpp:188–274`
- **What:** Untested: empty voters (header says it throws; `SingleAlternative` and fewer-than-two alternatives return without touching the voters, S02b contracts); duplicate alternatives (they are matched by value and kept); even n or ties (two alternatives equidistant for every voter); `k` outside [1, n]; `x` or `y` not in `alternatives`; collinear or one-dimensional voters other than the single supermajority case.
- **Failure scenario:** Behaviour on these inputs can change with no failing test. For example, duplicate alternatives are both kept today; a "dedupe" refactor would drop one silently.
- **Suggested fix:** One test each with a hand-derived expectation; pin the empty-voters behaviour after the owner confirms the contract.
- **Also affects:** none.

### S04c-012 [P3, confirmed] Heart tests to be removed (Q1)
- **Where:** `tests/unit/test_heart.cpp` (306 lines, 17 tests); `CMakeLists.txt:186–189` (`test_heart`, `HeartTest`); `tests/unit/test_geometry_integration.cpp:13, 21–22, 30–31, 38, 57, 367–400, 494–501, 511–524`. Decision: Q1
- **What:** Q1's Implies list names `test_geometry_integration.cpp:367–400`. Two more Heart tests are outside that range: `Integration_Theorems.HeartSubsetOfUncoveredSet_Cycle` (494–501) and `CondorcetWinner_HeartEqualsUncoveredEqualsCore` (511–524), plus the file header (lines 13, 21–22: "Heart ⊆ Uncovered Set") and the include and using-declarations (38, 57). The "Heart ⊆ Uncovered Set" theorem the tests assert is the reverse of the relation Q1 gives for a correct Heart (UC ⊆ Heart). Not reviewed for correctness.
- **Failure scenario:** n/a (removal list). Removing `heart.h` without these lines breaks the build of `test_geometry_integration`.
- **Suggested fix:** Remove with the Q1 work, including the two extra tests and the header comment lines.
- **Also affects:** `test_c_api.cpp` Heart tests (4f).

### S04c-013 [P3, confirmed] Centrality tests have no non-finite or extreme-magnitude cases
- **Where:** `tests/unit/test_centrality.cpp`
- **What:** The values and error messages are well tested (every function: odd, even, single, empty, sign handling, message contents; independent hand values). Missing: NaN and Inf coordinates, magnitudes near `DBL_MAX`, duplicates (median with repeated values).
- **Failure scenario:** `$SCR4C/u1_misc.out`: `centroid_2d` of two points (1e308, 1e308) returns (inf, inf), and `marginal_median_2d` with even n returns (inf, inf) (the sum overflows; the correct value is 1e308). `geometric_mean_2d` returns NaN or Inf for NaN or Inf x with no error. The median with duplicates {1,1,1,5} is correct (1,1). The code issues are listed under Out-of-slice notes; they are extreme inputs, so P3.
- **Suggested fix:** Add the three cases once the owner decides whether the core validates finiteness (the C API does) or documents the precondition.
- **Also affects:** none.

### S04c-014 [P3, confirmed] Misleading names, comments and small duplicates
- **Where:** several, listed below
- **What:**
  - `test_yolk.cpp:159` `CollinearVoters_AlwaysYAxis` asserts the lines are y = 0 (the x-axis), and only by checking that one point (0.5, 0) lies on each line (also `:181`).
  - `test_yolk.cpp:44` helper comment says "Signed distance"; `distance_to_line` is unsigned. The signed variant and its sign convention are not tested directly.
  - `test_yolk.cpp:331–334` says all other quantile lines pass through the centroid G; they pass through a voter (distance 0 to G only for the line through G).
  - `test_geometry_integration.cpp:24–31` says "Two voter configurations" and describes three; `CondorcetWinner_HeartEqualsUncoveredEqualsCore` never calls `core_2d`; `std::array` is used (line 404) without `#include <array>`; `WinsetOps_*_EqualsSelf` assert non-emptiness only.
  - `Integration_UncoveredSet.UncoveredSet_SubsetOfAlternatives` and `Integration_Theorems.UncoveredSet_SubsetOfAlternatives_Cycle` duplicate each other and are true by construction (the function filters its input).
  - `YolkRadius.MatchesYolk2d` compares `yolk_radius` with `yolk_2d(...).radius`; the former is a one-line wrapper.
- **Failure scenario:** Readers rely on the names and comments; the wrong comments hide what is actually checked.
- **Suggested fix:** Rename and fix comments as part of the S04c-001 to S04c-006 rewrites.
- **Also affects:** none.

### S04c-015 [P3, confirmed] Tests repeat the wrong Yolk citation and treat "Yolk centre ∈ uncovered set" as a guarantee
- **Where:** `tests/unit/test_yolk.cpp:18`; `tests/unit/test_uncovered_set.cpp:40`, `:311–318`. Known from S02b-017; additional instances.
- **What:** The tests cite "Feld, Grofman & Miller (1988), AJPS 32(2)". The paper in `references/` is Public Choice 59: 37–50, and does not prove that the Yolk centre is in the uncovered set. `BBoxContainsYolkCenter` is the only place a test depends on the claim (Q23 is unanswered; see Open questions).
- **Failure scenario:** n/a (documentation).
- **Suggested fix:** Fix the citation when S02b-017 is fixed; keep or drop the test after Q23.
- **Also affects:** none.

### S04c-016 [P3, plausible] Condorcet and core expectations rely on sampled-polygon emptiness (Decision: Q5)
- **Where:** `tests/unit/test_geometry_integration.cpp:229–242`, `:447–467`
- **What:** `MedianVoter_HasEmptyWinset`, `Core2D_FindsMedianVoterCore` and `Core2D_Cycle_NoCore` use `winset_is_empty` on 64-sample polygons as the oracle. They pass because the symmetric collinear case and the clear cycle are far from the sampling error; they do not test the near-Plott cases Q5 requires (δ = 0.03, 0.01, 0.003 rad must be non-empty). The 4b tests (`test_core`, `test_winset`) own those cases.
- **Failure scenario:** After the exact-emptiness change (Q5) these tests stay valid, but they do not contribute to proving it; they should be switched to the exact predicate.
- **Suggested fix:** Replace `winset_is_empty` by the exact `is_majority_winner_2d` once added; keep the cycle case.
- **Also affects:** `test_core.cpp`, `test_winset.cpp` (4b).

## Findings map

Earlier findings are from `stage_02b_geometry.md`. SCP-001 to SCP-004 are not in this slice's files (winset and level-set semi-axes, a C API length check, random voter draws): no test here touches them.

| Earlier finding | Severity | Status | Test name or note |
|---|---|---|---|
| S02b-001 equal non-unit weights, winset | P0 | no test in this slice | Integration tests use unit weights only (`kEuc`). 4b owns it. |
| S02b-002 Yolk never leaves the centroid | P0 | cannot catch | All `Yolk2d.*`, `Integration_Yolk.*`, `Core2D_FindsMedianVoterCore` pass with a centroid stub (S04c-001). |
| S02b-003 zero weight aborts in `winset_2d` | P0 | fixed and tested (PR #1) for uncovered boundary | `UncoveredSetBoundary.ZeroSalienceWeight_Throws` (checks message). Winset/core variants: 4b. |
| S02b-004 Voronoi drops cells | P1 | outside this slice | 4b (`test_voronoi_2d`). |
| S02b-005 Yolk family, one band edge | P1 | no test | S04c-002; `Unanimity_Shifts…` cannot catch (S04c-003). |
| S02b-006 weighted majority: half wins both ways | P1 | cannot catch in this slice | `WeightedMajority_UniformWeights_MatchesMajority` uses n = 5 (S04c-010). 4b owns the primary test. |
| S02b-007 Voronoi bisector in double | P2 | outside this slice | 4b. |
| S02b-008 uncovered boundary: not an outer bound, degenerate, no limit | P2 | no test | S04c-008; the only tests assert ≥ 1 vertex. |
| S02b-009 `yolk_2d` cost; `n_sample` unvalidated | P2 | no test | S04c-009; `n_sample` is never passed. |
| S02b-010 winset 1e-12 cutoff, drops polygons | P2 | outside this slice | 4b. |
| S02b-011 duplicate voters, degenerate line | P2 | no test | S04c-009. |
| S02b-012 hull ⊇ Pareto set for L1 | P2 | outside this slice | `Integration_ConvexHull.*` check only containment of centroid, kA, and kSQ outside. Docs-only finding. |
| S02b-013 Copeland winner in uncovered set | P3 | not tested | Integration tests use a configuration with a Condorcet winner. Not affected. |
| S02b-014 `core_2d` unanimity comment | P3 | not tested | No integration test with k = n. |
| S02b-015 grid default 30 vs 15 | P3 | no test | No C++ test computes the default (S04c-008). |
| S02b-016 Yolk described differently everywhere | P3 | n/a | Test header (`test_yolk.cpp:4–21`) describes C2 as a Yolk solver without caveat. |
| S02b-017 citations | P3 | repeated in tests | S04c-015. |
| S02b-019 Heart leftovers | P3 | to be removed | S04c-012. |
| Q18 (weights, PR #1) | n/a | tested for the uncovered boundary | `UncoveredSetBoundary.ZeroSalienceWeight_Throws`; the discrete `uncovered_set` still accepts weight 0 (`EXPECT_NO_THROW` with no value check). |

## Tool leads checked
- `coverage_report.txt` shows `yolk.h` at 43.46% lines (134 of 237 missed) → not a defect: llvm-cov warns "2155 functions have mismatched data" for the combined multi-binary report (`$SCR/stage4/coverage_report.err`), and the `yolk_2d` body shows 0 hits although 22 calls happen. Re-checked per the coordinator's warning: I ran `test_yolk`, `test_uncovered_set`, `test_centrality` and `test_geometry_integration` from `build_cov` with a fresh `LLVM_PROFILE_FILE` into `$SCR4C/prof/` (all pass: 32, 16, 24, 32 tests), merged to `$SCR4C/fresh.profdata`, and ran `llvm-cov` on each slice binary only: `yolk.h` 97.45% lines (5 of 196 missed), 94.64% branches (3 of 56 missed). Uncovered: `SampledLine::dist` (yolk.h:352–353, never called) and the all-collocated early return (:408–409, matches the untested branch in S04c-009). Nothing was rebuilt.
- `uncovered_set.h` and `centrality.h` coverage 100% → coverage is complete; the gaps are in what the tests assert (S04c-006, S04c-007, S04c-008).
- Stage 2b lead "winset.h 57% / geom2d.h 33%, note for Stage 4" → belongs to 4b, not examined here.
- Stage 2b lead "Yolk tests cover only configurations whose centroid is the Yolk centre" → confirmed and demonstrated (S04c-001).
- Stage 2b lead "winset tests have no non-unit or non-Euclidean weights" → the integration file adds none (`kEuc` only); see S04c-007 for the uncovered set.
- `[[nodiscard]]` `-Wunused-result` warnings in `test_centrality.cpp` (Stage 0) → not reported (tools already catch it; Stage 0 owns it).
- Determinism, timing and CI labels → all five files are deterministic and registered (`CMakeLists.txt:160–199`); none carries the `benchmark` label (only the performance tests do, `CMakeLists.txt:124–127`); runtimes in `ctest_release.txt`: YolkTest 0.12 s, UncoveredSetTest 0.46 s, HeartTest 0.06 s, CentralityTest 0.06 s, GeometryIntegrationTest 0.15 s.
- Disabled or skipped tests → none (`DISABLED`, `GTEST_SKIP` grep empty).
- Suspected pasted-back expected values → `AnnotatedLines.LaingOlmstedBear_*` (6 limiting lines, listed pairs): I checked them against an independent exact computation, which agrees (`$SCR4C/lob_independent.out`). Not a defect. The equilateral Yolk (√3/6) and the right-triangle lines come from hand calculations; the other value tests are small enough to be hand-derived.
- Thread safety (Q9) → no concurrent-use tests in these files; the ThreadSanitizer stress test is already in Q9's Implies list, so not re-reported.

## Open questions

1. **Even-n and supermajority Yolk (Q22, unanswered).** Until the owner chooses (touch both band edges, or one), value tests for even n and k > simple majority cannot be written. Invariance tests (reflection, translation, permutation) can be written now and are independent of Q22 (S04c-002).
2. **Yolk centre in the uncovered set (Q23, unanswered).** `UncoveredSetBoundary.EquilateralTriangle_BBoxContainsYolkCenter` is the only test that relies on it, in the weak bounding-box form. Keep or delete?
3. **Uncovered set boundary of 1 or 2 vertices (Q25, unanswered).** `ResolutionParameter_NonEmpty` and `AllVotersCollocated_BoundaryNearVoter` accept a 1-vertex result. After the owner decides, add a test that pins the chosen behaviour (S04c-008).
4. **Yolk in the bindings (Q14, unanswered).** If the Yolk stays only in the C API and is later replaced, S04c-001 and S04c-002 still apply to the replacement; if it is withdrawn like the Heart, these tests go with it.

## Out-of-slice notes
- `include/socialchoicelab/geometry/centrality.h:130–140` and `:75–86`: `centroid_2d` sums coordinates before dividing, so finite inputs near `DBL_MAX` return Inf; `marginal_median_2d` with even n computes `(a + b) / 2.0`, which overflows the same way (`$SCR4C/u1_misc.out`). `geometric_mean_2d` accepts NaN and Inf (the positivity check compares `min <= 0`, which NaN passes). Not in stage 02b's report; extreme inputs only, P3 for the code.
- `include/socialchoicelab/geometry/yolk.h:349–354`: `SampledLine::dist` is never called (dead code; the solver uses inline `std::abs`). Coverage lines 352–353 (fresh per-binary run).
- `include/socialchoicelab/geometry/yolk.h`: the k out-of-range error from `yolk_2d` is prefixed "majority:" (it comes from `detail::resolve_k`), not "yolk_2d:". Minor for the end-user error rule.
- `test_c_api.cpp` (4e/4f): the Yolk and uncovered-set C API tests should be checked for the same symmetric-only configurations and the same non-default-metric gap.
