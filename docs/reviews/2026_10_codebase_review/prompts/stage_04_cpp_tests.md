# Stage 4 prompt — C++ tests

**Model:** Sonnet, high effort. **Agents:** 6, one per slice (4a–4f), run in parallel.
Each agent is told which slice it owns; everything above "Slices" applies to all.

**Before launch (review session):** from the commit below, outside the repo's `build/`,
build into the scratchpad:
- `$SCR/stage4/build_release`: Release, all targets; run `ctest --output-on-failure` once
  and save the output to `$SCR/stage4/ctest_release.txt`.
- `$SCR/stage4/build_cov`: Debug with `-fprofile-instr-generate -fcoverage-mapping`, as in
  Stage 0 §5; run `ctest`, merge the profiles, and save `llvm-cov report` to
  `$SCR/stage4/coverage_report.txt` and per-file uncovered lines (`llvm-cov show
  -show-line-counts-or-regions`, limited to `include/socialchoicelab/` and `src/`) to
  `$SCR/stage4/coverage_show/`.
Agents use these read-only.

---

You are running Stage 4<slice> of a staged code review of socialchoicelab
(`/Users/robiragan/Dropbox/SocialChoiceStudioDev/socialchoicelab`), commit `2771655`. Your
slice is part of the C++ GoogleTest suite in `tests/unit/`. You review **the tests, not the
code under test**: do they check the right things, would they catch the bugs already found,
and do any of them lock in behaviour the owner has decided is wrong?

## Read first, in this order

1. `docs/reviews/2026_10_codebase_review/review_rubric.md`: rules, severity, finding
   format, the **Owner decisions** section and the **Tests** checklist. Follow it exactly.
2. `docs/reviews/2026_10_codebase_review/owner_questions.md`: the cross-cutting principle
   and every **answered** question (Q1–Q12, Q18), including "For reviewers" and "Implies".
   Unanswered questions (Q13–Q17, Q19–Q33) are open, not defects.
3. `docs/reviews/2026_10_codebase_review/stage_01_architecture_map.md`, sections 1–7.
4. The **Findings** and **Contracts** sections of the earlier reports listed for your slice,
   plus `copilot_findings.md`. You need their finding IDs and failure scenarios; do not
   re-review the code under test.
5. `docs/reviews/2026_10_codebase_review/stage_00_baseline.md`: test results (S00-001)
   and coverage, for context. Coverage there is from an older commit; use the fresh report
   below.
6. The fresh coverage for the files your tests target:
   `$SCR/stage4/coverage_report.txt` and `$SCR/stage4/coverage_show/`, where
   `SCR=/private/tmp/claude-501/-Users-robiragan-Dropbox-SocialChoiceStudioDev/2402f97e-221e-494b-88cb-fc63c41efc50/scratchpad`.
   `$SCR/stage4/ctest_release.txt` has the current test results.

Then read **every line of your slice's test files** in full. Read code under test only to
settle a specific question.

## How to work

- **Assertions.** Each public function has tests whose assertions check values, not just
  "no error" or "no throw". Tolerances fit the magnitudes (S00-001 is a 1e-15 tolerance
  below double spacing). Expected values come from an independent source (hand
  calculation, the paper, a known case), not from the code's own output pasted back in;
  say which you suspect and why.
- **Edge cases** from the rubric checklist (empty input, one voter or candidate, ties,
  duplicate or collinear points, NaN/Inf, extreme magnitudes, integer limits, invalid
  arguments): which are tested, which are missing.
- **Determinism.** Fixed seeds; no dependence on time, test order or shared state; timing
  or benchmark assertions that can fail on a slow machine (CI runs `ctest -LE benchmark`;
  check which tests carry that label).
- **Findings map (most important).** For every P0 and P1 finding in your slice's earlier
  reports, and every SCP item in your area, state one of: *a test asserts the wrong
  behaviour* (name it; the fix must change it), *a test exists but cannot catch it* (why),
  *no test* (gap), or *fixed and tested* (PR #1 items). Do the same, briefly, for P2s.
  Stage 11 uses this table to plan tests for the fixes.
- **Owner decisions.** Tests that contradict an answered question are findings (e.g. a test
  that expects Chebyshev at p ≥ 100 contradicts Q2; a test calling 0.5 "simple majority"
  touches Q6). Heart tests are withdrawn code (Q1): list them once as P3.
- **Coverage.** Use the fresh coverage to name untested branches that matter (error paths,
  boundary cases), not to restate percentages.
- **Hygiene.** Duplicated tests, dead or disabled tests, misleading test names, tests that
  test the wrong function, files not registered with `add_test`.
- **Demonstrate when you can.** Write scratch GoogleTest files or small programs under
  `$SCR/stage4<slice>/`, compiled against the repo headers (and `libscs_api` from
  `$SCR/stage4/build_release` for C API slices). Typical uses: show that a test still
  passes when the behaviour it claims to check is wrong, or that a missing test would fail
  today. A demonstrated finding is `confirmed`; include the file path and output.

Severity for test findings: a test that asserts behaviour that is a P0/P1 bug, or a P0/P1
bug with no test that could catch it, is **P1**; other gaps and weak assertions are P2;
naming and duplication are P3. Do not re-report the underlying code bug; cite its ID.

## Hard constraints

- Do not modify any repository file except your output file. Do not commit. Do not use or
  rebuild the repository's `build/` or the shared Stage 4 builds.
- Do not run `scripts/check-all.sh`, `lint.sh format` or `check-bindings.sh` (they write
  to the repo).
- Never read or print `.env` or `.github_token`.
- Do not read `old_codebases/` or `agent_conversations/`.
- Do not start sub-agents.
- If your slice turns out too large to read in full, stop and say which parts you did not
  read rather than skimming.

## Output

Write `docs/reviews/2026_10_codebase_review/stage_04<slice>_<name>.md`, using the
structure in the rubric, plus after Findings:

```
## Findings map
| Earlier finding | Severity | Status: wrong-behaviour test / cannot catch / no test / fixed and tested | Test name or note |

## Tool leads checked
One line per coverage gap or Stage 0 lead you checked: lead → finding ID / not a defect (why).
```

When done, reply with: your Summary section, the list of finding IDs with severity and
title, the count of earlier P0/P1s with no test, and any open questions for the owner.

---

## Slices

Line counts are at `2771655`. They replace stage_01 §9, which predates PR #1.

### 4a — core and preference tests → `stage_04a_core_preference_tests.md`

**Files (2,098 lines):** `tests/unit/` `test_core_types`, `test_distance_functions`,
`test_distance_comparison`, `test_distance_speed`, `test_performance_comparison`,
`test_loss_functions`, `test_level_set`, `test_prng`.

**Earlier reports:** `stage_02a_core_rng_preference.md`; SCP-001, SCP-002.
**Owner decisions:** Q2 (S00-001 lives here), Q4, Q18.

### 4b — geometry I tests → `stage_04b_geometry_i_tests.md`

**Files (1,935 lines):** `test_geom2d`, `test_convex_hull`, `test_majority`, `test_winset`,
`test_winset_ops`, `test_winset_1d`, `test_voronoi_2d`, `test_core`, `test_copeland`.

**Earlier reports:** `stage_02b_geometry.md` (findings on these headers); SCP-001.
**Owner decisions:** Q2, Q5, Q6, Q7, Q18.

### 4c — geometry II tests → `stage_04c_geometry_ii_tests.md`

**Files (2,055 lines):** `test_yolk`, `test_uncovered_set`, `test_heart`,
`test_centrality`, `test_geometry_integration`.

**Earlier reports:** `stage_02b_geometry.md` (Yolk, uncovered set, centrality; S02b-002 is
the Yolk P0). `test_heart` is withdrawn code (Q1): note it once as P3; do not review it.
**Owner decisions:** Q1, Q5, Q9. Q14, Q22, Q23 and Q25 are unanswered.

### 4d — aggregation and competition tests → `stage_04d_aggregation_competition_tests.md`

**Files (2,901 lines):** `test_profile`, `test_voting_rules`,
`test_aggregation_properties`, `test_aggregation_integration`, `test_voter_sampler`,
`test_seat_allocation`, `test_competitor_initialization`, `test_competition_strategies`,
`test_competition_motion`, `test_competition_election_feedback`,
`test_competition_engine`, `test_competition_termination`,
`test_competition_experiment_runner`.

**Earlier reports:** `stage_02c_aggregation.md`, `stage_02d_competition.md`; SCP-004.
**Owner decisions:** Q3, Q4, Q7, Q8, Q10. Q26–Q33 are unanswered.

### 4e — C API tests I → `stage_04e_c_api_tests_i.md`

**Range:** `tests/unit/test_c_api.cpp` lines 1–1910 (1,910 lines): helpers, Distance,
Loss, LevelSet1d/2d, IcInterval1d, Version, ErrorCodes, Pairwise, ToPolygon, IcPolygon2d,
LevelSetPolygon2d, StreamManager, Competition, ConvexHull, Majority, PairwiseMatrix,
WeightedMajority, Winset, Voronoi2d, WinsetInterval1d.

**Earlier reports:** `stage_03a_helpers_geometry.md`, `stage_03c_rng_competition.md`,
`stage_03h_header_contract.md`; SCP-001, SCP-003.
**Owner decisions:** Q2, Q4–Q12, Q18.

### 4f — C API tests II → `stage_04f_c_api_tests_ii.md`

**Range:** `tests/unit/test_c_api.cpp` lines 1911–3671 (1,761 lines), plus lines 1–48
for the shared helpers: Yolk, Copeland, Condorcet, Core, UncoveredSet(+Boundary), Heart,
Profile, VoterSampler, voting rules, Approval, RankByScores, PairwiseRanking, Pareto,
CondorcetProfile, ScalarDomains, ErrorPaths, NonFinite, SalienceWeights,
NonFiniteInput (the last three groups came with PR #1).

**Earlier reports:** `stage_03b_geometry_aggregation.md` (S03b-001 is the Stage 3 P0),
`stage_03a_helpers_geometry.md` (NonFinite and weights), `stage_03h_header_contract.md`;
SCP-004.
**Owner decisions:** Q1, Q3, Q4, Q5, Q9, Q18. Q14 is unanswered.
