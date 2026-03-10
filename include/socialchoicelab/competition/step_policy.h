// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <socialchoicelab/competition/adaptation.h>

#include <cmath>
#include <random>

namespace socialchoicelab::competition {

constexpr char kMotionStepSizesStreamName[] = "competition_motion_step_sizes";

namespace detail {

[[nodiscard]] inline double objective_delta(
    const CompetitorState& competitor,
    CompetitionObjectiveKind objective_kind) {
  if (!competitor.previous_round_metrics.has_value() ||
      !competitor.current_round_metrics.has_value()) {
    return 0.0;
  }

  const double previous_value =
      objective_value(*competitor.previous_round_metrics, objective_kind);
  const double current_value =
      objective_value(*competitor.current_round_metrics, objective_kind);
  return std::abs(current_value - previous_value);
}

}  // namespace detail

[[nodiscard]] inline double resolve_step_size(
    const CompetitorState& competitor, const StepPolicyConfig& config,
    CompetitionObjectiveKind objective_kind,
    core::rng::StreamManager* stream_manager) {
  validate_step_policy(config);

  switch (config.kind) {
    case StepPolicyKind::kFixed: {
      if (config.jitter > 0.0 && stream_manager != nullptr) {
        auto& stream = stream_manager->get_stream(kMotionStepSizesStreamName);
        std::uniform_real_distribution<double> dist(-config.jitter,
                                                    config.jitter);
        return config.fixed_step_size * (1.0 + dist(stream.engine()));
      }
      return config.fixed_step_size;
    }
    case StepPolicyKind::kRandomUniform: {
      if (stream_manager == nullptr) {
        throw std::invalid_argument(
            "resolve_step_size: random-uniform step policy requires a "
            "StreamManager.");
      }
      auto& stream = stream_manager->get_stream(kMotionStepSizesStreamName);
      std::uniform_real_distribution<double> dist(config.min_step_size,
                                                  config.max_step_size);
      return dist(stream.engine());
    }
    case StepPolicyKind::kShareDeltaProportional:
      return config.proportionality_constant *
             detail::objective_delta(competitor, objective_kind);
  }
  return 0.0;
}

}  // namespace socialchoicelab::competition
