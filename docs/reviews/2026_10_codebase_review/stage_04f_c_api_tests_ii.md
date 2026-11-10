# Stage 04f — C API tests II
Model / effort: Sonnet / high   Commit: 2771655   Files reviewed: 1 file, 1,809 lines: `tests/unit/test_c_api.cpp` lines 1–48 (shared helpers) and 1911–3671 (109 `TEST`s in 26 suites). Also read: `make_euclidean_dist` (line 1258) and `make_triangle_winset` (line 1477), which this range depends on.
Not read: nothing in the range was skipped. Lines 49–1910 are slice 4e.

Scratch programs and outputs are in
`/private/tmp/claude-501/-Users-robiragan-Dropbox-SocialChoiceStudioDev/2402f97e-221e-494b-88cb-fc63c41efc50/scratchpad/stage4f/`
(`$S4F` below): `probe.cpp`, `probe2.cpp`, `identity_mutant.cpp`, `yolk_lp.py`, `build.sh`. They link the read-only
`libscs_api.dylib` from `stage4/build_release` (Release). Each case runs in a forked child so a crash shows up as a signal.

## Summary
- **Counts:** 0 P0, 7 P1, 12 P2, 3 P3 (22 findings). Findings are about tests; the underlying bugs keep their own IDs.
- **None of the six P0/P1 findings in `stage_03b` has a test that could catch it** (S03b-001..006: five have no test, one is covered only by tests that cannot detect it). Neither do the Stage 2 items that reach this range: S02b-002 (Yolk P0), S02b-005, S02c-002, S02c-003, SCP-001, SCP-003, SCP-004. In all, 8 earlier P0/P1s have no test and 5 are touched only by tests that cannot catch them. The two PR #1 P0s that live here, S02b-001 and S02b-003, are fixed and tested, and the tests are discriminating (see the Findings map).
- **Most important (S04f-001, P1):** both Yolk tests use inputs where the Yolk centre is the voter centroid, so a solver that never leaves the centroid (S02b-002) passes. On an asymmetric 5-voter profile the library returns radius 3.180; an independent LP gives 2.594 (`$S4F/yolk_lp.py`).
- **Second (S04f-002, P1):** the only random-tie-break tests exercise the null-manager error. No test in the range ever runs a random tie-break to success, so the S03b-001 process abort (unregistered stream name) and Q3/Q4 determinism are both invisible. Coverage shows `scs_api.cpp:3034` (the `get_stream` call that throws) has zero hits.
- **Ranking and tie-break tests are weaker than they look (S04f-010):** every expected ranking in the range is the identity permutation `[0,1,2]`, so a function that ignores its input passes (demonstrated). Three of the four `OneWinner_SmallestIndex` tests use a fixture with a unique winner, so they never reach the tie-break.
- **Owner decisions:** no test in the range asserts behaviour the owner has decided is wrong, except the Heart tests (Q1, S04f-021, P3). Seeded tests contain no expected values, so Q4's RNG change will not break them, but for the same reason they cannot prove Q4 (S04f-007).

## Contracts this layer provides
What this part of the suite pins today, and what it does not, for Stage 11 and anyone changing the C API.

**Pinned (value-checked against hand calculation)**
- Copeland ordering on one 3-alternative collinear profile; Condorcet winner index; core found at the median voter (tolerance 0.1).
- Plurality, Borda, antiplurality, scoring-rule scores and winner sets on one 3×3 fixture (values are correct by hand: plurality 2/1/0, Borda 5/4/0, antiplurality 3/3/0). Fractional scoring weights (3.5/2.5/0).
- Spatial approval scores with a ≤-threshold check at distance 5 vs 1 (not at equality). Top-k approval for k = 1, 2, and k = 0 and 4 rejected.
- Pareto set and per-alternative efficiency; ordinal Condorcet winner present/absent.
- Salience weights: equal non-unit weights give radius r/w, an indifference curve through the status quo, and a correct one-voter winset (S02a-001, S02b-001, both fixed). Weights ≤ 0 are rejected by the winset factories, `scs_core_2d` and the uncovered-set boundary, with messages that name the function. Weight 0 still masks a dimension in a distance-only call.
- NaN/Inf rejection (`INVALID_ARGUMENT` or `NULL` plus a message) for: distance, `scs_winset_2d` voters, utility matrix, approval threshold, scoring weights, rank scores, winset query point, Voronoi sites and bounding box.
- Size-query pattern (`NULL`/0 capacity) for `all_winners`, `scs_pareto_set`, the uncovered set; `BUFFER_TOO_SMALL` plus the required count for `scs_antiplurality_all_winners`.
- `scs_api_version` error-buffer behaviour for null buffer, length 0 and length 1.
- ABI constants `SCS_TIEBREAK_RANDOM == 0`, `SCS_TIEBREAK_SMALLEST_INDEX == 1`, `SCS_MAJORITY_SIMPLE == -1`.

**Not pinned (nothing in the range would fail if it changed)**
- The Yolk beyond two symmetric inputs; any Yolk scale; any `scs_yolk_2d` argument validation.
- Duplicate alternatives; out-of-range indices in the two consistency predicates; zero, negative or huge counts anywhere.
- Any seeded output value; same-seed reproducibility; any random tie-break success path.
- The code returned for an undersized fixed-length buffer (13 functions), unknown enums, invalid `tie_break`, `grid_resolution` below 2.
- The `not found` outputs of `scs_condorcet_winner_2d` (-1) and `scs_core_2d` ((0,0)); no 2D profile without a Condorcet winner or core.
- The three centrality functions (no test anywhere in `test_c_api.cpp`).

## Findings

### S04f-001 [P1, confirmed] Yolk tests cannot detect that the solver returns the centroid (S02b-002, P0)
- **Where:** tests/unit/test_c_api.cpp:1911–1941 (`CApi_Yolk.EquilateralTriangle_CenterAndRadius`, `.CollinearVoters_RadiusNearZero`); also :1943–1963
- **What:** Both value tests use inputs whose Yolk centre is the voter centroid by symmetry (equilateral triangle; three collinear voters). S02b-002 says `yolk_2d` returns the centroid for every input, so both pass. The tolerances (0.05 on centre and radius) would also admit an answer 8% off. There is no asymmetric case, no even-n or k-majority case (S02b-005), and no case with duplicate voters (S02b-011).
- **Failure scenario:** `$S4F/probe.cpp`, Release build:

  | Voters | Library centre | Library r | LP centre | LP r |
  |---|---|---|---|---|
  | equilateral (test 1) | (1, 0.5774) | 0.5774 | (1, 0.5774) | 0.5774 |
  | collinear (test 2) | (1, 0) | 0 | on the x-axis | 0 |
  | (0,0) (10,0) (0,1) (10,1) (5,8) | (5, 2.00000003), the centroid | 3.1800 | (5, 3.1065) | 2.5936 |
  | (0,0) (4,1) (1,7) (9,3) (6,6) | (4.000000015, 3.4), the centroid | 1.9606 | (4.694, 3.306) | 1.6520 |

  The LP (`$S4F/yolk_lp.py`) is independent of the library: limiting median lines for odd n are the lines through two voters with (n−3)/2 and (n−1)/2 voters strictly on either side; it minimises r subject to |n·x − c| ≤ r. It agrees with Stage 2b's 2.593 for the third row. A test with the third row, asserting radius 2.5936 ± 0.01, fails today.
- **Suggested fix:** Add asymmetric known-answer tests (the two non-symmetric rows above, expected values from the LP), an even-n case, and tighten the symmetric tolerances to about 1e-6. Expected values must come from the LP or a published example, not from the library.
- **Also affects:** `test_yolk.cpp` (slice 4c) likely has the same blind spot (same S02b-002 evidence).

### S04f-002 [P1, confirmed] No test for the unregistered-stream abort (S03b-001, P0); no random tie-break success path anywhere in the range
- **Where:** tests/unit/test_c_api.cpp:2640–2650 (`CApi_Plurality.RandomTieBreak_NullMgr_ReturnsError`), :3030–3037 (`CApi_RankByScores.RandomTieBreak_NullMgr_ReturnsError`); `SCS_TIEBREAK_RANDOM` appears only at lines 2646, 3034, 3240, 3242. Code: src/c_api/scs_api.cpp:3034
- **What:** The two tests pass a null manager, which hits the "requires mgr and stream_name" branch of `resolve_prng`. The line that throws on an unregistered name (`get_stream`, scs_api.cpp:3034) has zero hits in the fresh coverage. Seven entry points share the pattern (`scs_plurality_one_winner`, `scs_borda_one_winner`, `scs_borda_ranking`, `scs_antiplurality_one_winner`, `scs_scoring_rule_one_winner`, `scs_rank_by_scores`, `scs_pairwise_ranking_from_matrix`); none has a test with a real manager. No test runs a random tie-break to completion, so Q3's advice ("simulations should use `random` with a `StreamManager`") and same-seed reproducibility (Q4) have no C API coverage.
- **Failure scenario:** `$S4F/probe.cpp` case `unregistered_stream`: manager with allowlist `["registered"]`, `scs_plurality_one_winner(..., SCS_TIEBREAK_RANDOM, m, "typo", ...)` → `libc++abi: terminating due to uncaught exception of type std::invalid_argument`, child killed by signal 6. A regression test written today would take down the whole `test_c_api` binary (every later test lost), so it must land together with the S03b-001 fix.
- **Suggested fix:** Per entry point: (a) a test with a registered stream and a real tie, run twice with the same seed, asserting the same winner and that it is one of the tied alternatives; (b) a test with an unregistered name asserting `SCS_ERROR_INVALID_ARGUMENT` and a message naming the stream. Use a profile with a genuine tie (the existing fixture has one for antiplurality only).
- **Also affects:** Python and R binding tests (Stage 6) should get the same regression.

### S04f-003 [P1, confirmed] No test for duplicate alternatives in the uncovered set (S03b-003)
- **Where:** tests/unit/test_c_api.cpp:2072–2106 (`CApi_UncoveredSet.*`)
- **What:** The three tests use one profile whose uncovered set is the single Condorcet winner at index 0. With one result, index 0, the `find_alt_index` mapping (first alternative with equal coordinates) cannot be told from a correct one. No test has two alternatives at the same point, or a result containing a non-zero index.
- **Failure scenario:** `$S4F/probe.cpp` case `uncovered_dup`: alternatives `(1,1) (1,1) (3,3)`, voters `(1,1) (1,1.1) (1.1,1)` → `n=2 idx=[0,0]`, expected `[0,1]`. A test asserting `[0,1]` fails today. A test with a non-duplicate result at index 2 (for example alternatives reordered so the winner is last) is also missing; it would pass today but would guard the mapping.
- **Suggested fix:** Add the duplicate case once the owner chooses between "accept with correct indices" and "reject with an error" (Stage 3b open question 4); add a case whose uncovered set contains indices other than 0.
- **Also affects:** `test_uncovered_set.cpp` (slice 4c) for the core function.

### S04f-004 [P1, confirmed] No test for out-of-range indices in the Condorcet predicates (S03b-006, S02c-004); the pair of tests cannot tell the two predicates apart
- **Where:** tests/unit/test_c_api.cpp:3185–3221 (`CApi_CondorcetProfile.CondorcetConsistent_OnlyWinnerPasses`, `.MajorityConsistent_OnlyCWPasses`)
- **What:** (a) Only indices 0 and 1 are tested; index 2, -1 and 999 are not, although `CApi_Pareto.OutOfRange_ReturnsError` (:3125) shows the pattern for the sibling function. (b) Both tests use a profile with a Condorcet winner. On such a profile `is_condorcet_consistent` and `is_majority_consistent` return identical results by construction; they differ only when there is no Condorcet winner (vacuous true vs false, `condorcet_consistency.h:99–141`). The two tests have identical assertions, so either function could be swapped for the other and both pass. The `CApi_CondorcetProfile.NoWinner_OutWinnerUntouched` cycle profile (:3165) is never fed to the predicates.
- **Failure scenario:** `$S4F/probe.cpp`: `scs_is_selected_by_condorcet_consistent_rules(p, 999, ...)` → `rc=0 out=0`; `scs_is_selected_by_majority_consistent_rules(p, -1, ...)` → `rc=0 out=1`; `scs_is_pareto_efficient(p, 999, ...)` → `rc=1`. On the Condorcet-cycle profile from line 3169: alternatives 0, 1, 2 give `condorcet_consistent=1` and `majority_consistent=0` for each, a distinction no current test makes. A test expecting `rc != SCS_OK` for index 999 fails today.
- **Suggested fix:** Add out-of-range tests (-1, n, 999) for both predicates; add the cycle profile to both; add the Felsenthal counter-example from S02c-003 (5 voters, 2×B>A>C, 2×C>A>B, 1×A>B>C) once Q28 is answered. Rename or merge the duplicate pair accordingly.
- **Also affects:** `test_aggregation_properties.cpp:204–246` (slice 4d), where S02c-003 says a test encodes the non-standard direction.

### S04f-005 [P1, confirmed] No test of zero, negative or overflowing counts (S03b-002, S03b-009)
- **Where:** tests/unit/test_c_api.cpp:1981–2106 (Copeland, Condorcet, Core, UncoveredSet), :2259–2404 (Profile), :2410–2478 (VoterSampler), :2862–2910 (ApprovalSpatial); coverage: scs_api.cpp:2142–2146 (`copeland n_alts >= 1`), 2877–2881 (`draw_voters n_voters >= 1`), 2680–2684, 2759–2769, 2808–2826 have zero hits
- **What:** Every count in the range is small and valid. No test passes 0, a negative number, or a value near `INT_MAX/2`. These are the inputs S03b-002 (positive-overflow and wrapped-negative counts, SIGBUS/SIGSEGV) and S03b-009 (`"vector"` message with `INTERNAL`, `found=1` with no voters, live handle for `n_alts=0`) describe.
- **Failure scenario:** `$S4F/probe.cpp` and `probe2.cpp`, Release build:
  - `scs_has_condorcet_winner_2d(a, -1073741825, v, 3, ...)` → child killed by signal 11.
  - `scs_has_condorcet_winner_2d(a, 3, v, -1, ...)` and `scs_uncovered_set_2d(a, -1, ...)` → `rc=2 msg='vector'`.
  - `scs_condorcet_winner_2d(a, 1, v, 0, ...)` → `rc=0 found=1 win=0`.
  - `scs_profile_build_spatial(a, 0, v, 1, ...)` → live handle.
  A test expecting `SCS_ERROR_INVALID_ARGUMENT` and a message naming the argument fails today for each.
- **Suggested fix:** Add a table-driven test over the fixed-count functions with counts {0, -1, INT_MIN, INT_MAX/2+1}, expecting `INVALID_ARGUMENT` and a message that names the argument and value. Put it in the same PR as the shared count-check helper.
- **Also affects:** slice 4e functions (same helper).

### S04f-006 [P1, confirmed] No Yolk test at realistic coordinate scales (S03b-004)
- **Where:** tests/unit/test_c_api.cpp:1911–1963; `scs_core_2d` test :2045
- **What:** Every Yolk and core test uses coordinates between 0 and 3. S03b-004 says the Yolk returns NaN with `SCS_OK` at large scale and `scs_core_2d` then fails with a message naming an input the caller never supplied. No test scales a profile.
- **Failure scenario:** `$S4F/probe.cpp` case `yolk_scale`, voters `(0,0) (10,0) (0,1) (10,1) (5,8)` multiplied by m: m = 1 → centre (5, 2), r 3.18; m = 1e4, 1e7, 3e7 → centre (NaN, NaN), r 0, `rc = 0`. This profile already fails at m = 1e4 (coordinates around 1e5), earlier than the m = 3e7 Stage 3b found for its voter set. A scale-invariance test (yolk of the scaled profile equals the scaled yolk, within 1e-6 relative) fails today.
- **Suggested fix:** Add a scale test at m ∈ {1e-3, 1, 1e4, 1e8} for the Yolk and for `scs_core_2d` (found flag and centre scale together). It also guards the tiny-scale `core_2d` lead in Stage 3b's out-of-slice notes.
- **Also affects:** `test_yolk.cpp`, `test_core.cpp`.

### S04f-007 [P1, confirmed (logic)] Seeded tests assert no values, so they cannot catch a change in draw order or algorithm (S03b-005, SCP-004, Q4)
- **Where:** tests/unit/test_c_api.cpp:2328–2404 (`CApi_Profile.ImpartialCulture_CorrectDims`, `.UniformSpatial_CorrectDims`, `.GaussianSpatial_CorrectDims`), :2410–2446 (`CApi_VoterSampler.*`)
- **What:** The generator tests check dimensions, `rc`, range and finiteness only. No test compares two runs from the same seed, no test compares against a golden value, and no test checks that an impartial-culture ranking is a permutation. S03b-005 (x and y drawn in compiler-dependent order in `profile_uniform_spatial` and `profile_gaussian_spatial`) and SCP-004 (same pattern in `profile_generators.h`) therefore have no test that could fail. Q4 requires cross-platform golden-value tests (exact for uniform, integer, bernoulli, discrete, shuffle); there is no seam for them in this range.
- **Failure scenario:** swap the x and y draws, or change the seeding of stream `"gen"`; every test in the range still passes.
- **Suggested fix:** For each generator: same seed → identical output on two fresh managers; different seed → different output; a golden-value test with expected values from the Q4 reference algorithms (computed independently, then checked in on macOS and Ubuntu); impartial culture: every row is a permutation of 0..n_alts−1.
- **Also affects:** none new; the existing tests have no expected values to regenerate when Q4 lands, which is a benefit of the current weakness but not a defence.

### S04f-008 [P2, confirmed] Uncovered-set boundary tests accept a single-vertex result and never check a vertex
- **Where:** tests/unit/test_c_api.cpp:2112–2178 (`CApi_UncoveredSetBoundary.*`)
- **What:** The comment at :2113 says the 5-voter pentagon "provides a non-trivial uncovered set region". The assertions are `pairs > 0` and "size query equals fill count". `HeapMatchesFill` compares two calls of the same deterministic function. No vertex is checked for finiteness, for lying inside the voters' bounding box plus margin, or for forming a polygon.
- **Failure scenario:** `$S4F/probe.cpp` case `grid`: the test's pentagon at grid 5 returns `pairs = 1`, a single point. All three tests pass on it. This is the "collapses to a point" behaviour in S02b-008 and the open Q25; the tests give no signal either way. `grid_resolution` -5, 0, 1 and 2 all return `SCS_OK` with the same 4-vertex result (S03b-011), also untested.
- **Suggested fix:** Assert a minimum vertex count (once Q25 is answered), finiteness, and containment in the voter bounding box; assert `grid_resolution < 2` is rejected once S03b-011 is fixed. Correct the comment.
- **Also affects:** `test_uncovered_set.cpp` (slice 4c).

### S04f-009 [P2, confirmed] Condorcet, core and Copeland tests miss every "not found" case and do not check score values
- **Where:** tests/unit/test_c_api.cpp:1981–2066; coverage scs_api.cpp:2298–2300 (`*out_found = 0; *out_winner_idx = -1`) and 2336–2338 (`*out_found = 0; *out_x = *out_y = 0.0`) have zero hits
- **What:** (a) `scs_condorcet_winner_2d` and `scs_core_2d` are tested only where a winner exists, so the "not found" outputs the header and Stage 3b document (-1; (0,0)) are never checked. No test has a spatial profile with no Condorcet winner or no core (for example the equilateral triangle, where `scs_core_2d` gives `found=0 (0,0)` today). (b) `CApi_Copeland.Scores_CondorcetWinnerHasMaxScore` uses `EXPECT_GT` between scores; the fixture's true scores are +2, 0, −2 (comment at :1969) but never asserted, and the real library output `[2, 0, -2]` was confirmed by the probe. (c) No Copeland tie, no `n_alts = 1`, no short-buffer test, no `scs_copeland_winner_2d` tie (probe: tie gives scores `[0,0]`, winner 0). (d) Q5's Implies asks for near-Plott tests (δ = 0.03, 0.01, 0.003 must be non-empty; exact symmetry empty); the range has none (S03b-015).
- **Failure scenario:** change the Copeland score to wins only, or flip the not-found output to `winner = 0`; every test still passes.
- **Suggested fix:** Assert the exact Copeland scores; add the no-winner, no-core and tie cases with the documented outputs; add the Q5 δ cases when the exact test lands.
- **Also affects:** none.

### S04f-010 [P2, confirmed] Ranking tests all expect the identity permutation; tie-break tests mostly contain no tie
- **Where:** tests/unit/test_c_api.cpp:2697–2713 (`CApi_Borda.Ranking_Correct`), :2996–3020 (`CApi_RankByScores.CorrectOrder`, `.TiedScoresSmallestIndex`), :3043–3065 (`CApi_PairwiseRanking.CorrectOrder`); tie-break: :2619–2632 (`CApi_Plurality.OneWinner_SmallestIndex`), :2682–2695 (Borda), :2819–2834 (ScoringRule)
- **What:** Each ranking test's expected result is `[0, 1, 2]`. A function that ignores its input and returns the identity passes all four. The plurality, Borda and scoring-rule fixtures (`make_three_voter_profile`) have a unique winner (scores 2/1/0, 5/4/0, 5/4/0), so the three `OneWinner_SmallestIndex` tests never reach the tie-break; only `CApi_Antiplurality.OneWinner_SmallestIndex` (tied A, B) does. `TiedScoresSmallestIndex` ties the top two alternatives only; a tie at the bottom or between non-adjacent entries is not tested.
- **Failure scenario:** `$S4F/identity_mutant.cpp`: a stub returning the identity passes both `RankByScores` tests ("mutant passes test: YES" twice). The real library on scores `(0,4,5)` returns `[2 1 0]` and on `(1,3,3)` returns `[1 2 0]`; tests with those inputs would discriminate. The pairwise-ranking test's comment ("Copeland scores: A=2, B=1, C=0") uses wins only, while the Copeland tests use wins minus losses (`[2,0,-2]`); the ranking is the same either way, so the test cannot help settle Q27.
- **Suggested fix:** Use inputs whose correct output is not the identity, with a tie at the bottom and a tie between non-adjacent entries; build a profile with a plurality/Borda tie for the three tie-break tests. Add a pairwise-matrix case with a tie (S02c-002).
- **Also affects:** none.

### S04f-011 [P2, confirmed] The undersized-buffer paths, and the inconsistent error codes (S03b-013, S03h-003), are almost entirely untested
- **Where:** tests/unit/test_c_api.cpp: only `CApi_Antiplurality.AllWinners_BufferTooSmall_ReturnsError` (:3310) exercises a too-small buffer. Coverage at zero hits: scs_api.cpp:2149–2153 (Copeland), 2382–2387 (`uncovered_set_2d`), 2109–2113 (boundary fill), 2885–2890 (`draw_voters`), 2962–2967 and 2990–2995 (`get_ranking`, `export_rankings`), 3074–3079, 3144–3149 and the sibling score functions
- **What:** `CApi_UncoveredSet.BufferTooSmall` (:2096) is misnamed: it passes capacity 0 and checks the size-query path returns `OK`. No test passes a non-null buffer that is too small for the uncovered set, the boundary, Copeland scores, the score functions, `get_ranking`, `export_rankings` or `draw_voters`. The two different codes for the same mistake (`INVALID_ARGUMENT` for 13 functions, `BUFFER_TOO_SMALL` for the rest) are therefore neither pinned nor flagged.
- **Failure scenario:** `$S4F/probe.cpp`: Copeland with `out_len = 2` for 3 alternatives → `rc=1` (INVALID_ARGUMENT); `scs_draw_voters` with a short buffer → `rc=3`. When S03b-013 is fixed, nothing tells the fixer which tests to flip, and nothing fails if only some of the 13 are changed.
- **Suggested fix:** One table-driven test over all fixed-length output functions, expecting one agreed code and the required size in the message; rename `BufferTooSmall` to `ZeroCapacityIsSizeQuery` and add a real too-small case (capacity 1 with 2 results).
- **Also affects:** Python exception types (S03b-013).

### S04f-012 [P2, confirmed] Enum, `tie_break`, `grid_resolution` and `num_samples` validation is untested
- **Where:** tests/unit/test_c_api.cpp:1911–2178, :2410–2478, :2640–3037; Decision links: none (S03b-008, S03b-010, S03b-011, S03a-002, S03h-002)
- **What:** No test passes an out-of-range `SCS_TieBreak`, `SCS_VoterSamplerKind`, distance type, loss type, a Yolk `num_samples` ≤ 0, a non-Euclidean Yolk config, or a `grid_resolution` below 2.
- **Failure scenario:** `$S4F/probe.cpp`: `scs_plurality_one_winner(p, 99, NULL, NULL, ...)` → `rc=0` with a unique winner but an error on a tie (data-dependent, S03b-008); `scs_yolk_2d` with Manhattan config → `rc=0` and the Euclidean radius 3.1800; `num_samples = -1` → `rc=0`; `grid_resolution` -5, 0, 1 → `rc=0` with the grid-2 result. The unknown-`SCS_VoterSamplerKind` branch (scs_api.cpp:2901–2904) has zero hits.
- **Suggested fix:** Add one test per input with the expected end-user error once each fix lands; run them under UBSan too (S03h-002 notes out-of-range enum values are undefined behaviour before any `default:` branch).
- **Also affects:** none.

### S04f-013 [P2, confirmed] Non-finite and extreme-value tests skip most of the functions in the range
- **Where:** tests/unit/test_c_api.cpp:3379–3457 (`CApi_NonFinite`), :3613–3671 (`CApi_NonFiniteInput`)
- **What:** PR #1's finiteness checks are tested for distance, `scs_winset_2d`, the utility matrix, approval threshold, scoring weights, rank scores, `scs_winset_contains_point_2d` and Voronoi. Not tested: Copeland, `scs_condorcet_winner_2d`, `scs_has_condorcet_winner_2d`, `scs_core_2d`, `scs_uncovered_set_2d`, the boundary, `scs_yolk_2d` (voters), approval alternatives and voters, `scs_profile_build_spatial`, `scs_draw_voters` and the generators' `lo`, `hi`, `mean`, `stddev`. Infinite values appear only in three tests (Inf alt, Inf threshold, Inf score), and Voronoi tests use NaN sites only. `RankByScores` uses Inf but not NaN, which matters because S02c-006 says `rank_by_scores` loops forever on NaN if the boundary check is ever missed. No test uses finite but extreme coordinates (SCP-001, S03a-007) or a huge `n_sites` (SCP-003).
- **Failure scenario:** remove the finiteness check from `scs_copeland_scores_2d` or `scs_yolk_2d`; no test fails. `$S4F/probe.cpp` shows the checks hold today (all of the listed functions return `rc=1` or `NULL` for a NaN voter, NaN `lo`, and so on), so the missing tests would pass and guard them.
- **Suggested fix:** A table-driven test: for each coordinate/numeric argument of each function, substitute NaN, +Inf, −Inf and expect `INVALID_ARGUMENT` with a message naming the argument. Add an extreme-finite case (±1e308) for winset, Voronoi and indifference curves on a Release build once SCP-001 is fixed.
- **Also affects:** none.

### S04f-014 [P2, confirmed] Constructor failure paths never run, so the handle leak (S03b-007, S03a-001) cannot show up; constructor assertions are weak
- **Where:** tests/unit/test_c_api.cpp:2259–2404; coverage scs_api.cpp:2658–2666, 2700–2709, 2729–2738, 2786–2795, 2844–2853 (the `catch` blocks of the five profile constructors) have zero hits
- **What:** The constructor error tests (`BuildSpatial_Error_NullAlts`, `FromUtilityMatrix_Error_NullPointer`, `ImpartialCulture_Error_NullManager`, `UniformSpatial_Error_BadDims`) all fail before the `new SCS_ProfileImpl{}` line, so the leaking path (core throws after allocation) is never taken. Stage 3b and the leak run saw 0 leaks in this suite for exactly this reason. Success tests check only `n_voters` and `n_alts` (and, for the two hand-built cases, the top-ranked alternative); no ranking is checked in full, no tie in distances is tested (Stage 1 invariant 6: equal distances go to the smaller index), and `clone` is not checked for independence (destroying the original, then reading the clone).
- **Failure scenario:** `$S4F/probe2.cpp`: `scs_profile_build_spatial(a, 3, v, 0, ...)` → `NULL` with the core message; per Stage 3b this leaks 48 bytes per call. No test calls it. A leak test also needs a leak detector: macOS arm64 has no LeakSanitizer, so this has to run on Linux CI (or under `leaks`).
- **Suggested fix:** Add a failing-after-allocation case per constructor (zero voters) and run the suite under LSan on Linux; assert full rankings and a distance tie in `BuildSpatial`; destroy the original before reading the clone.
- **Also affects:** winset factories (slice 4e).

### S04f-015 [P2, confirmed] Voter-sampler tests are weak
- **Where:** tests/unit/test_c_api.cpp:2410–2478
- **What:** `Uniform_CorrectLength` asserts only `rc == OK` (the output length is not checked). `Gaussian_AllFinite` checks only finiteness; 200 draws from N(0, 0.5²) could be checked for mean and spread with fixed seed. The error tests use `EXPECT_NE(rc, SCS_OK)` rather than the specific code. No test covers n_voters ≤ 0, `lo ≥ hi`, `stddev ≤ 0`, an unknown kind, an undersized buffer (`BUFFER_TOO_SMALL`, the one place that code is used), or a non-finite parameter.
- **Failure scenario:** a sampler that writes zeros, or only half the buffer, passes `Uniform_CorrectLength`, `Uniform_AllInRange` (0 ∈ [−1, 1]) and `Gaussian_AllFinite`. The probe shows the real library gives `rc=1` for `lo > hi`, NaN `lo`, bad kind and `stddev < 0`, and `rc=3` for a short buffer, so those tests would pass today.
- **Suggested fix:** Check that the buffer is fully written (sentinel fill), that two seeds differ and one seed repeats, a loose moment check for the Gaussian, and the specific codes.
- **Also affects:** none.

### S04f-016 [P2, confirmed] Q18 weight rejection is only partly tested
- **Where:** tests/unit/test_c_api.cpp:3526–3597 (`CApi_SalienceWeights.ShapeFunctionsRejectNonPositiveWeights`, `.WinsetRejectsNonFiniteWeights`)
- **What:** Q18 says every 2D function that builds a bounded shape from weights rejects zero, negative and non-finite weights. The test loops over the four distance types only for `scs_winset_2d`; the weighted winset, veto winset, `scs_core_2d` and the uncovered boundary are tested with Euclidean only. Non-finite weights are tested only for `scs_winset_2d`, and only in the first position; a bad second weight is not tested. `scs_level_set_2d`, `scs_ic_polygon_2d` and `scs_winset_2d_export_boundary` are not tested for rejection anywhere in the file (grep of lines 1–1910 and the range). `scs_winset_2d_export_boundary` returns `INTERNAL` for these errors (S03a-008), so the omission also hides that.
- **Failure scenario:** `$S4F/probe.cpp` case `weights_other`: a NaN or 0 in the second weight position → `NULL` (weighted, veto), `rc=1` (core, boundary, `level_set_2d`, `ic_polygon_2d`) today, so the missing tests would pass and protect Q18. Remove the check from `scs_level_set_2d` and the suite stays green.
- **Suggested fix:** Parameterise over {function} × {distance type} × {position 0, 1} × {0, −1, NaN, Inf}; include `level_set_2d`, `ic_polygon_2d` and the export function (expecting `INVALID_ARGUMENT` once S03a-008 is fixed).
- **Also affects:** none.

### S04f-017 [P2, confirmed] Spatial approval: no equality-at-threshold or empty-result test
- **Where:** tests/unit/test_c_api.cpp:2862–2910
- **What:** Stage 1 invariant 11 says "approve iff distance ≤ threshold; empty result if no approvals". The tests use distances well away from the threshold (0, 0.5, 5 vs 1.0), and no test has a voter at exactly the threshold or a profile where nobody approves anything.
- **Failure scenario:** change `≤` to `<`; every test still passes. `$S4F/probe.cpp` shows current behaviour is as the invariant says (voter at distance 1.0, threshold 1.0 → score `[1, 0]`; nobody approves → `n = 0` winners), so the tests would pass today.
- **Suggested fix:** Add both cases. Note the equality case depends on exact floating-point equality of a computed distance; use a case where the distance is exactly representable (1.0 along an axis).
- **Also affects:** none.

### S04f-018 [P2, confirmed] The three centrality functions have no C API test
- **Where:** tests/unit/test_c_api.cpp (no occurrence of `scs_marginal_median_2d`, `scs_centroid_2d` or `scs_geometric_mean_2d`); coverage scs_api.cpp:4030–4130 has zero hits
- **What:** Stage 0 and Stage 3b noted that these are exercised only through Python and R. Within the C++ suite, argument validation (null, `n_voters < 1`, non-finite, non-positive coordinates for the geometric mean) and the value paths are untested at the C boundary, and the negative/overflow count pattern (S03b-002) applies to them too.
- **Failure scenario:** break `scs_centroid_2d`'s output assignment; `test_c_api` still passes (Python and R would catch it later).
- **Suggested fix:** One test per function with a hand-computed value (centroid of (0,0), (2,0), (1,3) is (1,1); geometric mean of (1,4), (4,1) is (2,2)), plus its null, empty and non-finite error cases.
- **Also affects:** none.

### S04f-019 [P3, confirmed] The error-buffer contract is tested only through `scs_api_version`
- **Where:** tests/unit/test_c_api.cpp:3255–3278 (`CApi_ErrorPaths.*`)
- **What:** The contract (Stage 3 reports and `scs_api.h`) is: null or non-positive length is safe, messages are truncated and always NUL-terminated, the buffer is untouched on success. The tests cover null, length 0 and length 1 on one function. Negative length, truncation (length 5), and untouched-on-success are not tested. The `CApi_ErrorPaths.NullErrBuf_StillReturnsCode` test has an unused variable (`major`, line 3258, cast to void).
- **Failure scenario:** `$S4F/probe.cpp` case `errbuf` shows the contract holds today (length -1 safe; length 5 gives `'scs_'`, NUL at index 4; success leaves the buffer untouched), so the missing tests would pass. A change to `set_error` that wrote past `err_buf_len` would not fail any test unless run under ASan.
- **Suggested fix:** Add negative length, truncation to a small length with a canary after the buffer, and untouched-on-success; run against a function that produces a long message.
- **Also affects:** none.

### S04f-020 [P2, confirmed] `CApi_Winset.UnionSelf_NonEmpty` checks too little for its stated property
- **Where:** tests/unit/test_c_api.cpp:3281–3304
- **What:** The comment says "X ∪ X = X". The assertion is only that the union is non-empty. A union that returned an arbitrary non-empty region, or only one operand's half, would pass. The test also sits in the "C8 additional coverage sweep" instead of next to the other Winset tests (slice 4e).
- **Failure scenario:** replace `scs_winset_union` with a function that returns a clone of its first operand shifted by 1.0; the test passes.
- **Suggested fix:** Compare bounding boxes and several `contains_point` results (inside, outside, near the boundary) between the union and the original; move the test into the Winset group.
- **Also affects:** none.

### S04f-021 [P3, confirmed] Heart tests to be removed (Q1)
- **Where:** tests/unit/test_c_api.cpp:2180–2253 (`CApi_Heart` ×3, `CApi_HeartBoundary` ×2); :3576–3580 (the `scs_heart_boundary_2d` assertions inside `CApi_SalienceWeights.ShapeFunctionsRejectNonPositiveWeights`)
- **What:** Withdrawn code (Q1). Listed once and not reviewed. The last item is inside a test that must otherwise stay, so removal means editing that test, not deleting it.
- **Failure scenario:** none; listed for removal.
- **Suggested fix:** Delete the five tests and the two assertions with the Heart C API removal (Q1 Implies).
- **Also affects:** none.

### S04f-022 [P3, confirmed] Hygiene: misleading names and comments, duplicate tests, fragile setup
- **Where:** tests/unit/test_c_api.cpp as listed
- **What and failure scenario, per item:**
  - :2096 `CApi_UncoveredSet.BufferTooSmall` tests the size query (see S04f-011); :2410 `Uniform_CorrectLength` does not check length (S04f-015); :3185 `CondorcetConsistent_OnlyWinnerPasses` never tests that index 2 fails.
  - :2098 `int tiny[0] = {}` is a zero-length array (the Stage 0 `-Wzero-length-array` warning, previously line 2097); passing `nullptr` with capacity 0 states the intent and compiles cleanly.
  - :2551 (fixture header) says antiplurality scores are "A=0, B=0, C=3 → C placed last 3×", while the test at :2733–2737 asserts 3/3/0. A reader of the header will expect the opposite. The header also lists "Top-2 approval: A=3, B=3, C=0" correctly.
  - :3258–3260 dead variable `major`.
  - :3284 `UnionSelf_NonEmpty` is in the wrong section (S04f-020).
  - :3239 and :3246 comment calls the tie-break values "ABI-stable constants" (Q12 language, an extra instance beyond S03h-011).
  - :1969–1970 (Copeland: wins minus losses) and :3046–3061 (pairwise ranking: wins only) use the word "Copeland" for different scores (S02c-002; Q27 open).
  - :2129–2147, :2235–2253 `FillBuffer`: the size-query call's return code is ignored; if it failed, `pairs` stays -1 and `static_cast<size_t>(pairs * 2)` asks for a huge vector, so a library error shows as an allocation exception, not an assertion.
  - :3644 `EXPECT_EQ(heap.xy, nullptr)` is checked on a value-initialised struct (`SCS_VoronoiCellsHeap heap{}`), so it passes even if the function never zeroes `out`; S03h-015 (not zeroed for a NULL `sites_xy`) cannot show. Pre-fill the struct with a non-zero pattern and add the NULL-sites case.
  - :3165–3183 asserts that `scs_condorcet_winner_profile` leaves `out_winner` untouched, while `scs_condorcet_winner_2d` writes -1 for the same situation (S03h "not-found outputs differ per function"); the test is correct for the documented behaviour but should be revisited if the convention is unified.
  - :1976 `make_euc2d` returns a config pointing into a function-local `static double w[2]`; harmless today, but shared mutable-looking state in a test helper.
- **Suggested fix:** Fix each as listed.
- **Also affects:** none.

## Findings map

"Wrong-behaviour test" means a test asserts the buggy behaviour (none found in the range except Heart). "Cannot catch" means a test touches the area but would pass with the bug present. "Fixed and tested" is for the PR #1 items. Earlier findings outside lines 1–48 and 1911–3671 are marked "4e" and not assessed here.

| Earlier finding | Severity | Status | Test name or note |
|---|---|---|---|
| S03b-001 unregistered stream aborts | P0 | no test | Only the null-manager branch is tested (`CApi_Plurality.RandomTieBreak_NullMgr_ReturnsError`, `CApi_RankByScores.RandomTieBreak_NullMgr_ReturnsError`); scs_api.cpp:3034 has zero hits; probe aborts with signal 6. S04f-002 |
| S03b-002 `n * 2` before range check | P1 | no test | No count outside a small valid range in the whole range; probe: SIGSEGV at n_alts = -1073741825. S04f-005 |
| S03b-003 duplicate alternatives, same index | P1 | no test | `CApi_UncoveredSet.*` use a single-winner profile (index 0). Probe `[0,0]`. S04f-003 |
| S03b-004 Yolk NaN at large coordinates | P1 | no test | `CApi_Yolk.*` use coordinates 0–3. Probe NaN at m = 1e4 for one profile. S04f-006 |
| S03b-005 draw order of x/y in generators | P1 | cannot catch | `CApi_Profile.UniformSpatial_CorrectDims`, `.GaussianSpatial_CorrectDims` check dimensions only. S04f-007 |
| S03b-006 predicates accept any index | P1 | no test | `CApi_CondorcetProfile.CondorcetConsistent_OnlyWinnerPasses`, `.MajorityConsistent_OnlyCWPasses` test indices 0 and 1 only; `CApi_Pareto.OutOfRange_ReturnsError` shows the pattern. S04f-004 |
| S03b-007 profile handle leak on core throw | P2 | no test | Constructor error tests fail before allocation; catch blocks have zero hits. S04f-014 |
| S03b-008 `tie_break` not validated | P2 | no test | S04f-012 |
| S03b-009 negative/zero counts | P2 | no test | S04f-005 |
| S03b-010 Yolk ignores `dist_cfg`, `num_samples` unchecked | P2 | no test | `CApi_Yolk.*` all use Euclidean, 720. S04f-012 |
| S03b-011 `grid_resolution` clamped | P2 | no test | Boundary tests use 5 only. S04f-008, S04f-012 |
| S03b-012 range width overflow | P2 | no test | `CApi_VoterSampler.*` use [-1, 1]. S04f-007 area; no separate finding |
| S03b-013 two codes for a short buffer | P3 | no test | S04f-011 |
| S03b-014 negative `k` accepted | P3 | no test | `k` is only `SCS_MAJORITY_SIMPLE` in the range |
| S03b-015 `scs_core_2d` doc overclaims | P3 | no test | No Q5 near-Plott case. S04f-009 |
| S03b-017 Heart C API | P3 | tests to remove | S04f-021 |
| S02b-002 Yolk returns centroid | P0 | cannot catch | `CApi_Yolk.EquilateralTriangle_CenterAndRadius`, `.CollinearVoters_RadiusNearZero`. S04f-001 |
| S02b-005 Yolk line family, even n | P1 | no test | No even-n or k-majority Yolk test. S04f-001 |
| S02b-001 equal non-unit weights, wrong size | P0 | fixed and tested | `CApi_SalienceWeights.LevelSetEqualNonUnitWeightsRadiusIsROverW`, `.IcPolygonEqualNonUnitWeightsPassesThroughSQ`, `.WinsetEqualNonUnitWeightsOneVoter`. Values are hand-derived (radius 2/w; sq on the curve; 0.9 inside, 1.5 outside); with the old size, w = 2 puts 0.9 outside and w = 0.5 puts 1.5 inside, so both w values fail |
| S02b-003 zero weight aborts | P0 | fixed and tested | `CApi_SalienceWeights.ShapeFunctionsRejectNonPositiveWeights` (partial: S04f-016), `.WinsetRejectsNonFiniteWeights` |
| S02a-001 level-set size for equal non-unit weights | P1 | fixed and tested | as S02b-001 |
| PR #1 finiteness at the boundary | n/a | fixed and tested, partly | `CApi_NonFinite.*`, `CApi_NonFiniteInput.*`; many functions untested. S04f-013 |
| S02c-002 `pairwise_ranking` wins only | P1 | cannot catch | `CApi_PairwiseRanking.CorrectOrder`: no tie, identity output. S04f-010 (Q27 open) |
| S02c-003 `is_majority_consistent` definition | P1 | cannot catch | Both predicate tests give identical results on a profile with a Condorcet winner. S04f-004 (Q28 open) |
| S02c-004 = S03b-006 | P1 | no test | S04f-004 |
| S02c-006 `rank_by_scores` loops on NaN | P2 | cannot catch | `CApi_NonFinite.RankByScores_InfScores_ReturnsInvalidArgument` uses Inf, not NaN. S04f-013 |
| S02c-007 zero alternatives accepted | P2 | no test | S04f-005 |
| SCP-001 finite inputs give Inf vertices | P2 | no test | `CApi_NonFiniteInput.*` use NaN/Inf only. S04f-013 |
| SCP-002 equal-weights tolerance | P2 | no test | Weight tests use 2.0 and 0.5 only (`CApi_SalienceWeights.*`) |
| SCP-003 Voronoi `n_sites * 2` | P1 | no test | `CApi_NonFiniteInput.VoronoiRejects*` use 2 sites. S04f-013 |
| SCP-004 coordinate draw order | P1 | cannot catch | S04f-007 |
| S03a-001 winset factory leak | P1 | no test | `CApi_NonFinite.Winset2d_NaNInVoterIdeals_ReturnsNull` and the weight tests fail before allocation. S04f-014 |
| S03a-002 unknown enums replaced | P1 | no test | S04f-012 |
| S03a-003..006 | P1 | 4e | Not in my range; a grep of lines 1–1910 found no test with `n_vertices > 4`, `n_alts` near 46341, a negative hull count or NaN to `scs_winset_interval_1d` input (only output NaN is checked) |
| S03a-007 extreme finite coordinates | P2 | no test | S04f-013 |
| S03a-008 `export_boundary` returns INTERNAL | P2 | no test in my range | Not called by the weight tests. S04f-016 |
| S03h-001 0.5 "simple weighted majority" | P1 | 4e | Out of range. Line 1419 of the file ("with equal weights and threshold=0.5, result must agree with simple majority") is the test to check |
| S03h-003 two codes for a short buffer | P2 | no test | S04f-011 |
| S03h-006 overclaiming emptiness/core | P2 | no test | `CApi_Core.CollinearVoters_CoreFound` is consistent with an exact test; Q5 δ cases missing. S04f-009 |
| S03h-015 heap failure state | P3 | cannot catch | `CApi_NonFiniteInput.VoronoiRejectsNaNSite` checks a value-initialised struct. S04f-022 |
| Q1 Heart | P3 | tests to remove | S04f-021 |
| Q3 explicit tie-break at the C API | n/a | consistent | All tests pass an explicit value |
| Q4 portable draws, golden values | n/a | no test | S04f-007 |
| Q18 zero weight | n/a | partly tested | S04f-016 |

Earlier P0/P1 findings that reach this range: S03b-001..006 (6; S02c-004 is the same as S03b-006), S02b-002, S02b-005, S02c-002, S02c-003, SCP-001, SCP-003, SCP-004 (13 in all). Of these 13: 0 fixed and tested, 0 covered by a test that asserts the wrong behaviour, **8 with no test** (S03b-001, 002, 003, 004, 006; S02b-005; SCP-001; SCP-003) and **5 touched only by tests that cannot catch them** (S03b-005, S02b-002, S02c-002, S02c-003, SCP-004). S03a-001 and S03a-002 also have no test; S03a-003..006 and S03h-001 are in 4e. The PR #1 P0s S02b-001 and S02b-003 are fixed and tested.

## Tool leads checked
- Coverage, scs_api.cpp:2298–2300 and 2336–2338 (`not found` outputs of `scs_condorcet_winner_2d`, `scs_core_2d`) → S04f-009 (no test).
- Coverage, scs_api.cpp:3034 (`get_stream` throw in `resolve_prng`) → S04f-002 (no test; confirmed abort).
- Coverage, scs_api.cpp:2109–2113, 2149–2153, 2382–2387, 2885–2890, 2962–2967, 2990–2995, 3074–3079 and siblings (undersized buffers) → S04f-011.
- Coverage, scs_api.cpp:2142–2146, 2680–2684, 2759–2769, 2808–2826, 2877–2881, 2901–2904 (count and parameter validation) → S04f-005, S04f-012, S04f-015.
- Coverage, `catch` blocks of the five profile constructors (2658–2666, 2700–2709, 2729–2738, 2786–2795, 2844–2853) → S04f-014.
- Coverage, scs_api.cpp:4030–4130 (centrality functions, zero hits) → S04f-018.
- Coverage, scs_api.cpp:2097 `return -1` in `find_alt_index` → not a defect: Stage 3b already notes the branch is unreachable (the point always comes from `alts`).
- Coverage, `catch (const std::exception&)` / `INTERNAL` returns throughout the range (about 60 lines at zero hits) → not a defect for tests: they need a throwing core or `bad_alloc`; S03a-010 and S03a-013 cover the design side. No finding.
- Coverage, `yolk.h`: the shared report's 43% was re-checked as instructed (fresh run of `test_c_api` from `build_cov` plus `libscs_api.dylib` only, profile in `$S4F/cov/`): 42.79% lines (127 of 222 missed), 22% of functions executed. That figure is per-instantiation and I did not map the missed lines to the optimiser loop, so it is not used as evidence for any finding. The same run reproduces the shared `scs_api.cpp` numbers (844 missed regions, 1,522 missed lines), so every `scs_api.cpp` line cited above stands.
- Stage 0 `-Wzero-length-array` at test_c_api.cpp:2097 → S04f-022 (now line 2098).
- Stage 0 `implicit-widening-of-multiplication-result` at test_c_api.cpp:2116, 2132, 2152, 2222, 2238 → not a defect: loop index `2 * i` with `i < 5` on a 10-element array.
- Stage 0 `-Wunused-result` → none in this range.
- Stage 0 never-called functions (`scs_marginal_median_2d`, `scs_centroid_2d`, `scs_geometric_mean_2d`) → S04f-018.
- Determinism → all seeds fixed (7, 3, 42, 99, 1u); no time, no shared state beyond a const array and one static weights array; no timing assertions. `CApiTest` is not labelled `benchmark` (CMakeLists.txt:341; labels only on the two speed tests) and ran in 0.19 s. Not a defect.
- Registration → `test_c_api` is registered as `CApiTest` (CMakeLists.txt:338–341); passes in `ctest_release.txt` (36/36 line). No unregistered file.
- Tolerances → no S00-001-style sub-spacing tolerance in the range. The loosest are Yolk 0.05 and core 0.1 (S04f-001, S04f-009); the tightest are 1e-12 on radii and vertex 0 (correct for the values involved).
- Expected values from the code's own output → none suspected: the hand-checkable scores (plurality, Borda, antiplurality, scoring-rule, Copeland, approval) are correct by independent calculation, and weight tests use closed-form values.
- Stage 3b S03b-016 (end-user messages) → the range checks message content only for `winset_2d: salience_weights[`, `finite and > 0`, `winset_contains`, `sites`, `bounding box`; error text elsewhere is checked for `strlen > 0` only. Not a separate finding; folded into the "assert a message that names the argument" suggestions.

## Open questions
1. **Q14 (unanswered):** if the Yolk stays in the C API only, S04f-001 and S04f-006 are needed now; if it is withdrawn or replaced, the four Yolk tests are rewritten with the replacement. Which?
2. **S03b-003:** accept duplicate alternatives with correct indices, or reject them? The test in S04f-003 differs.
3. **Q25 (unanswered):** should the uncovered-set boundary have a minimum vertex count? S04f-008 needs the answer before the assertion can be written.
4. **Q27 and Q28 (unanswered):** S04f-004 and S04f-010 propose cases (Felsenthal example, pairwise ties) whose expected results depend on these answers.
5. **Not-found output convention:** `scs_condorcet_winner_2d` writes -1, `scs_condorcet_winner_profile` leaves the output untouched, `scs_core_2d` writes (0, 0). The tests at :3165–3183 pin the "untouched" behaviour. Keep per-function conventions, or unify?

## Out-of-slice notes
- `tests/unit/test_c_api.cpp:1419` (slice 4e): comment "With equal weights and threshold=0.5, result must agree with simple majority", which touches S03h-001 and Q6 (0.5 is not "simple majority" when both directions can win). 4e should check whether the test asserts the problematic behaviour.
- `test_yolk.cpp` (slice 4c) probably shares the S04f-001 blind spot; `test_aggregation_properties.cpp:204–246` (slice 4d) encodes the non-standard majority-consistency direction (S02c-003).
- Leak detection for S03b-007/S03a-001 needs a Linux job with LSan or `leaks` on macOS; neither is part of `ctest` today (macOS arm64 has no LeakSanitizer).
