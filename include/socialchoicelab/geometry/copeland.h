// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// copeland.h — Copeland scores and Copeland winner (F3).
//
// The Copeland score of an alternative is the number of alternatives it
// beats minus the number that beat it under the k-majority rule.  The
// Copeland winner is the alternative with the highest score.
//
// In the spatial voting literature the Copeland winner is sometimes called
// the "strong point" — the alternative with the smallest expected win-set
// area (Banks 1985; Straffin 1980).
//
// Properties:
//   - If a Condorcet winner exists it has the unique maximum Copeland score.
//   - In a Condorcet cycle all alternatives have the same Copeland score (0
//     for a three-alternative pure cycle).
//   - The Copeland winner always lies in the uncovered set (Miller 1980).
//   - Tie-breaking: copeland_winner returns the first maximum-score
//     alternative in the order of the input alternatives vector.
//
// References:
//   Copeland (1951), "A Reasonable Social Welfare Function," unpublished.
//   Miller (1980), AJPS 24(1), 68–96.
//   Banks (1985), "Sophisticated voting outcomes and agenda control,"
//     Social Choice and Welfare 1, 295–306.

#include <socialchoicelab/geometry/majority.h>

#include <stdexcept>
#include <string>
#include <vector>

namespace socialchoicelab::geometry {

// ===========================================================================
// F3 — Copeland scores
// ===========================================================================

/**
 * @brief Compute the k-majority Copeland score for each alternative.
 *
 * score[i] = #{j ≠ i : majority_prefers(alt[i], alt[j])}
 *          − #{j ≠ i : majority_prefers(alt[j], alt[i])}
 *
 * Ties (neither beats the other) contribute 0 to both sides.
 *
 * Time complexity: O(m² · n) where m = |alternatives|, n = |voter_ideals|.
 *
 * @param alternatives  Finite set of alternatives (at least 1).
 * @param voter_ideals  Voter ideal points.
 * @param cfg           Distance configuration.
 * @param k             Majority threshold; -1 = ⌊n/2⌋+1.
 * @return              Vector of Copeland scores, parallel to alternatives.
 * @throws std::invalid_argument if alternatives or voter_ideals is empty.
 */
[[nodiscard]] inline std::vector<int> copeland_scores(
    const std::vector<core::types::Point2d>& alternatives,
    const std::vector<core::types::Point2d>& voter_ideals,
    const DistConfig& cfg = {}, int k = -1) {
  if (alternatives.empty()) {
    throw std::invalid_argument(
        "copeland_scores: alternatives must not be empty.");
  }
  if (voter_ideals.empty()) {
    throw std::invalid_argument(
        "copeland_scores: voter_ideals must not be empty.");
  }

  const int m = static_cast<int>(alternatives.size());
  std::vector<int> scores(static_cast<std::size_t>(m), 0);

  for (int i = 0; i < m; ++i) {
    for (int j = i + 1; j < m; ++j) {
      bool ij = majority_prefers(alternatives[static_cast<std::size_t>(i)],
                                 alternatives[static_cast<std::size_t>(j)],
                                 voter_ideals, cfg, k);
      bool ji = majority_prefers(alternatives[static_cast<std::size_t>(j)],
                                 alternatives[static_cast<std::size_t>(i)],
                                 voter_ideals, cfg, k);
      if (ij) {
        ++scores[static_cast<std::size_t>(i)];
        --scores[static_cast<std::size_t>(j)];
      } else if (ji) {
        --scores[static_cast<std::size_t>(i)];
        ++scores[static_cast<std::size_t>(j)];
      }
      // Tie: no change to either score.
    }
  }
  return scores;
}

// ===========================================================================
// F3 — Copeland winner
// ===========================================================================

/**
 * @brief Return the k-majority Copeland winner.
 *
 * The Copeland winner is the alternative with the maximum Copeland score.
 * Ties are broken by order of appearance in @p alternatives (the first
 * maximum-score alternative is returned).
 *
 * If a Condorcet winner exists it is guaranteed to have a strictly higher
 * score than any other alternative and is returned.
 *
 * @param alternatives  Finite set of alternatives (at least 1).
 * @param voter_ideals  Voter ideal points.
 * @param cfg           Distance configuration.
 * @param k             Majority threshold; -1 = ⌊n/2⌋+1.
 * @return              The Copeland winner (point from alternatives).
 * @throws std::invalid_argument if alternatives or voter_ideals is empty.
 */
[[nodiscard]] inline core::types::Point2d copeland_winner(
    const std::vector<core::types::Point2d>& alternatives,
    const std::vector<core::types::Point2d>& voter_ideals,
    const DistConfig& cfg = {}, int k = -1) {
  const std::vector<int> scores =
      copeland_scores(alternatives, voter_ideals, cfg, k);

  std::size_t best_idx = 0;
  for (std::size_t i = 1; i < scores.size(); ++i) {
    if (scores[i] > scores[best_idx]) best_idx = i;
  }
  return alternatives[best_idx];
}

}  // namespace socialchoicelab::geometry
