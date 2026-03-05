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

**Majority preference:** Given voter ideal points `p₁…pₙ` and two alternatives
`x`, `y`, alternative `x` is *majority preferred* to `y` if more than half of
voters are closer (under their distance metric) to `x` than to `y`.

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
| **B1** | **Majority preference relation** | `include/socialchoicelab/geometry/majority.h`: `majority_prefers(alternatives x, y, voter_ideals, distance_config) → bool` — returns true if strictly more than half of voters are (strictly) closer to `x` than `y` under the configured distance. Also `preference_margin(x, y, voter_ideals, ...) → int` (net count: #prefer_x − #prefer_y). Tests: even/odd voter counts, ties, single voter, all prefer same. | ⬜ |
| **B2** | **Pairwise comparison matrix** | `pairwise_matrix(alternatives, voter_ideals, dist_cfg) → Eigen::MatrixXi` where entry (i,j) = preference_margin(alt_i, alt_j). Used downstream for Condorcet winner detection and Borda count. Tests: known 3-alternative example with Condorcet cycle. | ⬜ |
| **B3** | **Winset computation (2D)** | `include/socialchoicelab/geometry/winset.h`: `winset_2d(status_quo, voter_ideals, dist_cfg) → Polygon2E` — the exact convex polygon of all points that beat the status quo by strict majority under Euclidean distance. Algorithm: each voter's indifference circle through the SQ defines a half-plane (the closer side); the winset is the intersection of the majority-coalition half-planes. Use CGAL half-plane intersection (`CGAL::halfplane_intersection_2`). Tests: 3-voter triangle (known closed form), 5-voter symmetric case, degenerate (Condorcet winner → empty winset). | ⬜ |
| **B4** | **Winset: Minkowski distance extension** | Extend `winset_2d` to support Manhattan and general Minkowski p via the `DistanceConfig`. For non-Euclidean metrics the indifference boundary is not a circle — use the exact `Polygon2d` representation from `level_set_2d` (already implemented) as the boundary for each voter, then compute the half-polygon intersection. Tests: Manhattan (diamond boundaries), Chebyshev (square boundaries). | ⬜ |

### Phase C — Yolk

| Step | Task | Deliverable | Status |
|------|------|-------------|--------|
| **C1** | **Median lines** | `include/socialchoicelab/geometry/yolk.h` (detail): `median_lines_2d(voter_ideals) → std::vector<Line2E>` — for each pair of adjacent median hyperplanes in the x- and y-directions, produce the bounding lines. In 2D with `n` voters, the median lines are perpendicular bisectors between specific voter pairs. Document the Feld-Grofman-Miller algorithm used. | ⬜ |
| **C2** | **Smallest enclosing circle of lines** | Implement `yolk_2d(voter_ideals) → Circle2d`: the smallest circle that intersects (touches or crosses) every median line. Algorithm: iterate over candidate circles defined by triples of median lines (analogous to smallest enclosing circle of points); use CGAL exact arithmetic throughout. Return the circle center and radius as `core::types::Point2d` + `double` (converting back from CGAL exact). Tests: known 3-voter equilateral triangle (Yolk = centroid circle), 5-voter symmetric case, odd/even n. | ⬜ |
| **C3** | **Yolk radius as instability measure** | Add `yolk_radius(voter_ideals) → double` convenience function. Document: large radius → high agenda-setter power / electoral instability; zero radius → Condorcet winner exists. Tests: Condorcet winner case (radius ≈ 0), spread-out voter case. | ⬜ |

### Phase D — Uncovered Set

| Step | Task | Deliverable | Status |
|------|------|-------------|--------|
| **D1** | **Covering relation** | `include/socialchoicelab/geometry/uncovered_set.h`: `covers(x, y, voter_ideals, dist_cfg) → bool` — x covers y iff (1) majority_prefers(x, y) and (2) for all z with majority_prefers(z, x), majority_prefers(z, y). For a finite set of alternatives. Tests: textbook Condorcet cycle (no covers), one alternative dominated by another, all voters identical (all alternatives tied). | ⬜ |
| **D2** | **Uncovered set over a finite set** | `uncovered_set(alternatives, voter_ideals, dist_cfg) → std::vector<Point2d>` — the subset of alternatives not covered by any other. Tests: known examples from McKelvey (1986) and Miller (1980) with exact voter configurations; verify the uncovered set contains the Condorcet winner when one exists. | ⬜ |
| **D3** | **Uncovered set boundary (continuous)** | `uncovered_set_boundary_2d(voter_ideals, dist_cfg, grid_resolution) → Polygon2E` — approximates the uncovered set region in continuous policy space by evaluating coverage on a grid of candidate points, then computing the convex hull of uncovered points. Document that this is an approximation; exact continuous uncovered set computation is left for a future milestone. Tests: verify region contains Yolk center; region shrinks as voter dispersion decreases. | ⬜ |

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
