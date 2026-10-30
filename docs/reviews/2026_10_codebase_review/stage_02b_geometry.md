# Stage 02b — Geometry
Model / effort: Opus / high   Commit: 3f19579 (sources unchanged at HEAD 3a4e908)   Files reviewed: 12 headers, 2,740 lines
Not read (if any): none. `heart.h` deliberately not reviewed (Q1, withdrawn).

Scratch programs and their output are in
`SCR=/private/tmp/claude-501/-Users-robiragan-Dropbox-SocialChoiceStudioDev/2402f97e-221e-494b-88cb-fc63c41efc50/scratchpad/stage2b/`.
Build: `SCR/build.sh` (Apple clang, `-std=c++20 -O2`, repo `include/`, Eigen 3.4.0 from
`build/_deps/eigen-src`, Homebrew CGAL 6.1.1 / GMP / MPFR). C API demos link
`SCR/scs_api.o` compiled from `src/c_api/scs_api.cpp` at this commit. The Stage 0
`build_release` tree no longer contains `compile_commands.json`, so flags were taken from
`CMakeLists.txt`.

## Summary

- **3 P0, 3 P1, 6 P2, 8 P3.** All P0 and P1 findings are confirmed by scratch programs.
- **The Yolk is wrong for almost every input (S02b-002).** `yolk_2d`'s subgradient step is
  scaled by `f − best`, which is ≈0 from the first iteration, so the solver never leaves its
  starting point. It returns the voter centroid as the centre, to within 3e-8, and the
  radius from there. Against a brute-force minimax the radius is 15% to 5× too large.
  Tests pass only because every Yolk test uses a configuration whose centroid is the Yolk
  centre. `core_2d` uses this centre as its first candidate.
- **Equal non-unit salience weights give the wrong winset (S02b-001).** With weights
  (2, 2), every preferred-to polygon is twice too large (area 12.5 instead of 3.14). With
  (0.5, 0.5) it is half the size. Weights (2, 2.000001) give the right answer. The same
  bug is in `level_set.h` (slice 2a), so IC overlays are wrong too.
- **A zero salience weight aborts the process (S02b-003).** Convention B allows weight 0
  (dimension masking), and the C API lets it through, but `winset_2d` builds a polygon with
  Inf/NaN vertices. GMP then raises SIGFPE (exit 136) in R or Python. Any non-finite double
  that reaches an EPEC constructor does the same; this is a contract Stage 3 must enforce.
- **Voronoi drops whole cells on ordinary input (S02b-004).** For sites (0,0), (2,0),
  (0,2), two of the three cells come back empty. The cause is that clipping emits a
  duplicate vertex whenever a bisector passes through an existing vertex, and
  `is_simple()` then rejects the cell. Other P1s: the Yolk line family covers only one
  edge of each median band for even n and supermajorities (S02b-005), and
  `weighted_majority_prefers` lets exactly half the weight win, in both directions at once
  (S02b-006, Decision: Q6).

## Contracts this layer provides

Everything is header-only and stateless: there is no global or static mutable state in the
slice, and each call is deterministic (no RNG). Inputs are taken by const reference or
value, results are returned by value, and no pointer is retained.

- **Non-finite input.** No geometry function except `convex_hull_2d` checks for
  NaN/Inf. A non-finite double that reaches an EPEC constructor (`to_exact`, `Point2E(x,y)`,
  `bbox_to_polygon`) **aborts the process with SIGFPE** (GMP `mpq_set_d`). It does not throw.
  Callers (the C API) must reject non-finite coordinates, weights, status quo and bbox
  values beforehand. Zero salience weights also produce non-finite values inside
  `winset_2d` (S02b-003). The C API must reject them (or the core must be changed) until
  that is fixed.
- **Exceptions.** Only `std::invalid_argument` is thrown for bad input. Messages are
  function-prefixed but often lack the "how to fix" part.
  - `majority_prefers`, `preference_margin`, `pairwise_matrix`, `copeland_scores`,
    `winset_2d(_petal)`: throw on empty voters, k ∉ [1, n] (any negative k means simple
    majority), or `num_samples < 4`. They also throw whatever `calculate_distance` throws
    (weights size ≠ 2, negative weights, bad p).
  - `uncovered_set`, `has_condorcet_winner` and `condorcet_winner` return without
    throwing when they have fewer than 2 alternatives, even if `voter_ideals` is empty.
  - `yolk_2d`, `quantile_lines_2d*` and `core_2d` throw when n < 2. `yolk_2d` does not
    validate `n_sample`; a negative value becomes `std::length_error`.
  - `voronoi_cells_2d` throws when n_sites < 1 or min ≥ max. A NaN bbox passes that check
    and aborts (see above).
  - `winset_interval_1d` never throws: `n_voters ≤ 0` gives {NaN, NaN}, and a null pointer
    with n > 0 is UB.
  - Allocation failure surfaces as `std::bad_alloc` or `std::length_error`.
- **k convention.** -1 (any negative) means ⌊n/2⌋+1. `pairwise_matrix` validates k but
  ignores it. Majority is strict: equidistant voters count for neither side, and distances
  are compared in double with `<` (no tolerance).
- **Metric support.**
  - Majority, Copeland, the uncovered set and the winset honour `DistConfig` (Euclidean,
    Manhattan, Chebyshev, Minkowski).
  - Yolk, quantile lines, Voronoi and `winset_interval_1d` are Euclidean only. Yolk
    ignores any metric it is given. In 1D every weighted metric with a strictly positive
    weight induces the same preferences, so the 1D result holds for all such metrics. With
    weight 0 every alternative ties: the weighted winset is empty, but
    `winset_interval_1d` (unweighted) still returns a non-empty interval, so it is not
    valid for masked dimensions. (Correction from Copilot review of PR #3, verified.)
  - Weighted winsets ignore the metric's weights on vote weight (Q6 pending).
- **Exactness.**
  - Exact: winset Boolean operations, `winset_contains` (exact point-in-polygon on
    exact polygons), convex hull, `quantile_lines_2d*`, and Voronoi clipping predicates.
  - Not exact: preferred-to sets are inscribed polygons with `num_samples` vertices,
    sampled in double. The winset is therefore a subset of the true winset, and "empty"
    is not exact (Q5).
  - Double only: `yolk_2d`, the Voronoi bisector (S02b-007), majority counts, and all
    grid boundaries.
  - Outputs are converted with `to_numeric` (nearest double).
- **Output shape and ordering.**
  - `convex_hull_2d`: CCW. It returns 1 vertex for a single (or all-identical) point and
    2 for collinear points.
  - `voronoi_cells_2d`: `result[i]` is site i's cell, CCW and not closed. An empty
    vector means an empty cell or one rejected by `is_simple()` (S02b-004).
  - `uncovered_set`: a subset that keeps the input order. Alternatives are matched by
    value, so duplicates are kept.
  - `copeland_winner`: ties go to the smallest index.
  - `condorcet_winner`: returns the first winner. It returns nullopt when the winner
    appears twice.
  - `uncovered_set_boundary_2d`: the convex hull of the uncovered grid points. It can
    have 1 or 2 vertices (S02b-008).
- **Winset regions.** `WinsetRegion` is a CGAL `General_polygon_set_2`, which may contain
  holes and several components. `winset_contains` tests strict interior. **It is not safe
  to read one region from two threads at once**, because `polygons_with_holes()` writes
  face flags (Q9). Separate regions on separate threads are fine.
- **Units.** Distances, radii and areas are in input coordinate units. The Yolk radius is
  Euclidean. Salience weights scale coordinate differences (Convention B).

## Findings

### S02b-001 [P0, confirmed] Equal non-unit salience weights make every preferred-to polygon the wrong size
- **Where:** include/socialchoicelab/geometry/winset.h:131–132
- **What:** For Euclidean distance with equal weights, `pts_polygon` builds
  `Circle2d{voter_ideal, r}`, where `r` is the *weighted* distance d = w·|Δ|. The level
  set is a circle of radius r/w (= `a0`), so the polygon is w times too large. The ellipse
  branch, used for unequal weights, correctly uses `a0 = r/w0`. The result jumps
  discontinuously at w0 = w1.
- **Failure scenario:** `SCR/t01_equal_weights.cpp`: one voter at (0,0), status quo (1,0),
  k = 1.
  - Weights (2,2): the area is 12.55 instead of 3.14, and `winset_contains((1.5,0))` = 1,
    although `majority_prefers((1.5,0), sq)` = 0.
  - Weights (0.5,0.5): `winset_contains((0.9,0))` = 0.
  - Weights (2, 2.000001): correct.

  `SCR/t18_winset_consistency.cpp`, random 7-voter cases: with weights (2,2), 7,868 of
  13,268 points reported inside the winset are not majority-preferred to sq. Every other
  metric and weight combination tested gives 0. The C API, R and Python pass weights
  through unnormalised (`to_dist_config`, `_types.py:210`, `utils.R:46`), so the bug is
  reachable from every layer.
- **Suggested fix:** Use `Circle2d{voter_ideal, a0}`, or better, have `pts_polygon` call
  `level_set_2d`/`ic_polygon_2d` once those are fixed too, so there is one shape builder.
  Add winset tests with equal non-unit weights and with every metric branch.
- **Also affects:** `preference/indifference/level_set.h:236–237` has the same bug
  (slice 2a; `SCR/t19_levelset_equal_weights.cpp`: the IC through sq=(1,0) with weights
  (2,2) has radius 2 instead of 1), so `ic_polygon_2d`, `scs_level_set_2d`, the IC and
  preferred-region overlays in R/Python, and competition IC overlays are affected.
  `weighted_winset_2d`, `winset_with_veto_2d`, `core_2d` and the winset boolean ops
  inherit the bug.

### S02b-002 [P0, confirmed] `yolk_2d` never moves from the voter centroid
- **Where:** include/socialchoicelab/geometry/yolk.h:430–466 (step rule at :457–459)
- **What:** The step is `alpha * (max_d − best_r + 1e-10·spread) / (iter+1)`. On iteration
  1, `best_r` is set to `max_d`, so the gap is 1e-10·spread. After that, the iterate barely
  moves, so `max_d − best_r` stays ≈0 and the step stays ≈1e-10·spread/iter. The solver
  returns the centroid, its starting point, and the radius at the centroid. This is a
  failed optimiser, separate from the known LP-yolk limitation (K1). (Polyak's rule needs
  `f − f*`, where f* is a lower bound or estimate, not the best value found so far.)
- **Failure scenario:**
  - `SCR/t06_yolk_centroid_exact.cpp`: voters (0,0), (10,0), (0,1), (10,1), (5,8) give a
    centre (4.99999999999988, 2.00000003). The centroid is (5, 2).
  - `SCR/t05_yolk_optimizer.cpp`: a brute-force minimax over a denser family of the same
    lines (20,000 directions, both band edges) gives these results:

    | Case | `yolk_2d` radius | Brute-force radius |
    |---|---|---|
    | 5 voters | 0.990 | 0.862 |
    | 5 voters | 3.180 | 2.593 |
    | 11 voters | 1.130 | 0.310 |
    | 11 voters | 1.497 | 0.281 |
    | 11 voters | 1.438 | 0.525 |

    The `yolk_2d` centre equals the centroid every time.

  The existing tests use an equilateral triangle and symmetric collinear voters, where the
  centroid is the Yolk centre, so they pass. The header claims radius accuracy "≈ 0.005 ×
  spread" (:320–322), which is false.
- **Suggested fix:** Replace the solver. The minimax over a finite line set is a small LP
  (min r subject to |aᵢx + bᵢy + cᵢ| ≤ r), solvable exactly or with CGAL's LP solver. The
  planned exact replacement (Gudmundsson & Wong 2019, Liu & Tovey 2023) is the long-term
  route. Add tests with asymmetric voters whose Yolk is known.
- **Also affects:** `core_2d` (core.h:161, its first candidate is the centroid);
  `scs_yolk_2d` (scs_api.cpp:1969). It is not bound in R/Python (Q14 pending), so users
  reach it only through the C API. Check any paper draft or documentation figure produced
  with `yolk_2d`.

### S02b-003 [P0, confirmed] Zero salience weight aborts the process in `winset_2d`
- **Where:** include/socialchoicelab/geometry/winset.h:116–119, :139–181; geom2d.h:59–61
- **What:** Convention B allows weight 0 (dimension masking), and `calculate_distance`
  accepts it. `pts_polygon` then computes `a0 = r/0 = Inf`. The sampled ellipse, diamond,
  square or superellipse has Inf/NaN vertices (cos·Inf = NaN at t = π/2). `to_exact` lifts
  them into EPEC, and GMP raises SIGFPE. `level_set_2d` rejects w ≤ 0 with an exception,
  but `pts_polygon` does not. Mathematically, the preferred-to set with a zero weight is an
  unbounded strip, so it cannot be a bounded polygon at all.
- **Failure scenario:**
  - `SCR/t02_zero_weight.cpp`: weights {0, 1} exit with 136 (SIGFPE) for Euclidean,
    Manhattan, Chebyshev and Minkowski alike.
  - `SCR/t03_capi_zero_weight.c`: `scs_winset_2d(0.3, 0.5, …, weights {0,1}, …)` kills the
    process (exit 136) instead of returning an error. `to_dist_config` checks only that
    the weights are finite. In R or Python, this takes down the session.
- **Suggested fix:** In `winset_2d` (and the veto, weighted and petal variants), throw
  `std::invalid_argument` naming the weight and suggesting a positive value, or clip the
  strip to a documented bounding box (owner decision, see Open questions). Also guard
  `a0`/`a1` overflow when r/w is huge.
- **Also affects:** C API `to_dist_config` (scs_api.cpp:160) should reject non-positive
  weights for 2D geometry calls. The R and Python `make_dist_config` do not validate
  weights either.

### S02b-004 [P1, confirmed] Voronoi cells are dropped when a bisector passes through an existing vertex
- **Where:** include/socialchoicelab/geometry/voronoi_2d.h:103–114, :188
- **What:** `clip_polygon_by_halfplane` treats boundary vertices as "in". When `v_prev` lies
  exactly on the line and `v_curr` is out, it pushes the intersection point, which is
  `v_prev` again. The cell gets consecutive duplicate vertices, `Polygon_2::is_simple()`
  returns false, and `voronoi_cells_2d` replaces the cell with an empty vector. This
  happens whenever a bisector hits a box corner or a Voronoi vertex created by an earlier
  clip, which is common for symmetric or integer configurations.
- **Failure scenario:** `SCR/t10_voronoi.cpp`, with root cause shown in `SCR/t11_voronoi_rootcause.cpp`:
  - Sites (0,0), (1,1) in box [-1,2]²: both cells are empty. Site 0's cell has vertices
    (-1,-1), (2,-1), (2,-1), (-1,2), (-1,2).
  - Sites (0,0), (2,0), (0,2) in [-1,3]²: the cells of sites 1 and 2 are empty, and only
    site 0's cell (area 4) is returned.
  - The same sites in [-1,3.5]×[-1,3.2] give three correct cells.

  The R/Python competition overlays (`scs_voronoi_cells_2d_heap`) would draw missing
  candidate regions.
- **Suggested fix:** Do not emit the intersection point when the previous vertex is on the
  line (Sutherland–Hodgman with "on" handled once), or deduplicate consecutive equal
  vertices before `is_simple`. Simpler: intersect with `CGAL::Polygon_2` half-plane
  clipping or build cells with CGAL's Voronoi/Apollonius adaptor. Add tests with
  bisectors through corners and through shared Voronoi vertices.
- **Also affects:** C API `scs_voronoi_cells_2d(_heap)`, and the R/Python Voronoi overlays.

### S02b-005 [P1, confirmed] Yolk line family uses only one edge of each median band (even n, k-majority)
- **Where:** include/socialchoicelab/geometry/yolk.h:357–375 (also `quantile_lines_2d`, :112–139)
- **What:** For each direction θ ∈ [0, π), the code takes the k-th smallest projection. A
  line is a k-quantile (median) line when neither open side holds k voters, so in each
  direction the median lines form a band [proj₍ₙ₋ₖ₊₁₎, proj₍ₖ₎]. Koehler (1992, SCW
  9:37, `references/social_choice/core/Koehler_SCW1992_MedianLinesYolk.pdf`, p. 37) defines
  even-n median lines this way: "at least n/2 … on and to each side". For odd n with
  simple majority, the two indices coincide. For even n and for every k > ⌊n/2⌋+1, the
  other edge (proj₍ₙ₋ₖ₊₁₎, which corresponds to direction θ+π) is never generated for the
  sampled directions. Critical directions supply it only incidentally, depending on voter
  index order.
- **Failure scenario:** `SCR/t09_yolk_line_family.cpp` solves both families with the same
  brute-force minimax, which factors out S02b-002:

  | Case | Code's family | Full family |
  |---|---|---|
  | 8 voters, simple majority | r = 0.369 at (0.344, −0.539) | r = 0.430 at (0.554, −0.663) |
  | 7 voters, k = 5 | r = 1.036 | r = 1.067 |

  At the code family's optimum (8 voters), the full-family maximum distance is 0.648, so
  that disk misses median lines. `SCR/t04_yolk_even.cpp` shows `yolk_2d(−X)` ≠ `−yolk_2d(X)`
  in radius for k = 4 and k = 5 (1.8385 vs 1.8439, 2.6870 vs 2.8636).
- **Suggested fix:** For each direction, add both band edges (indices k−1 and n−k), or
  sample θ over [0, 2π). Document the k-quantile definition with a source.
- **Also affects:** `scs_yolk_2d`; `core_2d` candidate choice.

### S02b-006 [P1, confirmed] `weighted_majority_prefers` lets exactly half the weight win, in both directions
- **Where:** include/socialchoicelab/geometry/majority.h:226–228, :243, :262–280. Decision: Q6
- **What:** The default is `threshold = 0.5` with `x_weight >= threshold × total_weight`,
  so exactly half the weight wins. The relation is then not asymmetric: x ≻ y and y ≻ x
  can both hold. The doc admits that this reduces to k = ⌈n/2⌉, not ⌊n/2⌋+1. Sums are in
  double, so an exact half may be decided either way by rounding. Q6's Implies list
  covers the weighted *winset* and the binding thresholds, but not this function, which
  is exposed as `scs_weighted_majority_prefers_2d` and Python's
  `weighted_majority_prefers_2d`. NaN weights pass the `w <= 0.0` check, although the
  C API filters non-finite values.
- **Failure scenario:** `SCR/t15_weighted_majority.cpp`:
  - Two voters with weights {1,1}: `weighted(x,y)` = 1, `weighted(y,x)` = 1, and
    `majority_prefers(x,y)` = 0.
  - The x side holds 0.3 of {0.3,0.1,0.2}: the result is 0. The same proportions as
    {3,1,2} give 1. Scaling the weights changes the answer.
- **Suggested fix:** Apply Q6's rule here: the default is "more than half"; an explicit
  threshold means "at least". Compare sums exactly (rational, or scaled integers), and
  reject non-finite weights.
- **Also affects:** `scs_weighted_majority_prefers_2d` (scs_api.cpp:1284), Python
  `_functions.py:563`, and R if it is bound there.

### S02b-007 [P2, confirmed] Voronoi bisector built in double: nearly coincident sites lose their cell
- **Where:** include/socialchoicelab/geometry/voronoi_2d.h:50–64. Decision: Q7
- **What:** `perp_bisector_line` converts the exact sites to double, rounds the midpoint,
  and builds the line from two rounded points. When sites differ by a few ulps, the
  rounded midpoint can land on a site, so `oriented_side(ai)` is `ON_ORIENTED_BOUNDARY`
  and the cell collapses. It can also land beyond the site, which gives the wrong half.
  Q7's Implies list covers exactly co-located sites only. Competitors that converge
  numerically, but not bit-exactly, fall in this gap.
- **Failure scenario:** `SCR/t10_voronoi.cpp` (b): sites (1,0) and (1+2⁻⁵², 0) in box
  [-1,3]×[-1,1]. Site 0 gets an empty cell, and site 1 gets area 4, although every point
  with x < 1 is nearer site 0. Case (c) shows the known Q7 co-located overlap: two
  identical cells of area 4.
- **Suggested fix:** Use the exact bisector (`CGAL::bisector(ai, aj)` on EPEC points), so
  each site is strictly on its own side whenever ai ≠ aj. Apply Q7's shared-cell rule to
  exact equality only.
- **Also affects:** Voronoi overlays in R/Python competition canvases.

### S02b-008 [P2, confirmed] Uncovered-set boundary is not an outer bound, collapses to a point, and has no size limit
- **Where:** include/socialchoicelab/geometry/uncovered_set.h:115, :170–177, :236–238, :252–264
- **What:**
  - The doc calls the hull "a conservative outer bound of the true continuous uncovered
    set within the sampled region". Covering is only tested against grid challengers and
    grid coverers, so the result is neither an inner nor an outer bound. At normal sizes
    the hull degenerates to one grid point.
  - The stated complexities are wrong. `covers` loops over all m alternatives, so
    `uncovered_set` is O(m³n), not O(m²n). The boundary is O(res⁶·n), not O(res⁴·n).
  - `grid_resolution` is silently clamped to ≥ 2, has no upper bound, and `res*res`
    overflows `int` for res > 46,340. This is the clang-tidy widening-cast lead.
- **Failure scenario:** `SCR/t14_uncovered_perf.cpp`, N(0,1) voters at the C++ default
  res = 30:

  | Voters | Hull vertices | Area | Time |
  |---|---|---|---|
  | 51 | 1 | 0 | 1.3 s |
  | 101 | 1 | 0 | 1.7 s |
  | 11 | 6 | 0.098 | 1.0 s |

  The single point cannot contain the (positive-area) uncovered set. For 11 voters at
  res = 15, the area is 0.028, so the result depends strongly on resolution. At res =
  1,000 (C API, no cap), the computation needs about 10¹² `covers` calls.
- **Suggested fix:** Describe the result as "convex hull of grid points uncovered relative
  to the grid; neither inner nor outer bound". Return or flag degenerate (< 3-vertex)
  results. Validate grid_resolution in [2, documented maximum] with an end-user error.
  Cache the m×m beats matrix (as `heart()` did) to get O(m²n + m³). Correct the
  complexity notes.
- **Also affects:** `scs_uncovered_set_boundary_2d(_heap)` (no grid_resolution check);
  R/Python `uncovered_set_boundary_2d`; the `layer_uncovered_set` overlay.

### S02b-009 [P2, confirmed] `yolk_2d` cost is O(n²·10⁴) with no limit; `n_sample` is unvalidated
- **Where:** include/socialchoicelab/geometry/yolk.h:336–337, :369, :379–405, :433–464
- **What:**
  - The code builds n(n−1)/2 + n_sample lines, each with an O(n log n) sort, then runs
    10,000 iterations over all lines. The voter spread is also computed in O(n²).
  - `n_sample` is never validated. A negative value makes `reserve` throw `length_error`,
    which the C API reports as INTERNAL.
  - `n*(n−1)/2` overflows `int` for n > 46,341 (clang-tidy lead at :369; same at :110,
    :201).
- **Failure scenario:** `SCR/t08_yolk_perf.cpp`: n = 100 takes 0.08 s, n = 300 takes
  0.77 s, and n = 600 takes 3.9 s. At that scaling, n = 5,000 takes many minutes, and the
  line array alone is about 300 MB. Nothing stops a caller from passing it.
- **Suggested fix:** Validate `n_sample ≥ 0` with a clear error. Cap n or document the
  cost. The LP replacement in S02b-002 also removes the 10⁴-iteration loop.
- **Also affects:** `scs_yolk_2d`.

### S02b-010 [P2, confirmed] `winset_2d` uses an absolute 1e-12 cut-off and silently drops non-simple polygons
- **Where:** include/socialchoicelab/geometry/winset.h:114, :186, :277–281
- **What:** A voter whose (weighted) distance to sq is < 1e-12 in absolute terms is
  treated as being at sq and skipped. A preferred-to polygon that becomes non-simple after
  rounding (a tiny radius on large coordinates, where vertices collapse to the same
  double) is also silently skipped. Both change who can be in a winning coalition, with no
  error, and the result is not scale-invariant.
- **Failure scenario:** `SCR/t16_winset_scale.cpp`: the same 5-voter configuration scaled
  by 1, 1e-6 and 1e-12 has a normalised area of 3.946. Scaled by 1e-13, it is reported
  empty.
- **Suggested fix:** Skip only voters exactly at sq (r == 0), or use a tolerance relative
  to the coordinate scale. Throw (or report) when a non-degenerate voter's polygon is not
  simple, instead of dropping it.
- **Also affects:** veto and weighted winsets; `core_2d`.

### S02b-011 [P2, confirmed] Duplicate voters give degenerate `0x + 0y + 0 = 0` quantile lines flagged "limiting"
- **Where:** include/socialchoicelab/geometry/yolk.h:117–139, :205–224, :248–256
- **What:** For a pair of identical ideal points, dx = dy = 0, so `Line2E(0, 0, −0)` is
  constructed, which violates CGAL's Line_2 precondition. In the annotated version,
  `pair_proj == pk` (0 == 0) marks it limiting, so `limiting_quantile_lines_2d` returns it.
  `signed_distance_to_line` hides it by returning 0. (`yolk_2d` itself skips such pairs.)
- **Failure scenario:** `SCR/t07_quantile_leak.cpp`: voters 2 and 3 at (0.3, 1) give
  "pair (2,3): degenerate line a=b=0, c=-0, is_limiting=1". Duplicate ideal points are
  common in survey and grid data.
- **Suggested fix:** Skip pairs with identical points in both functions, document that the
  output has at most n(n−1)/2 entries, and add a duplicate-voter test.
- **Also affects:** none outside C++; not in the C API.

### S02b-012 [P2, confirmed] "Pareto set ⊆ convex hull (outer bound)" is false for Manhattan
- **Where:** include/socialchoicelab/geometry/convex_hull.h:21–23; docs/architecture/geometry_design.md:135–137, :455–456
- **What:** The header and design doc say that under non-Euclidean metrics the Pareto set
  is a subset of the hull, so the hull is an outer bound. Under L1 the Pareto set can
  extend outside the hull. ROADMAP K3 and the canvas overlay schema rely on the
  "outer bound" wording when labelling the hull.
- **Failure scenario:** `SCR/t13_hull_pareto_l1.cpp`: Manhattan voters (0,0) and (1,1).
  The hull is the segment between them; x = (1,0) lies off it. Among 361,201 grid
  points, none Pareto-dominates x (1e-12 tolerance), so x is Pareto-optimal and outside
  the hull. The whole unit square is Pareto here.
- **Suggested fix:** Say the hull is the Pareto set only for Euclidean (and uniformly
  weighted, affinely equivalent) preferences, and that for other metrics it is neither
  an inner nor an outer bound in general. Check ROADMAP and `canvas_overlay_schema.md`
  wording.
- **Also affects:** docs only (ROADMAP §Non-Euclidean, canvas_overlay_schema.md §Metric support).

### S02b-013 [P3, confirmed] "The Copeland winner always lies in the uncovered set" fails when majority ties exist
- **Where:** include/socialchoicelab/geometry/copeland.h:20; docs/architecture/geometry_design.md:387
- **What:** The property holds for tournaments (complete, asymmetric). Spatial majority has
  ties (equidistant voters, even n, k-majority), and with the first-index tie-break the
  returned Copeland winner can be covered. The header also attributes the "strong point"
  to Banks (1985); the strong point is Straffin's (1980), and Banks (1985) is about
  sophisticated voting.
- **Failure scenario:** `SCR/t17_copeland_uncovered.cpp`: n = 5 voters (0,4), (4,0), (4,4),
  (1,3), (4,1) and alternatives (2,3), (1,0), (0,3), (4,3), (3,4), (0,2). Scores are
  3, −4, −1, 3, 2, −3. The winner returned is (2,3), which is covered by (4,3).
- **Suggested fix:** Qualify the claim ("for tournaments, e.g. odd n with no ties"), or
  break Copeland ties in favour of uncovered alternatives. Fix the strong-point citation.

### S02b-014 [P3, confirmed] `core_2d` unanimity comment contradicts itself and the theory
- **Where:** include/socialchoicelab/geometry/core.h:135–139 (and :21–22)
- **What:** The comment says that under unanimity a Condorcet winner "always exists", then
  that every interior hull point loses and the only winner is a common ideal point. Under
  k = n, no point is unanimously preferred to a Pareto-optimal point, so the core is the
  whole Pareto set (the hull, for Euclidean preferences): many points, not one.
  `core_2d` returns just the first candidate (the Yolk centre, which is the centroid; see
  S02b-002).
- **Failure scenario:** For voters (0,0), (1,0), (0,1) with k = 3, every hull point is
  undominated. The comment says only a common ideal point can be.
- **Suggested fix:** State that under unanimity the core equals the Pareto set, and that
  `core_2d` returns one element of it.

### S02b-015 [P3, confirmed] Uncovered-set grid default is 30 in C++ but 15 in the C API and both bindings
- **Where:** include/socialchoicelab/geometry/uncovered_set.h:171, :186, :194; geometry_design.md:321; scs_api.h:61 (`SCS_DEFAULT_BOUNDARY_GRID_RESOLUTION 15`)
- **What:** The C++ default and its doc ("900 grid points … < 1 s for n ≤ 20") describe
  res = 30. Users of R/Python get 15 (geometry.R:189, _geometry.py:307). S02b-008 shows
  how much the output changes between the two (area 0.028 vs 0.098 for the same 11
  voters). Res = 30 takes 1.0–1.5 s at n = 11–21, so the "< 1 s" claim is borderline.
- **Suggested fix:** Pick one default, use it in all layers, and document the run time it
  implies.

### S02b-016 [P3, confirmed] The Yolk is described differently in each place, and none matches the code
- **Where:** include/scs_api.h:580–585; include/socialchoicelab/geometry/yolk.h:9–11, :22–30, :309–322; docs/architecture/geometry_design.md:276–297
- **What:**
  - The C API says it computes the LP yolk (limiting median lines only). The code uses
    720 sampled directions plus all critical directions, so its line set is a superset
    of the limiting lines.
  - The header says C2 and C3 are "future" (:9–11) and gives an accuracy bound that does
    not hold (S02b-002).
  - The design doc shows `yolk_2d(voter_ideals, k)` without `n_sample`, describes "pairs
    of orthogonal directions", and calls the centre the "centre of gravity".
  - The design doc attributes the 4r result to McKelvey's 1979 chaos theorem. The 4r
    bound on the uncovered set and on sophisticated outcomes is McKelvey (1986); see
    FGM 1988, p. 38.
- **Failure scenario:** A reader of `scs_api.h` expects LP-yolk radii (lower bounds on
  the yolk). The code returns a centroid-based upper bound instead.
- **Suggested fix:** Rewrite all three descriptions once the solver is replaced, with
  the line family and accuracy stated in one place.

### S02b-017 [P3, confirmed] Citations do not match the sources in `references/`, and two claims rest on them
- **Where:** yolk.h:44–45, :325–327, :488–489; uncovered_set.h:33–34, :180–182; winset.h:432–434 vs geometry_design.md:242, :495; geometry_design.md:527–528
- **What:**
  - **FGM 1988.** The code cites "Feld, Grofman & Miller (1988), 'Centripetal Forces in
    Spatial Voting: The Yolk and the Grid,' AJPS 32(2), 459–481". The paper in
    `references/social_choice/core/FeldGrpfmanMillerYolk.pdf` is "Centripetal forces in
    spatial voting: On the size of the Yolk", *Public Choice* 59: 37–50 (1988). It
    assumes n odd (p. 39) and does not state "Yolk ⊆ Uncovered Set" (uncovered_set.h:34)
    or that "the Yolk center is always contained in the uncovered set" (:180–182). It
    states the reverse kind of bound: the uncovered set lies within 4r of the yolk
    centre (p. 38).
  - **Koehler.** The design doc cites Koehler (1992) "The size of the yolk: computations
    for odd and even-sized committees", SCW 9: 231–245. The Koehler PDF in `references/`
    is a different paper: "Limiting median lines frequently determine the yolk: a
    rejoinder", SCW 9: 37–41 (1992). The cited paper is not in `references/`.
  - **Tsebelis.** Veto players are cited as Tsebelis (1995) in code and Tsebelis (2002)
    in the design doc. Neither is in `references/`.
- **Failure scenario:** Tests and docs treat "Yolk centre ∈ uncovered set" as a
  theoretical guarantee (uncovered_set.h:179–182) without a source that proves it.
- **Suggested fix:** Correct the FGM citation. Drop or source the two inclusion claims.
  Add the papers actually relied on to `REFERENCE_INDEX.md`.

### S02b-018 [P3, confirmed] `geometry_design.md` documents API that does not exist
- **Where:** docs/architecture/geometry_design.md:202–204 (`winset_area`), :457–461 (`min_area`), :248–252, :199
- **What:**
  - `winset_area(const WinsetRegion&)` and a `min_area` option are documented but exist
    nowhere in `include/` or `src/`.
  - §7 calls `winset_is_empty` "no point majority-beats sq". That line is covered by Q5.
  - The weighted section is covered by Q6.
  - §9 is listed in S02b-016.
- **Failure scenario:** A user following §15's advice ("use `min_area > 0`") finds no such
  parameter.
- **Suggested fix:** Remove or implement `winset_area`/`min_area`, and regenerate the API
  blocks from the headers.

### S02b-019 [P3, confirmed] Heart leftovers to be removed (Q1)
- **Where:** Decision: Q1. Beyond Q1's Implies list:
  - include/socialchoicelab/geometry/geom2d.h:43 ("Heart approximation")
  - geometry_design.md:18, :27, :296–297, :466–468, :494, :504–505, :529–530
  - CMakeLists.txt:186–189 (`test_heart` target) and :354–358 (`scratch/dump_heart.cpp`)
- **What:** Withdrawn concept still referenced (not reviewed for correctness).
- **Failure scenario:** n/a (removal list).
- **Suggested fix:** Remove with the Q1 work.

### S02b-020 [P3, confirmed] Small API hygiene items in majority/core
- **Where:** majority.h:68–76, :174–200; core.h:58–114
- **What:**
  - `resolve_k` treats every negative k as "simple majority", although the docs say -1.
  - `pairwise_matrix` takes k, validates it and ignores it.
  - `has_condorcet_winner` duplicates `condorcet_winner` line for line; it could be
    `condorcet_winner(...).has_value()`.
- **Failure scenario:** `majority_prefers(..., k = -7)` silently means simple majority, and
  a caller passing k to `pairwise_matrix` expects it to matter.
- **Suggested fix:** Accept only -1 or [1, n]. Drop k from `pairwise_matrix`. Implement
  `has_condorcet_winner` via `condorcet_winner`.

## Tool leads checked
- clang-tidy `NewDeleteLeaks` yolk.h:229 → false positive. CGAL Lazy handle
  reference-counting confuses the analyser. `leaks --atExit` over 2,000 calls to
  `limiting_quantile_lines_2d` reports 0 leaks (`SCR/t07_quantile_leak.cpp`).
- `misplaced-widening-cast` uncovered_set.h:238 → real for res > 46,340 (int overflow in
  `res*res`); part of S02b-008.
- `misplaced-widening-cast` yolk.h:369 → real for n > 46,341 (and for negative
  n_sample); part of S02b-009.
- `implicit-widening-of-multiplication-result` voronoi_2d.h:178 → false positive in
  practice. `2*i` overflows only for n_sites > 2³⁰, and the O(n²) clipping is infeasible
  long before that.
- `easily-swappable-parameters` (uncovered_set.h:80, :194; voronoi_2d.h:127, :164;
  winset.h:260, :321, :513; winset_1d.h:44; yolk.h:336) → design note, not a defect. The
  `(grid_resolution, k)` order of `uncovered_set_boundary_2d` already differs between R
  and Python (Stage 1, for Stage 6).
- Coverage `geom2d.h` 33% → correct but partly untested. `orientation` and
  `bounded_side` are tested directly in `test_geom2d.cpp`; the low figure looks like
  inline-header attribution. Note for Stage 4.
- Coverage `winset.h` 57% → `test_winset.cpp` has no Manhattan, Chebyshev, Minkowski or
  weighted-Euclidean case, so the untested `pts_polygon` branches are where S02b-001
  hides. Those branches are correct for unequal weights (`SCR/t18_winset_consistency.cpp`).
  Note for Stage 4.
- Stage 1, Yolk "LP-yolk approximation" → worse than an approximation: S02b-002
  (optimiser), S02b-005 (line family), S02b-016 (descriptions).
- Stage 1, grid approximations → S02b-008.
- Stage 1, default grid 30 vs 15 → S02b-015.
- Stage 1, `core_2d` unanimity comment → S02b-014.
- Stage 1, Voronoi double bisector then exact clip → S02b-007 (plus S02b-004, found while
  checking it).
- Exact vs double boundaries → checked every conversion:
  - `to_exact` of sampled preferred-to polygons: the approximation is known (Q5); the
    absolute cut-off is S02b-010.
  - `winset_contains`: exact.
  - Convex hull: exact on double inputs, lossless.
  - `quantile_lines_2d`: exact.
  - `yolk_2d`: recomputes lines in double (no exactness loss that matters beside
    S02b-002 and S02b-005).
  - Voronoi bisector: S02b-007.
  - `to_numeric` outputs: nearest double, display only.
  - Non-finite values into EPEC abort the process (see Contracts; S02b-003).
- Q2 `winset.h:164` p ≥ 100 branch → known (Q2), not re-reported. No other unscaled
  `pow` in this slice.
- Q9 → no static or global mutable state in the slice. The shared-region read race is
  already recorded (Q9).

## Open questions
1. **Zero salience weights in 2D geometry.** Convention B allows them, but the
   preferred-to set becomes an unbounded strip. Should winset, Yolk, uncovered-set and
   Voronoi calls reject weight 0 with an end-user error, or clip strips to a documented
   box? Today the winset path aborts the process (S02b-003).
2. **k-Yolk definition for even n and supermajority.** Should the Yolk meet both edges of
   each median band (Koehler 1992's even-n definition), or one edge as now? This needs a
   cited source for the k-majority case.
3. **"Yolk centre ∈ uncovered set".** Is this a result the project wants to rely on in
   tests and docs? The cited FGM 1988 paper does not prove it.
4. **Voronoi shared cells (Q7).** Should "co-located" mean bit-identical, or within a
   tolerance? With an exact bisector (S02b-007), only bit-identical sites are degenerate.
5. **`uncovered_set_boundary_2d` output.** Is a 1- or 2-vertex result acceptable, or
   should it be flagged or raised as an error (S02b-008)?

## Out-of-slice notes
- `preference/indifference/level_set.h:236–237` (2a): equal non-unit weights build a
  circle of radius r instead of r/w (`SCR/t19_levelset_equal_weights.cpp`: radius 2,
  should be 1). This is the same bug as S02b-001 and affects `ic_polygon_2d` and all IC
  overlays.
- `src/c_api/scs_api.cpp:1777` (3a): `scs_voronoi_cells_2d` does not check sites or bbox
  for finiteness. A NaN site or NaN bbox bound kills the process with SIGFPE
  (`SCR/t12_capi_voronoi_nan.c`, exit 136 in both modes). Check `_heap` too.
- `src/c_api/scs_api.cpp:160` (3a): `to_dist_config` accepts zero and negative weights.
  Zero weights crash winsets (S02b-003).
- `src/c_api/scs_api.cpp:2345` (3b): `scs_uncovered_set_boundary_2d` does not validate
  `grid_resolution` or `n_voters` (n_voters ≤ 0 goes to `validate_finite_doubles` with a
  non-positive length).
- R `make_dist_config` and Python `make_dist_config`/`DistanceConfig` do not validate
  weights (positive, finite, length), although the Python docstring says "strictly
  positive".
- **Stage 4:** the Yolk tests cover only configurations whose centroid is the Yolk
  centre, which is why S02b-002 went undetected. The winset tests have no non-unit or
  non-Euclidean weights (S02b-001). The Voronoi tests have no bisectors through
  vertices (S02b-004).
