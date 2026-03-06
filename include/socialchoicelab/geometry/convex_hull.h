// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <CGAL/convex_hull_2.h>
#include <socialchoicelab/geometry/geom2d.h>

#include <stdexcept>
#include <string>
#include <vector>

/**
 * @file convex_hull.h
 * @brief 2D convex hull of voter ideal points.
 *
 * Under Euclidean preferences the convex hull of the voter ideal points equals
 * the Pareto set — the set of alternatives not unanimously dominated by any
 * other. This module provides an exact CGAL-based computation of that hull.
 *
 * For non-Euclidean metrics (Manhattan, Minkowski p) the Pareto set is a
 * subset of the convex hull; the hull serves as an outer bound. See the open
 * question in geometry_plan.md § Pareto set for the generalisation.
 *
 * Reference: CGAL manual § 2D Convex Hulls and Extreme Points.
 */

namespace socialchoicelab::geometry {

/**
 * @brief Compute the 2D convex hull of a set of voter ideal points.
 *
 * Accepts floating-point Eigen points (the numeric representation used
 * throughout the core layer), converts them to exact CGAL points, and returns
 * the hull as an exact CCW polygon.
 *
 * Degenerate cases handled:
 *   - 1 point:                 returns a single-vertex polygon.
 *   - 2 points (or collinear): returns the two extreme points.
 *   - General position:        returns the CCW convex hull.
 *
 * @param pts  Voter ideal points (Eigen double). Must be non-empty; all
 *             coordinates must be finite (no NaN or Inf).
 * @return     Exact convex-hull polygon in CCW orientation (EPEC).
 * @throws std::invalid_argument if pts is empty or contains non-finite values.
 */
inline Polygon2E convex_hull_2d(const std::vector<core::types::Point2d>& pts) {
  if (pts.empty()) {
    throw std::invalid_argument(
        "convex_hull_2d: input must contain at least 1 point (got 0). "
        "Provide at least one voter ideal point.");
  }

  for (std::size_t i = 0; i < pts.size(); ++i) {
    if (!pts[i].allFinite()) {
      throw std::invalid_argument(
          "convex_hull_2d: point at index " + std::to_string(i) +
          " contains a non-finite coordinate (NaN or Inf). "
          "All voter ideal point coordinates must be finite real numbers.");
    }
  }

  std::vector<Point2E> exact_pts;
  exact_pts.reserve(pts.size());
  for (const auto& p : pts) {
    exact_pts.push_back(to_exact(p));
  }

  std::vector<Point2E> hull_pts;
  hull_pts.reserve(exact_pts.size());
  CGAL::convex_hull_2(exact_pts.begin(), exact_pts.end(),
                      std::back_inserter(hull_pts));

  return Polygon2E(hull_pts.begin(), hull_pts.end());
}

}  // namespace socialchoicelab::geometry
