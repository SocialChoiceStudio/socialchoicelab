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

// Returns indices (into `competitors`) of all active competitors, preserving
// their original order. Active-flag semantics: an inactive competitor is fully
// excluded from the election (voters do not rank it as an alternative) and does
// not adapt or move. This models a candidate who has dropped out of the race.
//
// Design decision (2026-03-09): We chose "fully excluded" over "frozen on
// ballot" because the Laver/Fowler literature frames inactivity as party death
// — the party ceases to be an option for voters, not merely a non-campaigning
// one. This also keeps the election feedback clean: vote_totals and seat_shares
// correctly reflect the contest among live candidates only.
//
// Future extension: birth/death dynamics will flip `active` mid-run when a
// competitor falls below a survival threshold. The engine processes the flag
// every round, so the transition takes effect immediately in the next round
// without any additional bookkeeping.
[[nodiscard]] inline std::vector<int> active_competitor_indices(
    const std::vector<CompetitorState>& competitors) {
  std::vector<int> indices;
  indices.reserve(competitors.size());
  for (size_t i = 0; i < competitors.size(); ++i) {
    if (competitors[i].active) indices.push_back(static_cast<int>(i));
  }
  return indices;
}

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

  // At least one competitor must be active. An all-inactive field means no
  // alternatives exist for voters to rank, which is an invalid election.
  const auto active = active_competitor_indices(competitors);
  if (active.empty()) {
    throw std::invalid_argument(
        "validate_engine_inputs: at least one competitor must be active.");
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

// Scatters feedback from an active-only election back into a full-sized
// ElectionFeedback whose vectors are indexed by total competitor count.
// Inactive competitor slots receive zero vote totals, zero shares, zero seats,
// and empty supporter lists. The embedded profile retains the active-only
// alternative count, which is the correct record of the actual election.
//
// Why scatter into a full-sized struct rather than carrying only active-sized
// feedback? The C API functions (scs_competition_trace_round_vote_shares, etc.)
// return fixed-length buffers indexed by total competitor count. Storing
// full-sized feedback in the trace record lets those functions remain a simple
// memcpy without needing an additional active→total index mapping at read time.
[[nodiscard]] inline ElectionFeedback scatter_feedback(
    const ElectionFeedback& active_feedback,
    const std::vector<int>& active_indices, size_t total_count) {
  ElectionFeedback full;
  full.profile = active_feedback.profile;
  full.vote_totals.assign(total_count, 0);
  full.vote_shares.assign(total_count, 0.0);
  full.seats.assign(total_count, 0);
  full.seat_shares.assign(total_count, 0.0);
  full.supporter_indices.assign(total_count, {});
  full.supporter_ideal_points.assign(total_count, {});

  for (size_t k = 0; k < active_indices.size(); ++k) {
    const int i = active_indices[k];
    full.vote_totals[i] = active_feedback.vote_totals[k];
    full.vote_shares[i] = active_feedback.vote_shares[k];
    full.seats[i] = active_feedback.seats[k];
    full.seat_shares[i] = active_feedback.seat_shares[k];
    full.supporter_indices[i] = active_feedback.supporter_indices[k];
    full.supporter_ideal_points[i] = active_feedback.supporter_ideal_points[k];
  }

  // Rebuild the leader ranking over all competitors. Inactive competitors (zero
  // votes) will sort to the end, which is the correct interpretation: they are
  // not in contention.
  full.leader_ranking = rank_by_totals(full.vote_totals);
  return full;
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

  // --- Active/inactive partitioning ---
  //
  // Only active competitors participate in the election and may move.
  // Inactive competitors are fully excluded: they are not alternatives voters
  // rank, they receive zero metrics, and their position is frozen for the
  // round.
  //
  // We compute active indices once and reuse them throughout to keep the
  // three phases (election, adaptation context, motion) consistent.
  const std::vector<int> active_indices =
      detail::active_competitor_indices(competitors);

  // Build the position vector for the active-only election.
  std::vector<PointNd> active_positions;
  active_positions.reserve(active_indices.size());
  for (const int idx : active_indices) {
    active_positions.push_back(competitors[idx].position);
  }

  // --- Election phase (active competitors only) ---
  //
  // active_feedback is sized to active_count, not total competitor count.
  const auto active_feedback = evaluate_election_feedback(
      active_positions, voter_ideals, config.election_feedback);

  // Scatter active_feedback into a full-sized struct (indexed by total count)
  // so that the trace record and C API can always index by competitor id
  // without needing to know which competitors were active in each round.
  const ElectionFeedback feedback = detail::scatter_feedback(
      active_feedback, active_indices, competitors.size());

  // Apply the full-sized feedback to all competitors. Inactive competitors
  // receive zero current_round_metrics (correctly reflecting their exclusion).
  const auto evaluated_competitors =
      apply_feedback_to_competitors(competitors, feedback);

  // --- Adaptation context (active competitors only) ---
  //
  // The AdaptationContext is built from active competitors and their
  // supporter data. Strategies must only see and react to active competitors:
  //   - Hunter uses objective deltas of the calling competitor (active).
  //   - Aggregator computes the mean of its own supporters (active slot only).
  //   - Predator targets the leader among active competitors.
  //
  // We build active_evaluated by extracting post-feedback states for active
  // competitors. supporter_ideal_points comes from active_feedback (not the
  // scattered full version) because it is indexed in active-competitor order.
  std::vector<CompetitorState> active_evaluated;
  active_evaluated.reserve(active_indices.size());
  for (const int idx : active_indices) {
    active_evaluated.push_back(evaluated_competitors[idx]);
  }

  AdaptationContext context{
    config.bounds, config.objective_kind, active_evaluated,
    active_feedback.supporter_ideal_points, stream_manager};

  // --- Motion phase ---
  //
  // Start next_competitors as a copy of evaluated_competitors so that inactive
  // competitors carry their frozen positions and zero metrics forward without
  // any special-casing in the motion loop.
  std::vector<CompetitorState> next_competitors = evaluated_competitors;

  for (const int i : active_indices) {
    const auto& competitor = evaluated_competitors[i];
    const auto* strategy = registry.find(competitor.strategy_kind);
    if (strategy == nullptr) {
      throw std::invalid_argument(
          "run_competition_round: no registered strategy for competitor kind.");
    }
    const auto decision = strategy->adapt(competitor, context);
    next_competitors[i] = advance_competitor_state(
        competitor, decision, config.bounds, config.step_policy,
        config.boundary_policy, config.objective_kind, stream_manager);
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
