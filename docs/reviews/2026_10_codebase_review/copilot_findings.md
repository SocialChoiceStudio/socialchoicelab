# Copilot review findings (PRs #1 and #3)

GitHub Copilot reviewed the early-fix PR #1 and the Stage 2 documents PR #3 on 2026-10-29.
Each point below was checked against the code at `66c761b`. Copilot's documentation
corrections were applied directly to `owner_questions.md` (Q18, Q26),
`stage_02b_geometry.md` and `stage_02c_aggregation.md`. The code issues are recorded here
in the rubric format, for Stage 10 verification and the Stage 11 fix plan.

IDs are `SCP-nnn`. Confidence follows the rubric: `confirmed (logic)` means the code
makes the failure unambiguous, but no program was run.

### SCP-001 [P2, plausible] Finite semi-axes can still produce infinite vertices
- **Where:** include/socialchoicelab/preference/indifference/level_set.h:208 (finiteness
  check on `a0`/`a1` only); `to_polygon` (:345); geometry/winset.h `pts_polygon` (~:110)
- **What:** PR #1 checks that the semi-axes are finite, but each vertex is
  centre + semi-axis·cos/sin, and that sum can overflow to Inf. The Inf vertex then goes
  to `to_exact` and CGAL, which aborts the process (the S02b-003 crash class).
- **Failure scenario (from Copilot, not run):** weights (1e-155, 1e-155), voter (1e308, 0),
  status quo (0, 0): semi-axes 1e308, first vertex x = 1e308 + 1e308 = Inf.
- **Suggested fix:** check sampled vertices for finiteness before exact conversion and
  raise an end-user error. Inputs are extreme, hence P2.
- **Also affects:** every winset-based C API, R and Python call.

### SCP-002 [P2, confirmed (logic)] Equal-weights test treats very small unequal weights as equal
- **Where:** include/socialchoicelab/preference/indifference/level_set.h:163–170
  (`equal_weights_2d`); constants in core/numeric_constants.h:25–28
- **What:** the tolerance is max(1e-9·scale, 1e-12). The absolute floor dominates for
  weights near 1e-12, so (1e-13, 1e-12), which differ tenfold, count as equal and get a
  circle instead of an ellipse.
- **Failure scenario:** one voter at (0, 0), status quo (100, 0), weights (1e-13, 1e-12):
  a circle of radius 100 is built in place of an ellipse with semi-axes (100, 10), so
  (0, 50) is wrongly inside the preferred-to set.
- **Suggested fix:** use a relative-only comparison, and add a test with scaled unequal
  weights. Weights this small are unusual, hence P2.
- **Also affects:** indifference curves and winsets in every layer.

### SCP-003 [P1, confirmed (logic)] Voronoi input check overflows on huge site counts
- **Where:** src/c_api/scs_api.cpp:1787 (`validate_voronoi_inputs`), helper
  `validate_finite_doubles` at :123
- **What:** `n_sites * 2` is computed before `n_sites` is range-checked. For
  |n_sites| > INT_MAX/2 this is signed overflow (undefined behaviour). In practice it wraps:
  a large positive count becomes negative and the finiteness check is skipped; a large
  negative count can become a large positive scan length that reads past the buffer.
- **Failure scenario:** `scs_voronoi_cells_2d(sites, -1073741825, …)` → overflow in the
  check meant to protect the call. Reachable only by calling the C API directly; R and
  Python pass real lengths.
- **Suggested fix:** reject `n_sites` outside [1, INT_MAX/2] in the helper before scanning;
  this covers all three Voronoi entry points.
- **Also affects:** Stage 3 should check every other `n * 2` and `n * dim` length in the
  C API for the same pattern.

### SCP-004 [P1, confirmed (logic)] Random voter coordinates are drawn in compiler-dependent order
- **Where:** include/socialchoicelab/aggregation/profile_generators.h:71, :112
  (Decision: Q4)
- **What:** `emplace_back(x_dist(prng.engine()), y_dist(prng.engine()))` makes both draws
  arguments of one call. C++ leaves argument evaluation order unspecified, so one compiler
  may draw x first and another y first. Q4 requires the same seed to give the same
  voters on every platform.
- **Failure scenario:** the same seed builds a profile with x and y draws swapped between
  compilers, e.g. Clang on macOS and GCC on Linux (CI runs both). Seeded results then
  differ across platforms. Not demonstrated; the order each compiler uses was not checked.
- **Suggested fix:** draw into named locals (`const double x = …; const double y = …;`)
  before `emplace_back`, as part of the Q4 RNG work. Check other generators and
  competition initialisation for the same pattern.
- **Also affects:** seeded profiles in R and Python, and the website.
