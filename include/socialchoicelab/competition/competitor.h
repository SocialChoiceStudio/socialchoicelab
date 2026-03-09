// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <socialchoicelab/competition/competition_types.h>

#include <optional>
#include <vector>

namespace socialchoicelab::competition {

struct CompetitorRoundMetrics {
  double vote_share = 0.0;
  double seat_share = 0.0;
  int vote_total = 0;
  int seats_won = 0;
};

struct CompetitorState {
  int id = -1;
  CompetitorStrategyKind strategy_kind = CompetitorStrategyKind::kSticker;
  PointNd position;
  PointNd heading;
  double current_step_size = 0.0;
  bool active = true;
  std::optional<CompetitorRoundMetrics> previous_round_metrics;
  std::optional<CompetitorRoundMetrics> current_round_metrics;
};

struct CompetitorSpec {
  CompetitorStrategyKind strategy_kind = CompetitorStrategyKind::kSticker;
  std::optional<PointNd> initial_position;
  std::optional<PointNd> initial_heading;
  bool active = true;
};

struct CompetitionInitializationConfig {
  CompetitionBounds bounds;
  PositionInitializationMode position_mode =
      PositionInitializationMode::kUniformRandom;
  HeadingInitializationMode heading_mode =
      HeadingInitializationMode::kRandomUnit;
  StepPolicyConfig step_policy;
  std::vector<CompetitorSpec> competitor_specs;
};

struct CompetitionRoundSnapshot {
  int round_index = 0;
  std::vector<CompetitorState> competitors;
};

}  // namespace socialchoicelab::competition
