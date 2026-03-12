// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <socialchoicelab/competition/competitor.h>
#include <socialchoicelab/competition/initialization.h>
#include <socialchoicelab/core/rng/stream_manager.h>

#include <stdexcept>
#include <vector>

namespace socialchoicelab::competition {

constexpr char kHunterAdaptationStreamName[] = "competition_adaptation_hunter";

enum class CompetitionObjectiveKind {
  kVoteShare,
  kSeatShare,
};

struct StrategyDecision {
  PointNd next_heading;
};

struct AdaptationContext {
  const CompetitionBounds& bounds;
  CompetitionObjectiveKind objective_kind;
  const std::vector<CompetitorState>& competitors;
  const std::vector<std::vector<PointNd>>& supporter_ideal_points;
  core::rng::StreamManager* stream_manager = nullptr;
};

namespace detail {

[[nodiscard]] inline double objective_value(
    const CompetitorRoundMetrics& metrics, CompetitionObjectiveKind objective) {
  switch (objective) {
    case CompetitionObjectiveKind::kVoteShare:
      return metrics.vote_share;
    case CompetitionObjectiveKind::kSeatShare:
      return metrics.seat_share;
  }
  return 0.0;
}

inline void validate_adaptation_context(const AdaptationContext& context) {
  validate_bounds(context.bounds);
  if (context.competitors.empty()) {
    throw std::invalid_argument(
        "validate_adaptation_context: competitors must not be empty.");
  }
  if (context.supporter_ideal_points.size() != context.competitors.size()) {
    throw std::invalid_argument(
        "validate_adaptation_context: supporter_ideal_points size must match "
        "competitors size.");
  }

  const int dimension = competition_dimension(context.bounds);
  for (size_t i = 0; i < context.competitors.size(); ++i) {
    if (context.competitors[i].position.size() != dimension ||
        context.competitors[i].heading.size() != dimension) {
      throw std::invalid_argument(
          "validate_adaptation_context: competitor state dimension mismatch.");
    }
    for (const auto& supporter : context.supporter_ideal_points[i]) {
      if (supporter.size() != dimension) {
        throw std::invalid_argument(
            "validate_adaptation_context: supporter dimension mismatch.");
      }
    }
  }
}

[[nodiscard]] inline int find_competitor_index(const AdaptationContext& context,
                                               int competitor_id) {
  for (size_t i = 0; i < context.competitors.size(); ++i) {
    if (context.competitors[i].id == competitor_id) {
      return static_cast<int>(i);
    }
  }
  throw std::invalid_argument(
      "find_competitor_index: competitor id not present in adaptation "
      "context.");
}

[[nodiscard]] inline int leader_index(const AdaptationContext& context) {
  double best_value = 0.0;
  bool initialized = false;
  int best_index = -1;

  for (size_t i = 0; i < context.competitors.size(); ++i) {
    const auto& competitor = context.competitors[i];
    if (!competitor.current_round_metrics.has_value()) {
      throw std::invalid_argument(
          "leader_index: every competitor must have current_round_metrics.");
    }
    const double value = objective_value(*competitor.current_round_metrics,
                                         context.objective_kind);
    if (!initialized || value > best_value) {
      best_value = value;
      best_index = static_cast<int>(i);
      initialized = true;
    }
  }

  return best_index;
}

[[nodiscard]] inline PointNd supporter_mean(
    const std::vector<PointNd>& supporters, int dimension) {
  PointNd mean = PointNd::Zero(dimension);
  if (supporters.empty()) return mean;
  for (const auto& supporter : supporters) {
    mean += supporter;
  }
  return mean / static_cast<double>(supporters.size());
}

// Sample a unit vector uniformly from the backward half-space of
// `forward_heading` (all directions d such that d · forward_heading <= 0).
//
// In 1D the backward half-space is a single point — the negation — so no
// randomness is required. In dimensions >= 2 the Fowler-Laver Hunter
// definition specifies a uniform draw from the backward hemisphere: sample a
// random unit vector and reflect it into the backward half-space when needed.
[[nodiscard]] inline PointNd random_in_backward_halfspace(
    int dimension, const PointNd& forward_heading, core::rng::PRNG& prng) {
  if (dimension == 1) {
    return -forward_heading;
  }
  PointNd v = random_unit_heading(dimension, prng);
  if (v.dot(forward_heading) > 0.0) {
    v = -v;
  }
  return v;
}

}  // namespace detail

}  // namespace socialchoicelab::competition
