// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// condorcet_consistency.h — Ordinal Condorcet and majority consistency (W3).
//
// These functions operate purely on ordinal profiles (rank comparisons), not
// on spatial geometry.  They complement the geometry layer's
// has_condorcet_winner / condorcet_winner functions which operate on spatial
// voter ideal points.
//
// Strict majority: alternative a beats b iff more than n/2 voters rank a
// ahead of b.  For n voters this means count > n/2 (integer division).
//
// Functions:
//   has_condorcet_winner_profile()  — does the majority relation have a CW?
//   condorcet_winner_profile()      — return the CW index, or nullopt.
//                                     Category 2: no tie-breaking.
//   is_condorcet_consistent()       — does winner equal the CW (if one exists)?
//   is_majority_consistent()        — does winner beat every other alternative
//                                     by strict majority?
//
// References:
//   Condorcet, M. J. A. N. de (1785). Essai sur l'Application …
//   Arrow, K. J. (1951). Social Choice and Individual Values.

#include <socialchoicelab/aggregation/profile.h>

#include <optional>
#include <vector>

namespace socialchoicelab::aggregation {

namespace detail {

/// Count the number of voters who strictly prefer alternative a over b
/// (i.e. a appears at a lower rank index than b in their ranking).
inline int pairwise_count(const Profile& profile, int a, int b) {
  int count = 0;
  for (const auto& r : profile.rankings) {
    int pos_a = -1, pos_b = -1;
    for (int pos = 0; pos < profile.n_alts; ++pos) {
      if (r[pos] == a) pos_a = pos;
      if (r[pos] == b) pos_b = pos;
    }
    if (pos_a < pos_b) ++count;
  }
  return count;
}

/// True iff strict majority of voters prefer a to b.
inline bool majority_prefers_ordinal(const Profile& profile, int a, int b) {
  return pairwise_count(profile, a, b) > profile.n_voters / 2;
}

}  // namespace detail

/**
 * @brief Check whether the majority relation derived from the profile has a
 *        Condorcet winner.
 *
 * A Condorcet winner beats every other alternative by strict majority.
 *
 * @param profile  Well-formed preference profile.
 * @return         true iff a Condorcet winner exists.
 */
[[nodiscard]] inline bool has_condorcet_winner_profile(const Profile& profile) {
  for (int a = 0; a < profile.n_alts; ++a) {
    bool beats_all = true;
    for (int b = 0; b < profile.n_alts && beats_all; ++b) {
      if (b != a && !detail::majority_prefers_ordinal(profile, a, b)) {
        beats_all = false;
      }
    }
    if (beats_all) return true;
  }
  return false;
}

/**
 * @brief Return the Condorcet winner, or nullopt if none exists.
 *
 * Category 2 rule: the result is either a unique winner or absence.
 * Tie-breaking does not apply — if no Condorcet winner exists, return nullopt.
 *
 * @param profile  Well-formed preference profile.
 * @return         Index of the Condorcet winner, or std::nullopt.
 */
[[nodiscard]] inline std::optional<int> condorcet_winner_profile(
    const Profile& profile) {
  for (int a = 0; a < profile.n_alts; ++a) {
    bool beats_all = true;
    for (int b = 0; b < profile.n_alts && beats_all; ++b) {
      if (b != a && !detail::majority_prefers_ordinal(profile, a, b)) {
        beats_all = false;
      }
    }
    if (beats_all) return a;
  }
  return std::nullopt;
}

/**
 * @brief Check whether a given winner is Condorcet consistent.
 *
 * Returns true in two cases:
 *   (a) No Condorcet winner exists (vacuously consistent: any winner is ok).
 *   (b) A Condorcet winner exists and winner == that Condorcet winner.
 *
 * @param profile  Well-formed preference profile.
 * @param winner   Index of the candidate to test (0-based).
 * @return         true iff winner is Condorcet consistent.
 */
[[nodiscard]] inline bool is_condorcet_consistent(const Profile& profile,
                                                  int winner) {
  const auto cw = condorcet_winner_profile(profile);
  if (!cw.has_value()) return true;  // no CW: vacuously consistent
  return *cw == winner;
}

/**
 * @brief Check whether a given alternative beats every other by strict
 *        majority.
 *
 * Majority consistency is a stronger condition than Condorcet consistency:
 * it requires the winner to be the Condorcet winner AND for one to exist.
 * If no Condorcet winner exists this returns false for every candidate.
 *
 * @param profile  Well-formed preference profile.
 * @param winner   Index of the candidate to test (0-based).
 * @return         true iff winner beats every other alternative by strict
 *                 majority.
 */
[[nodiscard]] inline bool is_majority_consistent(const Profile& profile,
                                                 int winner) {
  for (int b = 0; b < profile.n_alts; ++b) {
    if (b == winner) continue;
    if (!detail::majority_prefers_ordinal(profile, winner, b)) return false;
  }
  return true;
}

}  // namespace socialchoicelab::aggregation
