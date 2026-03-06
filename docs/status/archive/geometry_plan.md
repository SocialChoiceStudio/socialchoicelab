# Geometry Plan

Short-to-medium-term plan to deliver **Layer 3 (Geometry Services)** of the
SocialChoiceLab C++ core. Covers the geometry foundation (CGAL integration,
exact 2D primitives, convex hull), core geometry services (majority preference,
winsets, Yolk, uncovered set), and extended winset services (set operations,
core detection, Copeland winner, veto players, weighted voting, transaction costs).

**Authority:** This plan is the source for "what's next" for geometry work.
When a step is done, mark it ✅ Done and update [where_we_are.md](where_we_are.md).

**References:**
- [Design Document](../architecture/design_document.md) — Layer 3 Geometry Services
- [Implementation Priority Guide](../references/implementation_priority.md) — Phase 1 geometry
- [Milestone gates](milestone_gates.md) — geometry milestone definition of done
- [Roadmap](roadmap.md) — near/mid-term context

---

## Scope

**In scope:**

- CGAL EPEC integration into CMake and CI
- Exact 2D type layer: `Point2E`, `Segment2E`, `Polygon2E`, conversion to/from Eigen `Point2d`
- Convex hull of 2D point sets
- Basic 2D predicates: orientation, point-in-polygon, bounded-side
- Majority preference relation (pairwise majority given voter ideal points)
- 2D winset: the set of points beating a given status quo by strict majority
- Yolk computation: smallest circle intersecting all median lines
- Uncovered set (Heart): set of alternatives not covered by any other

**Out of scope for this plan:**

- 3D / N-D geometry (Phase 3 in implementation priority guide)
- Voting rules and aggregation (separate plan, depends on geometry)
- c_api extensions for geometry (follow-on once geometry is stable)
- R / Python bindings
- Visualization / plot helpers (petal display, winset overlays — belong in R/Python plot layer)
- Preference model extensions: directional model, complementarity/tradeoff metric, bounded rationality (see Roadmap § Possible future extensions)

---

## Background: key concepts

**Pareto set:** The set of alternatives that are not Pareto dominated — no other
alternative makes every voter at least as well off and at least one voter strictly
better off. For **Euclidean preferences** the Pareto set equals the convex hull of
the voter ideal points (a well-known result); this is why Phase A3 (convex hull)
directly yields the Pareto set in the Euclidean case. For **non-Euclidean metrics**
(Manhattan, Chebyshev, Minkowski p) the Pareto set is always contained within the
convex hull but may be a proper subset — the convex hull gives an outer bound, not
the exact set. The uncovered set and Heart are always subsets of the Pareto set.

> **Open question:** Does a generalised Pareto set exist for non-Euclidean
> preferences that has an equally clean geometric characterisation? Revisit when
> non-Euclidean winsets (Phase B4) are implemented and we have concrete examples
> of how the Pareto set differs from the convex hull under those metrics.

**Preferred-to set:** For voter `i` with ideal point `pᵢ` and a given status quo
`q`, the preferred-to set is the set of all alternatives that voter `i` strictly
prefers to `q` — all points strictly closer to `pᵢ` than `q` is. Geometrically
this is the open interior of voter `i`'s indifference curve through `q`. For
Euclidean preferences it is an open disk centred at `pᵢ` with radius `d(pᵢ, q)`;
for other Minkowski metrics it is the interior of the corresponding level-set shape
(diamond, square, etc.). The winset `W(q, k)` is exactly the set of points that
lie inside at least `k` voters' preferred-to sets simultaneously. No new
implementation is needed — `level_set_2d` (core) already computes the boundary,
and Phase B3 uses the interiors implicitly when building the winset.

**Majority preference (k-majority):** Alternative `x` is *k-majority preferred*
to `y` given voter ideal points `p₁…pₙ` if at least `k` voters are strictly
closer to `x` than to `y` under their distance metric. The threshold `k` can be
any value from 1 to n: `k = 1` is unanimity-for-x (any voter prefers x),
`k = ⌊n/2⌋ + 1` is simple majority, `k = ⌈2n/3⌉` is a two-thirds supermajority,
`k = n` is unanimity. All functions in this plan accept `k` as a parameter
defaulting to simple majority.

**Winset W(q, k):** The set of all alternatives that beat status quo `q` by at
least `k` voters — i.e. the alternatives at least `k` voters prefer to `q`. In
2D Euclidean space this is a (possibly non-convex) polygon formed by intersecting
the half-planes on the preferred side of each voter's indifference curve through
`q`, for the winning coalition size `k`. Larger `k` (supermajority) yields a
smaller or empty winset; a Condorcet winner produces an empty winset for any
`k ≤ ⌊n/2⌋ + 1`. Under simple majority (`k = ⌊n/2⌋ + 1`) this is the classical
winset. Exact computation requires CGAL.

**Yolk (k-Yolk):** The smallest circle that intersects every k-quantile line — a
line in 2D where exactly `k` voters are on one side (the minimum needed to achieve
k-majority). The k-Yolk center approximates the "center of politics" under the
given majority rule, and its radius measures electoral instability: a larger radius
means more agenda-setter power. Under simple majority (`k = ⌊n/2⌋ + 1`) the
k-quantile lines are the classical median lines and the k-Yolk reduces to the
classical Yolk (Feld, Grofman, Miller). A supermajority k-Yolk may be displaced
from the simple-majority Yolk toward the minority side.

**Uncovered set (k-uncovered set):** Alternative `x` *k-covers* `y` if (1)
`x` is k-majority preferred to `y`, and (2) everything k-majority preferred to
`x` is also k-majority preferred to `y`. The **k-uncovered set** is the set of
alternatives not k-covered by any other. It is a superset of the k-core (the
Condorcet winner under k-majority, if one exists) and a superset of the k-Heart.
Under simple majority (`k = ⌊n/2⌋ + 1`) this is the classical uncovered set.
Larger k (supermajority) expands the uncovered set because the covering relation
becomes harder to satisfy.

**Heart (k-Heart):** The set of points in policy space that cannot be defeated
through any sequence of k-majority moves staying within the k-majority win sets
implied by voter preferences. It is a *tighter* stability concept than the
uncovered set: k-Heart ⊆ k-Uncovered Set, and the two coincide only when a
Condorcet winner exists. The k-Heart is located near the k-Yolk center. A larger
k (supermajority) expands the k-Heart because each move requires broader support.
Under simple majority (`k = ⌊n/2⌋ + 1`) this is the classical Heart. Computing it
exactly requires an iterative fixed-point algorithm over k-majority win sets; in
practice a discrete or grid approximation is used.

**Core (k-core):** The set of alternatives with an empty k-majority winset — no
other alternative can beat them by k-majority. A k-majority Condorcet winner, if
one exists, is the unique element of the k-core. In continuous 2D space the core
is generically empty under simple majority (McKelvey's chaos theorem), but becomes
non-empty more often as k increases (higher k → stronger status quo bias). Under
simple majority (`k = ⌊n/2⌋ + 1`) this is the classical core.

**Copeland winner / strong point (k-Copeland winner):** The alternative that beats
or ties the most others in k-majority pairwise comparisons, i.e. the alternative
with the highest k-majority Copeland score (number of alternatives it k-beats minus
number that k-beat it). Always exists. Contained in the k-uncovered set. In
continuous policy space this is equivalently called the **strong point**: the point
that minimises the area of its own win-set, i.e. the fewest other alternatives beat
it (Feld et al. 1987, Definition 9; Shapley and Owen 1989). The strong point can be
computed as the SOV-weighted average of voter ideal points (Godfrey, Grofman &
Feld 2011). Under simple majority (`k = ⌊n/2⌋ + 1`) this is the classical Copeland
winner / strong point. Higher k makes it harder to beat alternatives, compressing
scores toward zero.

**Veto players:** Actors whose agreement is required for any change from the status
quo. Geometrically, each veto player constrains the k-majority winset: only
alternatives inside that player's preferred-to-SQ region (their indifference curve
through the SQ) can be adopted. The effective winset is `W(q, k)` intersected with
every veto player's preferred region. A single veto player at the SQ produces an
empty effective winset regardless of k. The veto player constraint is orthogonal to
k: increasing k shrinks the winset, adding veto players shrinks it further.

**Weighted voting:** Each voter carries a vote weight `wᵢ` (not necessarily 1).
The k-majority count is replaced by a weight-sum threshold: alternative `x` beats
`y` if the total weight of voters who prefer `x` exceeds a fraction `k` of total
weight (e.g. `k = 0.5` for simple majority, `k = 2/3` for supermajority). This
generalises the equal-weight k-majority without changing the geometric structure
(same cutlines, different counting rule). Compatible with all concepts above.

**Winset set operations:** Given any two k-majority winsets (e.g. `W(q₁, k)` and
`W(q₂, k)`, or `W(q, k₁)` and `W(q, k₂)` under different thresholds), the useful
analytical operations are union, intersection, difference, and symmetric difference.
The k parameter is set when the winsets are built; the set operations themselves
are purely geometric. CGAL provides exact polygon boolean operations for all four.

---

## Step-by-step plan

### Phase A — Geometry Foundation

| Step | Task | Deliverable | Status |
|------|------|-------------|--------|
| **A1** | **CGAL integration** | Add CGAL to CMakeLists.txt (`find_package(CGAL REQUIRED)` with system install; add install steps to CI workflow for Ubuntu and macOS). Define kernel aliases in `include/socialchoicelab/core/kernels.h`: `EpecKernel` (Exact_predicates_exact_constructions_kernel) and `EpicKernel` (Exact_predicates_inexact_constructions_kernel). Verify build on both platforms. | ✅ Done |
| **A2** | **Exact 2D type layer** | `include/socialchoicelab/geometry/geom2d.h`: type aliases `Point2E`, `Segment2E`, `Polygon2E` (using CGAL EPEC types). Conversion functions `to_exact(Point2d) → Point2E` and `to_numeric(Point2E) → Point2d`. Basic predicates: `orientation(a, b, c)` (LEFT_TURN / RIGHT_TURN / COLLINEAR), `bounded_side(polygon, point)` (ON_BOUNDED_SIDE / ON_BOUNDARY / ON_UNBOUNDED_SIDE). Tests in `tests/unit/test_geom2d.cpp`. | ✅ Done |
| **A3** | **Convex hull 2D** | `include/socialchoicelab/geometry/convex_hull.h`: `convex_hull_2d(std::vector<Point2d>) → Polygon2E` (CGAL `convex_hull_2`). Input validation: at least 1 point, all finite. Handle degenerate cases: 1 point returns single-vertex polygon; 2 collinear points returns 2-vertex polygon; general position returns CCW hull. Tests cover: triangle, square, random point cloud, collinear input, single point. | ✅ Done |
| **A4** | **Design doc update** | Add `docs/architecture/geometry_design.md`: kernel policy rationale (EPEC vs EPIC), type layer design, conversion strategy, CGAL dependency rationale. Update `design_document.md` Layer 3 to point to this doc and mark it in progress. | ✅ Done |

### Phase B — Majority Preference and Winsets

| Step | Task | Deliverable | Status |
|------|------|-------------|--------|
| **B1** | **Majority preference relation (with k-majority)** | `include/socialchoicelab/geometry/majority.h`: `majority_prefers(x, y, voter_ideals, dist_cfg, k = n/2+1) → bool` — returns true if at least `k` voters are strictly closer to `x` than `y`. Default `k` is simple majority; caller can pass any value for supermajority (e.g. 2/3·n) or unanimity (n). Also `preference_margin(x, y, voter_ideals, dist_cfg) → int` (net count: #prefer_x − #prefer_y, sign indicates winner). Tests: simple majority (even/odd n, ties, single voter); k=n (unanimity); k=2/3·n (supermajority); all-prefer-same. | ✅ Done |
| **B2** | **Pairwise comparison matrix** | `pairwise_matrix(alternatives, voter_ideals, dist_cfg, k = n/2+1) → Eigen::MatrixXi` where entry (i,j) = preference_margin(alt_i, alt_j). `k` threshold propagated so the matrix reflects k-majority dominance. Used downstream for Condorcet winner detection and Borda count. Tests: known 3-alternative Condorcet cycle; supermajority that eliminates some dominance edges. | ✅ Done |
| **B3** | **Winset computation (2D, with k-majority)** | `include/socialchoicelab/geometry/winset.h`: `winset_2d(status_quo, voter_ideals, dist_cfg, k = n/2+1) → Polygon2E` — exact polygon of all points beating the status quo by at least `k` voters under Euclidean distance. Algorithm: each voter's indifference circle through the SQ defines a half-plane (the closer side); the winset is the intersection of the k-majority coalition half-planes. Use CGAL half-plane intersection (`CGAL::halfplane_intersection_2`). Tests: 3-voter triangle simple majority (known closed form); 5-voter symmetric case; supermajority (smaller winset); k=n (Pareto set); degenerate (Condorcet winner → empty winset). | ✅ Done |

> **⚠️ Algorithm note — review before implementing B3.**
> The "half-plane intersection" description above is incorrect and must be corrected before coding begins.
>
> **Why it is wrong:** Each voter's preferred-to set (PTS) is an open **disk** centred at their ideal point with radius `d(pᵢ, q)` — a bounded, circular region. The condition `d(x, pᵢ) < d(pᵢ, q)` is quadratic in `x` and has no exact reformulation as a linear half-plane. `CGAL::halfplane_intersection_2` operates on true linear half-planes and will give an incorrect result.
>
> **Correct algorithm — the petal method:**
> 1. For each voter `i`, compute the indifference disk: centre = `pᵢ`, radius = `d(pᵢ, q)`. Approximate as a convex polygon.
> 2. Enumerate all **C(n, k) minimal winning coalitions** (subsets of exactly `k` voters).
> 3. For each coalition `{i, j, l, …}`: sequentially intersect the `k` disks → one **petal** (a lens-shaped convex region, always anchored at `q` on one boundary). Many petals will be empty — skip them.
> 4. **Win set = union of all non-empty petals.**
>
> CGAL primitives to use: `CGAL::Polygon_2` for disk approximations and `CGAL::Boolean_set_operations_2` (intersection then join) rather than `halfplane_intersection_2`.
>
> The agent implementing B3 should derive the algorithm independently from the petal method description above.

| **B4** | **Winset: Minkowski distance extension** | Extend `winset_2d` to support Manhattan and general Minkowski p via the `DistanceConfig`. For non-Euclidean metrics the indifference boundary is not a circle — use the exact `Polygon2d` representation from `level_set_2d` (already implemented) as the boundary for each voter, then compute the half-polygon intersection. `k` parameter passes through unchanged. Tests: Manhattan (diamond boundaries), Chebyshev (square boundaries), each with simple and supermajority k. | ✅ Done |

> **📐 Design note — polygon approximation, area threshold, and degenerate cases.**
>
> The winset implementation uses polygon approximations of preferred-to sets (Lp balls sampled at `num_samples` vertices). This introduces two interacting parameters that must be considered together, analogous to the `k_near_zero_rel` / `k_near_zero_abs` pattern in `numeric_constants.h`:
>
> **Approximation resolution (`num_samples`):** More vertices → smaller boundary error → spurious intersection area near geometrically-tangent cases shrinks toward zero.
>
> **Area threshold (`min_area`):** Rather than asking CGAL "is this polygon set exactly empty?", ask "is the area meaningfully large?" — area > ε relative to problem scale. A tangent case (true area = 0) produces area ≈ 0 < ε → correctly empty. A genuine non-empty winset has area proportional to the problem scale → well above ε → correctly non-empty.
>
> **When this matters:** The median voter theorem (Feld & Grofman 1987, Theorem 1') guarantees an empty winset when the SQ is the collinear median voter's ideal. In that configuration the two flanking voters' preferred-to disks are exactly tangent at the SQ. With finite `num_samples` the polygon approximation may produce a tiny spurious overlap of area ≈ δ². A `min_area` threshold ε >> δ² but ε << (any genuine winset area) resolves this without false positives.
>
> **Action items:**
> - Add `winset_area(ws) → double` to `winset.h` (sum of face areas from `polygons_with_holes`).
> - Add optional `min_area` parameter to `winset_is_empty(ws, min_area = 0.0)`.
> - Document the relationship between `num_samples`, boundary error δ, and safe `min_area` choices.
> - Revisit once Phase C (Yolk) is implemented: the Feld–Grofman (1991) monotonicity result (winset radius increases along rays from the yolk center) provides the first quantitative, non-degenerate tests of winset size comparison.

### Phase C — Yolk

| Step | Task | Deliverable | Status |
|------|------|-------------|--------|
| **C1** | **k-quantile lines** | `include/socialchoicelab/geometry/yolk.h` (detail): `quantile_lines_2d(voter_ideals, k = n/2+1) → std::vector<Line2E>` — produces the k-quantile lines: lines where exactly `k` voters are on one side (the minimum for k-majority). Under simple majority (`k = ⌊n/2⌋ + 1`) these are the classical median lines. Document the Feld-Grofman-Miller algorithm and the generalisation to arbitrary k. | ✅ Done |
| **C2** | **k-Yolk (smallest enclosing circle of k-quantile lines)** | `yolk_2d(voter_ideals, k = n/2+1) → Circle2d`: the smallest circle intersecting every k-quantile line. Algorithm: iterate over candidate circles defined by triples of k-quantile lines; use CGAL exact arithmetic throughout. Return center and radius as `core::types::Point2d` + `double`. Tests: 3-voter equilateral triangle under simple majority (classical Yolk = centroid); same configuration under supermajority (k-Yolk displaced); verify radius = 0 iff k-majority Condorcet winner exists; odd/even n. | ✅ Done |
| **C3** | **Yolk radius as instability measure** | `yolk_radius(voter_ideals, k = n/2+1) → double` convenience function. Document: large radius → high agenda-setter power / electoral instability under the given k-majority rule; zero radius → k-majority Condorcet winner exists; supermajority k generally produces a different (possibly larger) radius than simple majority. Tests: Condorcet winner case (radius ≈ 0); spread-out voter case; compare radius across several k values on the same voter configuration. | ✅ Done |

### Phase D — Uncovered Set

| Step | Task | Deliverable | Status |
|------|------|-------------|--------|
| **D1** | **Covering relation (with k-majority)** | `include/socialchoicelab/geometry/uncovered_set.h`: `covers(x, y, alternatives, voter_ideals, dist_cfg, k = n/2+1) → bool` — x k-covers y iff (1) k_majority_prefers(x, y) and (2) for all z in alternatives with k_majority_prefers(z, x), k_majority_prefers(z, y). Tests: textbook Condorcet cycle (no covers under simple majority); supermajority k that changes which alternatives cover; Condorcet winner covers all others. | ✅ Done |
| **D2** | **Uncovered set over a finite set** | `uncovered_set(alternatives, voter_ideals, dist_cfg, k = n/2+1) → std::vector<Point2d>` — the subset of alternatives not k-covered by any other. Under simple majority (default) this is the standard uncovered set. Tests: known examples from McKelvey (1986) and Miller (1980); verify uncovered set contains the Condorcet winner when one exists; verify supermajority k expands the uncovered set. | ✅ Done |
| **D3** | **Uncovered set boundary (continuous)** | `uncovered_set_boundary_2d(voter_ideals, dist_cfg, grid_resolution, k = n/2+1) → Polygon2E` — approximates the uncovered set region in continuous policy space by evaluating k-coverage on a grid of candidate points, then computing the convex hull of uncovered points. Document that this is an approximation; exact continuous uncovered set computation is left for a future milestone. Tests: verify region contains Yolk center; region expands with larger k; region shrinks as voter dispersion decreases. | ✅ Done |

### Phase F — Extended Winset Services

| Step | Task | Deliverable | Status |
|------|------|-------------|--------|
| **F1** | **Winset set operations** | `winset_ops.h`: `winset_union`, `winset_intersection`, `winset_difference`, `winset_symmetric_difference` on `WinsetRegion`. Tests: identity ops, intersection with empty, self-operations. | ✅ |
| **F2** | **Core detection** | `core.h`: `has_condorcet_winner`, `condorcet_winner` (finite), `core_2d` (continuous, via Yolk centre + winset check). Tests: spatial Condorcet cycle; one dominator; collinear voters; error on <2 voters. | ✅ |
| **F3** | **Copeland winner / strong point** | `copeland.h`: `copeland_scores`, `copeland_winner`. Tie-breaking by first-in-vector. `weighted_majority_prefers` in `majority.h`. Tests: cycle → all zero; Condorcet winner → max score; scores sum to zero; supermajority; tie-breaking. | ✅ |
| **F4** | **Veto players** | `winset_with_veto_2d` added to `winset.h`. Standard winset intersected with each veto player's `pts_polygon`. Tests: no veto; veto at SQ → empty; far veto → unchanged; two veto players shrink winset. | ✅ |
| **F5** | **Weighted voting** | `weighted_majority_prefers` in `majority.h`; `weighted_winset_2d` in `winset.h` using voter-expansion with GCD reduction. Tests: uniform weights match standard; triple-weight voter changes outcome; 50%-threshold tie; unanimity; invalid-input throws. | ✅ |

### Phase G — Heart

| Step | Task | Deliverable | Status |
|------|------|-------------|--------|
| **G1** | **Heart over a finite alternative set** | `heart.h`: `heart(alternatives, voters, cfg, k)` — pre-computes m×m beats matrix (O(m²n)), starts from the Uncovered Set, then iterates T(H) = {x∈H : ∀y that beats x, ∃z∈H that beats y} until stable (at most m iters). Complexity O(m²n + m⁴). Tests in `test_heart.cpp`: edge cases; Condorcet winner → singleton; 3-way cycle → all three; Heart⊆Uncovered Set (theorem check); non-empty guarantee; supermajority monotonicity; 4-alt containment check. | ✅ |
| **G2** | **Heart boundary (continuous approximation)** | `heart_boundary_2d(voters, cfg, grid_resolution=15, k)` in `heart.h` — bounding box + 30% margin, grid×grid candidates, applies `heart()`, returns `convex_hull_2d` of survivors. Tests: error on <2 voters; equilateral triangle → non-trivial polygon; collinear voters → centroid near median; Heart bbox ⊆ uncovered set bbox; all-same voters → near ideal. | ✅ |

### Phase E — Tests, documentation, and CI

| Step | Task | Deliverable | Status |
|------|------|-------------|--------|
| **E1** | **Integration tests** | `tests/unit/test_geometry_integration.cpp`: 32 tests chaining convex hull → majority (pairwise matrix) → winset (empty/non-empty, ops, veto, weighted) → Yolk → uncovered set (finite + boundary) → Heart (finite + boundary) → Copeland → core_2d → weighted majority. Two voter configurations: 5-voter 2D cluster (Condorcet winner) and verified 3-alt cycle. Tests verify correctness of each layer and all cross-layer subset theorems (Heart ⊆ Uncovered Set, etc.). | ✅ |
| **E2** | **CI: CGAL on Ubuntu and macOS** | Verify CI green on both platforms after CGAL install steps are in `.github/workflows/ci.yml`. | ✅ Done |
| **E3** | **Documentation and citation verification** | `docs/architecture/geometry_design.md` completed with full API surface (sections 4–15) for all implemented concepts (type layer, convex hull, majority, winset + ops, Yolk, uncovered set, core, Copeland, Heart, veto players, weighted voting), known limitations, and citation table (section 16). All 14 concept citations verified ✅. Two recommendations flagged: add Tovey (1990)/Koehler (1992) for Yolk algorithm; document voter-expansion equivalence in code. `design_document.md` Layer 3 updated to "implemented". `where_we_are.md` updated. | ✅ |

---

## Definition of done

- Steps A1–G2 and E1–E3 all ✅ Done.
- CGAL EPEC integrated; builds on Ubuntu and macOS in CI.
- Exact 2D types, convex hull, majority relation (k-majority, weighted), winset, winset set operations, core detection, Copeland winner, veto players, Yolk, uncovered set, and Heart all implemented, tested, and documented.
- CI green (format, build, tests, lint) on both platforms.
- `geometry_design.md` complete; `design_document.md` Layer 3 marked implemented.

---

## CGAL dependency notes

CGAL requires **GMP** and **MPFR** (arbitrary-precision arithmetic) as transitive
dependencies. These are available via:

| Platform | Install |
|---|---|
| Ubuntu (CI) | `sudo apt-get install -y libcgal-dev libgmp-dev libmpfr-dev` |
| macOS (CI + local) | `brew install cgal` (pulls GMP and MPFR automatically) |

CGAL is header-only for most algorithms; only a few modules require linking against
`libCGAL`. We will use `find_package(CGAL REQUIRED)` and
`target_link_libraries(... CGAL::CGAL)`.

CGAL is **not** fetched via FetchContent — it is too large and has system-level
dependencies that CMake FetchContent cannot satisfy. A system install is required.

---

## Algorithm references

| Concept | Primary reference |
|---|---|
| Yolk | Feld, Grofman & Miller (1988), "Centripetal Forces in Spatial Voting" |
| Uncovered set / Heart | McKelvey (1986), "Covering, Dominance, and Institution-Free Properties of Social Choice" |
| Median hyperplanes | Plott (1967), "A Notion of Equilibrium and Its Possibility Under Majority Rule" |
| Half-plane intersection | CGAL manual § 2D Intersection of Half-planes |
| Convex hull | CGAL manual § 2D Convex Hulls and Extreme Points |
| Core / chaos theorem | McKelvey (1976), "Intransitivities in Multidimensional Voting Models" |
| Copeland winner / strong point | Copeland (1951); Feld et al. (1987) Definition 9; Godfrey, Grofman & Feld (2011) for SOV-based computation |
| Veto players | Tsebelis (2002), "Veto Players: How Political Institutions Work" |
| Weighted voting | Felsenthal & Machover (1998), "The Measurement of Voting Power" |
| Polygon boolean operations | CGAL manual § 2D Regularized Boolean Set-Operations |
| Heart | Schofield (1999), "The Heart of a Polity"; McKelvey (1986) for relation to uncovered set |
