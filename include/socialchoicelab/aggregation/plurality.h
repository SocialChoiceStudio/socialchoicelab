// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// plurality.h — Plurality voting rule (Category 3: ties need breaking).
//
// Each voter contributes 1 point to their most-preferred alternative.
// The alternative(s) with the most first-place votes win.
//
// Functions:
//   plurality_scores()      — raw first-place vote counts (no tie-breaking).
//   plurality_all_winners() — all alternatives sharing the top score.
//   plurality_one_winner()  — a single winner; tie-breaking via TieBreak.

#include <socialchoicelab/aggregation/profile.h>
#include <socialchoicelab/aggregation/tie_break.h>

#include <algorithm>
#include <vector>

namespace socialchoicelab::aggregation {

/**
 * @brief Compute plurality scores: first-place vote counts per alternative.
 *
 * @param profile  Well-formed preference profile.
 * @return         Vector of length n_alts; scores[j] = number of voters
 *                 ranking alternative j first.
 */
[[nodiscard]] inline std::vector<int> plurality_scores(const Profile& profile) {
  std::vector<int> scores(profile.n_alts, 0);
  for (const auto& r : profile.rankings) {
    if (!r.empty()) ++scores[r[0]];
  }
  return scores;
}

/**
 * @brief Return all alternatives with the highest plurality score.
 *
 * Returns 1 element when the top score is unique, 2+ when tied.
 * Results are sorted by ascending index.
 *
 * @param profile  Well-formed preference profile.
 * @return         Indices of tied plurality winners.
 */
[[nodiscard]] inline std::vector<int> plurality_all_winners(
    const Profile& profile) {
  const auto scores = plurality_scores(profile);
  if (scores.empty()) return {};
  const int top = *std::max_element(scores.begin(), scores.end());
  std::vector<int> winners;
  for (int j = 0; j < profile.n_alts; ++j) {
    if (scores[j] == top) winners.push_back(j);
  }
  return winners;
}

/**
 * @brief Return a single plurality winner, resolving ties via tie_break.
 *
 * @param profile    Well-formed preference profile.
 * @param tie_break  Policy when multiple alternatives are tied (default:
 *                   kRandom).
 * @param prng       Required when tie_break == kRandom; ignored otherwise.
 * @return           Index of the selected winner.
 */
[[nodiscard]] inline int plurality_one_winner(
    const Profile& profile, TieBreak tie_break = TieBreak::kRandom,
    core::rng::PRNG* prng = nullptr) {
  return break_tie(plurality_all_winners(profile), tie_break, prng);
}

}  // namespace socialchoicelab::aggregation
