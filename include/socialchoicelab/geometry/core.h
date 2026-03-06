// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// core.h — Condorcet winner / core detection (F2).
//
// Provides:
//   has_condorcet_winner(alternatives, voter_ideals, cfg, k) → bool
//   condorcet_winner(alternatives, voter_ideals, cfg, k) → optional<Point2d>
//   core_2d(voter_ideals, cfg, k, num_samples) → optional<Point2d>
//
// The "core" of a voting game is the set of undominated outcomes.  For
// spatial majority-rule voting in continuous 2D policy space, the core is
// either empty (the generic case, by McKelvey's chaos theorem) or the single
// Condorcet winner — the point x* such that no other point is preferred by
// a k-majority.
//
// Under simple majority in 2D, a Condorcet winner requires Plott's (1967)
// radial symmetry conditions (rare; zero measure in the space of voter
// configurations).  Under unanimity (k = n) a Condorcet winner always exists
// at the voter Pareto set.
//
// References:
//   Plott (1967), "A Notion of Equilibrium and Its Possibility Under Majority
//     Rule," American Economic Review 57(4), 787–806.
//   McKelvey (1979), "General Conditions for Global Intransitivities in Formal
//     Voting Models," Econometrica 47(5), 1085–1112.
//   Schofield (1983), "Generic Instability of Majority Rule," Review of
//     Economic Studies 50(4), 695–705.

#include <socialchoicelab/geometry/majority.h>
#include <socialchoicelab/geometry/winset.h>
#include <socialchoicelab/geometry/yolk.h>

#include <optional>
#include <vector>

namespace socialchoicelab::geometry {

// ===========================================================================
// F2 — Core detection (finite alternative set)
// ===========================================================================

/**
 * @brief Returns true if any alternative k-majority-beats all others.
 *
 * An alternative a is a k-majority Condorcet winner iff for every other
 * alternative b, at least k voters strictly prefer a to b.
 *
 * @param alternatives  Finite set of alternatives (at least 1).
 * @param voter_ideals  Voter ideal points.
 * @param cfg           Distance configuration.
 * @param k             Majority threshold; -1 = ⌊n/2⌋+1.
 * @return true iff a Condorcet winner exists among the alternatives.
 * @throws std::invalid_argument if voter_ideals is empty or k ∉ [1, n].
 */
[[nodiscard]] inline bool has_condorcet_winner(
    const std::vector<core::types::Point2d>& alternatives,
    const std::vector<core::types::Point2d>& voter_ideals,
    const DistConfig& cfg = {}, int k = -1) {
  if (alternatives.empty()) return false;
  const int m = static_cast<int>(alternatives.size());
  for (int i = 0; i < m; ++i) {
    bool beats_all = true;
    for (int j = 0; j < m && beats_all; ++j) {
      if (i == j) continue;
      if (!majority_prefers(alternatives[static_cast<std::size_t>(i)],
                            alternatives[static_cast<std::size_t>(j)],
                            voter_ideals, cfg, k)) {
        beats_all = false;
      }
    }
    if (beats_all) return true;
  }
  return false;
}

/**
 * @brief Returns the k-majority Condorcet winner among a finite alternative
 *        set, or std::nullopt if none exists.
 *
 * A Condorcet winner is the unique alternative (if it exists) that beats
 * every other alternative under k-majority.  When multiple alternatives are
 * tied (can happen only when no alternative beats all others), returns
 * std::nullopt.
 *
 * @param alternatives  Finite set of alternatives.
 * @param voter_ideals  Voter ideal points.
 * @param cfg           Distance configuration.
 * @param k             Majority threshold; -1 = ⌊n/2⌋+1.
 * @return              The Condorcet winner, or std::nullopt.
 * @throws std::invalid_argument if voter_ideals is empty or k ∉ [1, n].
 */
[[nodiscard]] inline std::optional<core::types::Point2d> condorcet_winner(
    const std::vector<core::types::Point2d>& alternatives,
    const std::vector<core::types::Point2d>& voter_ideals,
    const DistConfig& cfg = {}, int k = -1) {
  if (alternatives.empty()) return std::nullopt;
  const int m = static_cast<int>(alternatives.size());
  for (int i = 0; i < m; ++i) {
    bool beats_all = true;
    for (int j = 0; j < m && beats_all; ++j) {
      if (i == j) continue;
      if (!majority_prefers(alternatives[static_cast<std::size_t>(i)],
                            alternatives[static_cast<std::size_t>(j)],
                            voter_ideals, cfg, k)) {
        beats_all = false;
      }
    }
    if (beats_all) return alternatives[static_cast<std::size_t>(i)];
  }
  return std::nullopt;
}

// ===========================================================================
// F2 — Core detection (continuous 2D policy space)
// ===========================================================================

/**
 * @brief Searches for the k-majority Condorcet winner in continuous 2D policy
 *        space, returning it if found or std::nullopt otherwise.
 *
 * Strategy: the Yolk centre c* (which minimises the maximum distance to any
 * k-quantile line) is the best candidate for a Condorcet winner.  If the
 * winset W(c*) is empty — i.e. no point beats c* — then c* IS the Condorcet
 * winner.  The voter ideal points are also checked as secondary candidates
 * (they are the Condorcet winner in the median-voter case on a 1D issue
 * dimension embedded in 2D).
 *
 * Notes:
 * - Under simple majority in 2D, a Condorcet winner requires Plott's (1967)
 *   radial symmetry conditions (measure-zero in the space of configurations).
 *   core_2d will return std::nullopt for almost all configurations.
 * - Under unanimity (k = n), the Condorcet winner always exists.  For
 *   Euclidean preferences, every point in the interior of the voter convex
 *   hull loses to at least one other point under unanimity; the only
 *   Condorcet winner is the voter ideal point itself when all voters have the
 *   same ideal.  This function detects these cases.
 * - Accuracy is limited by num_samples (winset polygon approximation) and
 *   the Yolk solver's numerical precision.
 *
 * @param voter_ideals  At least 2 voter ideal points.
 * @param cfg           Distance configuration.
 * @param k             Majority threshold; -1 = ⌊n/2⌋+1.
 * @param num_samples   Winset polygon approximation quality (default 64).
 * @return              Approximate Condorcet winner point, or std::nullopt.
 * @throws std::invalid_argument if voter_ideals has < 2 points.
 */
[[nodiscard]] inline std::optional<core::types::Point2d> core_2d(
    const std::vector<core::types::Point2d>& voter_ideals,
    const DistConfig& cfg = {}, int k = -1, int num_samples = 64) {
  if (voter_ideals.size() < 2) {
    throw std::invalid_argument("core_2d: need at least 2 voter ideals (got " +
                                std::to_string(voter_ideals.size()) + ").");
  }

  // Candidates: Yolk centre + all voter ideal points.
  std::vector<core::types::Point2d> candidates;
  candidates.reserve(voter_ideals.size() + 1);
  candidates.push_back(yolk_2d(voter_ideals, k).center);
  for (const auto& v : voter_ideals) candidates.push_back(v);

  for (const auto& c : candidates) {
    if (winset_is_empty(winset_2d(c, voter_ideals, cfg, k, num_samples))) {
      return c;
    }
  }
  return std::nullopt;
}

}  // namespace socialchoicelab::geometry
