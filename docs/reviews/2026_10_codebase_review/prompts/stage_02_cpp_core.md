# Stage 2 prompt — C++ core

**Model:** Opus, high effort. **Agents:** 4, one per slice (2a, 2b, 2c, 2d), run in
parallel. Each agent is told which slice it owns; everything above "Slices" applies to all.

---

You are running Stage 2<slice> of a staged code review of socialchoicelab
(`/Users/robiragan/Dropbox/SocialChoiceStudioDev/socialchoicelab`), commit `3f19579` for
all source files. Your slice is part of the header-only C++20 core (Eigen; CGAL EPEC for
exact 2D geometry). You review for correctness first, then robustness and design.

## Read first, in this order

1. `docs/reviews/2026_10_codebase_review/review_rubric.md` — rules, severity, finding
   format, and the **Owner decisions** section. Follow it exactly.
2. `docs/reviews/2026_10_codebase_review/owner_questions.md` — read the cross-cutting
   principle and the questions listed for your slice below, in full, including their
   "For reviewers" and "Implies" paragraphs. These are the contract.
3. `docs/reviews/2026_10_codebase_review/stage_01_architecture_map.md` — sections 1–7
   and 10. Use it for context on other layers instead of reading them.
4. `docs/reviews/2026_10_codebase_review/stage_00_baseline.md` — the coverage, warnings,
   and clang-tidy entries for your files.
5. The design docs listed for your slice.

Then read **every file in your slice in full**. Read the minimum of other headers needed to
understand a call; do not review them.

## How to work

- **Check correctness against the source.** For each algorithm, compare the code with the
  definition in the design doc and the cited paper. Papers are in
  `/Users/robiragan/Dropbox/SocialChoiceStudioDev/references/` (index:
  `REFERENCE_INDEX.md`). If a source isn't there, say so rather than citing from memory.
- **Probe edge cases**, using the rubric checklist: empty input, one voter/candidate,
  ties, duplicate or collinear points, NaN/Inf, extreme magnitudes, integer overflow.
- **Demonstrate when you can.** Write small scratch programs or GoogleTest files under
  `SCR=/private/tmp/claude-501/-Users-robiragan-Dropbox-SocialChoiceStudioDev/2402f97e-221e-494b-88cb-fc63c41efc50/scratchpad/stage2<slice>/`
  that include headers from `<repo>/include`. Compile with Apple clang, `-std=c++20`, the
  same include paths and dependencies as `CMakeLists.txt` (read it to find them). An
  existing Release build is in `.../scratchpad/stage0/build_release` if you need its
  compile flags (`compile_commands.json`). A finding you demonstrated is `confirmed`;
  include the scratch file path and its output in the finding.
- **Use Stage 0 tool output as leads, not conclusions.** For each clang-tidy or coverage
  item in your slice listed below, decide: real defect (finding), false positive (one line
  under "Tool leads checked"), or untested-but-correct (note for Stage 4).
- **Write the "Contracts this layer provides" section carefully.** Stage 3 (C API) reviewers
  will rely on it instead of reading your headers: preconditions, what is thrown and when,
  ownership, units, ordering of outputs, determinism.

## Hard constraints

- Do not modify any repository file except your output file. Do not commit.
- Never read or print `.env` or `.github_token`.
- Do not read `old_codebases/` or `agent_conversations/`.
- If your slice turns out too large to read in full, stop and say which files you did not
  read rather than skimming.

## Output

Write `docs/reviews/2026_10_codebase_review/stage_02<slice>_<name>.md`, using the
structure in the rubric, plus one extra section after Findings:

```
## Tool leads checked
One line per Stage 0 lead in your slice: lead → finding ID / false positive (why) /
correct but untested.
```

When done, reply with: your Summary section, the list of finding IDs with severity and
title, and any open questions for the owner.

---

## Slices

### 2a — core, RNG, preference → `stage_02a_core_rng_preference.md`

**Files (about 1,900 lines):**
- `include/socialchoicelab/core/{types,linalg,kernels,numeric_constants}.h`
- `include/socialchoicelab/core/rng/{prng,stream_manager}.h`, `src/core/rng/stream_manager.cpp`
- `include/socialchoicelab/preference/distance/distance_functions.h`
- `include/socialchoicelab/preference/loss/loss_functions.h`
- `include/socialchoicelab/preference/indifference/level_set.h`

**Design docs:** `docs/architecture/stream_manager_design.md`,
`docs/architecture/indifference_design.md`.

**Owner decisions that apply:** Q2 (exact Minkowski), Q4 (reproducibility and RNG), Q9
(thread safety).

**Leads:** `level_set.h` 65% line coverage; Stage 1 hotspot on direct `engine()` calls
bypassing `draw_count_`; superellipse sampling at very large p (`level_set.h:335–337`,
listed in Q2's Implies: check it); loss functions near zero distance and at extreme
parameters.

### 2b — geometry → `stage_02b_geometry.md`

**Files (about 2,740 lines):**
`include/socialchoicelab/geometry/{geom2d,convex_hull,majority,centrality,winset,winset_ops,winset_1d,yolk,uncovered_set,core,copeland,voronoi_2d}.h`.
**Not reviewed:** `heart.h` (withdrawn, Q1); list Heart leftovers once as P3.

**Design docs:** `docs/architecture/geometry_design.md`.

**Owner decisions that apply:** Q1 (Heart withdrawn), Q2 (`winset.h` p ≥ 100 branch), Q5
(exact winset emptiness), Q6 (exact weighted winset), Q7 (Voronoi coincident sites), Q9
(thread safety; CGAL objects are not safe to share even for reads).

**Leads:**
- clang-tidy: possible leak at `yolk.h:229`.
- Coverage: `geom2d.h` 33%, `winset.h` 57%.
- Stage 1 hotspots: Yolk is an LP-yolk approximation (check against its cited source and
  how the docs describe it); uncovered-set and other boundaries are grid approximations;
  default grid resolution 30 in C++ but 15 in the C API and both bindings; `core_2d`
  comment about unanimity contradicts itself; Voronoi bisector built in double precision
  then clipped exactly.
- Exact (EPEC) vs. double boundaries: check every conversion between them for loss of
  exactness that changes a result.

### 2c — aggregation → `stage_02c_aggregation.md`

**Files (about 1,560 lines):**
`include/socialchoicelab/aggregation/{profile,profile_generators,voter_sampler,tie_break,plurality,borda,antiplurality,scoring_rule,approval,social_ranking,pareto,condorcet_consistency,seat_allocation}.h`.

**Design docs:** `docs/architecture/aggregation_design.md`.

**Owner decisions that apply:** Q3 (smallest-index default everywhere), Q4 (RNG: draws in
`tie_break.h`, `borda.h`, `social_ranking.h`, `profile_generators.h`), Q9.

**Leads:**
- Coverage: `voter_sampler.h` 51%.
- Seat allocation: check each method against its published definition (e.g. D'Hondt,
  Sainte-Laguë, Hare/largest remainder, whichever are implemented), including ties for
  the last seat and zero-vote parties.
- Properties claimed in `condorcet_consistency.h` and `pareto.h`: verify the definitions
  match the literature.
- Profiles from spatial voters: tie handling when a voter is equidistant (index ties are
  intended here per Q7 "Out of scope"; check that they are documented).

### 2d — competition → `stage_02d_competition.md`

**Files (about 1,930 lines):**
`include/socialchoicelab/competition/{competition_types,competitor,initialization,adaptation,strategy_registry,motion,step_policy,boundary_policy,election_feedback,engine,termination,state_signature,experiment_runner}.h`.

**Design docs:** `docs/architecture/competition_design.md`,
`docs/status/competition_plan.md`.

**Owner decisions that apply:** cross-cutting L&S principle, Q4 (RNG in `step_policy.h`,
`initialization.h`), Q7 (random tie-breaks for voters and Predator), Q8 (satisficing
replaces `hunter_sticker`; Predator default is "flag both ways"; heading and Hunter memory
rules), Q9. Q10 (competition bounds) is **unanswered**: report bounds behaviour as an open
question.

**Leads:**
- clang-tidy: `bugprone-exception-escape` at `strategy_registry.h:54`;
  `bugprone-unchecked-optional-access` in `initialization.h` (2 sites).
- Coverage: `termination.h` 60%, `competition_types.h` 64%.
- Stage 1 hotspot: `build_spatial_profile_nd` duplicates `build_spatial_profile`.
- Strategies (Sticker, Hunter, Aggregator, Predator): check each against L&S (2012) and
  Laver (2005) definitions; divergences are fine if documented (cross-cutting principle).
- Termination and state signatures: can a run stop early or loop wrongly because of
  floating-point comparison of positions?
- Experiment runner: per-run seeding, stream allowlist, aggregation of results across runs.
