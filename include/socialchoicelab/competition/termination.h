// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <socialchoicelab/competition/adaptation.h>
#include <socialchoicelab/competition/state_signature.h>

#include <algorithm>
#include <deque>
#include <string>
#include <vector>

namespace socialchoicelab::competition {

enum class TerminationReason {
  kMaxRounds,
  kConverged,
  kCycleDetected,
  kNoImprovementWindow,
};

struct TerminationConfig {
  bool stop_on_convergence = false;
  double position_epsilon = 1e-9;

  bool stop_on_cycle = false;
  int cycle_memory = 8;
  double signature_resolution = 1e-6;

  bool stop_on_no_improvement = false;
  int no_improvement_window = 3;
  double objective_epsilon = 1e-12;
};

inline void validate_termination_config(const TerminationConfig& config) {
  if (config.position_epsilon < 0.0) {
    throw std::invalid_argument(
        "validate_termination_config: position_epsilon must be non-negative.");
  }
  if (config.cycle_memory < 0) {
    throw std::invalid_argument(
        "validate_termination_config: cycle_memory must be non-negative.");
  }
  if (config.signature_resolution <= 0.0) {
    throw std::invalid_argument(
        "validate_termination_config: signature_resolution must be positive.");
  }
  if (config.no_improvement_window < 0) {
    throw std::invalid_argument(
        "validate_termination_config: no_improvement_window must be "
        "non-negative.");
  }
  if (config.objective_epsilon < 0.0) {
    throw std::invalid_argument(
        "validate_termination_config: objective_epsilon must be "
        "non-negative.");
  }
}

[[nodiscard]] inline double max_position_delta(
    const std::vector<CompetitorState>& before,
    const std::vector<CompetitorState>& after) {
  if (before.size() != after.size()) {
    throw std::invalid_argument(
        "max_position_delta: before and after must have the same size.");
  }

  double max_delta = 0.0;
  for (size_t i = 0; i < before.size(); ++i) {
    if (before[i].position.size() != after[i].position.size()) {
      throw std::invalid_argument(
          "max_position_delta: position dimension mismatch.");
    }
    max_delta =
        std::max(max_delta, (after[i].position - before[i].position).norm());
  }
  return max_delta;
}

[[nodiscard]] inline bool has_converged_by_position(
    const std::vector<CompetitorState>& before,
    const std::vector<CompetitorState>& after, double epsilon) {
  return max_position_delta(before, after) <= epsilon;
}

[[nodiscard]] inline bool has_cycle_signature(
    const std::deque<std::string>& recent_signatures,
    const std::string& candidate_signature) {
  return std::find(recent_signatures.begin(), recent_signatures.end(),
                   candidate_signature) != recent_signatures.end();
}

[[nodiscard]] inline bool round_has_any_improvement(
    const std::vector<CompetitorState>& evaluated_competitors,
    CompetitionObjectiveKind objective_kind, double epsilon) {
  for (const auto& competitor : evaluated_competitors) {
    if (!competitor.previous_round_metrics.has_value() ||
        !competitor.current_round_metrics.has_value()) {
      return true;
    }
    const double previous_value = detail::objective_value(
        *competitor.previous_round_metrics, objective_kind);
    const double current_value = detail::objective_value(
        *competitor.current_round_metrics, objective_kind);
    if (current_value > previous_value + epsilon) {
      return true;
    }
  }
  return false;
}

}  // namespace socialchoicelab::competition
