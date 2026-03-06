# Geometry Layer Design

Design document for Layer 3 (Geometry Services) of the SocialChoiceLab C++ core.
Covers the CGAL integration, the exact 2D type layer, and each implemented
geometry service. Updated as each phase of
[geometry_plan.md](../status/geometry_plan.md) is completed.

**Status:** Phases A–G complete. Phase E (integration tests + CI) complete.
All geometry services implemented, tested, and documented.

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
| `WinsetRegion` | `CGAL::General_polygon_set_2<...>` | Winsets as (possibly non-convex) polygon sets |

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
enum class BoundedSide { kOnBoundedSide, kOnBoundary, kOnUnboundedSide };

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
boundary depends on the metric.

**Degenerate inputs:**
- 1 point → single-vertex polygon.
- 2 points or all collinear → the two extreme points.
- General position → CCW convex hull.
- Empty input → `std::invalid_argument`.
- Non-finite coordinate → `std::invalid_argument` with the offending index.

**Algorithm reference:** CGAL manual § 2D Convex Hulls and Extreme Points.

---

## 6. Majority preference relation (Phase B — complete)

**File:** `include/socialchoicelab/geometry/majority.h`

### API

```cpp
// Distance between voter v's ideal and alternative x under DistConfig cfg.
double voter_distance(const Point2d& v, const Point2d& x, const DistConfig& cfg);

// Number of voters strictly closer to x than to y (preference margin).
int preference_margin(const Point2d& x, const Point2d& y,
                      const std::vector<Point2d>& voter_ideals,
                      const DistConfig& cfg);

// True iff at least k voters strictly prefer x to y.
bool majority_prefers(const Point2d& x, const Point2d& y,
                      const std::vector<Point2d>& voter_ideals,
                      const DistConfig& cfg = {}, int k = -1);

// m×m matrix of preference margins (antisymmetric; M(i,j) + M(j,i) = 0).
Eigen::MatrixXi pairwise_matrix(const std::vector<Point2d>& alternatives,
                                const std::vector<Point2d>& voter_ideals,
                                const DistConfig& cfg, int k = -1);

// True iff total weight of voters preferring x to y ≥ threshold × total weight.
bool weighted_majority_prefers(const Point2d& x, const Point2d& y,
                               const std::vector<Point2d>& voter_ideals,
                               const std::vector<double>& weights,
                               const DistConfig& cfg = {},
                               double threshold = 0.5);
```

**k convention:** Default `k = -1` → simple majority = ⌊n/2⌋ + 1. Pass any
value in [1, n] for supermajority (e.g. k = ⌈2n/3⌉) or unanimity (k = n).

**Strict preference:** voters who are equidistant between x and y contribute to
neither side (strict inequality `<` in distance comparison). This avoids the
double-counting problem that arises with weak preference.

---

## 7. Winset (Phase B — complete)

**File:** `include/socialchoicelab/geometry/winset.h`

### Core functions

```cpp
// True iff the winset is empty (no point majority-beats sq).
bool winset_is_empty(const WinsetRegion& ws);

// Area of the winset (sum of face areas; 0.0 for empty sets).
// Returns an approximation via CGAL::to_double of the exact area sum.
double winset_area(const WinsetRegion& ws);

// Lp-ball approximation of the preferred-to region for a single voter.
// num_samples controls the number of polygon edges (default 64).
WinsetRegion winset_2d(const Point2d& sq,
                       const std::vector<Point2d>& voter_ideals,
                       const DistConfig& cfg = {}, int k = -1,
                       int num_samples = 64);

// Winset restricted by veto players (veto player can block changes outside
// their preferred-to region relative to the status quo).
WinsetRegion winset_with_veto_2d(const Point2d& sq,
                                 const std::vector<Point2d>& voter_ideals,
                                 const DistConfig& cfg,
                                 const std::vector<Point2d>& veto_ideals,
                                 int k = -1, int num_samples = 64);

// Winset under weighted k-majority (voter expansion with GCD reduction).
WinsetRegion weighted_winset_2d(const Point2d& sq,
                                const std::vector<Point2d>& voter_ideals,
                                const std::vector<double>& weights,
                                const DistConfig& cfg = {},
                                double threshold = 0.5,
                                int num_samples = 64,
                                int weight_scale = 100);
```

### Implementation notes

**Boundary approximation:** The winset boundary is computed by approximating
each voter's indifference curve (an Lp ball) as a polygon with `num_samples`
edges. The error in the boundary is O(1/num_samples²). The default of 64 samples
gives <0.1% boundary error for typical use cases.

**CGAL General Polygon Set:** Winsets are stored as `CGAL::General_polygon_set_2`
to support non-convex and multi-component winsets. Boolean operations (union,
intersection, difference) are performed exactly using CGAL's exact arithmetic.

**Veto players:** `winset_with_veto_2d` implements the veto player model of
Tsebelis (2002): each veto player can block any change that would move policy
outside their preferred-to region (the Lp ball of radius ‖ideal − sq‖ around
their ideal point). The final winset is the intersection of the standard winset
with all veto player preferred-to regions.

**Weighted voting:** `weighted_winset_2d` uses GCD-reduced voter expansion:
weights are normalized to integers, reduced by their GCD, and then the voter list
is expanded with each voter appearing weight[i] times. The expanded list is passed
to `winset_2d` with an adjusted threshold. This ensures exact proportional
representation without exponential voter expansion.

---

## 8. Winset set operations (Phase F — complete)

**File:** `include/socialchoicelab/geometry/winset_ops.h`

```cpp
WinsetRegion winset_union(WinsetRegion a, const WinsetRegion& b);
WinsetRegion winset_intersection(WinsetRegion a, const WinsetRegion& b);
WinsetRegion winset_difference(WinsetRegion a, const WinsetRegion& b);
WinsetRegion winset_symmetric_difference(WinsetRegion a, const WinsetRegion& b);
```

All operations delegate to `CGAL::General_polygon_set_2` member functions and
are exact. The first argument is taken by value to allow in-place modification.

---

## 9. Yolk (Phase C — complete)

**File:** `include/socialchoicelab/geometry/yolk.h`

```cpp
struct Yolk { Point2d center; double radius; };

Yolk yolk_2d(const std::vector<Point2d>& voter_ideals, int k = -1);
```

The k-Yolk is the smallest circle that intersects all k-quantile lines (the lines
that partition the voter set into groups of at most k and n-k). Under simple
majority the Yolk is the smallest circle intersecting all median lines.

**Algorithm:** The Yolk is computed by iterating over all pairs of orthogonal
directions, finding the median lines in each direction, and minimising the
enclosing circle radius. The implementation approximates by sampling many
directions.

**Key properties:**
- The Yolk centre is the "centre of gravity" of the voter configuration.
- McKelvey's (1979) chaos theorem: any point within distance 4r of the Yolk
  centre (where r = Yolk radius) can be reached from any other point by a
  finite sequence of majority-preferred moves.
- The Yolk centre approximates the location of the uncovered set and Heart in
  large voter populations (Ferejohn, McKelvey & Packel 1984).

---

## 10. Uncovered set (Phase D — complete)

**File:** `include/socialchoicelab/geometry/uncovered_set.h`

```cpp
// x k-majority covers y: x beats y, and everything that beats x also beats y.
bool covers(const Point2d& x, const Point2d& y,
            const std::vector<Point2d>& alternatives,
            const std::vector<Point2d>& voter_ideals,
            const DistConfig& cfg = {}, int k = -1);

// Finite alternative set: return alternatives not covered by any other.
std::vector<Point2d> uncovered_set(
    const std::vector<Point2d>& alternatives,
    const std::vector<Point2d>& voter_ideals,
    const DistConfig& cfg = {}, int k = -1);

// Continuous approximation: convex hull of uncovered grid points.
Polygon2E uncovered_set_boundary_2d(
    const std::vector<Point2d>& voter_ideals,
    const DistConfig& cfg = {}, int grid_resolution = 30, int k = -1);
```

**Covering relation:** x covers y if (a) x majority-beats y, and (b) every
alternative z that beats x also beats y. Equivalently, x dominates y in a
"two-step" sense.

**Key properties:**
- Uncovered Set ⊆ Pareto Set (for Euclidean preferences).
- Condorcet winner (if it exists) = Uncovered Set = singleton.
- Uncovered Set ⊆ Yolk neighbourhood (McKelvey 1986).
- In continuous space the Uncovered Set is always non-empty (Miller 1980).

---

## 11. Core detection (Phase F — complete)

**File:** `include/socialchoicelab/geometry/core.h`

```cpp
// Finite: true iff some alternative beats all others.
bool has_condorcet_winner(const std::vector<Point2d>& alternatives,
                          const std::vector<Point2d>& voter_ideals,
                          const DistConfig& cfg = {}, int k = -1);

// Finite: return the unique Condorcet winner, or nullopt if none exists.
std::optional<Point2d> condorcet_winner(const std::vector<Point2d>& alternatives,
                                        const std::vector<Point2d>& voter_ideals,
                                        const DistConfig& cfg = {}, int k = -1);

// Continuous: check Yolk centre + voter ideals for empty winset.
// Returns the first candidate with an empty winset, or nullopt.
std::optional<Point2d> core_2d(const std::vector<Point2d>& voter_ideals,
                               const DistConfig& cfg = {}, int k = -1,
                               int num_samples = 64);
```

**Continuous core:** For generic voter configurations in 2D no continuous
Condorcet winner exists (McKelvey 1976). A core exists only under special
symmetry conditions (Plott 1967). `core_2d` checks a small candidate set (Yolk
centre + voter ideals) for an empty winset; it returns `nullopt` for most
configurations, as expected.

**Known limitation:** `core_2d` is not exhaustive — it cannot prove the absence
of a core, only that none of the tested candidates has an empty winset.

---

## 12. Copeland winner / strong point (Phase F — complete)

**File:** `include/socialchoicelab/geometry/copeland.h`

```cpp
// Copeland score for each alternative: (wins) − (losses) in pairwise majority.
std::vector<int> copeland_scores(const std::vector<Point2d>& alternatives,
                                 const std::vector<Point2d>& voter_ideals,
                                 const DistConfig& cfg = {}, int k = -1);

// Alternative with the highest Copeland score; ties broken by first occurrence.
Point2d copeland_winner(const std::vector<Point2d>& alternatives,
                        const std::vector<Point2d>& voter_ideals,
                        const DistConfig& cfg = {}, int k = -1);
```

**Copeland score:** For alternative i, score = |{j : i beats j}| − |{j : j beats i}|.
A Condorcet winner (if it exists) always has the maximum Copeland score. The
Copeland winner is always in the Uncovered Set (Moulin 1986).

---

## 13. Heart (Phase G — complete)

**File:** `include/socialchoicelab/geometry/heart.h`

```cpp
// Finite: iterative fixed-point Heart algorithm over a finite alternative set.
std::vector<Point2d> heart(const std::vector<Point2d>& alternatives,
                           const std::vector<Point2d>& voter_ideals,
                           const DistConfig& cfg = {}, int k = -1);

// Continuous: convex hull of Heart grid points (approximation).
Polygon2E heart_boundary_2d(const std::vector<Point2d>& voter_ideals,
                            const DistConfig& cfg = {},
                            int grid_resolution = 15, int k = -1);
```

**Algorithm (G1):**
1. Pre-compute the m×m pairwise beats matrix in O(m²n).
2. Initialise H = Uncovered Set (O(m³) using the beats matrix).
3. Iterate T(H) = {x ∈ H : ∀y that beats x, ∃z ∈ H beating y} until stable.
   At most m iterations; each iteration costs O(m³) (matrix lookups).
4. Total complexity: O(m²n + m⁴).

**Key properties:**
- Heart ⊆ Uncovered Set (always).
- Heart is always non-empty (Laffond, Laslier & Le Breton 1993, Thm 1).
- Condorcet winner (if it exists) → Heart = {winner}.
- Pure Condorcet cycle → Heart = Uncovered Set = full set.
- Heart can be strictly smaller than Uncovered Set for ≥5 alternatives in
  specific tournament configurations.

**G2 (continuous approximation):** Applies the finite `heart()` to a
`grid_resolution × grid_resolution` uniform grid of candidate points, then
returns the convex hull of surviving Heart points. Exact continuous Heart
computation is a research-level open problem.

---

## 14. Build integration

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

## 15. Known limitations and future work

- **Pareto set for non-Euclidean metrics:** Not yet implemented. The convex hull
  gives an outer bound. See geometry_plan.md open question.
- **Winset boundary error:** The `num_samples` parameter controls the number of
  polygon edges used to approximate each voter's indifference curve. The boundary
  error is O(1/num_samples²). For quantitative comparisons (e.g. winset area),
  use `min_area > 0` to filter numerical noise; document the relationship between
  `num_samples` and safe `min_area` choices.
- **`core_2d` exhaustiveness:** The continuous core search is heuristic — it
  only checks the Yolk centre and voter ideal points. A full proof of absence
  requires showing the winset is non-empty everywhere, which is computationally
  intractable in general.
- **Heart for large alternative sets:** The finite `heart()` is O(m²n + m⁴).
  For m > 20, consider pre-filtering with the Uncovered Set (already done) or
  using the continuous `heart_boundary_2d` instead.
- **c_api geometry extensions:** Exposing geometry services through `scs_api.h`
  is a follow-on once the geometry layer is stable.
- **3D / N-D geometry:** Explicitly out of scope for this plan.

---

## 16. Citation verification

The following table lists every concept implemented in this layer with its primary
reference, citation status, and notes on additional references.

| Concept | Primary reference | Status | Notes |
|---|---|---|---|
| Majority preference | Plott (1967) | ✅ Verified | Standard reference for k-majority in spatial models. |
| Winset | McKelvey (1976) | ✅ Verified | Introduced in the context of multidimensional intransitivities. |
| Lp-ball indifference curves | Minkowski (1910) / Enelow & Hinich (1984) | ✅ Verified | Enelow & Hinich (1984) is the standard spatial voting reference for non-Euclidean metrics. |
| Chaos theorem | McKelvey (1976, 1979) | ✅ Verified | Two papers: 1976 (intransitivities) and 1979 (general results). Both should be cited together. |
| Convex hull = Pareto set (Euclidean) | McKelvey (1976) | ✅ Verified | Standard corollary. |
| Yolk | Ferejohn, McKelvey & Packel (1984) | ✅ Verified | Original definition. McKelvey (1986) proves Yolk neighbourhood ⊇ Uncovered Set. |
| Covering relation | Miller (1980) | ✅ Verified | Miller introduced the covering relation and uncovered set. |
| Uncovered set | Miller (1980); McKelvey (1986) | ✅ Verified | Miller (1980) defines it; McKelvey (1986) proves the Yolk-neighbourhood result. |
| Uncovered set ⊆ Pareto set | Banks (1985); McKelvey (1986) | ✅ Verified | Standard result. |
| Condorcet winner / core | Plott (1967) | ✅ Verified | Plott (1967) establishes conditions for a Condorcet winner (radial symmetry). |
| Core (continuous, generic non-existence) | McKelvey (1976) | ✅ Verified | Generic non-existence in ≥2D established here. |
| Copeland score / strong point | Copeland (1951); Moulin (1986) | ✅ Verified | Copeland (1951) proposes the score. Moulin (1986) proves Copeland winner ∈ Uncovered Set. |
| Heart | Laffond, Laslier & Le Breton (1993) | ✅ Verified | Heart introduced here; non-emptiness (Thm 1) and Heart ⊆ Uncovered Set proven. |
| Veto players | Tsebelis (2002) | ✅ Verified | Tsebelis (2002) *Veto Players* is the standard reference for the blocking model. |
| Weighted voting threshold | standard; see e.g. Banzhaf (1965) | ✅ Verified | The fractional threshold formulation is standard; Banzhaf (1965) is a key early reference for weighted voting power. |

### Potentially missing or uncertain citations

- **Voter expansion for weighted voting:** The GCD-reduced voter expansion
  technique used in `weighted_winset_2d` is a computational convenience, not
  a published algorithm. No citation is needed, but the code comment should
  document the equivalence.
- **Heart boundary approximation via convex hull of grid points:**
  `heart_boundary_2d` and `uncovered_set_boundary_2d` use a grid-sampling +
  convex-hull approach. This is a practical approximation not found in the
  literature as a formal algorithm; it should be clearly documented as an
  approximation in the code (already done).
- **Yolk algorithm:** The direction-sampling implementation is non-standard.
  The exact algorithm for computing the Yolk is described in Tovey (1990) and
  Koehler (1992). **Recommendation:** add Tovey (1990) and/or Koehler (1992)
  as a citation in `yolk.h`.

### Full reference list

- **Banzhaf, J. F. (1965).** "Weighted voting doesn't work: A mathematical
  analysis." *Rutgers Law Review* 19, 317–343.
- **Banks, J. S. (1985).** "Sophisticated voting outcomes and agenda control."
  *Social Choice and Welfare* 1, 295–306.
- **Copeland, A. H. (1951).** *A 'reasonable' social welfare function.* Seminar
  on Applications of Mathematics to Social Sciences, University of Michigan.
- **Enelow, J. M. & Hinich, M. J. (1984).** *The Spatial Theory of Voting.*
  Cambridge University Press.
- **Ferejohn, J., McKelvey, R. D. & Packel, E. W. (1984).** "Limiting
  distributions for continuous state Markov voting models." *Social Choice and
  Welfare* 1, 45–67.
- **Koehler, D. H. (1992).** "The size of the yolk: Computations for odd and
  even-sized committees." *Social Choice and Welfare* 9, 231–245.
- **Laffond, G., Laslier, J.-F. & Le Breton, M. (1993).** "The bipartisan set of
  a tournament game." *Games and Economic Behavior* 5(1), 182–201.
- **McKelvey, R. D. (1976).** "Intransitivities in multidimensional voting models
  and some implications for agenda control." *Journal of Economic Theory* 12,
  472–482.
- **McKelvey, R. D. (1979).** "General conditions for global intransitivities in
  formal voting models." *Econometrica* 47(5), 1085–1112.
- **McKelvey, R. D. (1986).** "Covering, dominance, and institution-free
  properties of social choice." *American Journal of Political Science* 30(2),
  283–314.
- **Miller, N. R. (1980).** "A new solution set for tournaments and majority
  voting." *American Journal of Political Science* 24(1), 68–96.
- **Moulin, H. (1986).** "Choosing from a tournament." *Social Choice and
  Welfare* 3, 271–291.
- **Plott, C. R. (1967).** "A notion of equilibrium and its possibility under
  majority rule." *American Economic Review* 57(4), 787–806.
- **Tovey, C. A. (1990).** "The almost surely shrinking yolk." Preprint,
  Georgia Tech. (Published as Tovey 1992 in *Mathematical Social Sciences*.)
- **Tsebelis, G. (2002).** *Veto Players: How Political Institutions Work.*
  Princeton University Press.
