// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// approval.h — Approval voting rule (Category 1: natively returns a set).
//
// Two variants:
//   Spatial  — voter i approves alternative j iff dist(vᵢ, altⱼ) ≤ threshold.
//   Top-k    — voter i approves their top-k alternatives (ordinal).
//
// Both variants return all tied winners as a vector (the rule's definition
// does not impose a single winner).  There is no *_one_winner() variant;
// if the caller needs exactly one, they must apply a tie-breaker explicitly.
//
// References:
//   Brams & Fishburn (1978). "Approval voting." American Political Science
//   Review 72(3), 831–847.
//   Brams & Fishburn (1983). Approval Voting. Birkhäuser.

#include <socialchoicelab/aggregation/profile.h>
#include <socialchoicelab/geometry/majority.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace socialchoicelab::aggregation {

using core::types::Point2d;
using geometry::DistConfig;

// ---------------------------------------------------------------------------
// Spatial approval
// ---------------------------------------------------------------------------

/**
 * @brief Compute approval scores using a spatial distance threshold.
 *
 * Voter i approves alternative j iff dist(voter_ideals[i], alternatives[j])
 * ≤ threshold_distance.  Score = total approvals.
 *
 * @param alternatives        m alternative positions.
 * @param voter_ideals        n voter ideal points.
 * @param cfg                 Distance metric configuration.
 * @param threshold_distance  Approval radius (≥ 0).
 * @return                    Vector of length m with approval counts.
 * @throws std::invalid_argument if threshold_distance < 0.
 */
[[nodiscard]] inline std::vector<int> approval_scores_spatial(
    const std::vector<Point2d>& alternatives,
    const std::vector<Point2d>& voter_ideals, const DistConfig& cfg,
    double threshold_distance) {
  if (threshold_distance < 0.0) {
    throw std::invalid_argument(
        "approval_scores_spatial: threshold_distance must be non-negative "
        "(got " +
        std::to_string(threshold_distance) + ").");
  }
  const int m = static_cast<int>(alternatives.size());
  const int n = static_cast<int>(voter_ideals.size());
  std::vector<int> scores(m, 0);
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < m; ++j) {
      if (geometry::detail::voter_distance(voter_ideals[i], alternatives[j],
                                           cfg) <= threshold_distance) {
        ++scores[j];
      }
    }
  }
  return scores;
}

/**
 * @brief Return all alternatives with the maximum spatial approval score.
 *
 * Returns an empty vector if no voter approves any alternative (all scores 0).
 * Results are sorted by ascending index.
 *
 * @param alternatives        m alternative positions.
 * @param voter_ideals        n voter ideal points.
 * @param cfg                 Distance metric configuration.
 * @param threshold_distance  Approval radius (≥ 0).
 * @return                    Indices of alternatives with maximum approval.
 */
[[nodiscard]] inline std::vector<int> approval_all_winners_spatial(
    const std::vector<Point2d>& alternatives,
    const std::vector<Point2d>& voter_ideals, const DistConfig& cfg,
    double threshold_distance) {
  const auto scores = approval_scores_spatial(alternatives, voter_ideals, cfg,
                                              threshold_distance);
  if (scores.empty()) return {};
  const int top = *std::max_element(scores.begin(), scores.end());
  if (top == 0) return {};  // no approvals
  std::vector<int> winners;
  const int m = static_cast<int>(scores.size());
  for (int j = 0; j < m; ++j) {
    if (scores[j] == top) winners.push_back(j);
  }
  return winners;
}

// ---------------------------------------------------------------------------
// Ordinal (top-k) approval
// ---------------------------------------------------------------------------

/**
 * @brief Compute approval scores using a top-k ordinal threshold.
 *
 * Voter i approves their top-k alternatives (i.e. the k alternatives in
 * positions 0 … k-1 of their ranking).
 *
 * @param profile  Well-formed preference profile.
 * @param k        Number of approved alternatives per voter (1 ≤ k ≤ n_alts).
 * @return         Vector of length n_alts with approval counts.
 * @throws std::invalid_argument if k < 1 or k > n_alts.
 */
[[nodiscard]] inline std::vector<int> approval_scores_topk(
    const Profile& profile, int k) {
  if (k < 1 || k > profile.n_alts) {
    throw std::invalid_argument(
        "approval_scores_topk: k must be in [1, n_alts] (got k=" +
        std::to_string(k) + ", n_alts=" + std::to_string(profile.n_alts) +
        "). Enter a value between 1 and " + std::to_string(profile.n_alts) +
        ".");
  }
  std::vector<int> scores(profile.n_alts, 0);
  for (const auto& r : profile.rankings) {
    for (int rank = 0; rank < k; ++rank) ++scores[r[rank]];
  }
  return scores;
}

/**
 * @brief Return all alternatives with the maximum top-k approval score.
 *
 * Results are sorted by ascending index.
 *
 * @param profile  Well-formed preference profile.
 * @param k        Number of approved alternatives per voter.
 * @return         Indices of alternatives with maximum top-k approval.
 */
[[nodiscard]] inline std::vector<int> approval_all_winners_topk(
    const Profile& profile, int k) {
  const auto scores = approval_scores_topk(profile, k);
  if (scores.empty()) return {};
  const int top = *std::max_element(scores.begin(), scores.end());
  std::vector<int> winners;
  for (int j = 0; j < profile.n_alts; ++j) {
    if (scores[j] == top) winners.push_back(j);
  }
  return winners;
}

}  // namespace socialchoicelab::aggregation
