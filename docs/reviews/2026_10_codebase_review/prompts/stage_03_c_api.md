# Stage 3 prompt — C API

**Model:** Opus, high effort. **Agents:** 4, one per slice (3h, 3a, 3b, 3c), run in
parallel. Each agent is told which slice it owns; everything above "Slices" applies to all.

**Before launch (review session):** build the library once into
`$SCR/stage3/build` (Debug, with `-fsanitize=address,undefined` off so `leaks` works) and
`$SCR/stage3/build_asan` (Debug, ASan+UBSan), from the commit below, outside the repo's
`build/`. Agents use these builds read-only.

---

You are running Stage 3<slice> of a staged code review of socialchoicelab
(`/Users/robiragan/Dropbox/SocialChoiceStudioDev/socialchoicelab`), commit `e6772eb`. Your
slice is part of the stable C API (`include/scs_api.h`, `src/c_api/scs_api.cpp`), the only
boundary between the C++ core and the R/Python bindings. You review for correctness and
safety at the boundary first, then contracts, then design.

The baseline moved since Stage 2 (commit `3f19579`): PR #1 fixed S02a-001, S02b-001 and
S02b-003 and added input validation to the C API; PR #2 changed `scripts/pre-push.sh`.
Line numbers quoted in Stage 2 reports refer to `3f19579`; find code by function name.

## Read first, in this order

1. `docs/reviews/2026_10_codebase_review/review_rubric.md` — rules, severity, finding
   format, **Owner decisions** section, and the **C API boundary** checklist. Follow it
   exactly.
2. `docs/reviews/2026_10_codebase_review/owner_questions.md` — the cross-cutting principle
   and every **answered** question (Q1–Q12, Q18), including "For reviewers" and "Implies".
   Q10–Q12 are new and apply directly to this stage. Q13–Q17 and Q19–Q33 are unanswered:
   behaviour they cover is an open question, not a defect.
3. `docs/reviews/2026_10_codebase_review/stage_01_architecture_map.md` — sections 1–7
   and 10.
4. The **"Contracts this layer provides"** sections of the four Stage 2 reports
   (`stage_02a…`–`stage_02d…`). They describe what the core throws, requires and returns;
   rely on them instead of reading core headers. Read the core only to settle a specific
   question.
5. `docs/reviews/2026_10_codebase_review/copilot_findings.md` (SCP-001–004).
6. `docs/reviews/2026_10_codebase_review/stage_00_baseline.md` — the C API coverage
   section (`scs_api.cpp` 60% line coverage; the 7 never-called functions; uncovered error
   paths) and clang-tidy entries for `scs_api.cpp`.
7. The PR #1 description (`gh pr view 1`), so you know which validation is new.

Then read **every line of your slice** in full.

## How to work

- **The boundary contract.** For each entry point check: every C++ exception is caught and
  becomes an error code plus `err_buf` message; null and invalid handles and pointers are
  checked; buffer sizes and the size-query/fill pattern are correct (including `_heap`
  variants and who frees what); integer arguments are range-checked **before** any
  arithmetic on them (e.g. `n * 2`, `n * dim`; see SCP-003); every double that can reach
  CGAL EPEC is checked for finiteness (a non-finite value aborts the process — Stage 2b
  Contracts); enum values from C are validated, not defaulted; outputs are fully written
  on success and left in a documented state on failure.
- **Error messages** follow the rubric's end-user rule: what failed, what was entered, how
  to fix it.
- **Compare against the core contracts.** Where the C API's checks are weaker than what
  the core requires, that is a finding (crash or wrong result); where they are stricter or
  different, check that the header documents it.
- **Demonstrate when you can.** Write small C programs or GoogleTest files under
  `SCR=/private/tmp/claude-501/-Users-robiragan-Dropbox-SocialChoiceStudioDev/2402f97e-221e-494b-88cb-fc63c41efc50/scratchpad/stage3<slice>/`
  linking `libscs_api` from `$SCR/../stage3/build` (or `build_asan` for memory errors). A
  demonstrated finding is `confirmed`; include the file path and output.
- **Leak runs** (slices 3a, 3b, 3c): run your groups under macOS `leaks`, e.g.
  `MallocStackLogging=1 leaks --atExit -- ./test_c_api --gtest_filter='<groups>'`, from the
  Stage 3 build. LeakSanitizer is not available with Apple clang. For Python and R runs,
  report only leaks whose stack includes `libscs_api` or `scs_` frames; interpreter
  allocations are noise. Record the exact commands and the leak summary in your output.
- **Stage 2 leads.** Each slice below lists out-of-slice notes from Stage 2 and SCP
  findings. For each: confirm (finding), already fixed by PR #1 (one line under "Tool
  leads checked"), or not a defect (why). Do not re-report SCP-003; check for other
  instances of the same pattern.
- **Write "Contracts this layer provides" carefully.** Stage 5 (bindings) relies on it:
  for each function group, what is validated, error codes returned, ownership of outputs,
  size-query behaviour, thread-safety (Q9).

## Hard constraints

- Do not modify any repository file except your output file. Do not commit. Do not use or
  rebuild the repository's `build/`.
- Do not run `scripts/check-all.sh`, `lint.sh format` or `check-bindings.sh` (they write
  to the repo).
- Never read or print `.env` or `.github_token`.
- Do not read `old_codebases/` or `agent_conversations/`.
- If your slice turns out too large to read in full, stop and say which parts you did not
  read rather than skimming.

## Output

Write `docs/reviews/2026_10_codebase_review/stage_03<slice>_<name>.md`, using the
structure in the rubric, plus after Findings:

```
## Tool leads checked
One line per lead (Stage 0 coverage/clang-tidy, Stage 2 out-of-slice note, SCP item):
lead → finding ID / fixed in PR #1 / false positive (why) / correct but untested.

## Leak runs
Commands run and the summary (slices 3a–3c).
```

When done, reply with: your Summary section, the list of finding IDs with severity and
title, and any open questions for the owner.

---

## Slices

Line ranges are for `src/c_api/scs_api.cpp` (4,742 lines) at `e6772eb`. They replace the
ranges in stage_01 §9, which predate PR #1.

### 3h — header contract → `stage_03h_header_contract.md`

**Files:** `include/scs_api.h` (2,020 lines) and `docs/architecture/c_api_design.md` (641).
Read `scs_api.cpp` only to check a specific claim.

**Focus:** ABI types and struct layouts; ownership and free-function pairing for every
returned pointer or buffer; error codes and `err_buf` contract; size-query contracts;
thread-safety statements (Q9); versioning (Q11: one project version, currently 0.3.0) and
stability/deprecation language (Q12: not in force until the project opens); bounds (Q10:
`SCS_CompetitionEngineConfig` has none); header comments vs. the design doc; Heart
functions still declared (Q1, P3 once).

**Leads:** Stage 1 invariants marked "⚠ possible mismatch" that involve the header; the 7
never-called functions in Stage 0 (are they documented, and should they exist?).

### 3a — helpers, preference, geometry I → `stage_03a_helpers_geometry.md`

**Ranges:** 1–282 and 582–2034 (about 1,735 lines). Preamble, impl structs, version,
`scs_heap_free`, shared helpers (`set_error`, `validate_finite_doubles`, `copy_int_vector`,
`to_dist_config`, `to_cpp_dist`, loss mapping, `build_voters`), `SCS_StreamManagerImpl`,
distance, loss, level sets/IC (`scs_level_set_to_polygon`), convex hull, majority, winset
factories, lifecycle, boundary export, 1D winset, Voronoi (size/fill/heap), boolean ops,
clone.

**Owner decisions:** Q2, Q5, Q6, Q7 (Voronoi coincident sites), Q9, Q18.

**Leads:** Stage 2: `scs_level_set_to_polygon` copies uninitialised vertices and
mis-handles negative `n_vertices`; `to_cpp_dist` maps an unknown distance type to
Euclidean; `to_cpp_loss_config` does not validate; Voronoi finiteness and `to_dist_config`
zero weights (check PR #1's fix, including `_heap` variants). SCP-001, SCP-002 (check the
C API passes no unchecked extreme values), SCP-003 (look for other instances).

**Leak run filter:** `--gtest_filter='CApi_Version*:CApi_ErrorCodes*:CApi_ErrorPaths*:CApi_Distance*:CApi_SalienceWeights*:CApi_ScalarDomains*:CApi_Loss*:CApi_LevelSet*:CApi_IcPolygon2d*:CApi_IcInterval1d*:CApi_ToPolygon*:CApi_ConvexHull*:CApi_Majority*:CApi_WeightedMajority*:CApi_Pairwise.*:CApi_PairwiseMatrix*:CApi_Winset*:CApi_Voronoi2d*:CApi_NonFinite*'`.

### 3b — geometry II, aggregation → `stage_03b_geometry_aggregation.md`

**Ranges:** 2035–3575 and 3787–4131 (about 1,886 lines), plus 111–282 as context. Yolk, C4
helpers (`find_alt_index`, `export_polygon2e`), Copeland, Condorcet/core, uncovered set
(+heap), Heart (Q1: withdrawn, P3 once), profile constructors, `scs_draw_voters`, profile
lifecycle and export, C6 helpers (`resolve_prng`, `fill_int_winners`), voting rules,
approval, profile clone, C7 rankings, Pareto, Condorcet predicates, centrality.

**Owner decisions:** Q1, Q3, Q4, Q9, Q14 is unanswered (Yolk binding).

**Leads:** Stage 2: `scs_uncovered_set_boundary_2d` does not validate `grid_resolution` or
`n_voters`; profile `n_alts` negative values; `scs_yolk_2d` takes `dist_cfg` but ignores it
(Stage 2b). Stage 2c P1s S02c-001–004 (check whether the C API adds or hides any of them).
SCP-004 reaches `scs_draw_voters` and the profile generators. The three never-called
centrality functions (Stage 0).

**Leak run filter:** `--gtest_filter='CApi_Yolk*:CApi_Copeland*:CApi_Condorcet*:CApi_Core*:CApi_UncoveredSet*:CApi_Heart*:CApi_Profile*:CApi_VoterSampler*:CApi_Plurality*:CApi_Borda*:CApi_Antiplurality*:CApi_ScoringRule*:CApi_Approval*:CApi_RankByScores*:CApi_PairwiseRanking*:CApi_Pareto*'`;
plus `pytest python/tests` (with `PYTHONDONTWRITEBYTECODE=1 -p no:cacheprovider`) and
`SCS_LIB_PATH` pointing at the Stage 3 build. No C API test calls the centrality
functions (Stage 0); note that under "Tool leads checked".

### 3c — RNG and competition → `stage_03c_rng_competition.md`

**Ranges:** 283–581, 3576–3786 and 4132–4742 (about 1,121 lines), plus 111–282 as
context. Competition config mapping (dist, engine, step, termination), `copy_positions`,
shares, StreamManager lifecycle, PRNG draws, competition run, trace and experiment API.

**Owner decisions:** Q4 (reproducibility), Q7, Q8, Q9, **Q10 (bounds: optional, inferred
with a 10% relative margin, recorded with results)**.

**Leads:** Stage 2: `scs_competition_run_experiment` skips `validate_finite_doubles` on
positions, headings and voters; step and termination doubles are not checked for
finiteness; `to_cpp_termination_config` replaces `signature_resolution <= 0` with 1e-6
where the core would throw; bounds inference duplicated between run and experiment (Q10
Implies — not a new finding; report extra instances only); `scs_uniform_real` forwards
ranges whose width overflows (S02a-008). Stage 2d S02d-006 (engine without a
StreamManager). The two never-called trace/experiment accessors (Stage 0).

**Leak run filter:** `--gtest_filter='CApi_StreamManager*:CApi_Competition*'`; plus R
`r/tests/testthat/test_competition.R`, run from a copy of `r/` (as in Stage 0) so nothing
is written to the repo, with `SCS_LIB_PATH` pointing at the Stage 3 build.
