// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// centrality.h — Coordinate-wise centrality measures for 2D voter ideal
// points.
//
// Provides three summary points derived from a set of voter ideal points, each
// capturing a different notion of the "centre" of a voter configuration.
//
// Summary:
//   marginal_median_2d  — coordinate-wise median (issue-by-issue median voter)
//   centroid_2d         — coordinate-wise arithmetic mean (centre of mass)
//   geometric_mean_2d   — coordinate-wise geometric mean (log-space centroid;
//                         requires strictly positive coordinates)
//
// All functions require a non-empty voter set and throw std::invalid_argument
// on any contract violation.  None requires CGAL; all arithmetic is double.
//
// Background:
//   These statistics are frequently used in spatial voting models to locate a
//   reference point in policy space relative to the voter distribution.  None
//   is in general a majority-rule equilibrium (Condorcet winner) in 2D; they
//   serve as descriptive summaries, not as solution concepts.
//
// References:
//   Black, D. (1948). "On the rationale of group decision-making." Journal of
//     Political Economy, 56(1), 23–34. [Median Voter Theorem — 1D]
//   Enelow, J.M. & Hinich, M.J. (1984). The Spatial Theory of Voting.
//     Cambridge University Press.
//   Poole, K.T. & Rosenthal, H. (1985). "A spatial model for legislative roll
//     call analysis." American Journal of Political Science, 29(2), 357–384.
//     [NOMINATE; ideal points in [-1, 1]²]

#include <socialchoicelab/core/types.h>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace socialchoicelab::geometry {

using core::types::Point2d;

// ---------------------------------------------------------------------------
// marginal_median_2d
// ---------------------------------------------------------------------------

/**
 * @brief Coordinate-wise median of voter ideal points.
 *
 * Each output coordinate is the median of the corresponding input coordinates
 * computed independently across all voters.  Also called the *componentwise
 * median*, *axis-aligned median*, or *issue-by-issue median*.
 *
 * In spatial voting theory this corresponds to the outcome of
 * dimension-by-dimension majority voting under separable preferences: each
 * policy dimension is resolved independently by majority rule, yielding the
 * median voter on that dimension (Black 1948).  It is generally *not* a
 * majority-rule equilibrium in 2D — a point must satisfy Plott's (1967) radial
 * symmetry conditions to be a Condorcet winner.
 *
 * For an even number of voters the median is the average of the two middle
 * values (standard statistical convention), which may not coincide with any
 * voter ideal point.
 *
 * @param voter_ideals  Non-empty vector of 2D voter ideal points.
 * @returns Point2d     The coordinate-wise median.
 * @throws std::invalid_argument if voter_ideals is empty.
 */
[[nodiscard]] inline Point2d marginal_median_2d(
    const std::vector<Point2d>& voter_ideals) {
  if (voter_ideals.empty()) {
    throw std::invalid_argument(
        "marginal_median_2d: voter_ideals must not be empty.");
  }

  const std::size_t n = voter_ideals.size();
  std::vector<double> xs(n), ys(n);
  for (std::size_t i = 0; i < n; ++i) {
    xs[i] = voter_ideals[i].x();
    ys[i] = voter_ideals[i].y();
  }

  auto median_of = [](std::vector<double>& v) -> double {
    const std::size_t m = v.size();
    const std::size_t mid = m / 2;
    std::nth_element(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(mid),
                     v.end());
    if (m % 2 == 1) {
      return v[mid];
    }
    // Even n: average of the two middle values.
    const double upper = v[mid];
    std::nth_element(v.begin(),
                     v.begin() + static_cast<std::ptrdiff_t>(mid) - 1, v.end());
    return (v[mid - 1] + upper) / 2.0;
  };

  return Point2d(median_of(xs), median_of(ys));
}

// ---------------------------------------------------------------------------
// centroid_2d
// ---------------------------------------------------------------------------

/**
 * @brief Coordinate-wise arithmetic mean of voter ideal points.
 *
 * Each output coordinate is the arithmetic mean of the corresponding input
 * coordinates.  Also called the *barycenter*, *centre of mass*, or *mean voter
 * position*.
 *
 * The centroid is not generally a majority-rule equilibrium in 2D.  It appears
 * in results such as the Shapley-Owen power index: the strong point (spatial
 * Copeland winner) equals the SOV-weighted average of voter ideal points
 * (Shapley & Owen 1989).
 *
 * @param voter_ideals  Non-empty vector of 2D voter ideal points.
 * @returns Point2d     The coordinate-wise arithmetic mean.
 * @throws std::invalid_argument if voter_ideals is empty.
 */
[[nodiscard]] inline Point2d centroid_2d(
    const std::vector<Point2d>& voter_ideals) {
  if (voter_ideals.empty()) {
    throw std::invalid_argument("centroid_2d: voter_ideals must not be empty.");
  }

  double sum_x = 0.0, sum_y = 0.0;
  for (const auto& p : voter_ideals) {
    sum_x += p.x();
    sum_y += p.y();
  }
  const double n = static_cast<double>(voter_ideals.size());
  return Point2d(sum_x / n, sum_y / n);
}

// ---------------------------------------------------------------------------
// geometric_mean_2d
// ---------------------------------------------------------------------------

/**
 * @brief Coordinate-wise geometric mean of voter ideal points.
 *
 * Each output coordinate is the geometric mean of the corresponding input
 * coordinates: exp(mean(log(xᵢ))).  Equivalently, the nth root of the product
 * of values in each dimension.  Also called the *multiplicative mean* or
 * *log-space centroid* — it is the arithmetic mean computed in log-transformed
 * policy space.
 *
 * This measure may be appropriate when coordinates represent proportional or
 * ratio-scale quantities, for example budget shares or relative policy
 * distances.
 *
 * **Important — NOMINATE-scale data:**
 * Many empirical spatial voting models (W-NOMINATE, DW-NOMINATE) place ideal
 * points on the scale [-1, 1] per dimension.  The geometric mean is undefined
 * for non-positive values.  If your coordinates are on this scale, use
 * centroid_2d or marginal_median_2d instead, or restrict analysis to the
 * subset of voters with strictly positive coordinates in each dimension.
 *
 * @param voter_ideals  Non-empty vector of 2D voter ideal points; all
 *                      coordinates must be strictly positive.
 * @returns Point2d     The coordinate-wise geometric mean.
 * @throws std::invalid_argument if voter_ideals is empty or if any coordinate
 *                               is not strictly positive.
 */
[[nodiscard]] inline Point2d geometric_mean_2d(
    const std::vector<Point2d>& voter_ideals) {
  if (voter_ideals.empty()) {
    throw std::invalid_argument(
        "geometric_mean_2d: voter_ideals must not be empty.");
  }

  // Validate all coordinates and collect range info for a useful error message.
  double min_x = voter_ideals[0].x(), max_x = voter_ideals[0].x();
  double min_y = voter_ideals[0].y(), max_y = voter_ideals[0].y();
  for (const auto& p : voter_ideals) {
    min_x = std::min(min_x, p.x());
    max_x = std::max(max_x, p.x());
    min_y = std::min(min_y, p.y());
    max_y = std::max(max_y, p.y());
  }

  if (min_x <= 0.0 || min_y <= 0.0) {
    std::ostringstream msg;
    msg << "geometric_mean_2d: all coordinates must be strictly positive, but ";
    if (min_x <= 0.0) {
      msg << "x-coordinates range from " << min_x << " to " << max_x;
      if (min_y <= 0.0) {
        msg << " and y-coordinates range from " << min_y << " to " << max_y;
      }
    } else {
      msg << "y-coordinates range from " << min_y << " to " << max_y;
    }
    msg << ". The geometric mean is undefined for non-positive values. "
           "If your data uses NOMINATE scale [-1, 1], consider centroid_2d "
           "or marginal_median_2d instead, or restrict to a strictly positive "
           "subset of coordinates.";
    throw std::invalid_argument(msg.str());
  }

  double log_sum_x = 0.0, log_sum_y = 0.0;
  for (const auto& p : voter_ideals) {
    log_sum_x += std::log(p.x());
    log_sum_y += std::log(p.y());
  }
  const double n = static_cast<double>(voter_ideals.size());
  return Point2d(std::exp(log_sum_x / n), std::exp(log_sum_y / n));
}

}  // namespace socialchoicelab::geometry
