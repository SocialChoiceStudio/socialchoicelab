# Stage 04d — aggregation and competition tests
Model / effort: Sonnet 5.5 / high   Commit: 2771655   Files reviewed: 13, 2,901 lines
(`tests/unit/` `test_profile`, `test_voting_rules`, `test_aggregation_properties`,
`test_aggregation_integration`, `test_voter_sampler`, `test_seat_allocation`,
`test_competitor_initialization`, `test_competition_strategies`, `test_competition_motion`,
`test_competition_election_feedback`, `test_competition_engine`, `test_competition_termination`,
`test_competition_experiment_runner`)
Not read: none. Every line of all 13 files was read. Code under test was read only to settle
specific questions (tie rules, `leader_index`, `build_spatial_profile_nd`, `run_competition_experiment`,
`approval_scores_topk`, `scoring_rule_scores`, `break_tie`).

Scratch material: `SCR4D=/private/tmp/claude-501/-Users-robiragan-Dropbox-SocialChoiceStudioDev/2402f97e-221e-494b-88cb-fc63c41efc50/scratchpad/stage4d/`.
- `mut.sh` + `m1.py` … `m12.py`: **mutation tests**. Each copies `include/` to `mut_<id>/`, changes
  one header, rebuilds the real test files against the copy and runs them. The repository is untouched.
- `missing_tests.cpp` (output `missing_tests.out`): 10 GoogleTest cases that assert the intended
  behaviour and **fail on 2771655**. They are ready-made regression tests for Stage 11.
- `vacuous.cpp` (`vacuous.out`), `seeds.cpp` (`seeds.out`), `order_probe.cpp`, `majority_ref.cpp`
  (`majority_ref.out`), `seat_probe.cpp` (`seat_probe.out`), `m4probe.cpp`: probes cited below.
- Built with the flags from `build_release/compile_commands.json` (clang, `-std=c++20`, `-O1`).

**Coverage method.** Following the coordinator's warning, no percentage or "uncovered line" in this
report comes from the shared `coverage_report.txt`. I ran the 13 slice binaries from `build_cov`
with my own `LLVM_PROFILE_FILE` into `$SCR4D/prof/`, merged them and ran `llvm-cov` on those 13
binaries only (`$SCR4D/slice_report.txt`, `$SCR4D/show/all.txt`, `$SCR4D/show/zero.txt`). Nothing
was rebuilt. The shared report is wrong for three of the headers in this slice (slice-only vs shared):
`profile_generators.h` 88.2% vs 62.9%, `adaptation.h` 71.1% vs 54.8% (84 lines there, 83 here),
`competition_types.h` 64.3% vs 16.7%. The other 22 headers agree. Slice-only coverage of these 25 headers
is 83.7% of lines. Note that it measures what these 13 binaries execute, not what the C API tests execute.

## Summary
- **0 P0, 12 P1, 11 P2, 3 P3** (26 findings). The P1s are test findings under the rubric rule: a test
  that asserts a P1 behaviour, or a P1 bug that no test could catch. Eleven are confirmed by a mutation
  or a failing scratch test; S04d-009 is plausible only, because the intended behaviour (Q32) is open.
- **Three tests assert behaviour the owner has decided is wrong or that is itself a P1 bug**:
  - `CompetitionElectionFeedback.OneDimensionalPluralityFeedbackMatchesExpectations` contains an
    accidental voter tie (voter at 1.0 between competitors at 0 and 2) and expects the lowest index. Q7 forbids it (S04d-001).
  - `CompetitionStrategies.PredatorMovesTowardLeaderByCurrentObjective` expects a Predator to pick
    the first of two jointly largest parties. Q7 forbids it (S04d-002).
  - Four `is_majority_consistent` tests encode the non-standard definition (S04d-003). Q28 is open,
    so the outcome decides which of them change.
- **Of the 11 earlier P1 findings in this slice, none has a test that fails today**: 4 have no test,
  6 have a test that cannot catch the bug, 1 has a test asserting the wrong behaviour. There are no
  earlier P0s. Mutations that reproduce the P1 bugs (swapped x/y draw order) or other plausible defects
  (biased random tie-break, wrong k=2 approval, wrong lower-bound reflection, seat-share objective
  reading vote share, every experiment run using run-0 streams, 2D Hunter without a random draw)
  all **leave every existing test green**.
- **Two "reproducible from a seed" tests do not depend on the seed**:
  `HunterRoundIsReproducibleWithNamedStream` never touches a stream (1D Hunter reverses
  deterministically), and every experiment-runner test uses only Stickers, so the results are the same
  for any master seed (S04d-014).
- **Tests are deterministic and fast**: fixed seeds throughout, no clocks, no benchmark assertions,
  none of the 13 carries the `benchmark` label (CI's `ctest -LE benchmark` runs all of them), each
  runs in under 0.15 s. All 13 are registered with `add_test`. There are no disabled tests. No Heart
  code or tests, and no `hunter_sticker` test beyond a registry-size assertion (S04d-019), appear in
  this slice. No PR #1 fix touches this slice's code.

## Contracts this layer provides
For Stage 11 (test planning for the fixes) and later reviewers.

- **Registration and labels.** All 13 files are built as separate executables and registered as
  `ProfileTest`, `VoterSamplerTest`, `VotingRulesTest`, `AggregationPropertiesTest`,
  `AggregationIntegrationTest`, `CompetitorInitializationTest`, `CompetitionStrategiesTest`,
  `CompetitionMotionTest`, `SeatAllocationTest`, `CompetitionElectionFeedbackTest`,
  `CompetitionEngineTest`, `CompetitionTerminationTest`, `CompetitionExperimentRunnerTest`
  (CMakeLists.txt:223–321). None has a LABEL.
- **Randomness in tests.** Seeded tests only compare a seed with itself or apply statistical bounds
  (seed 2026 in the three statistical tests, seed 42 elsewhere). **No test in this slice has a
  golden value from the RNG**, so none breaks when Q4 changes the algorithms, and none can detect
  that they changed.
- **Fixtures that depend on current behaviour and must change with decided work:**
  - Q7: `test_competition_election_feedback.cpp:41–65`, `test_competition_strategies.cpp:230–252`.
  - Q8: `test_competitor_initialization.cpp:81–93` and the four Hunter tests that set
    `previous_round_metrics` directly (`test_competition_strategies.cpp:73–190`).
  - Q3: no test calls a `*_one_winner` function without an explicit `TieBreak`, so none needs to change, and none checks the new default.
  - Q26–Q33 (open): see S04d-026.
- **Dimension and metric coverage.** Aggregation tests use Euclidean with unit weights almost
  everywhere. All engine, termination and runner tests are 1D with Euclidean unit weights
  (S04d-022).
- **Helper duplication.** `point()`, `competitor()`, `make_condorcet_profile()` and
  `make_cycle_profile()` are re-declared in 6, 6, 2 and 2 files. A shared test header does not exist.

## Findings

### S04d-001 [P1, confirmed] Election-feedback test asserts lowest-index voter tie, seat tie and leader tie, all of which Q7/Q26 change
- **Where:** tests/unit/test_competition_election_feedback.cpp:41–65 (voter `point({1.0})` at line 51; expectations at 56–62). Decision: Q7 (Q26 open for the seat tie).
- **What:** The competitors are at 0, 2, 5 and one voter is at exactly 1.0, so that voter is equidistant (1.0 and 1.0) from competitors 0 and 2. The test expects `supporter_indices = {{0,1},{2},{3}}` and `vote_totals = {2,1,1}`, i.e. the voter goes to competitor 0 by lowest index. It also expects `seats = {1,1,0}` (two competitors tied on 1 vote, the seat goes to index 1) and `leader_ranking = {0,1,2}` (tied on votes, ranked by index). The tie looks accidental (the comment says nothing), but the expected values depend on it. Q7 requires equidistant voters to be split at random from a named stream.
- **Failure scenario:** Implementing Q7 as decided makes this test fail on half the seeds (the voter goes to competitor 2, giving `vote_totals = {1,2,1}`), and the test has no `StreamManager`. Mutation `m2` (ties go to the highest index): this test fails, and no other test in the 13 files notices (`test_competition_engine` 9, `termination` 7, `experiment_runner` 5 all pass). So it is the only test that locks the rule, and it does so by accident. A correct Q7 test is missing: two co-located competitors, many voters, split roughly equal, identical for a fixed seed (Q7 "Tests" list).
- **Suggested fix:** Move the voter to 0.9 (or 1.1) so the fixture is tie-free, and add separate explicit tie tests under Q7 (random split from a stream; tie-free run bit-identical before/after). Rewrite the seat and leader expectations per Q26.
- **Also affects:** none.

### S04d-002 [P1, confirmed] Predator test asserts that the first of two jointly largest parties is the target
- **Where:** tests/unit/test_competition_strategies.cpp:230–252. Decision: Q7 (Predator target ties); Q29 (open) concerns only the Predator tied with itself.
- **What:** Competitors 1 (at 3.0) and 2 (at -2.0) both have vote share 0.6; the Predator at 0 has 0.2. The test asserts heading `+1.0`, i.e. toward competitor 1, the lower index. The test name speaks of "the leader", suggesting the author did not mean it as a tie test. Q7 says the Predator's choice among equally large targets is uniform at random.
- **Failure scenario:** Mutation `m3` (`leader_index` picks the last maximum instead of the first): this test fails, and `test_competition_engine` still passes. After the Q7 change this test passes or fails by seed unless the fixture changes.
- **Suggested fix:** Give competitor 2 a share of 0.5 (unique leader) here, and add Q7 tests: two jointly largest targets are both chosen across seeds with roughly equal frequency, and a fixed seed repeats.
- **Also affects:** none.

### S04d-003 [P1, confirmed] Four tests encode `is_majority_consistent` as "is the Condorcet winner" (S02c-003); Q28 is open
- **Where:** tests/unit/test_aggregation_properties.cpp:202–247 (`IsMajorityConsistent_FalseForAll_Cycle`, `PluralityWinner_NeedNotBeMajorityConsistent`, `MajorityConsistency_ImpliesCondorcetConsistency`, and `IsMajorityConsistent_TrueForCW`/`_FalseForNonCW` as fixtures that cannot tell the definitions apart); also test_aggregation_integration.cpp:118, 181.
- **What:** I implemented the literature criterion (Felsenthal 2012, §3.2.1.3: if some alternative is first for more than n/2 voters it must win, otherwise any outcome satisfies it) in `$SCR4D/majority_ref.cpp` and evaluated the existing fixtures (`majority_ref.out`):
  - Cycle profile, w = 0, 1, 2: code returns 0, reference returns 1. The test expects 0 (false for all three).
  - Five-voter profile, w = A: code 0, reference 1. `PluralityWinner_NeedNotBeMajorityConsistent` expects false. Its name is also backwards for the literature definition: plurality satisfies the majority criterion.
  - `MajorityConsistency_ImpliesCondorcetConsistency` is the direction the S02c-003 finding calls backwards, and its loop runs on a profile where the two notions coincide, so it can never fail.
  - `TrueForCW` and `FalseForNonCW` agree under both definitions (A is first for 2 of 3 voters).
- **Failure scenario:** If Q28 is answered "redefine", `FalseForAll_Cycle` and `PluralityWinner_NeedNotBeMajorityConsistent` fail and must be inverted. If "rename", the tests keep passing but their names claim the wrong property and the assertions need new names. The test at line 229 also has a stream-of-consciousness comment ("wait let me recalc"). Because Q28 is unanswered, intended behaviour is unknown; severity follows the S02c-003 P1 under the rubric rule.
- **Suggested fix:** After Q28: add the 5-voter profile as the discriminating fixture; replace the implication test with "Condorcet winner ⟹ majority criterion satisfied".
- **Also affects:** none.

### S04d-004 [P1, confirmed] Hare tests use binary-exact quotients, so S02c-001 is invisible
- **Where:** tests/unit/test_seat_allocation.cpp:18–31; tests/unit/test_competition_election_feedback.cpp:67–87.
- **What:** Every Hare test has quotas that are exact in binary (200/5 = 40; 12 votes/seat; 15/3 = 5; 6/4 = 1.5 for equal parties). `{100,60,40}` with 5 seats does contain a remainder tie (0.5 vs 0.5) that is decided by the vote-total key, which is a useful test, but 2.5 and 1.5 are exact so rounding never decides it. (S02c-009 said no test has equal remainders; the worked example and the 2/2/2 election test do. The index tie at seat_allocation.h:114 is reached, but only through the election-feedback test, not `test_seat_allocation`.)
- **Failure scenario:** Scratch test `Missing.S02c001_HareExactRemainderTieUsesVotes` (`missing_tests.cpp`): `hare_largest_remainder_allocation({0,7,35}, 9)` returns `(0,2,7)`, expected `(0,1,8)`. Fails today (`seat_probe.out` shows the same values). All 5 existing seat tests pass.
- **Suggested fix:** Add that case and `({8980,57298,84238},160)` (both from S02c-001), plus an exhaustive small-range comparison against an integer-arithmetic reference.
- **Also affects:** none.

### S04d-005 [P1, confirmed] `pairwise_ranking` is tested only on a profile where it agrees with Copeland (S02c-002)
- **Where:** tests/unit/test_aggregation_integration.cpp:271–286.
- **What:** Both tests use `kCondorcetAlts`/`kCondorcetVoters`, where alternative A beats everyone. In that case wins-only and wins-minus-losses give the same first element. The two tests are also near duplicates (same input, same first-element assertion; one compares the index, one the point). `test_aggregation_properties.cpp:103` defers `pairwise_ranking` to these tests. No test has a pairwise tie, a cycle, or a score that depends on losses.
- **Failure scenario:** Scratch test `Missing.S02c002_PairwiseRankingIsWinsMinusLosses`: matrix with alt 1 beating alt 0, alt 0 beating alt 2, 1 vs 2 tied; Copeland order is (1,0,2), `pairwise_ranking` returns (0,1,2). Fails today.
- **Suggested fix:** After Q27, add that matrix, a 3-cycle (all scores equal) and the 8-voter spatial case from S02c-002 compared with `copeland_scores`.
- **Also affects:** none.

### S04d-006 [P1, confirmed] No test of out-of-range indices for the Condorcet and majority predicates (S02c-004)
- **Where:** tests/unit/test_aggregation_properties.cpp:170–247 (Pareto has out-of-range tests at 125–129 and 160–164; the Condorcet group has none).
- **What:** `is_condorcet_consistent` and `is_majority_consistent` get only valid indices. Coverage is 100% of lines because the bad index takes the same path.
- **Failure scenario:** Scratch test `Missing.S02c004_ConsistencyPredicatesRejectBadIndex`: `is_majority_consistent(cycle, 99)` and `is_condorcet_consistent(cycle, 99)` do not throw (both return true). Fails today.
- **Suggested fix:** Add `-1`, `n_alts`, `99` cases for both, mirroring the Pareto tests.
- **Also affects:** none.

### S04d-007 [P1, confirmed] Seeded tests cannot detect draw-order or portability changes (SCP-004, S02d-005, Q4)
- **Where:** tests/unit/test_profile.cpp:229–241, 287–295; test_voter_sampler.cpp:85–90, 110–115; test_competitor_initialization.cpp:134–163; test_competition_motion.cpp:74–91; test_competition_strategies.cpp:117–148.
- **What:** Every seeded test either compares a seed with itself, checks only ranges, or applies a statistical bound. None compares to a stored expected value. SCP-004 (x/y draw order unspecified) and the `std::` distribution sites in `initialization.h` (S02d-005) are exactly the kind of bug where "same seed twice" always passes.
- **Failure scenario:** Mutation `m1` (swap the x and y draws at `profile_generators.h:71`, `:112`, which is what a right-to-left compiler does): all 29 tests in `test_profile` and all 14 in `test_aggregation_integration` still pass. `order_probe.cpp` shows the same seed produces different rankings under the mutation (`021 201 102 …` vs `021 210 201 …`). `Gaussian_Reproducible`/`Uniform_Reproducible` in `test_voter_sampler` are likewise order-blind (and `voter_sampler.h` draws in a loop, so it is not affected by SCP-004, but no test would show a change).
- **Suggested fix:** Part of Q4: golden values for `uniform_spatial_profile`, `gaussian_spatial_profile`, `impartial_culture`, `draw_voters`, `break_tie` (random), the Hunter heading draw and `random_uniform` step sizes (see S04d-025 for the list). Include a test that draws x and y in a known order.
- **Also affects:** C API tests for the same functions (Stage 4e/4f).

### S04d-008 [P1, confirmed] No test for an Aggregator with no supporters (S02d-001)
- **Where:** tests/unit/test_competition_strategies.cpp:192–228; test_competition_engine.cpp:89–105, 169–209.
- **What:** All Aggregator tests give it at least one supporter. The empty-supporter branch is the bug, and `supporter_mean` returning zero runs only in untested conditions.
- **Failure scenario:** Scratch test `Missing.S02d001_AggregatorWithNoSupportersStandsStill`: Sticker at 5, Aggregator at 8, voters {4,5,6}; the Aggregator gets 0 votes and moves to 7 (expected 8). Fails today.
- **Suggested fix:** Add that test (1D and 2D, and mirrored to show the position does not depend on the origin).
- **Also affects:** none.

### S04d-009 [P1, plausible] Share-delta step test locks in the zero first step and nothing checks that anyone ever moves (S02d-002)
- **Where:** tests/unit/test_competition_motion.cpp:93–120. Decision: Q32 (open).
- **What:** `ProportionalStepUsesObjectiveDelta` (2.0 × |0.5 − 0.2| = 0.6, hand-checked) is a valid unit test of the formula. `ProportionalStepIsZeroOnFirstRound` asserts the first-round step is 0, which is the start of the fixed point S02d-002 describes. No test runs a multi-round engine with this policy.
- **Failure scenario:** A run with `kShareDeltaProportional` never moves; the suite is green. If Q32 changes the definition (gap to leader), `ProportionalStepIsZeroOnFirstRound` is likely to change. I did not write a scratch test, because the intended behaviour is undecided (Q32), so this is rated plausible for the test-side claim; the code-side finding is S02d-002.
- **Suggested fix:** After Q32, add a 50-round engine test that asserts a non-zero displacement, and revise the first-round test accordingly.
- **Also affects:** none.

### S04d-010 [P1, confirmed] Reflect boundary policy: one bounce at the upper bound is the only test (S02d-003)
- **Where:** tests/unit/test_competition_motion.cpp:136–141. Decision: Q30 (open).
- **What:** The single reflect test moves from 0.5 to 2.5 in [-1, 1]. Lower-bound reflection (boundary_policy.h:17), multiple bounces, large and infinite steps are untested. Slice-only coverage shows `boundary_policy.h:17` never runs.
- **Failure scenario:** Mutation `m8` (`value = lower;` instead of `lower + (lower - value)`): `test_competition_motion` (8 tests) and `test_competition_engine` (9) still pass. Scratch test `Missing.S02d003_ReflectHugeStepTerminates` (`apply_boundary_policy` with proposed 1e17) does not return within 8 s (`s02d003.out`, `exit=124`); it hangs today, so it must run under a timeout or after the fix.
- **Suggested fix:** Tests for lower-bound reflection, a step of exactly 2 × width, 1e6, 1e17 and `inf` (with a deadline), and that the result stays in the box.
- **Also affects:** none.

### S04d-011 [P1, confirmed] Signature test uses small magnitudes; cycle detection is tested only with a stationary state (S02d-004, S02d-013)
- **Where:** tests/unit/test_competition_termination.cpp:76–82, 135–150; engine.h:345–350.
- **What:** `StateSignatureRoundsDeterministically` uses coordinates 0 and 1.5, where 6 significant digits are enough. No test uses a coordinate with more than ~5 significant digits, or −0. The cycle test (`EngineStopsEarlyOnCycleSignature`) uses two Stickers, so the state is a fixed point and `kCycleDetected` fires after round 0 (this is S02d-013: a stationary state called a cycle, and the test asserts it). Slice-only coverage: engine.h lines 345–350 (push the signature, prune to `cycle_memory`) never execute, so the "not a cycle yet" path, a real period-2 cycle, and the memory window are all untested.
- **Failure scenario:** Scratch test `Missing.S02d004_SignatureKeepsFullResolutionAtLargeMagnitude`: signatures of 1234.5671 and 1234.5674 at resolution 1e-9 are both `"0:1234.57,|"`. Fails today.
- **Suggested fix:** Add that test, a −0/+0 test, a genuine period-2 cycle (for example an Aggregator oscillating 0, 1, 0, 1 under S02d-007 or two Hunters), a run that must not stop early, and a `cycle_memory` window test.
- **Also affects:** none.

### S04d-012 [P1, confirmed] No test that a run without a `StreamManager` is rejected, or that jitter without one is an error (S02d-006)
- **Where:** tests/unit/test_competition_engine.cpp:252–318; test_competition_motion.cpp (`nullptr` everywhere).
- **What:** Both jitter tests pass a `StreamManager`. Every other engine, motion and strategy test passes `nullptr`, which is how the code silently drops jitter. Q7 requires that a competition run cannot proceed without a `StreamManager`. The test at test_competition_strategies.cpp:172–190 (2D Hunter throws mid-run without a stream) is the only test of that requirement, and it pins the late failure, not an entry check.
- **Failure scenario:** Scratch test `Missing.S02d006_JitterWithoutStreamManagerIsRejected` (`jitter = 0.5`, `nullptr`, 3 rounds) does not throw. Fails today. Note this one depends on Q7 being applied at the core entry point, which the Q7 Implies list covers only at the C API; the test records what the earlier report recommends.
- **Suggested fix:** After the core entry point requires a stream manager, rewrite the `nullptr` tests to pass one, and add the missing-manager error test.
- **Also affects:** none.

### S04d-013 [P2, confirmed] Test asserts that a profile with zero alternatives is accepted (S02c-007)
- **Where:** tests/unit/test_profile.cpp:78–85 (`EmptyAlternatives_EmptyRankings`); test_profile.cpp:35–40 (`EmptyProfile_IsWellFormed` is fine, it builds the struct by hand).
- **What:** The test requires `build_spatial_profile({}, voters, cfg)` to succeed with `n_alts = 0`, which is the behaviour S02c-007 recommends rejecting. The uniform and Gaussian generators' alternatives check is also untested (no `{}` alternatives test).
- **Failure scenario:** After the fix this test fails and must become an `EXPECT_THROW`.
- **Suggested fix:** Change to a throw test with the end-user message check; add the same for the generators and `profile_from_utility_matrix` with 0 columns.
- **Also affects:** none.

### S04d-014 [P2, confirmed] "Reproducible from a seed" tests in the engine and runner do not depend on the seed
- **Where:** tests/unit/test_competition_engine.cpp:128–159; test_competition_experiment_runner.cpp:73–113.
- **What:**
  - `HunterRoundIsReproducibleWithNamedStream` uses a 1D Hunter. A 1D Hunter reverses deterministically and the step is fixed, so neither stream is read. `vacuous.out`: for seeds 2026, 1 and 987654321 the heading is −1 and the position −1, and both streams are untouched.
  - Every runner test uses two Stickers, which never draw. `vacuous.out`: master seeds 2026, 1 and 987654321 give the same `mean_final_vote_shares[0] = 0.666667` and `mean_rounds = 3`. `RepeatedExperimentIsReproducibleFromMasterSeed` compares `run_seed` values from two identical calls, so it cannot fail unless the function is non-deterministic.
- **Failure scenario:** Mutation `m11` (every run calls `reset_for_run(master_seed, 0)`, so all runs share streams): all 5 runner tests pass. The runner also does not test that run seeds differ across runs or against a hand value. `seeds.out` shows S02a-002 through the runner: `run_seed_for_index(0, 1) == run_seed_for_index(1, 0)` and `(2026, 1) == (2027, 0)`; nothing asserts distinctness. The documented contract "run i can be reproduced alone with `reset_for_run(master_seed, i)`" is untested, as are the allowlist (a custom strategy using another stream must throw) and Q31 (common random numbers in sweeps).
- **Suggested fix:** Use a 2D Hunter or `random_uniform` steps in the runner fixtures, assert that runs differ from each other and that run i alone matches run i of the experiment, and assert the seeds against independently computed values.
- **Also affects:** none.

### S04d-015 [P2, confirmed] Large parts of the strategy code are never executed or never checked for randomness
- **Where:** tests/unit/test_competition_strategies.cpp:73–190; slice-only coverage `strategy_registry.h` 74.0%, `adaptation.h` 71.1%.
- **What:**
  - 2D Hunter backward half-space: the test draws once and checks unit norm and `dot <= 0`. Mutation `m12` (always return `−forward`, ignoring the draw) passes `test_competition_strategies` (10) and `test_competition_engine` (9). The uniform-direction claim (L&S heading + 90 + random 180) is never checked.
  - Seat-share objective: `adaptation.h:41–42` never executes in any test (`objective_value` for `kSeatShare`); mutation `m10` (seat-share objective reads vote share) passes all 4 competition suites that use it. This is the objective that Q8's satisficing replacement for `hunter_sticker` will rely on.
  - Hunter fallback branches (no usable heading draws a random direction, 2D static support, 2D improvement) and `strategy_registry.h` lines 98–106, 139–147 are untested. Registry duplicate-registration errors (lines 32–43) and `leader_index` missing-metrics error (adaptation.h:95) are untested.
  - `kHunterSticker` (strategy_registry.h:164–230, about 65 lines) has no test; it is to be removed under Q8, so no test should be added.
- **Failure scenario:** The mutations above.
- **Suggested fix:** Check the Hunter direction over many draws (mean of the dot product with the forward heading near −2/π in 2D, all `dot ≤ 0`); test both objectives; test the fallback and error branches.
- **Also affects:** none.

### S04d-016 [P2, confirmed] Several assertions are too weak to detect a wrong implementation
- **Where:**
  - test_voting_rules.cpp:145–156 (`Plurality.OneWinner_Random_EqualProbability`).
  - test_voting_rules.cpp:340–362 (approval top-k).
  - test_voting_rules.cpp:368–405 (antiplurality).
  - test_voter_sampler.cpp:75–115.
  - test_aggregation_integration.cpp:292–303.
  - test_profile.cpp:243–261 and test_aggregation_integration.cpp:228–242.
- **What / scenarios:**
  - **Random tie-break.** 3000 draws, bounds 700–1300, where the standard deviation is about 26 (±12σ). Mutation `m4` (10% extra weight on the lowest index, so P = 0.40 instead of 0.33): counts 1168/912/920, test passes (`m4probe.cpp`).
  - **Approval top-k.** Only k = 1 and k = n_alts are checked. Mutation `m7` (k = 2 counts one choice) passes all 36 tests in `test_voting_rules`.
  - **Antiplurality.** `Scores_ScoreSum` checks only the sum, and `PluralityWinner_CanLoseAntiplurality` asserts only `plur_w != anti_w` even though the comment computes the expected scores (A = 2, B = 3, C = 1). Values are caught indirectly by `ScoringRule.RecoverAntiplurality`.
  - **Voter sampler.** Gaussian output is checked only for finiteness and length, so a variance-for-stddev bug passes. There is no test of the sample mean or variance, none for `d = 3`, none for NaN or infinite `hi`, and none for a hand-built `VoterSamplerConfig` with an unknown kind (voter_sampler.h:96–97 never runs; the validators can be bypassed because the struct is public).
  - **Uniform profile.** `Integration_UniformProfile.RunsWithoutError` is `EXPECT_NO_THROW` only (rubric: not a value assertion).
  - **Statistical tests.** The three fixed-seed statistical tests (chi-square at α = 1%, the Borda mean within 5%, tie-break counts) pass or fail deterministically for seed 2026. After Q4 regenerates the draws each has some chance of failing for reasons unrelated to correctness.
- **Suggested fix:** Use a chi-square or binomial test with a bound near 4σ on the random tie-break; add k = 2 on a profile with hand-computed scores; assert antiplurality values; add a mean/variance test for the Gaussian sampler; replace `RunsWithoutError` with assertions on the rankings.
- **Also affects:** none.

### S04d-017 [P2, confirmed] Termination and step/engine validation branches are untested (S02d-010)
- **Where:** slice-only coverage `termination.h` 60.3% (lines 42–62, 69–78, 111–112 never run), `competition_types.h` 64.3% (lines 74, 84, 93–96, 109–112, 116–147 never run), `engine.h` lines 98–145, `state_signature.h:18`, `step_policy.h:52–55, 65`.
- **What:** Not one test checks that a bad configuration is rejected: negative `max_rounds`, empty competitors or voters, a dimension mismatch, `lower >= upper`, negative or too-large `jitter`, `min > max` step, negative epsilons, a non-positive resolution, a missing `StreamManager` for `random_uniform`. These messages are part of the end-user error contract. S02d-010 (NaN and Inf accepted) is in this group.
- **Failure scenario:** Scratch test `Missing.S02d010_ValidationRejectsNaNAndInf`: `validate_step_policy` accepts NaN and +Inf for `fixed_step_size`. Fails today.
- **Suggested fix:** One parametrized test per validator listing every invalid field (including NaN, ±Inf) and checking the message contains the field name and the value entered.
- **Also affects:** none.

### S04d-018 [P2, confirmed] Seat-allocation tests miss every error branch and pin lowest-index ties that Q26 has left open
- **Where:** tests/unit/test_seat_allocation.cpp:13–37; test_competition_election_feedback.cpp:67–87. Decision: Q26 (open).
- **What:** Slice-only coverage `seat_allocation.h` 80.2%: all six `throw` branches (negative seats, negative votes, top-k with seats > n; lines 32–45, 69–77) never execute. Top-k boundary tie (`{10,7,7,2}, 2 → {1,1,0,0}`), Hare 2/2/2 with 4 seats (`{2,1,1}`) and the all-zero fallback (`{2,2,1}`) assert the "first listed wins" final key. `{50,50}, 1` gives `(1,0)` (`seat_probe.out`) and is not tested. The `{0,0,0}, 5` fallback with `seat_count > n` is tested; with `seat_count` much larger than the number of parties it is not.
- **Failure scenario:** Whichever of Q26 options (a), (b) is chosen, `PluralityTopKAssignsSeatsToHighestVoteTotals` and `TwoDimensionalHareFeedbackMatchesExpectations` change. Option (c) keeps them.
- **Suggested fix:** Add the error tests; mark the tie tests as "tie rule pending Q26" so they are changed with the decision.
- **Also affects:** none.

### S04d-019 [P2, confirmed] Registry test and Hunter fixtures depend on `hunter_sticker` and on `previous_round_metrics` (Q8)
- **Where:** tests/unit/test_competitor_initialization.cpp:81–93 (`EXPECT_EQ(registry.size(), 5u)` with the comment "…hunter_sticker"); test_competition_strategies.cpp:73–190; test_competition_engine.cpp:135–138, 232.
- **What:** Q8 removes `kHunterSticker`, so the size assertion contradicts an answered question. The test also lives in the initialization file though it tests the registry. Q8 also changes how a Hunter remembers its last move: four Hunter tests build their fixture by writing `previous_round_metrics`/`current_round_metrics` directly, using positional aggregate initialization `CompetitorRoundMetrics{0.6, 0.0, 0, 0}`, which breaks as soon as a field is added (comfort metric, support-before-move). No test asserts that a standing-still competitor keeps its heading (Q8 Implies), so nothing in this slice contradicts that part.
- **Failure scenario:** Removing `kHunterSticker` fails this test with `4u != 5u`; changing the Hunter memory invalidates the four fixtures.
- **Suggested fix:** Replace the count with a check that the four documented IDs resolve and `"hunter_sticker"` does not; build Hunter fixtures through a helper that sets the new memory field.
- **Also affects:** none.

### S04d-020 [P2, confirmed] No test of the default tie-break (Q3)
- **Where:** all of tests/unit/test_voting_rules.cpp, test_aggregation_properties.cpp, test_aggregation_integration.cpp.
- **What:** Every call passes `TieBreak::kSmallestIndex` explicitly (or `kRandom` with a PRNG). Q3 makes `smallest_index` the default for `*_one_winner`, `borda_ranking`, `rank_by_scores`, `pairwise_ranking`. Nothing asserts that calling them without a tie-break argument succeeds on a tie.
- **Failure scenario:** Scratch test `Missing.Q3_DefaultTieBreakIsSmallestIndex` (`plurality_one_winner(cycle_profile)`): throws today ("unknown file: Failure", `kRandom` with null PRNG). Fails today.
- **Suggested fix:** One default-argument test per function listed in Q3 Implies.
- **Also affects:** none.

### S04d-021 [P2, confirmed] No engine test asserts a multi-round trajectory with hand-computed values
- **Where:** tests/unit/test_competition_engine.cpp:68–318; test_competition_termination.cpp; test_competition_experiment_runner.cpp.
- **What:** The engine tests are single-round (`max_rounds = 1`), all-Sticker, or only check that something differs or is frozen. No test follows a Hunter, Aggregator or Predator for several rounds to an expected position. That is why S02d-001, S02d-007 and S02d-008 are all invisible:
  - `SynchronousAggregatorUsesCurrentRoundSupporters` has step 0.5 below the distance to the centroid (0.866), so it cannot show the overshoot (S02d-007). Scratch test `Missing.S02d007_AggregatorDoesNotOvershootCentroid` (Aggregator at 0, voters 0.2/0.3/0.4, step 1) gets position 1.0, outside [0.2, 0.4]. Fails today.
  - All runner and termination fixtures are Stickers, so positions never move and the "final shares" belong to the same positions as the "final positions" (S02d-008 cannot show).
  - The two jitter tests check qualitatively that steps are not integers, not the range; additive vs multiplicative jitter (R docs say additive, code is multiplicative) is not distinguished.
- **Suggested fix:** Add hand-computed 1D trajectories (Aggregator jumps or steps to the mean; Predator chases the leader; Hunter reverses on a loss) and a final-shares-match-final-positions test (after the S02d-008 decision).
- **Also affects:** none.

### S04d-022 [P2, confirmed] Distance configuration and dimension coverage is narrow
- **Where:** all competition tests; test_voting_rules.cpp:44 (`kEuc{}`); test_profile.cpp:29.
- **What:** Every aggregation test uses Euclidean with unit weights; no test builds a spatial profile or an approval profile with Manhattan, Chebyshev, Minkowski p, or unequal weights, or with a weight of 0 (Q18: weight 0 means "ignore this dimension" in distance). Every engine, termination and runner test is 1D; the only 2D tests are two Hare/Aggregator/Hunter unit tests. No test uses a large (1e4) or small (1e-4) coordinate scale, which Q2 and Q10 both ask for (Q10: scale invariance). No NaN or Inf input is tested for `build_spatial_profile`, `rank_by_scores` (S02c-006 loops forever on NaN, so such a test must run under a timeout), or the competition engine.
- **Failure scenario:** A defect in weighting or in the 2D path of `build_spatial_profile_nd` (S02d-011, a copy that has drifted) would pass the suite.
- **Suggested fix:** Parametrize the profile and election tests over the metric types and weights; add one 2D engine integration test (Hunter with a seeded stream); add the Q10 scale-invariance test.
- **Also affects:** none.

### S04d-023 [P2, confirmed] Other earlier P2 findings with no test that could fail
- **Where:** see the Findings map.
- **What:** No test could catch S02c-005 (scoring rule order dependence: the three scoring tests use exactly representable weights), S02c-006 (NaN loops forever), S02c-008 (equidistant voters give different answers via profile and geometry: the integration tests use tie-free fixtures), S02c-012 (non-square matrix in `pairwise_ranking`, and `Empty_ReturnsEmpty` asserts the code, contradicting the header's `@throws`), S02d-008, S02d-010, S02d-011 (`build_spatial_profile_nd` has no direct test and no dimension, empty-input or weight-mismatch error test; lines 50–81 never run), S02d-012 (seat ties) and S02d-018 (`seat_count` greater than active competitors; `evaluate_election_feedback` negative seats at election_feedback.h:130 never runs). Also `borda_ranking` with `kRandom`, `rank_by_scores` with a random shuffle and `break_tie` with an empty set never execute (borda.h:121–127, social_ranking.h:69, tie_break.h:51).
- **Failure scenario:** each is the S02c/S02d scenario already reported.
- **Suggested fix:** Add a test per item when the corresponding finding is fixed; the `kRandom` ranking tests belong with the Q4 portable shuffle.
- **Also affects:** none.

### S04d-024 [P3, confirmed] Naming, duplication and stale comments
- **Where and what:**
  - Typo `Random_NeedsProng` / `OneWinner_Random_NeedsProng` (test_voting_rules.cpp:139, test_aggregation_properties.cpp:85).
  - Unused `PRNG prng(1)` at test_voting_rules.cpp:132.
  - `Plurality.CondorcetNeedNotBePluralityWinner` (test_voting_rules.cpp:169) asserts that the Condorcet winner IS among the plurality winners, so it does not show what its name says.
  - Suite `Integration_Fishburn` (test_aggregation_integration.cpp:198) is named for a result that the file's own comment says does not hold.
  - The file header lists (f) Condorcet agree and (g) pairwise ranking, while the sections are labelled (f) pairwise and (g) uniform profile; (e) claims "Pareto ⊆ convex hull" but tests something else.
  - `Integration_PairwiseRanking.CopelandWinner_RankedFirst` and `.ConsistentWithCopelandWinner` are one test written twice.
  - `Integration_CondorcetWinner.IsCondorcetConsistent_AllRules` tests no rules.
  - The "wait let me recalc" comment at test_aggregation_properties.cpp:233.
  - `Pareto.Set_CycleProfile_AllEfficient` states a general claim (S02c-011) but covers 3 alternatives only.
  - `make_condorcet_profile`, `make_cycle_profile`, `point()`, `competitor()` are re-declared in 2 to 6 files. The registry tests sit in `test_competitor_initialization.cpp`.
  - `std::numeric_limits` used without `<limits>` (test_voter_sampler.cpp:38), `std::abs` without `<cmath>` (test_competition_engine.cpp:281). They compile through transitive includes.
  - `StateSignatureIsLocaleIndependent` replaces the global locale and restores it by hand (no RAII); fine in a single-threaded binary, fragile if an assertion is changed to `ASSERT`.
  - `EmptyProfile_IsWellFormed` (test_profile.cpp:35) is fine but sits beside S04d-013.
- **Failure scenario:** none; clarity only.
- **Suggested fix:** Rename; add a shared `tests/unit/test_helpers.h`; delete the duplicate test.
- **Also affects:** none.

### S04d-025 [P3, confirmed] Seeded sites in this slice that need Q4 golden-value tests (additional instances)
- **Where:** Decision: Q4 (the Implies list names the golden-value test but not these sites).
- **What:** Draw paths with no value-level test: `uniform_spatial_profile`, `gaussian_spatial_profile` (the two x/y emplace sites), `impartial_culture` (one shuffle per voter), `draw_voters` (uniform and normal), `break_tie` random, `borda_ranking`/`rank_by_scores` shuffle (both untested: borda.h:126, social_ranking.h:69), `random_unit_heading` (2D Hunter heading, and 1D ±1), `random_in_backward_halfspace`, `initial_step_size` and `random_uniform` step, `initialize_competitors` (the eager `value_or` draws, S02d-016). The statistical seed-2026 tests (S04d-016) will need re-checking after regeneration.
- **Failure scenario:** none today; this is the list for the Q4 test work.
- **Suggested fix:** One golden-value case per item, exact for uniform/integer/shuffle, last-bit tolerance for normal.
- **Also affects:** none.

### S04d-026 [P3, confirmed] Tests that pin behaviour awaiting an open owner question
- **Where and what (all will need a decision, then an edit):**
  - Q26 seat ties: `PluralityTopKAssignsSeatsToHighestVoteTotals`, `HareLargestRemainderAllZeroVotesFallsBackDeterministically`, `TwoDimensionalHareFeedbackMatchesExpectations` (S04d-018).
  - Q27/Q28: `IsMajorityConsistent_*` and the pairwise-ranking tests (S04d-003, -005).
  - Q29: no test for a Predator tied with itself (only the unique-leader case at test_competition_strategies.cpp:259–282, which stays correct under both options).
  - Q30: no test that the heading flips (or does not) after a reflection.
  - Q32: `ProportionalStepIsZeroOnFirstRound` (S04d-009).
  - Q33: no test of a Hunter reusing its step length under random steps.
  - Q31: no test of common random numbers across a sweep (`SweepRunsMultipleScenarios` uses scenarios with the same seed and checks only labels and `num_runs`).
- **Failure scenario:** none today.
- **Suggested fix:** Revisit each test when the question is answered.
- **Also affects:** none.

## Findings map

Statuses: **wrong-behaviour test** = a test asserts the wrong behaviour (the fix must change it);
**cannot catch** = a test exists but cannot catch it; **no test** = gap; **fixed and tested** = PR #1
items (none touch this slice).

| Earlier finding | Severity | Status | Test name or note |
|---|---|---|---|
| S02c-001 Hare remainders in double | P1 | cannot catch | all Hare tests binary-exact; S04d-004 (scratch test fails) |
| S02c-002 `pairwise_ranking` is not Copeland | P1 | cannot catch | `Integration_PairwiseRanking.*` use a Condorcet-winner profile; S04d-005 |
| S02c-003 `is_majority_consistent` definition | P1 | wrong-behaviour test | `IsMajorityConsistent_FalseForAll_Cycle`, `PluralityWinner_NeedNotBeMajorityConsistent`, `MajorityConsistency_ImpliesCondorcetConsistency`; Q28 open; S04d-003 |
| S02c-004 out-of-range indices return true | P1 | no test | S04d-006 |
| SCP-004 voter coordinate draw order | P1 | cannot catch | `UniformSpatialProfile.Reproducible_SameSeed`, `GaussianSpatialProfile.*`; mutation m1 survives; S04d-007 |
| S02d-001 Aggregator with no supporters | P1 | no test | S04d-008 |
| S02d-002 share-delta step never moves | P1 | cannot catch (and the first-round zero is locked in) | `ProportionalStepIsZeroOnFirstRound`; Q32 open; S04d-009 |
| S02d-003 `kReflect` hangs | P1 | no test | `ReflectBouncesBackIntoBounds` (one bounce); S04d-010 |
| S02d-004 signature 6 digits | P1 | cannot catch | `StateSignatureRoundsDeterministically` (small values); S04d-011 |
| S02d-005 `std` distributions in initialization | P1 | cannot catch | `RandomInitializationIsReproducibleAcrossStreamManagers` (same seed only), `RandomUniformStepIsSeededAndReproducible`; Q4; S04d-007 |
| S02d-006 core engine without `StreamManager` | P1 | no test | jitter tests pass a manager; S04d-012 |
| S02c-005 scoring rule not anonymous | P2 | no test | weights in the 3 scoring tests are exact; S04d-023 |
| S02c-006 `rank_by_scores` loops on NaN | P2 | no test | S04d-023 (run under a timeout) |
| S02c-007 zero alternatives accepted | P2 | wrong-behaviour test | `BuildSpatialProfile.EmptyAlternatives_EmptyRankings`; S04d-013 |
| S02c-008 profile vs geometry on equidistant voters | P2 | no test | integration fixtures are tie-free; S04d-023 |
| S02c-009 seat tests miss error branches | P2 | no test | still 5 tests, six `throw` lines unexecuted; correction recorded in S04d-004/-018 |
| S02c-010, -011, -013 docs | P3 | n/a | docs only; `Pareto.Set_CycleProfile_AllEfficient` covers 3 alternatives (see S04d-024) |
| S02c-012 `social_ranking.h` contract text | P3 | wrong-behaviour test (mild) | `RankByScores.Empty_ReturnsEmpty` asserts the code against the header's `@throws`; no non-square test |
| S02d-007 Aggregator overshoots | P2 | cannot catch | `SynchronousAggregatorUsesCurrentRoundSupporters` (step < distance); scratch test fails; S04d-021 |
| S02d-008 final shares vs final positions | P2 | cannot catch | all fixtures Stickers; S04d-021 |
| S02d-009 trace memory | P2 | no test | not a behaviour test; no memory or size check anywhere in the slice |
| S02d-010 NaN/Inf validation | P2 | no test | S04d-017 (scratch test fails) |
| S02d-011 `build_spatial_profile_nd` duplicate | P2 | no test | no direct test; lines 50–81 unexecuted; S04d-023 |
| S02d-012 seat ties lowest index | P2 | wrong-behaviour test | Q26 open; `TwoDimensionalHareFeedbackMatchesExpectations`, top-k tie in `test_seat_allocation`; S04d-018, -001 |
| S02d-013 stationary state is a "cycle" | P3 | wrong-behaviour test | `EngineStopsEarlyOnCycleSignature` (test_competition_termination.cpp:135–150); S04d-011 |
| S02d-014 tied Predator moves | P3 | wrong-behaviour test | `PredatorMovesTowardLeaderByCurrentObjective` (two tied non-self leaders, expects index 1); S04d-002 |
| S02d-015 docs | P3 | n/a | docs only |
| S02d-016 `initialize_competitors` eager draws, `kZero` heading | P3 | no test | tests do not check draw alignment or the un-normalised `kZero` heading; S04d-025 |
| S02d-017 `find` `noexcept` | P3 | no test | not testable without an allocation failure |
| S02d-018 `seat_count` vs active competitors | P3 | no test | S04d-023 |
| Q7 voter tie / Predator tie (decided) | P1 | wrong-behaviour test | S04d-001, S04d-002 |
| Q8 `hunter_sticker` removal (decided) | P2 | wrong-behaviour test | `BuiltInRegistryContainsBaselineStrategies` (count 5); S04d-019 |
| Q3 default tie-break (decided) | P2 | no test | S04d-020 |

Count of earlier P0/P1 findings in this slice with no test that could catch them today: **11 of 11**
(0 P0; P1: 4 no test, 6 cannot catch, 1 wrong-behaviour test). Fixed and tested: none (PR #1 did not
touch this slice).

## Tool leads checked
- Shared `coverage_report.txt` for this slice → **unreliable for three headers**; re-measured from the 13 slice binaries (see the header of this report). `profile_generators.h` 62.9% shared vs 88.2% slice; `adaptation.h` 54.8% vs 71.1%; `competition_types.h` 16.7% vs 64.3%.
- `profile_generators.h` slice-only gaps (lines 50–52, 96–98: `n_voters < 1` throws for uniform and Gaussian) → S04d-013 / S04d-023 (untested error paths; `impartial_culture` throws are tested).
- `borda.h` 86.8% (lines 121–127), `social_ranking.h` 95% (line 69), `tie_break.h` 88% (line 51), `pareto.h:36–38` → S04d-023 (untested `kRandom` ranking paths, empty tie set; the Pareto `rank_of` throw is unreachable for constructed profiles, not a defect).
- `voter_sampler.h` lines 96–97 (unknown kind) → S04d-016 (reachable only through a hand-built config; the 51% in Stage 2c was a merge artifact, slice-only is 95.6%).
- `seat_allocation.h` 80.2% (six throw branches) → S04d-018. Correction to S02c-009: the index tie at line 114 and equal remainders do run, through `test_competition_election_feedback` and `HareLargestRemainderMatchesWorkedExample`, but not through `test_seat_allocation`'s own tie-free fixtures, and none is rounding-sensitive.
- `engine.h` lines 98–145 (six validation throws), 268–270, 345–350 → S04d-017, S04d-011 (the cycle-window path never runs).
- `termination.h` 60.3% (lines 42–62, 69–78, 111–112) and `state_signature.h:18` → S04d-017.
- `competition_types.h` (lines 74, 84, 93–96, 109–147) → S04d-017; line 84 is the `hunter_sticker` ID, see S04d-019.
- `adaptation.h:41–44` (seat-share objective), `:50–70` (context validation), `:82–84`, `:95–97` → S04d-015, S04d-017.
- `strategy_registry.h` 74.0% (lines 98–106, 139–147, 155, 162, 164–230) → S04d-015; `kHunterSticker` block is withdrawn under Q8.
- `boundary_policy.h:17, 46, 54, 65` → S04d-010 (lower-bound reflect, `kStayPut` in-bounds return and 2D loop).
- `step_policy.h:52–55, 65` (random-uniform without manager; the `default` return) → S04d-017.
- `election_feedback.h` lines 50–81, 130–132, 170–173 → S04d-023.
- `experiment_runner.h` lines 98–100, 121–123, 205–216 (`num_runs < 1`, run without final metrics, empty sweep, empty label) → S04d-014 / S04d-017 (untested validation).
- `initialization.h` lines 30–41, 92–96, 107–128 → S04d-017, S04d-025 (the `kShareDeltaProportional` initial step returns 0, consistent with S02d-002).
- Benchmark/timing lead: none of the 13 files uses a clock or timing assertion, none is labelled `benchmark` (only `DistanceSpeedTest` and `PerformanceComparisonTest` carry it) → not a defect.
- Order dependence/shared state: every test builds its own objects; the only globals are the `static const DistConfig kEuc` and the profile/point constants (immutable) and the locale swap in one test, restored → not a defect.
- Heart in this slice → none (no `heart.h` include or test in the 13 files); nothing to list as P3.
- Q10 (bounds) → no test in this slice contradicts it (all fixtures are inside their bounds); the Q10 test items (explicit bounds, scale invariance, zero-range fallback, outside-bounds error) are not yet written; this slice contains only the C++ engine layer, so they belong mostly to the C API and bindings.
- Q18 → no zero-weight test in this slice (aggregation and competition use unit weights); no contradiction.

## Open questions
1. **Q28** (open) decides whether S04d-003's tests are rewritten or renamed. **Q26, Q27, Q29–Q33** (open)
   decide the tests listed in S04d-026; nothing new for the owner beyond those.
2. For the test plan only: the three fixed-seed statistical tests (S04d-016) have no robust fix
   without a policy. Options: multi-seed tests with a bound near 4σ and a documented seed, or
   deterministic golden values only. This is a team choice, not an owner decision about behaviour.

## Out-of-slice notes
- `seeds.out`: `run_seed_for_index(0, 1) == run_seed_for_index(1, 0)` (S02a-002 reaching the experiment
  runner); the runner tests do not exercise it (S04d-014). Not a new core finding.
- `scs_competition_run` and the bindings can start 2D Hunter runs where the only protection against
  S02d-006 is the C API; no C++ test covers a missing manager (S04d-012).
- The scratch tests in `$SCR4D/missing_tests.cpp` are a starting point for Stage 11; they are not
  in the repository, and the S02d-003 case hangs today and needs a timeout.
