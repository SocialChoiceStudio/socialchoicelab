# Early fix prompt — Stage 2 P0s #1 and #3

**Model:** Opus, high effort. **Agents:** 1, launched by the review session as a background
agent in its own **git worktree** of `socialchoicelab/`, on a new branch `fix/early-p0s`.
The review continues in the main checkout, so this agent must not change files there.
The owner approved this fix plan on 2026-10-29.

---

You are fixing two P0 bugs found by an ongoing code review of socialchoicelab. The review
itself is still running in the main checkout; your work happens only in your worktree on
branch `fix/early-p0s`.

Follow the project rules in `CLAUDE.md` and `.cursor/rules/*.mdc`. The owner has already
approved the plan in this prompt, so implement it without waiting for further approval; if
you need a decision this prompt does not cover, stop and report the question instead of
guessing. New behaviour needs tests; R and Python must
behave identically; end-user error messages say what failed, what was entered, and how to
fix it; no backward-compatibility shims.

## Read first

The review reports are untracked files in the **main checkout**, so read them by absolute
path:

- `/Users/robiragan/Dropbox/SocialChoiceStudioDev/socialchoicelab/docs/reviews/2026_10_codebase_review/stage_02a_core_rng_preference.md`
  — finding **S02a-001**, Open question 2, and the Out-of-slice note on
  `scs_level_set_to_polygon`.
- `/Users/robiragan/Dropbox/SocialChoiceStudioDev/socialchoicelab/docs/reviews/2026_10_codebase_review/stage_02b_geometry.md`
  — findings **S02b-001** and **S02b-003**, the "Contracts" note on non-finite input, Open
  question 1, and the Out-of-slice notes on `scs_voronoi_cells_2d` and `to_dist_config`.
- `docs/architecture/Design_Document.md` (Convention B, salience weights) and
  `docs/architecture/indifference_design.md`.

The scratch programs cited in those findings are in
`/private/tmp/claude-501/-Users-robiragan-Dropbox-SocialChoiceStudioDev/2402f97e-221e-494b-88cb-fc63c41efc50/scratchpad/stage2a/`
and `.../stage2b/`. They may be gone; if so, rebuild the reproductions as tests.

## Bug 1 — equal non-unit salience weights give the wrong size (S02a-001, S02b-001)

- `include/socialchoicelab/preference/indifference/level_set.h` (~line 237) and
  `include/socialchoicelab/geometry/winset.h` (~line 131) return `Circle2d{centre, r}` in
  the equal-weights Euclidean branch. `r` is the weighted distance; the radius must be
  `r / w` (`a0`, already computed). Weights (2, 2) give shapes twice the true size,
  (0.5, 0.5) half.
- Fix both. Prefer one internal "shape at weighted distance r" builder shared by
  `level_set_2d`, `ic_polygon_2d` and `geometry::detail::pts_polygon`, so the logic exists
  once — but keep the change small. Do **not** change the Minkowski p ≥ 100 Chebyshev
  branch or any other review finding; those are queued for the batch.
- Tests (C++ and C API, plus one R and one Python test through the public API):
  - weights (2, 2) and (0.5, 0.5): IC vertices lie at weighted distance r from the ideal
    point and the IC passes through the status quo;
  - a winset with equal non-unit weights agrees with `majority_prefers` on sample points
    (the S02b-001 reproduction: one voter at (0,0), sq (1,0), k = 1, weights (2,2) —
    (1.5, 0) must not be in the winset);
  - each distance type, so the other branches are covered too.

## Bug 3 — zero or non-finite input aborts the process (S02b-003)

A salience weight of 0 makes `pts_polygon` compute `r/0 = Inf`; the Inf/NaN vertices reach
CGAL EPEC and GMP raises SIGFPE, killing R or Python with no message. The same abort
happens for any non-finite double reaching EPEC (e.g. a NaN site or bbox in
`scs_voronoi_cells_2d`).

**Owner decision for this fix:** in 2D geometry that builds bounded shapes, a zero (or
negative) salience weight is an **error**, not a strip to be clipped. Dimension masking
(weight 0) stays allowed for plain distance and loss computations, as Convention B says.

- C++ core: `winset_2d` and its variants (weighted, veto, petal, and anything else that
  calls `pts_polygon`) throw `std::invalid_argument` for non-positive or non-finite
  weights before any shape is built. Also guard against `r / w` overflowing to Inf.
  `level_set_2d` already rejects w ≤ 0; make the messages consistent.
- C API: find every entry point whose inputs can reach an EPEC constructor
  (`to_exact`, `Point2E`, CGAL polygons) and make sure non-finite coordinates, weights,
  status quo, bbox values, and non-positive weights are rejected with an error code and an
  end-user message **before** the core is called. Start from `to_dist_config`
  (`src/c_api/scs_api.cpp` ~line 160; check all its callers — reject zero weights only
  where a 2D shape is built) and `scs_voronoi_cells_2d` (~line 1777, and its `_heap`
  variant). List what you checked in the PR.
- R and Python: no crash; the C API error must surface as a normal R/Python error with the
  same message in both. Only add binding-side validation if you add it to both. The Python
  `DistanceConfig` docstring says weights are "strictly positive" — make the R and Python
  docs match the actual rule (positive for 2D geometry, ≥ 0 for distances).
- Tests: C++ throws; C API returns an error (not a crash) for weight 0, negative weight,
  NaN/Inf weight, NaN site, NaN bbox; one R and one Python test each showing a clear error
  for weight 0 in a winset call.

## Out of scope

Everything else in the Stage 2 reports, including the Yolk P0 (S02b-002), which waits for
the batch. If you find another bug, note it in your final reply; do not fix it.

## Finish

1. Run `./scripts/check-all.sh` and the R and Python test suites; all must pass.
2. Update `CHANGELOG.md` (or the project's equivalent) and any docs that state the weight
   rules.
3. **Website:** check whether `../socialchoicestudiowebsite/backend/` can send zero
   salience weights to a geometry call. If so, say so in your final reply (do not edit the
   website).
4. Commit on `fix/early-p0s` in your worktree. Do **not** push or open a PR; the review
   session does that after the owner approves.
5. Final reply: worktree path and branch, commit SHA(s), what changed, the entry points
   now validated, tests added and results, the website note, and anything found but not
   fixed. This becomes the PR description.
