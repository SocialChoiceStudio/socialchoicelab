// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// social_ranking.h — Full social orderings from rule outputs (W1).
//
// Two functions:
//   rank_by_scores()   — sort alternatives by descending score; ties broken
//                        by tie_break.  Applies to any numeric score vector
//                        from V1–V5 or from geometry/copeland.
//   pairwise_ranking() — sort by Copeland score derived from a geometry-layer
//                        pairwise matrix; delegates to copeland_scores().

#include <socialchoicelab/aggregation/tie_break.h>
#include <socialchoicelab/geometry/majority.h>

#include <Eigen/Dense>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace socialchoicelab::aggregation {

/**
 * @brief Sort alternatives by descending score, with tie-breaking.
 *
 * Ties within a score group are ordered by tie_break:
 *   kSmallestIndex — ascending index within the group (deterministic).
 *   kRandom        — shuffled within the group (requires prng).
 *
 * @param scores     Score vector of length m (any numeric type via double).
 * @param tie_break  Policy for ordering tied alternatives (default: kRandom).
 * @param prng       Required when tie_break == kRandom; ignored otherwise.
 * @return           Alternative indices 0…m-1 in descending score order.
 * @throws std::invalid_argument if scores is empty, or if kRandom and prng
 *         is nullptr.
 */
[[nodiscard]] inline std::vector<int> rank_by_scores(
    const std::vector<double>& scores, TieBreak tie_break = TieBreak::kRandom,
    core::rng::PRNG* prng = nullptr) {
  const int m = static_cast<int>(scores.size());
  if (m == 0) return {};

  std::vector<int> order(m);
  std::iota(order.begin(), order.end(), 0);
  // Stable sort so that within tied groups relative order is consistent
  // before tie-breaking is applied below.
  std::stable_sort(order.begin(), order.end(),
                   [&](int a, int b) { return scores[a] > scores[b]; });

  // Apply tie-breaking within each score group.
  std::vector<int> result;
  result.reserve(m);
  int i = 0;
  while (i < m) {
    int j = i;
    while (j < m && scores[order[j]] == scores[order[i]]) ++j;
    std::vector<int> group(order.begin() + i, order.begin() + j);
    if (tie_break == TieBreak::kSmallestIndex) {
      std::sort(group.begin(), group.end());
    } else {
      if (prng == nullptr) {
        throw std::invalid_argument(
            "rank_by_scores: TieBreak::kRandom requires a non-null PRNG. "
            "Pass a PRNG& or use TieBreak::kSmallestIndex.");
      }
      std::shuffle(group.begin(), group.end(), prng->engine());
    }
    for (int idx : group) result.push_back(idx);
    i = j;
  }
  return result;
}

/**
 * @brief Sort alternatives by Copeland score derived from a pairwise matrix.
 *
 * Copeland score for alternative i = number of j where pairwise(i, j) > 0
 * (i.e. the number of alternatives that i beats by strict majority).
 *
 * The pairwise_matrix entry M(i, j) gives the margin by which i beats j;
 * see geometry/majority.h — pairwise_matrix().
 *
 * @param pairwise   Pairwise preference margin matrix (square, m × m).
 * @param tie_break  Policy for ordering tied alternatives (default: kRandom).
 * @param prng       Required when tie_break == kRandom; ignored otherwise.
 * @return           Alternative indices in descending Copeland-score order.
 */
[[nodiscard]] inline std::vector<int> pairwise_ranking(
    const Eigen::MatrixXi& pairwise, TieBreak tie_break = TieBreak::kRandom,
    core::rng::PRNG* prng = nullptr) {
  const int m = static_cast<int>(pairwise.rows());
  std::vector<double> scores(m, 0.0);
  for (int i = 0; i < m; ++i) {
    for (int j = 0; j < m; ++j) {
      if (i != j && pairwise(i, j) > 0) scores[i] += 1.0;
    }
  }
  return rank_by_scores(scores, tie_break, prng);
}

}  // namespace socialchoicelab::aggregation
