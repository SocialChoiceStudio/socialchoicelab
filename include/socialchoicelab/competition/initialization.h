// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <socialchoicelab/competition/competitor.h>
#include <socialchoicelab/core/rng/stream_manager.h>

#include <cmath>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace socialchoicelab::competition {

constexpr char kInitPositionsStreamName[] = "competition_init_positions";
constexpr char kInitHeadingsStreamName[] = "competition_init_headings";
constexpr char kInitStepSizesStreamName[] = "competition_init_step_sizes";

namespace detail {

[[nodiscard]] inline PointNd zero_vector(int dimension) {
  return PointNd::Zero(dimension);
}

inline void validate_point_dimension(const PointNd& point, int dimension,
                                     const std::string& label) {
  if (point.size() != dimension) {
    throw std::invalid_argument(label + ": point dimension mismatch.");
  }
}

inline void validate_heading(const PointNd& heading, int dimension,
                             const std::string& label) {
  validate_point_dimension(heading, dimension, label);
  const double norm = heading.norm();
  if (norm == 0.0) return;
  if (!std::isfinite(norm)) {
    throw std::invalid_argument(label + ": heading norm must be finite.");
  }
}

[[nodiscard]] inline PointNd normalize_or_zero(const PointNd& point) {
  const double norm = point.norm();
  if (norm == 0.0) return point;
  return point / norm;
}

[[nodiscard]] inline PointNd random_uniform_position(
    const CompetitionBounds& bounds, core::rng::PRNG& prng) {
  const int dimension = competition_dimension(bounds);
  PointNd point(dimension);
  for (int d = 0; d < dimension; ++d) {
    std::uniform_real_distribution<double> dist(bounds.lower[d],
                                                bounds.upper[d]);
    point[d] = dist(prng.engine());
  }
  return point;
}

[[nodiscard]] inline PointNd random_unit_heading(int dimension,
                                                 core::rng::PRNG& prng) {
  if (dimension == 1) {
    std::bernoulli_distribution dist(0.5);
    PointNd heading(1);
    heading[0] = dist(prng.engine()) ? 1.0 : -1.0;
    return heading;
  }

  PointNd heading(dimension);
  std::normal_distribution<double> dist(0.0, 1.0);
  double norm = 0.0;
  do {
    for (int d = 0; d < dimension; ++d) {
      heading[d] = dist(prng.engine());
    }
    norm = heading.norm();
  } while (norm == 0.0);
  return heading / norm;
}

[[nodiscard]] inline double initial_step_size(const StepPolicyConfig& config,
                                              core::rng::PRNG& prng) {
  switch (config.kind) {
    case StepPolicyKind::kFixed:
      return config.fixed_step_size;
    case StepPolicyKind::kRandomUniform: {
      std::uniform_real_distribution<double> dist(config.min_step_size,
                                                  config.max_step_size);
      return dist(prng.engine());
    }
    case StepPolicyKind::kShareDeltaProportional:
      return 0.0;
  }
  return 0.0;
}

}  // namespace detail

inline void validate_initialization_config(
    const CompetitionInitializationConfig& config) {
  validate_bounds(config.bounds);
  validate_step_policy(config.step_policy);

  if (config.competitor_specs.empty()) {
    throw std::invalid_argument(
        "validate_initialization_config: competitor_specs must not be empty.");
  }

  const int dimension = competition_dimension(config.bounds);
  for (const auto& spec : config.competitor_specs) {
    if (config.position_mode == PositionInitializationMode::kUserSpecified) {
      if (!spec.initial_position.has_value()) {
        throw std::invalid_argument(
            "validate_initialization_config: user-specified positions require "
            "every competitor to provide initial_position.");
      }
      detail::validate_point_dimension(*spec.initial_position, dimension,
                                       "validate_initialization_config");
    }

    if (config.heading_mode == HeadingInitializationMode::kUserSpecified) {
      if (!spec.initial_heading.has_value()) {
        throw std::invalid_argument(
            "validate_initialization_config: user-specified headings require "
            "every competitor to provide initial_heading.");
      }
      detail::validate_heading(*spec.initial_heading, dimension,
                               "validate_initialization_config");
    }

    if (spec.initial_position.has_value()) {
      detail::validate_point_dimension(*spec.initial_position, dimension,
                                       "validate_initialization_config");
    }
    if (spec.initial_heading.has_value()) {
      detail::validate_heading(*spec.initial_heading, dimension,
                               "validate_initialization_config");
    }
  }
}

[[nodiscard]] inline std::vector<CompetitorState> initialize_competitors(
    const CompetitionInitializationConfig& config,
    core::rng::StreamManager& stream_manager) {
  validate_initialization_config(config);

  const int dimension = competition_dimension(config.bounds);
  auto& positions_stream = stream_manager.get_stream(kInitPositionsStreamName);
  auto& headings_stream = stream_manager.get_stream(kInitHeadingsStreamName);
  auto& step_sizes_stream = stream_manager.get_stream(kInitStepSizesStreamName);

  std::vector<CompetitorState> competitors;
  competitors.reserve(config.competitor_specs.size());

  for (size_t i = 0; i < config.competitor_specs.size(); ++i) {
    const auto& spec = config.competitor_specs[i];

    PointNd position = detail::zero_vector(dimension);
    switch (config.position_mode) {
      case PositionInitializationMode::kUniformRandom:
        position = spec.initial_position.value_or(
            detail::random_uniform_position(config.bounds, positions_stream));
        break;
      case PositionInitializationMode::kUserSpecified:
        position = *spec.initial_position;
        break;
    }

    PointNd heading = detail::zero_vector(dimension);
    switch (config.heading_mode) {
      case HeadingInitializationMode::kZero:
        heading = spec.initial_heading.value_or(detail::zero_vector(dimension));
        break;
      case HeadingInitializationMode::kRandomUnit:
        heading = detail::normalize_or_zero(spec.initial_heading.value_or(
            detail::random_unit_heading(dimension, headings_stream)));
        break;
      case HeadingInitializationMode::kUserSpecified:
        heading = detail::normalize_or_zero(*spec.initial_heading);
        break;
    }

    CompetitorState state;
    state.id = static_cast<int>(i);
    state.strategy_kind = spec.strategy_kind;
    state.position = std::move(position);
    state.heading = std::move(heading);
    state.current_step_size =
        detail::initial_step_size(config.step_policy, step_sizes_stream);
    state.active = spec.active;
    competitors.push_back(std::move(state));
  }

  return competitors;
}

}  // namespace socialchoicelab::competition
