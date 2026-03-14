// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <socialchoicelab/core/types.h>

#include <stdexcept>
#include <string>

namespace socialchoicelab::competition {

using core::types::PointNd;

enum class CompetitorStrategyKind {
  kSticker,
  kHunter,
  kAggregator,
  kPredator,
  kHunterSticker,
};

enum class PositionInitializationMode {
  kUniformRandom,
  kUserSpecified,
};

enum class HeadingInitializationMode {
  kZero,
  kRandomUnit,
  kUserSpecified,
};

enum class StepPolicyKind {
  kFixed,
  kRandomUniform,
  kShareDeltaProportional,
};

enum class BoundaryPolicyKind {
  kProjectToBox,
  kStayPut,
  kReflect,
};

struct CompetitionBounds {
  PointNd lower;
  PointNd upper;
};

struct StepPolicyConfig {
  StepPolicyKind kind = StepPolicyKind::kFixed;
  double fixed_step_size = 0.0;
  double min_step_size = 0.0;
  double max_step_size = 0.0;
  double proportionality_constant = 1.0;
  double jitter = 0.0;
};

[[nodiscard]] inline const char* stable_strategy_id(
    CompetitorStrategyKind kind) noexcept {
  switch (kind) {
    case CompetitorStrategyKind::kSticker:
      return "sticker";
    case CompetitorStrategyKind::kHunter:
      return "hunter";
    case CompetitorStrategyKind::kAggregator:
      return "aggregator";
    case CompetitorStrategyKind::kPredator:
      return "predator";
    case CompetitorStrategyKind::kHunterSticker:
      return "hunter_sticker";
  }
  return "unknown";
}

[[nodiscard]] inline CompetitorStrategyKind strategy_kind_from_stable_id(
    const std::string& stable_id) {
  if (stable_id == "sticker") return CompetitorStrategyKind::kSticker;
  if (stable_id == "hunter") return CompetitorStrategyKind::kHunter;
  if (stable_id == "aggregator") return CompetitorStrategyKind::kAggregator;
  if (stable_id == "predator") return CompetitorStrategyKind::kPredator;
  if (stable_id == "hunter_sticker")
    return CompetitorStrategyKind::kHunterSticker;
  throw std::invalid_argument(
      "strategy_kind_from_stable_id: unknown strategy id \"" + stable_id +
      "\".");
}

[[nodiscard]] inline int competition_dimension(
    const CompetitionBounds& bounds) {
  if (bounds.lower.size() != bounds.upper.size()) {
    throw std::invalid_argument(
        "competition_dimension: lower and upper bounds must have the same "
        "dimension.");
  }
  return static_cast<int>(bounds.lower.size());
}

inline void validate_bounds(const CompetitionBounds& bounds) {
  const int dim = competition_dimension(bounds);
  if (dim != 1 && dim != 2) {
    throw std::invalid_argument(
        "validate_bounds: competition currently supports only 1D or 2D "
        "bounds.");
  }
  for (int d = 0; d < dim; ++d) {
    if (!(bounds.lower[d] < bounds.upper[d])) {
      throw std::invalid_argument(
          "validate_bounds: each lower bound must be strictly less than the "
          "matching upper bound.");
    }
  }
}

inline void validate_step_policy(const StepPolicyConfig& config) {
  switch (config.kind) {
    case StepPolicyKind::kFixed:
      if (config.fixed_step_size < 0.0) {
        throw std::invalid_argument(
            "validate_step_policy: fixed_step_size must be non-negative.");
      }
      if (config.jitter < 0.0 || config.jitter > 1.0) {
        throw std::invalid_argument(
            "validate_step_policy: jitter must be in [0, 1]. "
            "Got " +
            std::to_string(config.jitter) + ".");
      }
      return;
    case StepPolicyKind::kRandomUniform:
      if (config.min_step_size < 0.0 || config.max_step_size < 0.0) {
        throw std::invalid_argument(
            "validate_step_policy: random-uniform step sizes must be "
            "non-negative.");
      }
      if (config.min_step_size > config.max_step_size) {
        throw std::invalid_argument(
            "validate_step_policy: min_step_size must be less than or equal "
            "to max_step_size.");
      }
      return;
    case StepPolicyKind::kShareDeltaProportional:
      if (config.proportionality_constant < 0.0) {
        throw std::invalid_argument(
            "validate_step_policy: proportionality_constant must be "
            "non-negative.");
      }
      return;
  }
}

}  // namespace socialchoicelab::competition
