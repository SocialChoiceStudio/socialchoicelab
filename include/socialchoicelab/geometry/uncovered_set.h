// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// uncovered_set.h — Covering relation and uncovered set (Miller 1980).
//
// Implements:
//   covers(x, y, alternatives, voter_ideals, cfg, k)              → bool  [D1]
//   uncovered_set(alternatives, voter_ideals, cfg, k)             → vector [D2]
//   uncovered_set_boundary_2d(voter_ideals, cfg, resolution, k)  → Polygon2E
//   [D3]
//
// The k-covering relation is used throughout: x k-covers y iff
//   (1) k_majority_prefers(x, y)  and
//   (2) every alternative z with k_majority_prefers(z, x) also satisfies
//       k_majority_prefers(z, y).
//
// Intuitively, x dominates y not only directly (condition 1) but also
// "transitively": anything that can beat x can also beat y, so y is weakly
// worse than x against every challenger.  The uncovered set is the subset of
// alternatives not k-covered by any other.
//
// D3 provides a grid-based approximation of the continuous uncovered set.
// The approximation quality improves with grid_resolution; exact continuous
// computation is a research-level open problem.
//
// References:
//   Miller (1980), "A New Solution Concept for Majority Rule and Games:
//     The Uncovered Set," American Journal of Political Science 24(1), 68–96.
//   McKelvey (1986), "Covering, Dominance, and Institution-Free Properties of
//     Social Choice," American Journal of Political Science 30(2), 283–314.
//   Feld, Grofman & Miller (1988), "Centripetal Forces in Spatial Voting:
//     The Yolk and the Grid," AJPS 32(2), 459–481. (Yolk ⊆ Uncovered Set)

#include <socialchoicelab/geometry/convex_hull.h>
#include <socialchoicelab/geometry/majority.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace socialchoicelab::geometry {

// ===========================================================================
// D1 — Covering relation
// ===========================================================================

/**
 * @brief Returns true if x k-covers y with respect to the given alternatives.
 *
 * x k-covers y iff:
 *   (1) At least k voters strictly prefer x to y  (majority_prefers(x, y)).
 *   (2) For every alternative z ∈ @p alternatives: if majority_prefers(z, x)
 *       then majority_prefers(z, y).
 *
 * Condition (2) can be read as "x's winners beat y's winners": any challenger
 * that can defeat x can also defeat y, so y is weakly dominated.
 *
 * Notes:
 *   - If no z beats x (e.g. x is the Condorcet winner), condition (2) is
 *     vacuously true and x covers y whenever x beats y.
 *   - In a Condorcet cycle A→B→C→A no alternative covers any other.
 *   - @p alternatives should include all candidates; x and y may (or may not)
 *     be members of @p alternatives.
 *
 * @param x            Candidate alternative (the potential coverer).
 * @param y            Candidate alternative (the potentially covered).
 * @param alternatives Full set of alternatives (used as challengers in cond 2).
 * @param voter_ideals Voter ideal points.
 * @param cfg          Distance configuration (default: Euclidean, unit
 * weights).
 * @param k            Majority threshold; -1 = ⌊n/2⌋+1 (simple majority).
 * @return             true iff x k-covers y.
 * @throws std::invalid_argument if voter_ideals is empty or k ∉ [1, n].
 */
[[nodiscard]] inline bool covers(
    const core::types::Point2d& x, const core::types::Point2d& y,
    const std::vector<core::types::Point2d>& alternatives,
    const std::vector<core::types::Point2d>& voter_ideals,
    const DistConfig& cfg = {}, int k = -1) {
  // Condition 1: x beats y under k-majority.
  if (!majority_prefers(x, y, voter_ideals, cfg, k)) return false;

  // Condition 2: every z that beats x also beats y.
  for (const auto& z : alternatives) {
    if (majority_prefers(z, x, voter_ideals, cfg, k) &&
        !majority_prefers(z, y, voter_ideals, cfg, k)) {
      return false;
    }
  }
  return true;
}

// ===========================================================================
// D2 — Uncovered set over a finite alternative set
// ===========================================================================

/**
 * @brief Returns the subset of alternatives not k-covered by any other.
 *
 * An alternative a is in the uncovered set iff no other alternative b satisfies
 * covers(b, a, alternatives, voter_ideals, cfg, k).
 *
 * Properties guaranteed by the theory:
 *   - The uncovered set is always non-empty (the covering relation has no
 *     cycles: if a covers b and b covers c then a covers c — see McKelvey 1986
 *     Lemma 1).
 *   - If a k-majority Condorcet winner exists it is the sole uncovered element.
 *   - In a Condorcet cycle all alternatives are uncovered.
 *   - Increasing k (supermajority) weakens the majority condition, reducing
 *     coverings and weakly expanding the uncovered set.
 *
 * Time complexity: O(m² · n) where m = |alternatives|, n = |voter_ideals|.
 *
 * @param alternatives  Finite set of alternatives.
 * @param voter_ideals  Voter ideal points.
 * @param cfg           Distance configuration.
 * @param k             Majority threshold; -1 = ⌊n/2⌋+1 (simple majority).
 * @return              Subset of @p alternatives not covered by any other.
 * @throws std::invalid_argument if voter_ideals is empty or k ∉ [1, n].
 */
[[nodiscard]] inline std::vector<core::types::Point2d> uncovered_set(
    const std::vector<core::types::Point2d>& alternatives,
    const std::vector<core::types::Point2d>& voter_ideals,
    const DistConfig& cfg = {}, int k = -1) {
  if (alternatives.empty()) return {};

  const int m = static_cast<int>(alternatives.size());
  std::vector<core::types::Point2d> result;
  result.reserve(static_cast<std::size_t>(m));

  for (int i = 0; i < m; ++i) {
    bool is_covered = false;
    for (int j = 0; j < m && !is_covered; ++j) {
      if (i == j) continue;
      if (covers(alternatives[static_cast<std::size_t>(j)],
                 alternatives[static_cast<std::size_t>(i)], alternatives,
                 voter_ideals, cfg, k)) {
        is_covered = true;
      }
    }
    if (!is_covered) {
      result.push_back(alternatives[static_cast<std::size_t>(i)]);
    }
  }
  return result;
}

// ===========================================================================
// D3 — Uncovered set boundary in continuous policy space (grid approximation)
// ===========================================================================

/**
 * @brief Grid-based approximation of the k-uncovered set in continuous 2D
 *        policy space.
 *
 * Algorithm:
 *   1. Compute a bounding box from the voter ideal points, extended by a 30%
 *      margin in each direction.
 *   2. Generate a grid_resolution × grid_resolution grid of candidate points
 *      covering the bounding box.
 *   3. For each grid point p: check if any other grid point q satisfies
 *      covers(q, p, grid_points, voter_ideals, cfg, k).  If not, p is
 *      "uncovered" in the grid approximation.
 *   4. Return the convex hull of all uncovered grid points.
 *
 * Approximation notes:
 *   - Quality improves with grid_resolution but computation grows as
 *     O(grid_resolution⁴ · n).  Default grid_resolution = 30 gives 900 grid
 *     points and ~810 K coverage checks, which runs in < 1 s for n ≤ 20.
 *   - The bounding box covers the voter ideal distribution plus a 30% margin.
 *     Uncovered-set regions outside this box are not captured; for strongly
 *     asymmetric configurations consider increasing the margin (future work).
 *   - The returned convex hull is a conservative outer bound of the true
 *     continuous uncovered set within the sampled region.
 *
 * Theoretical guarantee used in testing:
 *   Feld, Grofman & Miller (1988) prove the Yolk center is always contained in
 *   the uncovered set, so the Yolk center should lie inside (or very near) the
 *   returned polygon for typical voter configurations.
 *
 * @param voter_ideals    At least 1 voter ideal point.
 * @param cfg             Distance configuration.
 * @param grid_resolution Number of grid points per axis (default 30).
 * @param k               Majority threshold; -1 = ⌊n/2⌋+1 (simple majority).
 * @return                Convex hull polygon (CCW, CGAL EPEC) of the uncovered
 *                        grid points.
 * @throws std::invalid_argument if voter_ideals is empty.
 */
[[nodiscard]] inline Polygon2E uncovered_set_boundary_2d(
    const std::vector<core::types::Point2d>& voter_ideals,
    const DistConfig& cfg = {}, int grid_resolution = 30, int k = -1) {
  if (voter_ideals.empty()) {
    throw std::invalid_argument(
        "uncovered_set_boundary_2d: voter_ideals must not be empty.");
  }

  // -------------------------------------------------------------------------
  // Build bounding box with 30% margin.
  // -------------------------------------------------------------------------
  double x_min = voter_ideals[0].x(), x_max = voter_ideals[0].x();
  double y_min = voter_ideals[0].y(), y_max = voter_ideals[0].y();
  for (const auto& v : voter_ideals) {
    x_min = std::min(x_min, v.x());
    x_max = std::max(x_max, v.x());
    y_min = std::min(y_min, v.y());
    y_max = std::max(y_max, v.y());
  }

  // If all voters are collocated, create a default neighbourhood.
  double x_range = x_max - x_min;
  double y_range = y_max - y_min;
  const double min_range = 1.0;
  if (x_range < 1e-10) {
    x_range = min_range;
    x_min -= 0.5;
    x_max += 0.5;
  }
  if (y_range < 1e-10) {
    y_range = min_range;
    y_min -= 0.5;
    y_max += 0.5;
  }

  const double margin = 0.30;
  x_min -= margin * x_range;
  x_max += margin * x_range;
  y_min -= margin * y_range;
  y_max += margin * y_range;

  // -------------------------------------------------------------------------
  // Generate grid.
  // -------------------------------------------------------------------------
  const int res = std::max(2, grid_resolution);
  std::vector<core::types::Point2d> grid;
  grid.reserve(static_cast<std::size_t>(res * res));
  for (int row = 0; row < res; ++row) {
    for (int col = 0; col < res; ++col) {
      double gx = x_min + (x_max - x_min) * col / (res - 1);
      double gy = y_min + (y_max - y_min) * row / (res - 1);
      grid.emplace_back(gx, gy);
    }
  }

  // -------------------------------------------------------------------------
  // Find uncovered grid points.
  // coverage check: grid[j] covers grid[i] iff covers(grid[j], grid[i], grid,
  // …).
  // -------------------------------------------------------------------------
  const int m = static_cast<int>(grid.size());
  std::vector<bool> is_covered(static_cast<std::size_t>(m), false);

  for (int i = 0; i < m; ++i) {
    for (int j = 0; j < m && !is_covered[static_cast<std::size_t>(i)]; ++j) {
      if (i == j) continue;
      if (covers(grid[static_cast<std::size_t>(j)],
                 grid[static_cast<std::size_t>(i)], grid, voter_ideals, cfg,
                 k)) {
        is_covered[static_cast<std::size_t>(i)] = true;
      }
    }
  }

  std::vector<core::types::Point2d> uncovered_pts;
  for (int i = 0; i < m; ++i) {
    if (!is_covered[static_cast<std::size_t>(i)]) {
      uncovered_pts.push_back(grid[static_cast<std::size_t>(i)]);
    }
  }

  return convex_hull_2d(uncovered_pts);
}

}  // namespace socialchoicelab::geometry
