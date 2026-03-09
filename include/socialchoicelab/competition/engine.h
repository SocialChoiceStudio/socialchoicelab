// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <socialchoicelab/competition/election_feedback.h>
#include <socialchoicelab/competition/motion.h>
#include <socialchoicelab/competition/strategy_registry.h>
#include <socialchoicelab/competition/termination.h>

#include <deque>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace socialchoicelab::competition {

struct CompetitionRoundRecord {
  int round_index = 0;
  std::vector<CompetitorState> evaluated_competitors;
  ElectionFeedback feedback;
};

struct CompetitionTrace {
  std::vector<CompetitionRoundRecord> rounds;
  std::vector<CompetitorState> final_competitors;
  TerminationReason termination_reason = TerminationReason::kMaxRounds;
  bool terminated_early = false;
};

struct CompetitionEngineConfig {
  CompetitionBounds bounds;
  ElectionFeedbackConfig election_feedback;
  StepPolicyConfig step_policy;
  BoundaryPolicyKind boundary_policy = BoundaryPolicyKind::kProjectToBox;
  CompetitionObjectiveKind objective_kind =
      CompetitionObjectiveKind::kVoteShare;
  int max_rounds = 1;
  TerminationConfig termination;
};

struct CompetitionRoundStepResult {
  CompetitionRoundRecord record;
  std::vector<CompetitorState> next_competitors;
};

namespace detail {

[[nodiscard]] inline std::vector<PointNd> competitor_positions(
    const std::vector<CompetitorState>& competitors) {
  std::vector<PointNd> positions;
  positions.reserve(competitors.size());
  for (const auto& competitor : competitors) {
    positions.push_back(competitor.position);
  }
  return positions;
}

inline void validate_engine_config(const CompetitionEngineConfig& config) {
  validate_bounds(config.bounds);
  validate_step_policy(config.step_policy);
  validate_termination_config(config.termination);
  if (config.max_rounds < 0) {
    throw std::invalid_argument(
        "validate_engine_config: max_rounds must be non-negative.");
  }
}

inline void validate_engine_inputs(
    const CompetitionEngineConfig& config,
    const std::vector<CompetitorState>& competitors,
    const std::vector<PointNd>& voter_ideals) {
  validate_engine_config(config);
  if (competitors.empty()) {
    throw std::invalid_argument(
        "validate_engine_inputs: competitors must not be empty.");
  }
  if (voter_ideals.empty()) {
    throw std::invalid_argument(
        "validate_engine_inputs: voter_ideals must not be empty.");
  }

  const int dimension = competition_dimension(config.bounds);
  if (static_cast<int>(
          config.election_feedback.distance_config.salience_weights.size()) !=
      dimension) {
    throw std::invalid_argument(
        "validate_engine_inputs: distance-config salience weights must match "
        "competition dimension.");
  }

  for (const auto& competitor : competitors) {
    if (competitor.position.size() != dimension ||
        competitor.heading.size() != dimension) {
      throw std::invalid_argument(
          "validate_engine_inputs: competitor state dimension mismatch.");
    }
  }
  for (const auto& voter : voter_ideals) {
    if (voter.size() != dimension) {
      throw std::invalid_argument(
          "validate_engine_inputs: voter ideal dimension mismatch.");
    }
  }
}

}  // namespace detail

[[nodiscard]] inline CompetitionRoundStepResult run_competition_round(
    const CompetitionEngineConfig& config,
    const std::vector<CompetitorState>& competitors,
    const std::vector<PointNd>& voter_ideals,
    core::rng::StreamManager* stream_manager,
    const StrategyRegistry& registry = StrategyRegistry::built_in(),
    int round_index = 0) {
  detail::validate_engine_inputs(config, competitors, voter_ideals);

  const auto feedback =
      evaluate_election_feedback(detail::competitor_positions(competitors),
                                 voter_ideals, config.election_feedback);
  const auto evaluated_competitors =
      apply_feedback_to_competitors(competitors, feedback);

  AdaptationContext context{config.bounds, config.objective_kind,
                            evaluated_competitors,
                            feedback.supporter_ideal_points, stream_manager};

  std::vector<CompetitorState> next_competitors;
  next_competitors.reserve(evaluated_competitors.size());

  for (const auto& competitor : evaluated_competitors) {
    const auto* strategy = registry.find(competitor.strategy_kind);
    if (strategy == nullptr) {
      throw std::invalid_argument(
          "run_competition_round: no registered strategy for competitor kind.");
    }
    const auto decision = strategy->adapt(competitor, context);
    next_competitors.push_back(advance_competitor_state(
        competitor, decision, config.bounds, config.step_policy,
        config.boundary_policy, config.objective_kind, stream_manager));
  }

  CompetitionRoundStepResult result;
  result.record.round_index = round_index;
  result.record.evaluated_competitors = evaluated_competitors;
  result.record.feedback = feedback;
  result.next_competitors = std::move(next_competitors);
  return result;
}

[[nodiscard]] inline CompetitionTrace run_fixed_round_competition(
    const CompetitionEngineConfig& config,
    const std::vector<CompetitorState>& initial_competitors,
    const std::vector<PointNd>& voter_ideals,
    core::rng::StreamManager* stream_manager,
    const StrategyRegistry& registry = StrategyRegistry::built_in()) {
  detail::validate_engine_inputs(config, initial_competitors, voter_ideals);

  CompetitionTrace trace;
  std::vector<CompetitorState> competitors = initial_competitors;
  std::deque<std::string> recent_signatures;
  if (config.termination.stop_on_cycle && config.termination.cycle_memory > 0) {
    recent_signatures.push_back(
        state_signature(competitors, config.termination.signature_resolution));
  }
  int no_improvement_streak = 0;

  for (int round = 0; round < config.max_rounds; ++round) {
    auto step = run_competition_round(config, competitors, voter_ideals,
                                      stream_manager, registry, round);
    trace.rounds.push_back(std::move(step.record));

    if (config.termination.stop_on_convergence &&
        has_converged_by_position(trace.rounds.back().evaluated_competitors,
                                  step.next_competitors,
                                  config.termination.position_epsilon)) {
      trace.termination_reason = TerminationReason::kConverged;
      trace.terminated_early = true;
      competitors = std::move(step.next_competitors);
      break;
    }

    if (config.termination.stop_on_no_improvement &&
        config.termination.no_improvement_window > 0) {
      if (round_has_any_improvement(trace.rounds.back().evaluated_competitors,
                                    config.objective_kind,
                                    config.termination.objective_epsilon)) {
        no_improvement_streak = 0;
      } else {
        ++no_improvement_streak;
      }
      if (no_improvement_streak >= config.termination.no_improvement_window) {
        trace.termination_reason = TerminationReason::kNoImprovementWindow;
        trace.terminated_early = true;
        competitors = std::move(step.next_competitors);
        break;
      }
    }

    if (config.termination.stop_on_cycle &&
        config.termination.cycle_memory > 0) {
      const std::string signature = state_signature(
          step.next_competitors, config.termination.signature_resolution);
      if (has_cycle_signature(recent_signatures, signature)) {
        trace.termination_reason = TerminationReason::kCycleDetected;
        trace.terminated_early = true;
        competitors = std::move(step.next_competitors);
        break;
      }
      recent_signatures.push_back(signature);
      while (static_cast<int>(recent_signatures.size()) >
             config.termination.cycle_memory) {
        recent_signatures.pop_front();
      }
    }

    competitors = std::move(step.next_competitors);
  }

  trace.final_competitors = std::move(competitors);
  return trace;
}

}  // namespace socialchoicelab::competition
