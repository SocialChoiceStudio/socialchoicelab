// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// heart.h — Heart and Heart boundary (Phases G1, G2).
//
// The Heart (Laffond, Laslier & Le Breton 1993) is the finest (smallest)
// non-empty subset of the uncovered set that is "internally stable" in the
// following sense: every alternative in the Heart is defended against any
// majority-winning challenger by at least one other element of the Heart.
//
// Formal definition (fixed-point characterisation):
//   H ⊆ alternatives is the Heart iff it is the smallest set satisfying:
//
//   H = { x ∈ H : for every y ∈ alternatives with majority_prefers(y, x),
//                 there exists z ∈ H with majority_prefers(z, y) }
//
// Equivalently, H is the unique fixed-point of the operator:
//
//   T(S) = { x ∈ S : for all y that k-majority beat x,
//                    ∃ z ∈ S that k-majority beats y }
//
// The algorithm starts from the Uncovered Set (which always contains H) and
// iterates T until convergence.  At most |alternatives| iterations suffice.
//
// Key theoretical properties:
//   1. Heart ⊆ Uncovered Set ⊆ Alternatives  (always).
//   2. Heart is always non-empty              (Laffond et al. 1993, Thm 1).
//   3. If a Condorcet winner exists, Heart = {Condorcet winner}.
//   4. If all alternatives form a Condorcet cycle,
//      Heart = Uncovered Set = full set.
//   5. For generic configurations with 5+ alternatives,
//      Heart can be strictly smaller than the Uncovered Set.
//
// G2 (heart_boundary_2d) approximates the continuous Heart via a grid of
// candidate points; this is an approximation — exact continuous Heart
// computation is a research-level open problem.
//
// References:
//   Laffond, Laslier & Le Breton (1993), "The Bipartisan Set of a Tournament
//     Game," Games and Economic Behavior 5(1), 182–201.
//   Banks (1985), "Sophisticated voting outcomes and agenda control,"
//     Social Choice and Welfare 1, 295–306.
//   McKelvey (1986), "Covering, dominance, and institution-free properties of
//     social choice," American Journal of Political Science 30(2), 283–314.

#include <socialchoicelab/geometry/convex_hull.h>
#include <socialchoicelab/geometry/majority.h>
#include <socialchoicelab/geometry/uncovered_set.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace socialchoicelab::geometry {

// ===========================================================================
// G1 — Heart over a finite alternative set
// ===========================================================================

/**
 * @brief Compute the k-majority Heart of a finite alternative set.
 *
 * The Heart is the smallest internally-stable subset of the Uncovered Set
 * (see module header for the formal definition).
 *
 * Algorithm:
 *   1. Pre-compute the m×m k-majority pairwise beats matrix in O(m²n).
 *   2. Start H = Uncovered Set (computed using the beats matrix in O(m³)).
 *   3. Iterate T(H) = {x ∈ H : ∀y beating x, ∃z ∈ H beating y} until stable.
 *      At most m iterations; each iteration costs O(m³) (matrix lookups only).
 *
 * Complexity: O(m²n + m⁴) where m = |alternatives|, n = |voter_ideals|.
 * For the small alternative sets typical in spatial voting (m ≤ 20,
 * n ≤ 500) this is fast.
 *
 * @param alternatives  Finite set of alternatives (may be empty).
 * @param voter_ideals  Voter ideal points (at least 1).
 * @param cfg           Distance configuration.
 * @param k             Majority threshold; -1 = ⌊n/2⌋+1.
 * @return              The Heart — a non-empty subset of alternatives
 *                      (unless alternatives itself is empty).
 * @throws std::invalid_argument if voter_ideals is empty or k ∉ [1, n].
 */
[[nodiscard]] inline std::vector<core::types::Point2d> heart(
    const std::vector<core::types::Point2d>& alternatives,
    const std::vector<core::types::Point2d>& voter_ideals,
    const DistConfig& cfg = {}, int k = -1) {
  if (alternatives.empty()) return {};
  if (voter_ideals.empty()) {
    throw std::invalid_argument("heart: voter_ideals must not be empty.");
  }

  const int m = static_cast<int>(alternatives.size());

  // Step 1: pre-compute the pairwise beats matrix.
  // beats[i][j] = true iff k-majority prefers alternatives[i] to
  // alternatives[j].
  std::vector<std::vector<bool>> beats(
      static_cast<std::size_t>(m),
      std::vector<bool>(static_cast<std::size_t>(m), false));
  for (int i = 0; i < m; ++i) {
    for (int j = 0; j < m; ++j) {
      if (i == j) continue;
      beats[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] =
          majority_prefers(alternatives[static_cast<std::size_t>(i)],
                           alternatives[static_cast<std::size_t>(j)],
                           voter_ideals, cfg, k);
    }
  }

  // Step 2: compute the uncovered set (index-based, using the beats matrix).
  // covers_idx(xi, yi): xi k-majority covers yi.
  auto covers_idx = [&](int xi, int yi) -> bool {
    if (!beats[static_cast<std::size_t>(xi)][static_cast<std::size_t>(yi)]) {
      return false;
    }
    for (int z = 0; z < m; ++z) {
      if (beats[static_cast<std::size_t>(z)][static_cast<std::size_t>(xi)] &&
          !beats[static_cast<std::size_t>(z)][static_cast<std::size_t>(yi)]) {
        return false;
      }
    }
    return true;
  };

  // H[i] = true iff alternative i is in the current candidate set.
  std::vector<bool> H(static_cast<std::size_t>(m), false);
  for (int i = 0; i < m; ++i) {
    bool covered = false;
    for (int j = 0; j < m && !covered; ++j) {
      if (i != j && covers_idx(j, i)) covered = true;
    }
    H[static_cast<std::size_t>(i)] = !covered;
  }

  // Step 3: iterate T(H) until stable.
  bool changed = true;
  while (changed) {
    changed = false;
    std::vector<bool> new_H = H;

    for (int x = 0; x < m; ++x) {
      if (!H[static_cast<std::size_t>(x)]) continue;

      // x stays iff for every y that beats x, ∃ z ∈ H that beats y.
      bool x_in_heart = true;
      for (int y = 0; y < m && x_in_heart; ++y) {
        if (!beats[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)]) {
          continue;
        }
        // y beats x: check if ∃ z ∈ H that beats y.
        bool z_exists = false;
        for (int z = 0; z < m && !z_exists; ++z) {
          if (H[static_cast<std::size_t>(z)] &&
              beats[static_cast<std::size_t>(z)][static_cast<std::size_t>(y)]) {
            z_exists = true;
          }
        }
        if (!z_exists) x_in_heart = false;
      }

      if (!x_in_heart) {
        new_H[static_cast<std::size_t>(x)] = false;
        changed = true;
      }
    }

    H = new_H;
  }

  std::vector<core::types::Point2d> result;
  result.reserve(static_cast<std::size_t>(m));
  for (int i = 0; i < m; ++i) {
    if (H[static_cast<std::size_t>(i)]) {
      result.push_back(alternatives[static_cast<std::size_t>(i)]);
    }
  }
  return result;
}

// ===========================================================================
// G2 — Heart boundary (continuous space approximation)
// ===========================================================================

/**
 * @brief Approximate the k-majority Heart boundary in continuous 2D policy
 *        space via a grid of candidate alternatives.
 *
 * Procedure:
 *   1. Compute the axis-aligned bounding box of voter_ideals, expanded by
 *      30 % in each direction (same margin as uncovered_set_boundary_2d).
 *   2. Generate a @p grid_resolution × @p grid_resolution uniform grid of
 *      candidate points inside the bounding box.
 *   3. Apply heart() to the full grid.
 *   4. Return the convex hull of the surviving Heart points.
 *
 * Accuracy note: the result is an approximation.  The true continuous Heart
 * (if formally defined) is a research-level open problem.  The grid approach
 * gives a useful polygon for visualisation and rough containment checks.
 * Increase @p grid_resolution for finer resolution at the cost of O(m⁴)
 * computational work where m = grid_resolution².
 *
 * Relationship to other regions:
 *   Heart boundary ⊆ Uncovered Set boundary ⊆ Winset envelope.
 *
 * @param voter_ideals    At least 2 voter ideal points.
 * @param cfg             Distance configuration.
 * @param grid_resolution Grid points per axis (default 15; total = 15²=225).
 * @param k               Majority threshold; -1 = ⌊n/2⌋+1.
 * @return                Convex hull of Heart grid points.
 * @throws std::invalid_argument if voter_ideals has < 2 points.
 */
[[nodiscard]] inline Polygon2E heart_boundary_2d(
    const std::vector<core::types::Point2d>& voter_ideals,
    const DistConfig& cfg = {}, int grid_resolution = 15, int k = -1) {
  if (voter_ideals.size() < 2) {
    throw std::invalid_argument(
        "heart_boundary_2d: need at least 2 voter ideals (got " +
        std::to_string(voter_ideals.size()) + ").");
  }
  if (grid_resolution < 2) {
    throw std::invalid_argument(
        "heart_boundary_2d: grid_resolution must be at least 2 (got " +
        std::to_string(grid_resolution) + ").");
  }

  // Compute bounding box + 30 % margin (matches uncovered_set_boundary_2d).
  double xmin = voter_ideals[0].x();
  double xmax = voter_ideals[0].x();
  double ymin = voter_ideals[0].y();
  double ymax = voter_ideals[0].y();
  for (const auto& v : voter_ideals) {
    xmin = std::min(xmin, v.x());
    xmax = std::max(xmax, v.x());
    ymin = std::min(ymin, v.y());
    ymax = std::max(ymax, v.y());
  }
  const double margin_x = std::max(0.3 * (xmax - xmin), 0.3);
  const double margin_y = std::max(0.3 * (ymax - ymin), 0.3);
  xmin -= margin_x;
  xmax += margin_x;
  ymin -= margin_y;
  ymax += margin_y;

  // Generate candidate grid.
  const int R = grid_resolution;
  std::vector<core::types::Point2d> grid;
  grid.reserve(static_cast<std::size_t>(R * R));
  for (int row = 0; row < R; ++row) {
    const double y = ymin + (ymax - ymin) * static_cast<double>(row) /
                                static_cast<double>(R - 1);
    for (int col = 0; col < R; ++col) {
      const double x = xmin + (xmax - xmin) * static_cast<double>(col) /
                                  static_cast<double>(R - 1);
      grid.push_back({x, y});
    }
  }

  // Apply heart() to the full grid as the alternative set.
  std::vector<core::types::Point2d> heart_pts =
      heart(grid, voter_ideals, cfg, k);

  if (heart_pts.empty()) return Polygon2E{};
  return convex_hull_2d(heart_pts);
}

}  // namespace socialchoicelab::geometry
