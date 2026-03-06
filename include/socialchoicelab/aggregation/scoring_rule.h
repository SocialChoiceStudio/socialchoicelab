// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// scoring_rule.h — Generic positional scoring rule (Category 3: ties need
// breaking).
//
// A positional scoring rule is defined by a non-increasing score vector
//   s = (s[0] ≥ s[1] ≥ … ≥ s[m-1]).
// Voter i contributes s[rank] to the alternative at position rank.
//
// Special cases recovered by choice of score_vector:
//   Plurality      — (1, 0, 0, …, 0)
//   Borda          — (m-1, m-2, …, 1, 0)
//   Anti-plurality — (1, 1, …, 1, 0)
//
// Reference: Young (1975). "Social choice scoring functions." SIAM Journal
// on Applied Mathematics 28(4), 824–838.
//
// Functions:
//   scoring_rule_scores()      — raw double scores per alternative.
//   scoring_rule_all_winners() — all tied top-scorers.
//   scoring_rule_one_winner()  — single winner; tie-breaking via TieBreak.

#include <socialchoicelab/aggregation/profile.h>
#include <socialchoicelab/aggregation/tie_break.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace socialchoicelab::aggregation {

/**
 * @brief Compute scores under a generic positional scoring rule.
 *
 * @param profile       Well-formed preference profile.
 * @param score_vector  Non-increasing vector of length n_alts; score_vector[r]
 *                      is the score awarded to the alternative at rank r.
 * @return              Vector of length n_alts with cumulative scores.
 * @throws std::invalid_argument if score_vector.size() ≠ n_alts or the vector
 *         is not non-increasing.
 */
[[nodiscard]] inline std::vector<double> scoring_rule_scores(
    const Profile& profile, const std::vector<double>& score_vector) {
  if (static_cast<int>(score_vector.size()) != profile.n_alts) {
    throw std::invalid_argument(
        "scoring_rule_scores: score_vector must have length n_alts (got " +
        std::to_string(score_vector.size()) + ", expected " +
        std::to_string(profile.n_alts) + ").");
  }
  for (int i = 1; i < profile.n_alts; ++i) {
    if (score_vector[i] > score_vector[i - 1]) {
      throw std::invalid_argument(
          "scoring_rule_scores: score_vector must be non-increasing "
          "(score_vector[" +
          std::to_string(i) + "]=" + std::to_string(score_vector[i]) +
          " > score_vector[" + std::to_string(i - 1) +
          "]=" + std::to_string(score_vector[i - 1]) +
          "). Provide a non-increasing score vector.");
    }
  }

  std::vector<double> scores(profile.n_alts, 0.0);
  for (const auto& r : profile.rankings) {
    for (int rank = 0; rank < profile.n_alts; ++rank) {
      scores[r[rank]] += score_vector[rank];
    }
  }
  return scores;
}

/**
 * @brief Return all alternatives with the highest score under a generic
 *        positional rule.
 *
 * Results are sorted by ascending index.
 *
 * @param profile       Well-formed preference profile.
 * @param score_vector  Non-increasing score vector of length n_alts.
 * @return              Indices of tied winners.
 */
[[nodiscard]] inline std::vector<int> scoring_rule_all_winners(
    const Profile& profile, const std::vector<double>& score_vector) {
  const auto scores = scoring_rule_scores(profile, score_vector);
  if (scores.empty()) return {};
  const double top = *std::max_element(scores.begin(), scores.end());
  std::vector<int> winners;
  for (int j = 0; j < profile.n_alts; ++j) {
    if (scores[j] == top) winners.push_back(j);
  }
  return winners;
}

/**
 * @brief Return a single winner under a generic positional rule.
 *
 * @param profile       Well-formed preference profile.
 * @param score_vector  Non-increasing score vector of length n_alts.
 * @param tie_break     Policy when multiple alternatives are tied (default:
 *                      kRandom).
 * @param prng          Required when tie_break == kRandom.
 * @return              Index of the selected winner.
 */
[[nodiscard]] inline int scoring_rule_one_winner(
    const Profile& profile, const std::vector<double>& score_vector,
    TieBreak tie_break = TieBreak::kRandom, core::rng::PRNG* prng = nullptr) {
  return break_tie(scoring_rule_all_winners(profile, score_vector), tie_break,
                   prng);
}

}  // namespace socialchoicelab::aggregation
