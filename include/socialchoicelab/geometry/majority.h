// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// majority.h — k-majority preference relation and pairwise comparison matrix.
//
// Implements:
//   majority_prefers(x, y, voter_ideals, cfg, k)  → bool
//   preference_margin(x, y, voter_ideals, cfg)     → int
//   simple_majority(n)                             → int
//   pairwise_matrix(alternatives, voter_ideals, cfg) → Eigen::MatrixXi
//
// All functions accept a k parameter (majority threshold). Default k = -1
// triggers automatic simple-majority (⌊n/2⌋ + 1). Pass any value in [1, n]
// for supermajority (k = ⌈2n/3⌉) or unanimity (k = n).
//
// References:
//   Plott (1967), "A Notion of Equilibrium and Its Possibility Under Majority
//   Rule", American Economic Review.
//   McKelvey (1976), "Intransitivities in Multidimensional Voting Models".

#include <socialchoicelab/core/types.h>
#include <socialchoicelab/preference/distance/distance_functions.h>

#include <Eigen/Dense>
#include <stdexcept>
#include <string>
#include <vector>

namespace socialchoicelab::geometry {

using socialchoicelab::core::types::Point2d;
namespace dist_ = socialchoicelab::preference::distance;

// ---------------------------------------------------------------------------
// Distance configuration
// ---------------------------------------------------------------------------

/**
 * @brief Bundles the distance metric parameters for majority computations.
 *
 * salience_weights must have the same length as the dimensionality of the
 * points being compared (2 for 2D spatial models). Default: Euclidean with
 * unit weights in 2D.
 */
struct DistConfig {
  dist_::DistanceType type = dist_::DistanceType::EUCLIDEAN;
  double order_p = 2.0;
  std::vector<double> salience_weights = {1.0, 1.0};
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/**
 * @brief Simple majority threshold: ⌊n/2⌋ + 1.
 *
 * This is the minimum number of voters required to form a strict majority
 * under the standard majority rule.
 */
[[nodiscard]] inline int simple_majority(int n) { return n / 2 + 1; }

namespace detail {

/// Resolve k: if k < 0 use simple_majority(n), else validate and return k.
inline int resolve_k(int k, int n) {
  const int threshold = (k < 0) ? simple_majority(n) : k;
  if (threshold < 1 || threshold > n) {
    throw std::invalid_argument(
        "majority: k must be in [1, n] (got k=" + std::to_string(threshold) +
        ", n=" + std::to_string(n) + ").");
  }
  return threshold;
}

/// Distance from p to alt under cfg.
inline double voter_distance(const Point2d& p, const Point2d& alt,
                             const DistConfig& cfg) {
  return dist_::calculate_distance(p, alt, cfg.type, cfg.order_p,
                                   cfg.salience_weights);
}

}  // namespace detail

// ---------------------------------------------------------------------------
// B1 — majority preference relation
// ---------------------------------------------------------------------------

/**
 * @brief Returns true if at least k voters strictly prefer x to y.
 *
 * Voter i strictly prefers x to y iff d(pᵢ, x) < d(pᵢ, y). Ties (d equal)
 * are not counted for either side.
 *
 * @param x            First alternative (2D point).
 * @param y            Second alternative (2D point).
 * @param voter_ideals Voter ideal points.
 * @param cfg          Distance configuration.
 * @param k            Majority threshold. Default (-1) = ⌊n/2⌋ + 1.
 *                     Pass any integer in [1, n] for supermajority or
 *                     unanimity.
 * @return true if at least k voters prefer x to y.
 * @throws std::invalid_argument if voter_ideals is empty or k is out of range.
 */
[[nodiscard]] inline bool majority_prefers(
    const Point2d& x, const Point2d& y,
    const std::vector<Point2d>& voter_ideals, const DistConfig& cfg,
    int k = -1) {
  const int n = static_cast<int>(voter_ideals.size());
  if (n == 0) {
    throw std::invalid_argument(
        "majority_prefers: voter_ideals must not be empty.");
  }
  const int threshold = detail::resolve_k(k, n);
  int count = 0;
  for (const Point2d& p : voter_ideals) {
    if (detail::voter_distance(p, x, cfg) < detail::voter_distance(p, y, cfg))
      ++count;
  }
  return count >= threshold;
}

/**
 * @brief Net preference margin: (#voters preferring x) − (#voters preferring
 * y).
 *
 * Positive → x is preferred by more voters; negative → y is preferred;
 * zero → tied. Voters at equal distance from x and y contribute 0.
 *
 * This function does not take a k parameter: the margin is a raw count
 * independent of the threshold. Use majority_prefers to apply a k threshold.
 *
 * @param x, y         Alternatives to compare.
 * @param voter_ideals Voter ideal points.
 * @param cfg          Distance configuration.
 * @return Signed preference margin in [−n, n].
 * @throws std::invalid_argument if voter_ideals is empty.
 */
[[nodiscard]] inline int preference_margin(
    const Point2d& x, const Point2d& y,
    const std::vector<Point2d>& voter_ideals, const DistConfig& cfg) {
  if (voter_ideals.empty()) {
    throw std::invalid_argument(
        "preference_margin: voter_ideals must not be empty.");
  }
  int margin = 0;
  for (const Point2d& p : voter_ideals) {
    const double dx = detail::voter_distance(p, x, cfg);
    const double dy = detail::voter_distance(p, y, cfg);
    if (dx < dy)
      ++margin;
    else if (dy < dx)
      --margin;
  }
  return margin;
}

// ---------------------------------------------------------------------------
// B2 — pairwise comparison matrix
// ---------------------------------------------------------------------------

/**
 * @brief Pairwise preference margin matrix over a finite alternative set.
 *
 * Entry M(i, j) = preference_margin(alternatives[i], alternatives[j]).
 * The matrix is anti-symmetric: M(i,j) = −M(j,i), and M(i,i) = 0.
 *
 * Positive M(i,j) means alternatives[i] is preferred by more voters.
 * To check k-majority dominance: alternatives[i] k-beats alternatives[j]
 * iff majority_prefers(alternatives[i], alternatives[j], voter_ideals, cfg, k).
 *
 * The k parameter is accepted for API consistency with other majority
 * functions but does not affect the matrix values (raw margins are
 * k-independent). Use majority_prefers directly when k-threshold matters.
 *
 * @param alternatives Finite set of alternatives.
 * @param voter_ideals Voter ideal points.
 * @param cfg          Distance configuration.
 * @param k            Majority threshold (accepted but not used in margin
 *                     computation; pass -1 for simple majority convention).
 * @return Eigen::MatrixXi of size (m × m) where m = alternatives.size().
 * @throws std::invalid_argument if alternatives or voter_ideals is empty.
 */
[[nodiscard]] inline Eigen::MatrixXi pairwise_matrix(
    const std::vector<Point2d>& alternatives,
    const std::vector<Point2d>& voter_ideals, const DistConfig& cfg,
    int k = -1) {
  const int m = static_cast<int>(alternatives.size());
  if (m == 0) {
    throw std::invalid_argument(
        "pairwise_matrix: alternatives must not be empty.");
  }
  if (voter_ideals.empty()) {
    throw std::invalid_argument(
        "pairwise_matrix: voter_ideals must not be empty.");
  }
  // k is validated but not used in the margin computation.
  detail::resolve_k(k, static_cast<int>(voter_ideals.size()));

  Eigen::MatrixXi M = Eigen::MatrixXi::Zero(m, m);
  for (int i = 0; i < m; ++i) {
    for (int j = i + 1; j < m; ++j) {
      const int margin = preference_margin(alternatives[i], alternatives[j],
                                           voter_ideals, cfg);
      M(i, j) = margin;
      M(j, i) = -margin;
    }
  }
  return M;
}

// ===========================================================================
// F5 — Weighted majority preference relation
// ===========================================================================

/**
 * @brief Returns true if the total weight of voters preferring x to y meets
 *        the given weight threshold.
 *
 * Voter i strictly prefers x to y iff d(pᵢ, x) < d(pᵢ, y).  Ties
 * contribute 0 weight to either side.  The result is:
 *
 *   Σ_{i: d(pᵢ,x) < d(pᵢ,y)} wᵢ  ≥  threshold × Σᵢ wᵢ
 *
 * With uniform weights (all wᵢ = c) and threshold = 0.5 this reduces to
 * majority_prefers with k = ⌈n/2⌉.
 *
 * @param x             First alternative.
 * @param y             Second alternative.
 * @param voter_ideals  Voter ideal points.
 * @param weights       Voting weights (positive; same length as voter_ideals).
 * @param cfg           Distance configuration.
 * @param threshold     Weight fraction for majority (default 0.5).
 * @return              true iff weighted majority prefers x to y.
 * @throws std::invalid_argument if inputs are invalid.
 */
[[nodiscard]] inline bool weighted_majority_prefers(
    const Point2d& x, const Point2d& y,
    const std::vector<Point2d>& voter_ideals,
    const std::vector<double>& weights, const DistConfig& cfg = {},
    double threshold = 0.5) {
  const int n = static_cast<int>(voter_ideals.size());
  if (n == 0) {
    throw std::invalid_argument(
        "weighted_majority_prefers: voter_ideals must not be empty.");
  }
  if (static_cast<int>(weights.size()) != n) {
    throw std::invalid_argument(
        "weighted_majority_prefers: weights must have the same length as "
        "voter_ideals (got " +
        std::to_string(weights.size()) + " weights for " + std::to_string(n) +
        " voters).");
  }
  if (threshold <= 0.0 || threshold > 1.0) {
    throw std::invalid_argument(
        "weighted_majority_prefers: threshold must be in (0, 1] (got " +
        std::to_string(threshold) + ").");
  }

  double total_weight = 0.0;
  double x_weight = 0.0;
  for (int i = 0; i < n; ++i) {
    const double w = weights[static_cast<std::size_t>(i)];
    if (w <= 0.0) {
      throw std::invalid_argument(
          "weighted_majority_prefers: all weights must be positive "
          "(weight[" +
          std::to_string(i) + "] = " + std::to_string(w) + ").");
    }
    total_weight += w;
    if (detail::voter_distance(voter_ideals[static_cast<std::size_t>(i)], x,
                               cfg) <
        detail::voter_distance(voter_ideals[static_cast<std::size_t>(i)], y,
                               cfg)) {
      x_weight += w;
    }
  }
  return x_weight >= threshold * total_weight;
}

}  // namespace socialchoicelab::geometry
