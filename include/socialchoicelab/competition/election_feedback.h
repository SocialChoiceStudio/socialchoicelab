// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <socialchoicelab/aggregation/plurality.h>
#include <socialchoicelab/aggregation/profile.h>
#include <socialchoicelab/aggregation/seat_allocation.h>
#include <socialchoicelab/competition/adaptation.h>
#include <socialchoicelab/preference/distance/distance_functions.h>

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

namespace socialchoicelab::competition {

enum class SeatRuleKind {
  kPluralityTopK,
  kHareLargestRemainder,
};

struct ElectionFeedbackConfig {
  geometry::DistConfig distance_config;
  int seat_count = 1;
  SeatRuleKind seat_rule = SeatRuleKind::kPluralityTopK;
};

struct ElectionFeedback {
  aggregation::Profile profile;
  std::vector<int> vote_totals;
  std::vector<double> vote_shares;
  std::vector<int> seats;
  std::vector<double> seat_shares;
  std::vector<std::vector<int>> supporter_indices;
  std::vector<std::vector<PointNd>> supporter_ideal_points;
  std::vector<int> leader_ranking;
};

namespace detail {

[[nodiscard]] inline aggregation::Profile build_spatial_profile_nd(
    const std::vector<PointNd>& alternatives,
    const std::vector<PointNd>& voter_ideals, const geometry::DistConfig& cfg) {
  const int n = static_cast<int>(voter_ideals.size());
  const int m = static_cast<int>(alternatives.size());
  if (n == 0) {
    throw std::invalid_argument(
        "build_spatial_profile_nd: voter_ideals must not be empty.");
  }
  if (m == 0) {
    throw std::invalid_argument(
        "build_spatial_profile_nd: alternatives must not be empty.");
  }

  const int dimension = static_cast<int>(alternatives[0].size());
  if (static_cast<int>(cfg.salience_weights.size()) != dimension) {
    throw std::invalid_argument(
        "build_spatial_profile_nd: salience_weights size must match "
        "alternative dimension.");
  }

  aggregation::Profile profile;
  profile.n_voters = n;
  profile.n_alts = m;
  profile.rankings.resize(n);

  for (const auto& alternative : alternatives) {
    if (alternative.size() != dimension) {
      throw std::invalid_argument(
          "build_spatial_profile_nd: alternative dimension mismatch.");
    }
  }

  for (int i = 0; i < n; ++i) {
    if (voter_ideals[i].size() != dimension) {
      throw std::invalid_argument(
          "build_spatial_profile_nd: voter dimension mismatch.");
    }

    std::vector<std::pair<double, int>> dists;
    dists.reserve(m);
    for (int j = 0; j < m; ++j) {
      dists.emplace_back(preference::distance::calculate_distance(
                             voter_ideals[i], alternatives[j], cfg.type,
                             cfg.order_p, cfg.salience_weights),
                         j);
    }
    std::sort(dists.begin(), dists.end());

    profile.rankings[i].resize(m);
    for (int r = 0; r < m; ++r) {
      profile.rankings[i][r] = dists[r].second;
    }
  }

  return profile;
}

[[nodiscard]] inline std::vector<double> shares_from_totals(
    const std::vector<int>& totals, int denominator) {
  std::vector<double> shares(totals.size(), 0.0);
  if (denominator == 0) return shares;
  for (size_t i = 0; i < totals.size(); ++i) {
    shares[i] = static_cast<double>(totals[i]) / denominator;
  }
  return shares;
}

[[nodiscard]] inline std::vector<int> rank_by_totals(
    const std::vector<int>& totals) {
  std::vector<int> order(totals.size());
  std::iota(order.begin(), order.end(), 0);
  std::sort(order.begin(), order.end(), [&](int a, int b) {
    if (totals[a] != totals[b]) return totals[a] > totals[b];
    return a < b;
  });
  return order;
}

}  // namespace detail

[[nodiscard]] inline ElectionFeedback evaluate_election_feedback(
    const std::vector<PointNd>& competitor_positions,
    const std::vector<PointNd>& voter_ideals,
    const ElectionFeedbackConfig& config) {
  if (config.seat_count < 0) {
    throw std::invalid_argument(
        "evaluate_election_feedback: seat_count must be non-negative.");
  }

  ElectionFeedback feedback;
  feedback.profile = detail::build_spatial_profile_nd(
      competitor_positions, voter_ideals, config.distance_config);
  feedback.vote_totals = aggregation::plurality_scores(feedback.profile);
  feedback.vote_shares = detail::shares_from_totals(feedback.vote_totals,
                                                    feedback.profile.n_voters);

  switch (config.seat_rule) {
    case SeatRuleKind::kPluralityTopK:
      feedback.seats = aggregation::plurality_top_k_seat_allocation(
          feedback.vote_totals, config.seat_count);
      break;
    case SeatRuleKind::kHareLargestRemainder:
      feedback.seats = aggregation::hare_largest_remainder_allocation(
          feedback.vote_totals, config.seat_count);
      break;
  }

  feedback.seat_shares =
      detail::shares_from_totals(feedback.seats, config.seat_count);
  feedback.supporter_indices.assign(feedback.profile.n_alts, {});
  feedback.supporter_ideal_points.assign(feedback.profile.n_alts, {});
  for (int voter = 0; voter < feedback.profile.n_voters; ++voter) {
    const int supported = feedback.profile.rankings[voter][0];
    feedback.supporter_indices[supported].push_back(voter);
    feedback.supporter_ideal_points[supported].push_back(voter_ideals[voter]);
  }
  feedback.leader_ranking = detail::rank_by_totals(feedback.vote_totals);

  return feedback;
}

[[nodiscard]] inline std::vector<CompetitorState> apply_feedback_to_competitors(
    const std::vector<CompetitorState>& competitors,
    const ElectionFeedback& feedback) {
  if (competitors.size() != feedback.vote_totals.size()) {
    throw std::invalid_argument(
        "apply_feedback_to_competitors: competitor count must match "
        "feedback competitor count.");
  }

  std::vector<CompetitorState> updated = competitors;
  for (size_t i = 0; i < updated.size(); ++i) {
    updated[i].previous_round_metrics = updated[i].current_round_metrics;
    CompetitorRoundMetrics metrics;
    metrics.vote_total = feedback.vote_totals[i];
    metrics.vote_share = feedback.vote_shares[i];
    metrics.seats_won = feedback.seats[i];
    metrics.seat_share = feedback.seat_shares[i];
    updated[i].current_round_metrics = metrics;
  }
  return updated;
}

}  // namespace socialchoicelab::competition
