// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <socialchoicelab/competition/boundary_policy.h>
#include <socialchoicelab/competition/step_policy.h>

namespace socialchoicelab::competition {

struct MotionResult {
  PointNd next_position;
  PointNd applied_heading;
  double applied_step_size = 0.0;
};

[[nodiscard]] inline MotionResult advance_competitor(
    const CompetitorState& competitor, const StrategyDecision& decision,
    const CompetitionBounds& bounds, const StepPolicyConfig& step_policy,
    BoundaryPolicyKind boundary_policy, CompetitionObjectiveKind objective_kind,
    core::rng::StreamManager* stream_manager) {
  validate_bounds(bounds);
  const int dimension = competition_dimension(bounds);
  detail::validate_point_dimension(competitor.position, dimension,
                                   "advance_competitor");
  detail::validate_point_dimension(decision.next_heading, dimension,
                                   "advance_competitor");

  MotionResult result;
  result.applied_heading = detail::normalize_or_zero(decision.next_heading);
  result.applied_step_size = resolve_step_size(competitor, step_policy,
                                               objective_kind, stream_manager);

  if (result.applied_heading.norm() == 0.0 || result.applied_step_size == 0.0) {
    result.next_position = competitor.position;
    return result;
  }

  const PointNd proposed_position =
      competitor.position + result.applied_heading * result.applied_step_size;
  result.next_position = apply_boundary_policy(
      competitor.position, proposed_position, bounds, boundary_policy);
  return result;
}

[[nodiscard]] inline CompetitorState advance_competitor_state(
    const CompetitorState& competitor, const StrategyDecision& decision,
    const CompetitionBounds& bounds, const StepPolicyConfig& step_policy,
    BoundaryPolicyKind boundary_policy, CompetitionObjectiveKind objective_kind,
    core::rng::StreamManager* stream_manager) {
  const auto motion =
      advance_competitor(competitor, decision, bounds, step_policy,
                         boundary_policy, objective_kind, stream_manager);
  CompetitorState updated = competitor;
  updated.position = motion.next_position;
  updated.heading = motion.applied_heading;
  updated.current_step_size = motion.applied_step_size;
  return updated;
}

}  // namespace socialchoicelab::competition
