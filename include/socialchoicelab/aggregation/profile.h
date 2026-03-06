// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// profile.h — Preference profile type and construction functions.
//
// A preference profile P = (≻₁, …, ≻ₙ) is a tuple of n complete, strict
// preference orderings over m alternatives.  In the C++ core:
//
//   rankings[i]    — voter i's ranking (most preferred first).
//   rankings[i][0] — voter i's first choice (index of most preferred alt).
//   rankings[i][m-1] — voter i's last choice (index of least preferred alt).
//
// All indices are 0-based (alternatives 0 … m-1, voters 0 … n-1).
// R bindings must increment by 1 at the binding boundary.

#include <socialchoicelab/geometry/majority.h>

#include <Eigen/Dense>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace socialchoicelab::aggregation {

using core::types::Point2d;
using geometry::DistConfig;

// ---------------------------------------------------------------------------
// Profile type
// ---------------------------------------------------------------------------

/**
 * @brief Ordinal preference profile: n voter rankings over m alternatives.
 *
 * rankings[i] is a permutation of 0 … n_alts-1, ordered most-preferred
 * first.  Construct with build_spatial_profile(),
 * profile_from_utility_matrix(), or one of the generators in
 * profile_generators.h.
 */
struct Profile {
  int n_voters = 0;  ///< Number of voters.
  int n_alts = 0;    ///< Number of alternatives.
  /// rankings[voter][rank] = alternative index at that rank (0 = best).
  std::vector<std::vector<int>> rankings;
};

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

/**
 * @brief Check that a profile is structurally valid.
 *
 * A well-formed profile satisfies:
 *   - rankings.size() == n_voters
 *   - each rankings[i].size() == n_alts
 *   - each rankings[i] is a permutation of 0 … n_alts-1
 *
 * @return true iff the profile is well-formed.
 */
[[nodiscard]] inline bool is_well_formed(const Profile& p) {
  if (static_cast<int>(p.rankings.size()) != p.n_voters) return false;
  for (const auto& r : p.rankings) {
    if (static_cast<int>(r.size()) != p.n_alts) return false;
    std::vector<bool> seen(p.n_alts, false);
    for (int idx : r) {
      if (idx < 0 || idx >= p.n_alts || seen[idx]) return false;
      seen[idx] = true;
    }
  }
  return true;
}

// ---------------------------------------------------------------------------
// Construction from spatial model
// ---------------------------------------------------------------------------

/**
 * @brief Build a profile from voter ideal points and alternatives.
 *
 * For each voter, alternatives are ranked by ascending Lp-distance (from
 * DistConfig).  Ties at exactly equal distance are broken by ascending
 * alternative index (deterministic).
 *
 * @param alternatives  m alternative positions.
 * @param voter_ideals  n voter ideal points.
 * @param cfg           Distance metric configuration (default: Euclidean).
 * @return              Well-formed profile with n_voters=n, n_alts=m.
 * @throws std::invalid_argument if voter_ideals is empty.
 */
[[nodiscard]] inline Profile build_spatial_profile(
    const std::vector<Point2d>& alternatives,
    const std::vector<Point2d>& voter_ideals, const DistConfig& cfg = {}) {
  const int n = static_cast<int>(voter_ideals.size());
  const int m = static_cast<int>(alternatives.size());

  if (n == 0) {
    throw std::invalid_argument(
        "build_spatial_profile: voter_ideals must not be empty.");
  }

  Profile profile;
  profile.n_voters = n;
  profile.n_alts = m;
  profile.rankings.resize(n);

  for (int i = 0; i < n; ++i) {
    // Pairs of (distance, alt_index): lexicographic sort gives ascending
    // distance with tie-breaking by ascending index.
    std::vector<std::pair<double, int>> dists;
    dists.reserve(m);
    for (int j = 0; j < m; ++j) {
      dists.emplace_back(geometry::detail::voter_distance(voter_ideals[i],
                                                          alternatives[j], cfg),
                         j);
    }
    std::sort(dists.begin(), dists.end());

    profile.rankings[i].resize(m);
    for (int r = 0; r < m; ++r) profile.rankings[i][r] = dists[r].second;
  }
  return profile;
}

// ---------------------------------------------------------------------------
// Construction from utility matrix
// ---------------------------------------------------------------------------

/**
 * @brief Build a profile from a raw utility matrix.
 *
 * utilities(i, j) is voter i's utility for alternative j (higher = better).
 * Voter i's ranking is determined by sorting columns of row i in descending
 * utility order.  Ties in utility are broken by ascending alternative index.
 *
 * @param utilities  Matrix of shape (n_voters × n_alts).
 * @return           Well-formed profile.
 * @throws std::invalid_argument if utilities has 0 rows.
 */
[[nodiscard]] inline Profile profile_from_utility_matrix(
    const Eigen::MatrixXd& utilities) {
  const int n = static_cast<int>(utilities.rows());
  const int m = static_cast<int>(utilities.cols());

  if (n == 0) {
    throw std::invalid_argument(
        "profile_from_utility_matrix: utilities matrix must have at least "
        "one row (voter).");
  }

  Profile profile;
  profile.n_voters = n;
  profile.n_alts = m;
  profile.rankings.resize(n);

  for (int i = 0; i < n; ++i) {
    std::vector<int> order(m);
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int a, int b) {
      if (utilities(i, a) != utilities(i, b))
        return utilities(i, a) > utilities(i, b);  // descending utility
      return a < b;                                // ascending index on ties
    });
    profile.rankings[i] = std::move(order);
  }
  return profile;
}

}  // namespace socialchoicelab::aggregation
