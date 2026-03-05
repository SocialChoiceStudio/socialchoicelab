# Indifference (Level-Set) Service — API Specification

**Status:** Spec + implemented (Steps 2–3 of core completion plan)  
**Layer:** 2. Preference Services  
**Implementation:** `include/socialchoicelab/preference/indifference/level_set.h`; tests: `tests/unit/test_level_set.cpp`

---

## Purpose

The **indifference** service answers: *given a voter’s ideal point, utility function (loss + distance), and a utility level, what is the set of points at which the voter is indifferent?* That set is the **level set** { x : u(x) = level }.

- In **1D**: the level set is 0, 1, or 2 points (symmetric around the ideal).
- In **2D**: the level set is a closed curve (an indifference curve); we return a finite sample of points on that curve.
- In **N-D** (N ≥ 3): the level set is a surface; sampling is deferred or offered in a later iteration.

Use cases: plotting indifference curves, finding boundaries of “acceptable” alternatives, and feeding into future geometry (e.g. intersection of several voters’ indifference sets).

---

## Contract

- **Stateless**: no stored state; all inputs passed per call.
- **Eigen-only** for this layer: inputs and outputs use `core::types` (Eigen vectors). No CGAL in the indifference API.
- **Reuse**: uses existing `preference::distance` and `preference::loss` APIs only; no re-implementation of distance or loss.

---

## Inputs

Every level-set call needs:

1. **Ideal point** — `Eigen::VectorXd` (or `PointNd` / `Point2d` from `core::types`). Dimension N = 1 or 2 for the specified entry points.
2. **Utility level** — `double`. The target value u(x) = level.
3. **Loss configuration** — same semantics as `distance_to_utility`:
   - `LossFunctionType`: LINEAR, QUADRATIC, GAUSSIAN, THRESHOLD
   - `sensitivity` (α), `max_loss`, `steepness`, `threshold` — same defaults and usage per type as in `loss_functions.h`.
4. **Distance configuration** — same semantics as `calculate_distance`:
   - `DistanceType`: EUCLIDEAN, MANHATTAN, CHEBYSHEV, MINKOWSKI
   - `order_p` (for MINKOWSKI; finite, ≥ 1)
   - `salience_weights`: `std::vector<double>` of length N; finite, ≥ 0; zero = dimension masking.

To keep the API manageable, use **config structs** (defined in the indifference header or a small preference config header):

- **`LossConfig`** — fields: `loss_type`, `sensitivity`, `max_loss`, `steepness`, `threshold`. Defaults match `distance_to_utility`.
- **`DistanceConfig`** — fields: `distance_type`, `order_p`, `salience_weights`. `salience_weights` size must equal ideal point dimension.

---

## Outputs

- **1D**  
  - Return type: `std::vector<socialchoicelab::core::types::PointNd>` (or `std::vector<Eigen::VectorXd>`).  
  - Size: 0 (level unreachable), 1 (level exactly at ideal or single touch), or 2 (symmetric pair).  
  - Order: unspecified (e.g. ascending coordinate).

- **2D**  
  The level set is returned as a **discriminated type** so that exact shapes (circle, ellipse, superellipse) are used when possible, with a polygon fallback. This supports fast intersection, compact memory, and accuracy.

  - **`LevelSet2d`** — one of:
    - **`Circle2d`** — center (Point2d), radius (double). Used for **Euclidean** distance with **equal** salience weights (any loss). Many real use cases have circular ICs.
    - **`Ellipse2d`** — center (Point2d), semi_axis_0, semi_axis_1 (double), axis-aligned with coordinates. Used for **Euclidean** with **unequal** salience weights.
    - **`Superellipse2d`** — center (Point2d), semi_axis_0, semi_axis_1 (double), exponent p (double). Axis-aligned. Used for **Minkowski** with 1 < p < ∞ (Lamé curve).
    - **`Polygon2d`** — ordered closed list of vertices (e.g. `std::vector<Point2d>`). Used for **Manhattan** (p=1) and **Chebyshev** (p=∞), where the IC is exactly a quadrilateral (4 vertices). Also available as a sampled approximation for any shape via `to_polygon(num_samples)` for plotting or polygon-based intersection.

  So: 2D returns **`LevelSet2d`** (a variant of the four shapes). Callers can test which shape they have, use circle/ellipse/superellipse for exact geometry or compact storage, or call **`to_polygon(num_samples)`** to get a polygon for drawing or for polygon–polygon intersection when needed.

---

## Entry points

- **`level_set_1d(ideal_point, utility_level, loss_config, distance_config)`**  
  - Precondition: `ideal_point.size() == 1`.  
  - Returns: `std::vector<PointNd>` of size 0, 1, or 2.

- **`level_set_2d(ideal_point, utility_level, loss_config, distance_config)`**  
  - Precondition: `ideal_point.size() == 2`.  
  - Returns: **`LevelSet2d`** (Circle2d, Ellipse2d, Superellipse2d, or Polygon2d as appropriate for the distance type and weights). No `num_samples`; exact shapes are used. For polygon (Manhattan/Chebyshev) the polygon has exactly 4 vertices.

- **`to_polygon(level_set_2d, num_samples)`**  
  - Given a `LevelSet2d` and optional `num_samples`, returns a **`Polygon2d`** (ordered closed vertices). For Circle/Ellipse/Superellipse this samples the curve; for Polygon2d returns a copy or the same. Enables plotting and polygon-based intersection without duplicating logic.

Invalid inputs (dimension mismatch, wrong weights size, invalid loss/distance params) throw `std::invalid_argument`, consistent with distance and loss.

---

## Algorithm notes (for implementation, Step 3)

- **Invert loss:** From target utility `level`, compute the unique distance `r` such that `distance_to_utility(r, loss_config) == level`. For LINEAR, QUADRATIC, GAUSSIAN, THRESHOLD this is closed-form (see implementation). If `level` is above maximum utility or below minimum (e.g. level > 0 for these loss types), no finite r exists; return empty (1D) or throw / return degenerate shape (2D as needed).

- **1D**: Get `r` from loss inverse. Level set in 1D is the two points at weighted distance r from ideal: with single weight w, points are `ideal[0] ± r/w`. If r is zero, return one point; if no r (level unreachable), return empty.

- **2D**: Get `r` from loss inverse. Then build the appropriate shape from distance type and weights:
  - **Euclidean, equal weights:** Circle2d(ideal, r).
  - **Euclidean, unequal weights:** Ellipse2d(ideal, r/w0, r/w1) (semi-axes along coordinates).
  - **Manhattan:** Polygon2d with 4 vertices: ideal ± (r/w0, 0), ideal ± (0, r/w1).
  - **Chebyshev:** Polygon2d with 4 vertices: rectangle at ideal ± (r/w0, r/w1).
  - **Minkowski (1 < p < ∞):** Superellipse2d(ideal, r/w0, r/w1, p).

---

## N-D and CGAL

- **N-D (N ≥ 3)**: Not in initial scope. Can be added later as `level_set_nd` with a specified sampling strategy (e.g. spherical angles + 1D root-find per ray).
- **CGAL**: Level-set representation (exact curves/surfaces) is out of scope for Layer 2. Output remains sampled points (Eigen). Future Layer 3 (geom2d/geom3d) may consume these points or produce exact representations.

---

## File and namespace

- Header: `include/socialchoicelab/preference/indifference/level_set.h` (or `indifference_functions.h`).
- Namespace: `socialchoicelab::preference::indifference`.
- Config structs can live in the same header or in `preference/config.h` if we want reuse elsewhere.

---

## Summary table

| Item        | Choice |
|------------|--------|
| Inputs     | Ideal point, utility level, `LossConfig`, `DistanceConfig`. |
| Output 1D  | `std::vector<PointNd>` size 0, 1, or 2. |
| Output 2D  | `LevelSet2d` = Circle2d \| Ellipse2d \| Superellipse2d \| Polygon2d (exact shapes). Optional `to_polygon(level_set, num_samples)` for sampled polygon. |
| Backend    | Eigen only; uses existing distance and loss APIs. |
| Preconditions | ideal.size() == 1 for 1D, == 2 for 2D; weights.size() == ideal.size(). |
| Errors     | `std::invalid_argument` for invalid config or dimension mismatch; level unreachable yields empty (1D) or documented degenerate behavior (2D). |
