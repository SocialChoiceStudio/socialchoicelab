// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// pareto.h — Ordinal Pareto efficiency checks (W2).
//
// Alternative a Pareto-dominates alternative b if:
//   (1) Every voter weakly prefers a to b (rank of a ≤ rank of b).
//   (2) At least one voter strictly prefers a (rank of a < rank of b).
//
// The Pareto set is the set of alternatives not Pareto-dominated by any other.
// It is always non-empty.
//
// References:
//   Pareto, V. (1906). Manuale di economia politica.
//   Arrow, K. J. (1951). Social Choice and Individual Values.

#include <socialchoicelab/aggregation/profile.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace socialchoicelab::aggregation {

namespace detail {

/// Return the rank (position in ranking, 0 = best) of alternative alt
/// for voter whose ranking is r.  O(m) scan.
inline int rank_of(const std::vector<int>& r, int alt) {
  for (int pos = 0; pos < static_cast<int>(r.size()); ++pos) {
    if (r[pos] == alt) return pos;
  }
  throw std::invalid_argument(
      "rank_of: alternative " + std::to_string(alt) +
      " not found in ranking. Check that the profile is well-formed.");
}

}  // namespace detail

/**
 * @brief Check whether alternative a Pareto-dominates alternative b.
 *
 * a Pareto-dominates b iff every voter weakly prefers a (rank_a ≤ rank_b)
 * and at least one voter strictly prefers a (rank_a < rank_b).
 *
 * @param profile  Well-formed preference profile.
 * @param a        Index of the potentially dominant alternative (0-based).
 * @param b        Index of the potentially dominated alternative (0-based).
 * @return         true iff a Pareto-dominates b.
 * @throws std::invalid_argument if a or b is out of [0, n_alts).
 */
[[nodiscard]] inline bool pareto_dominates(const Profile& profile, int a,
                                           int b) {
  if (a < 0 || a >= profile.n_alts) {
    throw std::invalid_argument(
        "pareto_dominates: alternative a=" + std::to_string(a) +
        " is out of range [0, " + std::to_string(profile.n_alts) + ").");
  }
  if (b < 0 || b >= profile.n_alts) {
    throw std::invalid_argument(
        "pareto_dominates: alternative b=" + std::to_string(b) +
        " is out of range [0, " + std::to_string(profile.n_alts) + ").");
  }
  if (a == b) return false;

  bool some_strict = false;
  for (const auto& r : profile.rankings) {
    const int pos_a = detail::rank_of(r, a);
    const int pos_b = detail::rank_of(r, b);
    if (pos_a > pos_b) return false;  // voter strictly prefers b
    if (pos_a < pos_b) some_strict = true;
  }
  return some_strict;
}

/**
 * @brief Return the Pareto set: all alternatives not Pareto-dominated.
 *
 * The Pareto set is always non-empty.  Results are sorted by ascending index.
 *
 * @param profile  Well-formed preference profile.
 * @return         Indices of Pareto-efficient alternatives.
 */
[[nodiscard]] inline std::vector<int> pareto_set(const Profile& profile) {
  std::vector<int> efficient;
  for (int a = 0; a < profile.n_alts; ++a) {
    bool dominated = false;
    for (int b = 0; b < profile.n_alts && !dominated; ++b) {
      if (b != a && pareto_dominates(profile, b, a)) dominated = true;
    }
    if (!dominated) efficient.push_back(a);
  }
  return efficient;
}

/**
 * @brief Check whether a given alternative is Pareto-efficient.
 *
 * @param profile  Well-formed preference profile.
 * @param winner   Index of the alternative to check (0-based).
 * @return         true iff winner is in the Pareto set.
 * @throws std::invalid_argument if winner is out of [0, n_alts).
 */
[[nodiscard]] inline bool is_pareto_efficient(const Profile& profile,
                                              int winner) {
  if (winner < 0 || winner >= profile.n_alts) {
    throw std::invalid_argument(
        "is_pareto_efficient: winner=" + std::to_string(winner) +
        " is out of range [0, " + std::to_string(profile.n_alts) +
        "). Enter a value between 0 and " + std::to_string(profile.n_alts - 1) +
        ".");
  }
  const auto ps = pareto_set(profile);
  return std::find(ps.begin(), ps.end(), winner) != ps.end();
}

}  // namespace socialchoicelab::aggregation
