# Geometry Layer Design

Design document for Layer 3 (Geometry Services) of the SocialChoiceLab C++ core.
Covers the CGAL integration, the exact 2D type layer, and each implemented
geometry service. Updated as each phase of
[geometry_plan.md](../status/geometry_plan.md) is completed.

**Status:** Phase A complete. Phases B–G in progress.

---

## 1. Motivation and scope

The preference and distance layers (Layers 1–2) compute voter utility and
indifference curves in floating-point arithmetic. Layer 3 adds exact geometric
computation for the spatial social-choice concepts that depend on correct geometric
predicates and constructions — winsets, Yolk, uncovered set, Heart, and related
solution concepts.

Exact arithmetic is required because:

- Winset boundary computation involves intersecting voter indifference arcs; a
  single wrong orientation test can produce topologically incorrect output.
- The Yolk is the smallest circle intersecting all median lines; near-degenerate
  voter configurations make floating-point comparisons unreliable.
- Downstream concepts (uncovered set, Heart, core detection) inherit any error from
  winset computation, so errors compound.

---

## 2. CGAL dependency

CGAL (Computational Geometry Algorithms Library) provides the exact arithmetic
kernel and the geometric algorithms used throughout this layer. CGAL is **not**
fetched via FetchContent — it is too large and has system-level dependencies
(GMP, MPFR) that CMake FetchContent cannot satisfy. A system install is required.

| Platform | Install |
|---|---|
| Ubuntu (CI) | `sudo apt-get install -y libcgal-dev libgmp-dev libmpfr-dev` |
| macOS (CI + local) | `brew install cgal` (pulls GMP and MPFR automatically) |

CGAL is linked via `find_package(CGAL REQUIRED)` and `target_link_libraries(... CGAL::CGAL)`.

**CGAL version:** 6.x (6.1.1 verified locally on macOS arm64).

---

## 3. Kernel policy

Two CGAL kernel aliases are defined in `include/socialchoicelab/core/kernels.h`:

```cpp
using EpecKernel = CGAL::Exact_predicates_exact_constructions_kernel;
using EpicKernel = CGAL::Exact_predicates_inexact_constructions_kernel;
```

**EpecKernel** is the project default for all geometry output types. It provides:
- Exact geometric predicates (orientation, bounded-side, in-circle) via filtered
  interval arithmetic — never a wrong sign decision.
- Exact constructions — intersection points, midpoints, and other derived
  coordinates are represented exactly using GMP/MPFR multi-precision numbers.
- Slower than EpicKernel; compilation of EPEC-using code is noticeably slower due
  to heavy template instantiation.

**EpicKernel** is available for internal algorithmic use where constructed point
coordinates do not need to be exact (e.g. purely predicate-based filtering). It is
not exposed in the public type aliases.

Rationale: the spatial social-choice algorithms in this project produce polygon
output (winsets, hulls, uncovered set boundaries) that is consumed by subsequent
computations. Using EpicKernel for output types would silently introduce rounding
into those polygons, corrupting downstream operations (e.g. intersection of two
approximately-computed winsets). EpecKernel eliminates this class of error at the
cost of compile-time and runtime performance.

---

## 4. Type layer (Phase A — complete)

**File:** `include/socialchoicelab/geometry/geom2d.h`

### Type aliases

| Alias | CGAL type | Purpose |
|---|---|---|
| `Point2E` | `CGAL::Point_2<EpecKernel>` | Voter ideal points, alternative positions, winset vertices |
| `Segment2E` | `CGAL::Segment_2<EpecKernel>` | Median lines, cutlines, perpendicular bisectors |
| `Polygon2E` | `CGAL::Polygon_2<EpecKernel>` | Convex hulls, winsets, solution concept boundaries (CCW by convention) |

### Conversions

The core layer uses `Eigen::Vector2d` (`Point2d`) for all floating-point
computation. The geometry layer uses `Point2E` for exact computation. Two inline
conversion functions bridge the boundary:

```cpp
Point2E to_exact(const core::types::Point2d& p);   // double → exact
core::types::Point2d to_numeric(const Point2E& p); // exact → double (lossy)
```

`to_numeric` uses `CGAL::to_double()` and loses precision. It should be used only
for final output or display — not for further exact computation.

### Predicates

```cpp
Orientation orientation(const Point2E& a, const Point2E& b, const Point2E& c);
BoundedSide bounded_side(const Polygon2E& polygon, const Point2E& pt);
```

Both use exact EPEC arithmetic. `bounded_side` requires a simple (non-self-
intersecting) polygon; behaviour is undefined for self-intersecting inputs. All
polygons produced by algorithms in this project are guaranteed simple.

---

## 5. Convex hull (Phase A — complete)

**File:** `include/socialchoicelab/geometry/convex_hull.h`

```cpp
Polygon2E convex_hull_2d(const std::vector<core::types::Point2d>& pts);
```

Accepts floating-point Eigen voter ideal points, converts them to exact CGAL
points, and calls `CGAL::convex_hull_2`. Output is a CCW exact polygon.

**Pareto set interpretation:** Under Euclidean preferences the convex hull of the
voter ideal points equals the Pareto set — the set of alternatives not unanimously
dominated. This is a standard result (see e.g. McKelvey 1976). For non-Euclidean
metrics the convex hull is an outer bound on the Pareto set; the exact Pareto set
boundary depends on the metric. See the open question in
[geometry_plan.md § Pareto set](../status/geometry_plan.md).

**Degenerate inputs:**
- 1 point → single-vertex polygon.
- 2 points or all collinear → the two extreme points.
- General position → CCW convex hull.
- Empty input → `std::invalid_argument`.
- Non-finite coordinate → `std::invalid_argument` with the offending index.

**Algorithm reference:** CGAL manual § 2D Convex Hulls and Extreme Points.

---

## 6. Build integration

The geometry layer is a CMake `INTERFACE` library (header-only; all CGAL types
are templates):

```cmake
add_library(socialchoicelab_geometry INTERFACE)
target_link_libraries(socialchoicelab_geometry INTERFACE
    socialchoicelab_core CGAL::CGAL)
```

Targets that use geometry link against `socialchoicelab_geometry`. The `CGAL::CGAL`
target marks its include directories as `SYSTEM`, suppressing warnings from CGAL
headers.

**GTest version note:** CGAL adds `/opt/homebrew/include` (or the system CGAL
prefix) to the include path before FetchContent gtest headers on macOS. If the
system gtest and FetchContent gtest differ in ABI (e.g. `std::string` vs
`const char*` for the first argument of `MakeAndRegisterTestInfo`), this causes a
linker error. The fix is to keep the FetchContent gtest tag in sync with the
system-installed version. Currently pinned to `v1.17.0` to match Homebrew.

---

## 7. Known limitations and future work

- **Pareto set for non-Euclidean metrics:** Not yet implemented. The convex hull
  gives an outer bound. See geometry_plan.md open question.
- **Winset, Yolk, uncovered set, Heart:** Planned in Phases B–G. This document
  will be extended as each phase completes.
- **c_api geometry extensions:** Exposing geometry services through `scs_api.h`
  is a follow-on once the geometry layer is stable.
- **3D / N-D geometry:** Explicitly out of scope for this plan.

---

## 8. References

| Concept | Reference |
|---|---|
| CGAL kernels | CGAL manual § 2D Geometry Kernel |
| Convex hull | CGAL manual § 2D Convex Hulls and Extreme Points |
| Pareto set (Euclidean = convex hull) | McKelvey (1976), "Intransitivities in Multidimensional Voting Models" |
