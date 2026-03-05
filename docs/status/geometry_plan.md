# Geometry Plan

Short-to-medium-term plan to deliver **Layer 3 (Geometry Services)** of the
SocialChoiceLab C++ core. Covers both the geometry foundation (CGAL integration,
exact 2D primitives, convex hull) and the first geometry services (majority
preference, winsets, Yolk, uncovered set).

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

---

## Background: key concepts

**Majority preference (k-majority):** Given voter ideal points `p₁…pₙ` and two
alternatives `x`, `y`, alternative `x` is *k-majority preferred* to `y` if at
least `k` voters are closer to `x` than to `y` under their distance metric.
Simple majority is the special case `k = ⌊n/2⌋ + 1`. All functions in this
plan accept `k` as an optional parameter (defaulting to simple majority) so
supermajority rules (e.g. 2/3·n) and unanimity (k = n) work without extra code.

**Winset W(q):** The set of all alternatives that beat status quo `q` by strict
majority. In 2D Euclidean space this is the union of half-planes (one per voter
majority coalition) — a convex region when voters are distributed nicely, but
generally a (possibly non-convex) polygon. Exact computation requires CGAL.

**Yolk:** The smallest circle that intersects every median hyperplane (a line in
2D perpendicular to an axis passing through the median voter position on that
axis). The Yolk's center approximates the "center of politics" and its radius
measures electoral instability (Feld, Grofman, Miller).

**Uncovered set:** Alternative `x` *covers* `y` if (1) majority prefers `x` to
`y`, and (2) everything a majority prefers to `x` is also preferred by a majority
to `y`. The **uncovered set** is the set of alternatives that no other alternative
covers. It is a subset of the Pareto set and a superset of the Condorcet winner
(if one exists). Also called the **Heart** in some literature (McKelvey 1986).

---

## Step-by-step plan

### Phase A — Geometry Foundation

| Step | Task | Deliverable | Status |
|------|------|-------------|--------|
| **A1** | **CGAL integration** | Add CGAL to CMakeLists.txt (`find_package(CGAL REQUIRED)` with system install; add install steps to CI workflow for Ubuntu and macOS). Define kernel aliases in `include/socialchoicelab/core/kernels.h`: `EpecKernel` (Exact_predicates_exact_constructions_kernel) and `EpicKernel` (Exact_predicates_inexact_constructions_kernel). Verify build on both platforms. | ⬜ |
| **A2** | **Exact 2D type layer** | `include/socialchoicelab/geometry/geom2d.h`: type aliases `Point2E`, `Segment2E`, `Polygon2E` (using CGAL EPEC types). Conversion functions `to_exact(Point2d) → Point2E` and `to_numeric(Point2E) → Point2d`. Basic predicates: `orientation(a, b, c)` (LEFT_TURN / RIGHT_TURN / COLLINEAR), `bounded_side(polygon, point)` (ON_BOUNDED_SIDE / ON_BOUNDARY / ON_UNBOUNDED_SIDE). Tests in `tests/unit/test_geom2d.cpp`. | ⬜ |
| **A3** | **Convex hull 2D** | `include/socialchoicelab/geometry/convex_hull.h`: `convex_hull_2d(std::vector<Point2d>) → Polygon2E` (CGAL `convex_hull_2`). Input validation: at least 1 point, all finite. Handle degenerate cases: 1 point returns single-vertex polygon; 2 collinear points returns 2-vertex polygon; general position returns CCW hull. Tests cover: triangle, square, random point cloud, collinear input, single point. | ⬜ |
| **A4** | **Design doc update** | Add `docs/architecture/geometry_design.md`: kernel policy rationale (EPEC vs EPIC), type layer design, conversion strategy, CGAL dependency rationale. Update `design_document.md` Layer 3 to point to this doc and mark it in progress. | ⬜ |

### Phase B — Majority Preference and Winsets

| Step | Task | Deliverable | Status |
|------|------|-------------|--------|
| **B1** | **Majority preference relation (with k-majority)** | `include/socialchoicelab/geometry/majority.h`: `majority_prefers(x, y, voter_ideals, dist_cfg, k = n/2+1) → bool` — returns true if at least `k` voters are strictly closer to `x` than `y`. Default `k` is simple majority; caller can pass any value for supermajority (e.g. 2/3·n) or unanimity (n). Also `preference_margin(x, y, voter_ideals, dist_cfg) → int` (net count: #prefer_x − #prefer_y, sign indicates winner). Tests: simple majority (even/odd n, ties, single voter); k=n (unanimity); k=2/3·n (supermajority); all-prefer-same. | ⬜ |
| **B2** | **Pairwise comparison matrix** | `pairwise_matrix(alternatives, voter_ideals, dist_cfg, k = n/2+1) → Eigen::MatrixXi` where entry (i,j) = preference_margin(alt_i, alt_j). `k` threshold propagated so the matrix reflects k-majority dominance. Used downstream for Condorcet winner detection and Borda count. Tests: known 3-alternative Condorcet cycle; supermajority that eliminates some dominance edges. | ⬜ |
| **B3** | **Winset computation (2D, with k-majority)** | `include/socialchoicelab/geometry/winset.h`: `winset_2d(status_quo, voter_ideals, dist_cfg, k = n/2+1) → Polygon2E` — exact polygon of all points beating the status quo by at least `k` voters under Euclidean distance. Algorithm: each voter's indifference circle through the SQ defines a half-plane (the closer side); the winset is the intersection of the k-majority coalition half-planes. Use CGAL half-plane intersection (`CGAL::halfplane_intersection_2`). Tests: 3-voter triangle simple majority (known closed form); 5-voter symmetric case; supermajority (smaller winset); k=n (Pareto set); degenerate (Condorcet winner → empty winset). | ⬜ |
| **B4** | **Winset: Minkowski distance extension** | Extend `winset_2d` to support Manhattan and general Minkowski p via the `DistanceConfig`. For non-Euclidean metrics the indifference boundary is not a circle — use the exact `Polygon2d` representation from `level_set_2d` (already implemented) as the boundary for each voter, then compute the half-polygon intersection. `k` parameter passes through unchanged. Tests: Manhattan (diamond boundaries), Chebyshev (square boundaries), each with simple and supermajority k. | ⬜ |

### Phase C — Yolk

| Step | Task | Deliverable | Status |
|------|------|-------------|--------|
| **C1** | **Median lines** | `include/socialchoicelab/geometry/yolk.h` (detail): `median_lines_2d(voter_ideals) → std::vector<Line2E>` — for each pair of adjacent median hyperplanes in the x- and y-directions, produce the bounding lines. In 2D with `n` voters, the median lines are perpendicular bisectors between specific voter pairs. Document the Feld-Grofman-Miller algorithm used. | ⬜ |
| **C2** | **Smallest enclosing circle of lines** | Implement `yolk_2d(voter_ideals) → Circle2d`: the smallest circle that intersects (touches or crosses) every median line. Algorithm: iterate over candidate circles defined by triples of median lines (analogous to smallest enclosing circle of points); use CGAL exact arithmetic throughout. Return the circle center and radius as `core::types::Point2d` + `double` (converting back from CGAL exact). Tests: known 3-voter equilateral triangle (Yolk = centroid circle), 5-voter symmetric case, odd/even n. | ⬜ |
| **C3** | **Yolk radius as instability measure** | Add `yolk_radius(voter_ideals) → double` convenience function. Document: large radius → high agenda-setter power / electoral instability; zero radius → Condorcet winner exists. Tests: Condorcet winner case (radius ≈ 0), spread-out voter case. | ⬜ |

### Phase D — Uncovered Set

| Step | Task | Deliverable | Status |
|------|------|-------------|--------|
| **D1** | **Covering relation (with k-majority)** | `include/socialchoicelab/geometry/uncovered_set.h`: `covers(x, y, alternatives, voter_ideals, dist_cfg, k = n/2+1) → bool` — x k-covers y iff (1) k_majority_prefers(x, y) and (2) for all z in alternatives with k_majority_prefers(z, x), k_majority_prefers(z, y). Tests: textbook Condorcet cycle (no covers under simple majority); supermajority k that changes which alternatives cover; Condorcet winner covers all others. | ⬜ |
| **D2** | **Uncovered set over a finite set** | `uncovered_set(alternatives, voter_ideals, dist_cfg, k = n/2+1) → std::vector<Point2d>` — the subset of alternatives not k-covered by any other. Under simple majority (default) this is the standard uncovered set. Tests: known examples from McKelvey (1986) and Miller (1980); verify uncovered set contains the Condorcet winner when one exists; verify supermajority k expands the uncovered set. | ⬜ |
| **D3** | **Uncovered set boundary (continuous)** | `uncovered_set_boundary_2d(voter_ideals, dist_cfg, grid_resolution, k = n/2+1) → Polygon2E` — approximates the uncovered set region in continuous policy space by evaluating k-coverage on a grid of candidate points, then computing the convex hull of uncovered points. Document that this is an approximation; exact continuous uncovered set computation is left for a future milestone. Tests: verify region contains Yolk center; region expands with larger k; region shrinks as voter dispersion decreases. | ⬜ |

### Phase E — Tests, documentation, and CI

| Step | Task | Deliverable | Status |
|------|------|-------------|--------|
| **E1** | **Integration tests** | `tests/unit/test_geometry_integration.cpp`: end-to-end tests that chain convex hull → majority relation → winset → Yolk → uncovered set on a single voter configuration. Verifies the layers compose correctly. | ⬜ |
| **E2** | **CI: CGAL on Ubuntu and macOS** | Add `sudo apt-get install -y libcgal-dev` to Ubuntu step and `brew install cgal` to macOS step in `.github/workflows/ci.yml`. Verify CI green on both platforms. | ⬜ |
| **E3** | **Final documentation** | Complete `docs/architecture/geometry_design.md` with full API surface, algorithm references (Feld-Grofman-Miller, McKelvey, Miller), and known limitations. Update `design_document.md` Layer 3 to "implemented". Update `where_we_are.md`. | ⬜ |

---

## Definition of done

- Steps A1–E3 all ✅ Done.
- CGAL EPEC integrated; builds on Ubuntu and macOS in CI.
- Exact 2D types, convex hull, majority relation, winset, Yolk, and uncovered set all implemented, tested, and documented.
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
