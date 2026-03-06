// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// antiplurality.h — Anti-plurality voting rule (Category 3: ties need
// breaking).
//
// Each voter gives 1 point to every alternative EXCEPT their least preferred.
// Equivalently, the score of alternative j is the number of voters who do NOT
// rank j last.
//
// Score identity: scores[j] = n_voters × (n_alts − 1) − last_place_votes[j],
// so the rule minimises last-place votes.
//
// Named and surveyed in Fishburn (1977) "Condorcet Social Choice Functions."
//
// Functions:
//   antiplurality_scores()      — raw scores (no tie-breaking).
//   antiplurality_all_winners() — all alternatives with the top score.
//   antiplurality_one_winner()  — a single winner; tie-breaking via TieBreak.

#include <socialchoicelab/aggregation/profile.h>
#include <socialchoicelab/aggregation/tie_break.h>

#include <algorithm>
#include <vector>

namespace socialchoicelab::aggregation {

/**
 * @brief Compute anti-plurality scores.
 *
 * scores[j] = number of voters for whom alternative j is NOT ranked last.
 *
 * @param profile  Well-formed preference profile.
 * @return         Vector of length n_alts with anti-plurality scores.
 */
[[nodiscard]] inline std::vector<int> antiplurality_scores(
    const Profile& profile) {
  const int m = profile.n_alts;
  std::vector<int> scores(m, 0);
  for (const auto& r : profile.rankings) {
    // Award 1 point to every position except the last.
    for (int rank = 0; rank < m - 1; ++rank) ++scores[r[rank]];
  }
  return scores;
}

/**
 * @brief Return all alternatives with the highest anti-plurality score.
 *
 * Results are sorted by ascending index.
 *
 * @param profile  Well-formed preference profile.
 * @return         Indices of tied anti-plurality winners.
 */
[[nodiscard]] inline std::vector<int> antiplurality_all_winners(
    const Profile& profile) {
  const auto scores = antiplurality_scores(profile);
  if (scores.empty()) return {};
  const int top = *std::max_element(scores.begin(), scores.end());
  std::vector<int> winners;
  for (int j = 0; j < profile.n_alts; ++j) {
    if (scores[j] == top) winners.push_back(j);
  }
  return winners;
}

/**
 * @brief Return a single anti-plurality winner, resolving ties via tie_break.
 *
 * @param profile    Well-formed preference profile.
 * @param tie_break  Policy when multiple alternatives are tied (default:
 *                   kRandom).
 * @param prng       Required when tie_break == kRandom; ignored otherwise.
 * @return           Index of the selected winner.
 */
[[nodiscard]] inline int antiplurality_one_winner(
    const Profile& profile, TieBreak tie_break = TieBreak::kRandom,
    core::rng::PRNG* prng = nullptr) {
  return break_tie(antiplurality_all_winners(profile), tie_break, prng);
}

}  // namespace socialchoicelab::aggregation
