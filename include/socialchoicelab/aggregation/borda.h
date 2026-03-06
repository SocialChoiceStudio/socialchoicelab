// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// borda.h — Borda count voting rule (Category 3: ties need breaking).
//
// Voter i gives score (m − 1 − rank) to the alternative at position rank in
// their ranking.  Total Borda score = sum over all voters.
//
// Introduced by Borda (1784; presented 1781) to the Académie des Sciences.
//
// Functions:
//   borda_scores()      — raw integer Borda scores per alternative.
//   borda_all_winners() — all alternatives sharing the top Borda score.
//   borda_one_winner()  — a single Borda winner; tie-breaking via TieBreak.
//   borda_ranking()     — full social ordering by descending Borda score;
//                         ties within score groups broken by tie_break.

#include <socialchoicelab/aggregation/profile.h>
#include <socialchoicelab/aggregation/tie_break.h>

#include <algorithm>
#include <numeric>
#include <vector>

namespace socialchoicelab::aggregation {

/**
 * @brief Compute Borda scores.
 *
 * Voter i contributes (m − 1 − rank) to the alternative at rank r in
 * rankings[i].  Total score is summed over all voters.
 *
 * @param profile  Well-formed preference profile.
 * @return         Vector of length n_alts with Borda scores.
 */
[[nodiscard]] inline std::vector<int> borda_scores(const Profile& profile) {
  const int m = profile.n_alts;
  std::vector<int> scores(m, 0);
  for (const auto& r : profile.rankings) {
    for (int rank = 0; rank < m; ++rank) {
      scores[r[rank]] += (m - 1 - rank);
    }
  }
  return scores;
}

/**
 * @brief Return all alternatives with the highest Borda score.
 *
 * Results are sorted by ascending index.
 *
 * @param profile  Well-formed preference profile.
 * @return         Indices of tied Borda winners.
 */
[[nodiscard]] inline std::vector<int> borda_all_winners(
    const Profile& profile) {
  const auto scores = borda_scores(profile);
  if (scores.empty()) return {};
  const int top = *std::max_element(scores.begin(), scores.end());
  std::vector<int> winners;
  for (int j = 0; j < profile.n_alts; ++j) {
    if (scores[j] == top) winners.push_back(j);
  }
  return winners;
}

/**
 * @brief Return a single Borda winner, resolving ties via tie_break.
 *
 * @param profile    Well-formed preference profile.
 * @param tie_break  Policy when multiple alternatives are tied (default:
 *                   kRandom).
 * @param prng       Required when tie_break == kRandom; ignored otherwise.
 * @return           Index of the selected winner.
 */
[[nodiscard]] inline int borda_one_winner(
    const Profile& profile, TieBreak tie_break = TieBreak::kRandom,
    core::rng::PRNG* prng = nullptr) {
  return break_tie(borda_all_winners(profile), tie_break, prng);
}

/**
 * @brief Produce a full social ordering by descending Borda score.
 *
 * Alternatives within each score group (tied) are ordered by tie_break.
 * With kSmallestIndex they appear in ascending index order; with kRandom
 * the order within each tied group is shuffled.
 *
 * @param profile    Well-formed preference profile.
 * @param tie_break  Policy for ordering tied alternatives (default: kRandom).
 * @param prng       Required when tie_break == kRandom; ignored otherwise.
 * @return           All alternative indices in descending Borda-score order.
 */
[[nodiscard]] inline std::vector<int> borda_ranking(
    const Profile& profile, TieBreak tie_break = TieBreak::kRandom,
    core::rng::PRNG* prng = nullptr) {
  const auto scores = borda_scores(profile);
  const int m = profile.n_alts;

  // Sort indices by descending score (stable within score groups for
  // determinism before tie-breaking is applied below).
  std::vector<int> order(m);
  std::iota(order.begin(), order.end(), 0);
  std::stable_sort(order.begin(), order.end(),
                   [&](int a, int b) { return scores[a] > scores[b]; });

  // Apply tie-breaking within each score group.
  std::vector<int> result;
  result.reserve(m);
  int i = 0;
  while (i < m) {
    int j = i;
    while (j < m && scores[order[j]] == scores[order[i]]) ++j;
    // order[i..j) share the same Borda score.
    std::vector<int> group(order.begin() + i, order.begin() + j);
    if (tie_break == TieBreak::kSmallestIndex) {
      std::sort(group.begin(), group.end());  // ascending index
    } else {
      if (prng == nullptr) {
        throw std::invalid_argument(
            "borda_ranking: TieBreak::kRandom requires a non-null PRNG. "
            "Pass a PRNG& or use TieBreak::kSmallestIndex.");
      }
      std::shuffle(group.begin(), group.end(), prng->engine());
    }
    for (int idx : group) result.push_back(idx);
    i = j;
  }
  return result;
}

}  // namespace socialchoicelab::aggregation
