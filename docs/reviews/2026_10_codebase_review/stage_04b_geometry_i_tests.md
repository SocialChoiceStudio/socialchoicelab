# Stage 04b — Geometry I tests
Model / effort: Sonnet 5.5 / high   Commit: 2771655   Files reviewed: 9 test files, 1,935 lines (all read in full)
Not read (if any): none. Code under test was read only to settle specific questions (`pts_polygon`, `level_set_at_distance_2d`, `winset_contains`, `winset_ops.h`, `winset_1d.h`, `voronoi_2d.h`, `core.h`).

Files: `tests/unit/test_geom2d`, `test_convex_hull`, `test_majority`, `test_winset`, `test_winset_ops`, `test_winset_1d`, `test_voronoi_2d`, `test_core`, `test_copeland` (`.cpp`). All nine are registered with `add_test` (Geom2dTest, ConvexHullTest, MajorityTest, WinsetTest, WinsetOpsTest, CoreTest, CopelandTest, Voronoi2dTest, WinSet1dTest) and all pass in `ctest_release.txt` (each under 0.15 s). None carries the `benchmark` label, none uses time or an RNG, and there are no `DISABLED_` or skipped tests, so the slice is deterministic and CI-stable.

Scratch material is in
`SCR4B=/private/tmp/claude-501/-Users-robiragan-Dropbox-SocialChoiceStudioDev/2402f97e-221e-494b-88cb-fc63c41efc50/scratchpad/stage4b/`:
`e1_voronoi.cpp`, `e2_misc.cpp`, `e3_gaps.cpp` (programs against the repo headers), `mut/` (header copies with injected bugs, each compiled against the real test file), `build.sh`, `prof/` and `slice.profdata` (per-binary coverage, see "Tool leads checked").

## Summary

- **4 P1, 12 P2, 3 P3.** No test in the slice is non-deterministic or timing-dependent, and none contains Heart code (nothing to list for Q1).
- **One test asserts a P1 bug.** `WeightedMajority.ThresholdAtExactly50Percent` (`test_copeland.cpp:171`) expects `weighted_majority_prefers(x,y)` and `(y,x)` to both be true. That is S02b-006 / Q6 written down as a requirement. The fix must change this test (S04b-001).
- **Three P1 bugs have no test that could catch them.** (a) Voronoi cells dropped when a bisector hits a vertex, S02b-004: the only Voronoi tests use bisectors that miss every vertex (S04b-003). (b) The weighted winset near the quota, Q6: the only tests use integer weights where the error is invisible (S04b-002). (c) Winset emptiness decided from sampled polygons, Q5: `core_2d` returns a "Condorcet winner" for the near-Plott cases δ = 0.03, 0.01 and 0.003, and no test has such a case (S04b-004).
- **The PR #1 fixes are properly tested.** Re-introducing the S02b-001 bug (equal non-unit weights give a circle of radius r, not r/w) into a copy of `level_set.h` makes `WinsetWeights.EqualNonUnitWeights_OneVoter_MatchesMajorityPrefers` and `WinsetWeights.AllMetrics_AgreeWithMajorityPrefersOnGrid` fail. S02b-003 (zero or non-finite weight) is covered for `winset_2d`, `winset_2d_petal`, `winset_with_veto_2d`, `weighted_winset_2d` and `core_2d`.
- **Many assertions are qualitative.** Mutants that make `winset_union` ignore its second argument, or return the intersection, survive all 19 `test_winset_ops` tests. A mutant that corrupts the Minkowski p ≥ 100 square branch survives all 32 `test_winset` tests (Q2 will need a test there). Convex-hull and Voronoi tests check counts and loose bounds, not vertices or the area partition.
- **The shared coverage report is wrong for two headers in this slice.** It shows `geom2d.h` 33% and `winset.h` 18.9% lines; running the slice's own binaries gives 100% and 92%. See "Tool leads checked".

## Contracts this layer provides

What the suite protects today, and what it does not, for later stages and for Stage 11 planning.

- **Protected with real value checks:** `simple_majority`; `majority_prefers` / `preference_margin` for Euclidean (strictness, ties count for neither side, k = 1..n); pairwise-matrix antisymmetry and a three-alternative cycle matrix (although the assertion on it is tautological, S04b-010); Copeland scores on hand-checked configurations (A = 2 in the dominance case; all zero in the cycle); finite Condorcet winner and cycle; `winset_interval_1d` endpoints for four right-of-median cases; hull vertex count and the CCW sign of the area.
- **Protected for salience weights (Q18, PR #1):** shape size under equal and unequal weights for Euclidean, Manhattan, Chebyshev and Minkowski p = 3 (against `majority_prefers` on a grid); rejection of zero, negative and non-finite weights with a stable message in the four winset builders and `core_2d` (zero only).
- **Not protected:** the shape of any winset beyond a handful of membership probes (no area, no vertex values); the result of `winset_union`, `winset_difference` and a non-trivial `winset_intersection`; exact (not polygon-level) emptiness (Q5); weighted winsets with non-integer weights or an exact-half split (Q6); Voronoi partition correctness; Minkowski p ≥ 100 winsets (Q2); extreme magnitudes and scale invariance (S02b-010, SCP-001); non-finite input to `to_exact` (the contract is an abort, which cannot be asserted without a death test); `core_2d` with any k other than -1.
- **Tests that encode current behaviour the owner may change:** the exact-half weighted majority (S04b-001); `pairwise_matrix` accepting and ignoring k (S04b-017); first-index Copeland tie-break (consistent with the docs and with Q3's smallest-index default; not a defect).

## Findings

### S04b-001 [P1, confirmed] A test asserts that exactly half the weight wins in both directions
- **Where:** tests/unit/test_copeland.cpp:171–184 (`WeightedMajority.ThresholdAtExactly50Percent`). Decision: Q6. Earlier finding: S02b-006.
- **What:** The test builds two voters with weights {1,1}, x preferred by one and y by the other, and asserts `weighted_majority_prefers(x, y, ...) == true` and `weighted_majority_prefers(y, x, ...) == true`, with the comment "Tie is resolved in favour of x at exactly 50%". Q6 says the default must mean more than half of the total weight, and an exact half must not win. The relation asserted here is not asymmetric. `majority_prefers(x, y)` is false for the same voters (`SCR4B/e2_misc.cpp`, E3b: wmp(a,b)=1, wmp(b,a)=1, majority_prefers(a,b)=0).
- **Failure scenario:** The owner-approved fix (more than half; threshold applies as "at least" only when explicit) makes this test fail, and anyone "fixing" the failure by loosening it re-bakes the bug. Until then the test guarantees that S02b-006 cannot be caught by the suite.
- **Suggested fix:** Replace it with: default threshold, weights {1,1}, exact half → both directions false; weights {1,1,2.004}, the two weight-1 voters → false; and an explicit `threshold = 0.5` case asserting "at least" semantics once the sentinel exists. Add the scaling case from S02b-006 ({0.3,0.1,0.2} vs {3,1,2} must agree).
- **Also affects:** `test_c_api.cpp` `scs_weighted_majority_prefers_2d` tests (slice 4e) should be checked for the same expectation.

### S04b-002 [P1, confirmed] No test can catch the weighted-winset errors that Q6 decided against
- **Where:** tests/unit/test_winset_ops.cpp:209–278 (the F5 block). Decision: Q6.
- **What:** All `WeightedWinset` tests use integer weights ({1,1,1}, {2,1,1}) and either threshold 0.5 with an odd voter count, or threshold 1.0. In that corner the copying approximation and the "half the weight wins" rule happen to agree with the exact answer. `UniformWeights_MatchesStandardWinset` (comment: "threshold = 0.5 → same result as winset_2d with k = 2/3") only holds for odd n. `DoubleWeightVoterChangesOutcome` has a conditional assertion (`if (!winset_is_empty(ws_equal))`) and a symmetric-difference check that any weighting change satisfies. None of them contains a near-quota case, an even split, a non-integer weight or a timing bound.
- **Failure scenario:** `SCR4B/e2_misc.cpp` (E3): 3 voters (0,0), (2,0), (1,2), sq (1,0.5), x = (1.0,0.2) preferred by voters 0 and 1 only. With weights {1,1,2.004} (two weight-1 voters hold 2.0 of 4.004, a losing coalition) `winset_contains(weighted_winset_2d(...), x)` is **true**; with {1,1,2} (exactly half) also true. All existing `test_winset_ops` tests pass at this commit.
- **Suggested fix:** Add the tests listed under Q6 "Implies": the 1/1/2.004 near-quota case (the point must be outside), an equal-weight even split (must match `winset_2d`), decimal vote-share weights for 6–7 parties finishing in under 1 s, and the size-limit error. Mark any timing test so it is excluded from `ctest -LE benchmark` if it can be slow.
- **Also affects:** `scs_weighted_winset_2d` tests (4e); Python `weighted_winset_2d` tests (Stage 6).

### S04b-003 [P1, confirmed] No Voronoi test could catch S02b-004 (cells dropped when a bisector hits a vertex)
- **Where:** tests/unit/test_voronoi_2d.cpp:15–55. Earlier finding: S02b-004 (P1).
- **What:** `TwoSitesBbox` has a bisector (x = 1) that meets the box edges away from any vertex. `ThreeSites` uses an equilateral triangle in an irregular box, so no bisector passes through an existing vertex. Both use per-vertex bounds only (`GE 3`, tolerance 0.01) and never check that the cells partition the box. Nothing uses integer or symmetric configurations, which is exactly where the bug fires.
- **Failure scenario:** `SCR4B/e1_voronoi.cpp`: sites (0,0), (2,0), (0,2) in [-1,3]²: cells have 4, 0 and 0 vertices; area sum 4 of 16. Sites (0,0), (1,1) in [-1,2]²: both cells empty (sum 0 of 9). The suite's own `ThreeSites` configuration sums to 1.82 = box area, so it passes. A single `EXPECT_GE(cell.size(), 3u)` on the first configuration would have failed today.
- **Suggested fix:** Add a helper that asserts the cell areas sum to the box area (within 1e-12 relative) and each cell contains its site, and run it on: integer symmetric sites with a bisector through a box corner, a bisector through a shared Voronoi vertex, and 3–6 random-looking sites. These are the Q7 / S02b-004 fix tests.
- **Also affects:** `test_c_api.cpp` Voronoi tests (4e) likely have the same blind spot.

### S04b-004 [P1, confirmed] No test has a core that sampled polygons would miss (Q5 near-Plott cases)
- **Where:** tests/unit/test_core.cpp:116–147, tests/unit/test_winset.cpp (WinsetEmpty group). Decision: Q5.
- **What:** Every "empty winset" assertion in the slice is either degenerate (all voters at sq) or a symmetric configuration where polygons are empty for the right reason. `Core2D.EquilateralTriangle_NoCondorcetWinner` is correct but would also pass if emptiness were decided by polygons. Q5 says tests must include near-Plott δ = 0.03, 0.01, 0.003 rad (non-empty) and exact symmetry (empty). None exists.
- **Failure scenario:** `SCR4B/e2_misc.cpp` (E4): five voters (0,0), (1,0) rotated by δ about the origin, (-1,0), (0,1), (0,-1), simple majority, 64 samples. The true winset at the origin is non-empty for every δ > 0, but `winset_is_empty(winset_2d(origin,...))` returns true for δ = 0.03, 0.01 and 0.003, and `core_2d` returns a Condorcet winner each time ("found (wrong)"). With δ = 0 (exact Plott) `core_2d` correctly finds (0,0).
- **Suggested fix:** Add the three δ cases (core must be absent, exact test says non-empty) plus the exact-symmetry case (core present), and the `is_majority_winner_2d` tests that Q5 asks for.
- **Also affects:** `scs_core_2d` / `scs_winset_is_empty` tests (4e/4f); R and Python core tests.

### S04b-005 [P2, confirmed] Winset Boolean-operation tests assert only emptiness; `winset_union` mutants survive
- **Where:** tests/unit/test_winset_ops.cpp:51–140
- **What:** The union tests end after `EXPECT_FALSE(winset_is_empty(u))`; the in-test comment says "We only assert non-empty here". No test checks a point inside `a` but outside `b`, a difference of two different regions, or an intersection of two overlapping non-empty regions.
- **Failure scenario:** Mutation testing (`SCR4B/mut/`, `winset_ops.h` copies compiled against the real `test_winset_ops.cpp`): `winset_union` returning `a` unchanged → 19/19 tests pass; `winset_union` returning `a ∩ b` → 19/19 pass. Other mutants are caught: `winset_difference` returning `a` fails 1 test, `winset_intersection` returning `a` fails 3 (two through the veto tests), `winset_symmetric_difference` as union fails 5.
- **Suggested fix:** Use `winset_contains` on points in only `a`, only `b`, both, neither, for each of the four operations; compare analytic areas for two overlapping discs (lens / union area) within the inscribed-polygon error.
- **Also affects:** none.

### S04b-006 [P2, plausible] Winset tests are qualitative: no area or vertex values, conditional assertions, untested branches
- **Where:** tests/unit/test_winset.cpp:225–249, :151–170 and the whole file; uncovered lines `winset.h:161–167` (hole path in `winset_contains`), `:207–209`, `:277–280` (`num_samples < 4`).
- **What:** Winset shape is only checked through a few membership probes. `WinsetSupermajority.LargerK_SmallerOrEqualWinset` asserts inclusion only `if` the probe point is in the larger-k winset; run today the probe (0.5,0.5) is in ws3 and ws4 but not ws5, so the "ws5 ⊆ ws4" branch never executes (`SCR4B/e2_misc.cpp`, E7). There is no area test (a two-voter lens has a closed-form area; the 64-gon bound is known), no point-inside-a-hole test, and no `num_samples < 4` test for `winset_2d` or `winset_2d_petal`. The small tests of unequal/odd k (k = 0, negative k other than -1) are missing too. The cross-check tests compare depth and petal only by boolean results at 6 points.
- **Failure scenario:** A winset that is a rotated or shifted copy of the right shape, or a polygon with the wrong hole, would pass every test in the file.
- **Suggested fix:** Add area checks against analytic values with the sample-count error bound, a region with a hole (point in the hole must be outside), make the monotonicity test unconditional over a grid of points, and test `num_samples < 4`.
- **Also affects:** none.

### S04b-007 [P2, confirmed] Minkowski p ≥ 100 winset branch has no test (Q2)
- **Where:** tests/unit/test_winset.cpp:421–461 (the only Minkowski case is p = 3); code `level_set.h:262–270` used by `pts_polygon`. Decision: Q2.
- **What:** Q2 removes the p ≥ 100 → square substitution. No winset test runs at p ≥ 100, so nothing pins either today's behaviour or the exact-Minkowski behaviour the owner wants.
- **Failure scenario:** `SCR4B/mut/include_p100`: corrupt one vertex of the p ≥ 100 square (×7) → `test_winset` still passes 32/32. After the Q2 fix the same lack of coverage hides any regression at p = 50, 99, 100, 1000.
- **Suggested fix:** Add a winset grid test like `AllMetrics_AgreeWithMajorityPrefersOnGrid` at p = 50, 99, 100 and 1000 (the near-boundary filter must scale with p) and one with very large and very small coordinates.
- **Also affects:** `test_level_set.cpp` (slice 4a) for the shape itself.

### S04b-008 [P2, confirmed] No test for SCP-001 or S02b-010 (extreme magnitudes in `winset_2d`)
- **Where:** tests/unit/test_winset.cpp:507–513 (`SemiAxisOverflowThrows` covers only the semi-axis check). Earlier findings: SCP-001, S02b-010.
- **What:** `SemiAxisOverflowThrows` uses weight 1e-310, which trips the semi-axis check but not the vertex overflow of SCP-001. There is no scale-invariance test.
- **Failure scenario:** `SCR4B/e3_gaps.cpp`: (1) SCP-001 inputs (weights 1e-155, voter 1e308, sq 0): with CGAL assertions on, `winset_2d` throws a CGAL assertion exception (not `std::invalid_argument`); built with `-DNDEBUG` it returns an empty winset silently, which is wrong. (2) The same five-voter configuration scaled by 1, 1e-6, 1e-12 is non-empty, but at 1e-13 `winset_is_empty` is true (S02b-010). A test for either would fail today.
- **Suggested fix:** Add (a) the SCP-001 input expecting `std::invalid_argument` with an end-user message; (b) a scale loop (1e-13 … 1e6) asserting identical emptiness and normalised area.
- **Also affects:** the veto, weighted and core builders; C API winset tests.

### S04b-009 [P2, confirmed] Voronoi tests use loose bounds, no vertex values, and miss the Q7 and S02b-007 inputs
- **Where:** tests/unit/test_voronoi_2d.cpp:15–55; uncovered branches `voronoi_2d.h:106`, `:112`, `:188`.
- **What:** Cell bounds use ±0.01 tolerances, so a bisector shifted by 0.009 passes (`TwoSitesBbox` accepts cell-0 x ≤ 1.01; true value 1). No test has n_sites = 1 (whole box returned, `SCR4B/e1_voronoi.cpp`: 4 vertices, area 1), co-located sites (Q7: both cells currently come back as the full box, area 16 each), nearly coincident sites (S02b-007), or an empty or non-simple cell (the `cell.size() < 3` and `is_simple()` branches have zero hits). Q7 says co-located sites must return one shared cell; no test pins that.
- **Failure scenario:** After the Q7 and S02b-007 work, nothing in this file would notice a regression to overlapping cells.
- **Suggested fix:** Assert exact vertex sets for the two-site case, add n = 1, co-located, nearly co-located (1 ulp apart), and collinear-site tests, plus an empty-cell test.
- **Also affects:** `test_c_api.cpp` Voronoi tests (4e).

### S04b-010 [P2, confirmed] `test_majority.cpp`: a tautological assertion on the cycle test, and thin edge cases
- **Where:** tests/unit/test_majority.cpp:236–275, :133–142, :151–159
- **What:**
  - `PairwiseMatrix.CondorcetCycle_ThreeAlternatives` asserts antisymmetry, zero diagonal, and `wins(0)+wins(1)+wins(2) == 3`. The last holds for every tie-free 3-alternative tournament, including one with a Condorcet winner (2+1+0). The comment says "verified by hand", yet the matrix is never asserted. Run today the matrix is [[0,1,-1],[-1,0,1],[1,-1,0]] and is a true cycle (`e2_misc.cpp`, E8), so the test is right by luck, not by assertion.
  - Non-Euclidean coverage is one Manhattan tie case. Chebyshev, Minkowski and non-unit weights are untested in `majority_prefers`.
  - Not tested: NaN or Inf coordinates (currently silently "false"), k = -1 given explicitly versus k = -7 (S02b-020), single voter with equal distance, extreme magnitudes, a weight-vector of the wrong length.
- **Failure scenario:** Changing a pairwise comparison for Chebyshev or weighted distance passes the suite.
- **Suggested fix:** Assert the three margins of the cycle matrix; add Chebyshev, Minkowski and weighted cases with hand-computed margins; add non-finite and negative-k cases (the decision on -1 only vs any negative goes with S02b-020).
- **Also affects:** none.

### S04b-011 [P2, confirmed] `winset_1d` tests never put the seat left of the median; one assertion is vacuous
- **Where:** tests/unit/test_winset_1d.cpp:21–75; uncovered `winset_1d.h:88–93`.
- **What:** All four non-empty cases have the seat to the right of the median, where the interval always ends at the largest critical value. The branch that closes the interval early (`in_ws && !majority`, lines 88–92) has zero hits. `LengthIsPositive` only asserts when the result is non-empty, so it passes if the function returns empty. Not tested: seat left of the median, n = 2, duplicate voters, all voters at the seat, negative n_voters.
- **Failure scenario:** Today the function is right in those cases (`e2_misc.cpp`, E6: seat -2 for voters {-1,0,1} gives (-2,2); n = 2 with the seat midway gives empty; all at seat gives empty). A mirror-image bug would be missed.
- **Suggested fix:** Add the mirror cases, n = 2, duplicates and all-at-seat; make `LengthIsPositive` assert non-emptiness.
- **Also affects:** none.

### S04b-012 [P2, confirmed] `core.h`: `core_2d` is tested only with k = -1 and loose tolerances
- **Where:** tests/unit/test_core.cpp:116–170. Earlier findings: S02b-002 (core_2d aspect), S02b-014, S02b-020.
- **What:**
  - Every `core_2d` call uses simple majority; unanimity and supermajority (S02b-014: under k = n the core is the Pareto set) are untested.
  - The collinear and same-ideal cases use tolerances 0.1 and 0.2 around a point they find exactly. The collinear case passes for any code that returns the median voter, whatever the Yolk centre, so the S02b-002 effect on `core_2d` is invisible (and, because `core_2d` only returns a candidate whose winset is empty, a wrong Yolk centre cannot give a wrong answer here; see the findings map).
  - Finite-alternative tests do not cover duplicate alternatives (the documented "winner appears twice → nullopt" contract), non-Euclidean metrics, or k ≠ -1 for `condorcet_winner` (one `has_condorcet_winner` case with k = 3 only).
  - An off-centroid median (voters (0,0), (1,0), (10,0); centroid 3.67) returns (1,0) today (`e2_misc.cpp`, E4), so a test for it would pass; the gap is protective.
- **Suggested fix:** Assert exact coordinates (the median voter) with 1e-12; add the unanimity and supermajority cases, duplicate alternatives and Manhattan.
- **Also affects:** none.

### S04b-013 [P2, plausible] Convex-hull tests check counts, not vertices; the "Pareto" test checks nothing about Pareto
- **Where:** tests/unit/test_convex_hull.cpp:34–121. Earlier finding: S02b-012.
- **What:** `Triangle`, `Square`, `InteriorPointExcluded` and `LargerPointCloud` assert `hull.size()` only. CCW is checked on one square by area sign. The test under the heading "Pareto set property (Euclidean)" only asserts that input points are inside their own hull, a property of any hull. Not tested: duplicate points, all-identical points (the contract returns 1 vertex), collinear points with an interior duplicate, near-collinear points, huge and tiny magnitudes, and the Manhattan counter-example from S02b-012.
- **Failure scenario:** A hull returning the right number of wrong vertices passes.
- **Suggested fix:** Compare the vertex sets exactly (rotation-invariant) and add the missing edge cases; rename or replace the "Pareto" test once S02b-012's wording is settled.
- **Also affects:** none.

### S04b-014 [P2, confirmed] Copeland tests use inequality assertions and have no tie, metric or k cases
- **Where:** tests/unit/test_copeland.cpp:60–94, :100–116. Earlier finding: S02b-013. Q27 is unanswered.
- **What:** `CondorcetWinner_HasMaxScore` asserts `scores[1] <= 0` and `scores[2] <= -1`; the exact values are -1 and -1 (`e2_misc.cpp`), so a wrong B score of -5 passes. `Supermajority_ChangesScores` checks only `scores[0] == 2`, the same as simple majority, so it does not show a change. No test has a pair decided by a tie (equidistant voters), Manhattan or Chebyshev, or an even voter count. The S02b-013 counterexample (Copeland winner covered by another alternative under ties) has no test either; whether that is a defect depends on Q27.
- **Suggested fix:** Assert exact score vectors; add a tie pair (contributes 0 to both), a k = n case that differs from simple majority, and a non-Euclidean case.
- **Also affects:** none.

### S04b-015 [P2, plausible] `test_geom2d.cpp` does not test the header's stated purpose (exactness near collinearity)
- **Where:** tests/unit/test_geom2d.cpp:55–90, :17–50
- **What:** `orientation`'s documentation says the result is never wrong for nearly collinear input. Every orientation test uses exactly collinear or clearly turning integer points. `to_exact` tests use exactly representable values (3.5, 7.25, ...), so they do not show that 0.1 is lifted to its exact double value rather than rounded. The contract that a non-finite value aborts the process (S02b contract) has no test, because it needs a death test.
- **Failure scenario:** None today (`e3_gaps.cpp` mode 3: points (0.1,0.1), (0.3,0.3), (0.2, nextafter(0.2)) give left-turn, correct), so this is a protective gap.
- **Suggested fix:** Add a near-collinear case (above), a left/right pair that double arithmetic gets wrong, and one `to_exact(0.1)` test comparing against the exact double value.
- **Also affects:** none.

### S04b-016 [P2, plausible] Q18 tests are partial
- **Where:** tests/unit/test_winset.cpp:463–497, tests/unit/test_core.cpp:158–170. Decision: Q18.
- **What:** `core_2d` is tested with weight 0 only (not negative, NaN or Inf), although Q18 lists "everything built on them" as rejecting all three. No test passes a weight vector of the wrong length (not 2) to a winset builder, which `to_dist_config` in the C API requires. Distances keep weight 0 meaning "ignore" (tested well in `ZeroWeightStillAllowedForMajorityPrefers`), but `condorcet_winner` and `copeland_scores` with weight 0 are not tested.
- **Suggested fix:** Loop `core_2d` over the same bad-weight table as the winset test; add length 1 and length 3 cases; add a weight-0 case for Copeland and `condorcet_winner`.
- **Also affects:** none.

### S04b-017 [P3, confirmed] Tests lock in `pairwise_matrix` accepting and ignoring k
- **Where:** tests/unit/test_majority.cpp:287–305. Earlier finding: S02b-020.
- **What:** `SupermajorityK_ValidationOnly` and `SupermajorityK_Valid_MatrixUnchanged` assert that k is validated and then has no effect. S02b-020 suggests dropping k from `pairwise_matrix`. If that is done, the tests must go too.
- **Suggested fix:** Decide with S02b-020; remove or rewrite both tests with it.

### S04b-018 [P3, confirmed] Misleading names, comments and duplicated fixtures
- **Where:**
  - test_core.cpp:96 `Supermajority_EliminatesWinner`: the comment concludes A is still the Condorcet winner under unanimity; nothing is eliminated.
  - test_copeland.cpp:84 `Supermajority_ChangesScores`: asserts one value that is the same under simple majority.
  - test_convex_hull.cpp:107 heading "Pareto set property": see S04b-013.
  - test_winset.cpp:35–36: says the equilateral centroid is (1/3, √3/6) and "the Plott equilibrium (unique Condorcet winner)"; the helper returns (0.5, √3/6), and lines 84–86 say the opposite.
  - test_winset_ops.cpp:51–66: the Union test has a six-line comment instead of an assertion.
  - The three-alternative cycle (A=(2,0), B=(0,2), C=(-2,0), voters (1,0.5), (-0.5,1.5), (-0.5,-1)) is copied in test_core.cpp, test_copeland.cpp and test_aggregation_integration.cpp:83–86; the dominance fixture (alternatives (0,0), (5,5), (5,-5)) is repeated in test_core.cpp and test_copeland.cpp.
- **Suggested fix:** Rename or fix; share the fixtures in one header.

### S04b-019 [P3, confirmed] `weighted_majority_prefers` tests live in `test_copeland.cpp`
- **Where:** tests/unit/test_copeland.cpp:118–199 (header comment line 5 admits it); function is in `majority.h`.
- **What:** Majority-with-weights tests sit in the Copeland file, so someone fixing S02b-006 or the Q6 threshold semantics would not look there, and `test_majority.cpp` has no `weighted_majority_prefers` tests at all. The slice has no C++ test of `has_condorcet_winner` with empty voters either.
- **Suggested fix:** Move the `WeightedMajority` group to `test_majority.cpp` when rewriting it for S04b-001.

## Findings map

For P0/P1 findings in `stage_02b_geometry.md`, SCP items in the geometry area, and the answered owner questions that touch this slice. Items in other slices are marked "n/a (slice)".

| Earlier finding | Severity | Status | Test name or note |
|---|---|---|---|
| S02b-001 (equal non-unit weights, winset) | P0 | fixed and tested | `WinsetWeights.EqualNonUnitWeights_OneVoter_MatchesMajorityPrefers`, `WinsetWeights.AllMetrics_AgreeWithMajorityPrefersOnGrid`. Mutation of `level_set.h` back to the old radius fails both (`SCR4B/mut/include_s01`). Level-set shape itself is 4a. |
| S02b-002 (Yolk never moves) | P0 | n/a (slice 4c); core_2d aspect: cannot catch | `Core2D.CollinearVoters_HaveCondorcetWinner` passes for any code that returns the median voter. A wrong Yolk centre cannot make `core_2d` return a wrong point, because the candidate must have an empty winset. |
| S02b-003 (zero weight aborts) | P0 | fixed and tested | `WinsetWeights.NonPositiveOrNonFiniteWeightThrows` (winset, petal, veto, weighted), `ZeroWeightThrowsEvenWhenVotersAtSQ`, `Core2D.ZeroSalienceWeight_Throws`. Gaps in S04b-016. |
| S02b-004 (Voronoi drops cells) | P1 | no test (cannot catch) | S04b-003; `Voronoi2d.TwoSitesBbox`, `ThreeSites` avoid the failing geometry. |
| S02b-005 (Yolk even n / k-majority) | P1 | n/a (slice 4c) | — |
| S02b-006 (`weighted_majority_prefers` half wins) | P1 | wrong-behaviour test | `WeightedMajority.ThresholdAtExactly50Percent` (S04b-001); weighted winset: no test (S04b-002). |
| S02b-007 (Voronoi bisector in double) | P2 | no test | S04b-009. |
| S02b-008, -009, -011 (uncovered set, Yolk cost, quantile lines) | P2 | n/a (slice 4c) | — |
| S02b-010 (absolute 1e-12 cut-off, scale) | P2 | no test | S04b-008; `e3_gaps` mode 2 shows a test would fail today. |
| S02b-012 (hull is not a Pareto outer bound) | P2 | no test (docs) | S04b-013; the "Pareto" test checks only hull containment. |
| S02b-013 (Copeland winner can be covered) | P3 | no test | S04b-014 (Q27 unanswered). |
| S02b-014 (`core_2d` unanimity comment) | P3 | no test | S04b-012. |
| S02b-015–018 (grid default, Yolk docs, citations, design doc) | P3 | n/a (docs / slice) | — |
| S02b-019 (Heart leftovers) | P3 | n/a | No Heart code or tests in this slice. |
| S02b-020 (k handling hygiene) | P3 | tests encode it | `PairwiseMatrix.SupermajorityK_*` (S04b-017); negative-k cases missing (S04b-010). |
| SCP-001 (finite semi-axes, infinite vertices) | P2 | no test | S04b-008; demonstrated (throw in assert builds, silent empty in `-DNDEBUG`). |
| SCP-002 (equal-weights test too coarse) | P2 | n/a (slice 4a, `level_set.h`) | Not reachable from this slice's tests. |
| Q2 (exact Minkowski, p ≥ 100) | decided | no test in slice | S04b-007; mutant survives. |
| Q5 (exact winset emptiness) | decided | no test | S04b-004. |
| Q6 (exact weighted winset / majority) | decided | wrong-behaviour test + no test | S04b-001, S04b-002. |
| Q7 (co-located Voronoi sites) | decided | no test | S04b-009. |
| Q18 (zero weight is an error in 2D geometry) | decided | tested, partial | `WinsetWeights.*`, `Core2D.ZeroSalienceWeight_Throws`; S04b-016. |

Earlier P0/P1 findings in this slice with **no test that could catch them**: **1** (S02b-004). Plus two decided items whose current wrong behaviour is untested: Q5 (S04b-004) and the Q6 weighted winset (S04b-002). One P1 is asserted by a test (S02b-006). S02b-001 and S02b-003 (P0) are fixed and tested. S02b-002 and S02b-005 belong to slice 4c.

## Tool leads checked

- Fresh merged coverage `geom2d.h` 33% → not a defect; artifact. The merged `llvm-cov report` (one object plus `-object` for the rest) attributes inline-header functions to the first binary only. I re-ran the nine slice binaries from `build_cov` with their own profiles (`SCR4B/prof/`, `slice.profdata`): geom2d.h 18/18 lines (100%), convex_hull.h 100%, majority.h 97.2% (3 lines missed), winset.h 92.2% (17 lines missed of 219), winset_ops.h 100%, winset_1d.h 92.0%, voronoi_2d.h 100% (lines; 76.8% of branches), core.h 100%, copeland.h 100%. Stage 2b's "33% looks like inline-header attribution" guess is confirmed.
- Fresh merged coverage `winset.h` 18.9% lines (Stage 0: 57%) → same artifact, not a defect. The real untested lines are the hole path of `winset_contains` (:161–167), the `num_samples < 4` errors in `winset_2d` and `winset_2d_petal` (:207–209, :277–280) and the empty-voters error in `weighted_winset_2d` (:462–464); these are in S04b-006.
- `majority.h` uncovered `weighted_majority_prefers` empty-voters throw (:258–260) → S04b-019 / S04b-001 (no empty-voter test for the weighted function).
- `winset_1d.h:88–93` uncovered → S04b-011 (seat left of the median never tested).
- `voronoi_2d.h` branch coverage 76.8% (branches at :106, :112, :188 never take the "false" side) → S04b-003, S04b-009.
- Stage 2b lead "winset tests have no non-unit or non-Euclidean weights" → already fixed by PR #1 for equal and unequal weights and four metrics; remaining gaps are p ≥ 100 (S04b-007) and extreme magnitudes (S04b-008).
- Stage 2b lead "Voronoi tests have no bisectors through vertices" → S04b-003, demonstrated.
- Stage 2b lead "Yolk tests cover only symmetric configurations" → slice 4c.
- Determinism and CI label lead → no finding. No test uses an RNG, the clock or shared state; none has a timing assertion; only the DistanceSpeed/Performance tests carry `benchmark` (slice 4a).
- Test registration lead → no finding; all nine files have `add_test` entries (CMakeLists.txt:140–209) and none is skipped.

## Open questions

1. **Does Q6's "more than half" rule apply to `weighted_majority_prefers`, not only to the weighted winset?** Q6's Implies list names the winset, the C API default and the bindings; S02b-006 noted the function itself is missing. S04b-001 assumes yes (the same rule at both layers); the answer decides how `ThresholdAtExactly50Percent` is rewritten and whether `scs_weighted_majority_prefers_2d` tests change.
2. **Copeland ties (Q27, unanswered):** should the tests assert tie pairs contribute 0 to both alternatives, and should the "Copeland winner is in the uncovered set" claim (S02b-013) be tested or dropped?

## Out-of-slice notes

- The shared Stage 4 coverage report (`stage4/coverage_report.txt`, built by `stage4/build.sh`) merges object files in a way that misreports every header whose inline functions appear in several test binaries. Other slices that quote "% lines" for `winset.h`, `geom2d.h`, `yolk.h`, `uncovered_set.h` or similar from that report should re-run their own binary (the same `llvm-cov` call with the single test binary and its own `.profraw`). Stage 0's 33% and 57% figures have the same cause.
- Tests in `test_aggregation_integration.cpp` (slice 4d) repeat the cycle fixture from `test_core.cpp`; they would need the same fixture decisions as S04b-018.
