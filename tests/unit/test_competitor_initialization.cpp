// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <socialchoicelab/competition/competition_types.h>
#include <socialchoicelab/competition/initialization.h>
#include <socialchoicelab/competition/strategy_registry.h>
#include <socialchoicelab/core/rng/stream_manager.h>

#include <optional>
#include <vector>

using socialchoicelab::competition::competition_dimension;
using socialchoicelab::competition::CompetitionBounds;
using socialchoicelab::competition::CompetitionInitializationConfig;
using socialchoicelab::competition::CompetitorSpec;
using socialchoicelab::competition::CompetitorStrategyKind;
using socialchoicelab::competition::HeadingInitializationMode;
using socialchoicelab::competition::initialize_competitors;
using socialchoicelab::competition::kInitHeadingsStreamName;
using socialchoicelab::competition::kInitPositionsStreamName;
using socialchoicelab::competition::kInitStepSizesStreamName;
using socialchoicelab::competition::PositionInitializationMode;
using socialchoicelab::competition::stable_strategy_id;
using socialchoicelab::competition::StepPolicyConfig;
using socialchoicelab::competition::StepPolicyKind;
using socialchoicelab::competition::strategy_kind_from_stable_id;
using socialchoicelab::competition::StrategyRegistry;
using socialchoicelab::competition::validate_bounds;
using socialchoicelab::competition::validate_initialization_config;
using socialchoicelab::core::rng::StreamManager;
using socialchoicelab::core::types::PointNd;

namespace {

PointNd point(std::initializer_list<double> values) {
  PointNd p(static_cast<int>(values.size()));
  int i = 0;
  for (double value : values) {
    p[i++] = value;
  }
  return p;
}

CompetitionInitializationConfig one_dimensional_random_config() {
  CompetitionInitializationConfig config;
  config.bounds = {point({-2.0}), point({3.0})};
  config.position_mode = PositionInitializationMode::kUniformRandom;
  config.heading_mode = HeadingInitializationMode::kRandomUnit;
  config.step_policy = StepPolicyConfig{StepPolicyKind::kFixed, 0.25};
  config.competitor_specs = {
    {CompetitorStrategyKind::kSticker, std::nullopt, std::nullopt, true},
    {CompetitorStrategyKind::kHunter, std::nullopt, std::nullopt, true},
    {CompetitorStrategyKind::kAggregator, std::nullopt, std::nullopt, true},
  };
  return config;
}

}  // namespace

TEST(CompetitionTypes, BoundsDimensionMustBeOneOrTwo) {
  CompetitionBounds one_dimensional{point({0.0}), point({1.0})};
  CompetitionBounds two_dimensional{point({0.0, 0.0}), point({1.0, 1.0})};
  EXPECT_EQ(competition_dimension(one_dimensional), 1);
  EXPECT_EQ(competition_dimension(two_dimensional), 2);

  CompetitionBounds three_dimensional{point({0.0, 0.0, 0.0}),
                                      point({1.0, 1.0, 1.0})};
  EXPECT_THROW(validate_bounds(three_dimensional), std::invalid_argument);
}

TEST(CompetitionTypes, StableStrategyIdsRoundTrip) {
  EXPECT_STREQ(stable_strategy_id(CompetitorStrategyKind::kSticker), "sticker");
  EXPECT_EQ(strategy_kind_from_stable_id("predator"),
            CompetitorStrategyKind::kPredator);
  EXPECT_THROW(
      []() { static_cast<void>(strategy_kind_from_stable_id("unknown")); }(),
      std::invalid_argument);
}

TEST(StrategyRegistry, BuiltInRegistryContainsBaselineStrategies) {
  const auto& registry = StrategyRegistry::built_in();
  EXPECT_EQ(registry.size(),
            5u);  // sticker, hunter, aggregator, predator, hunter_sticker

  const auto* hunter = registry.find("hunter");
  ASSERT_NE(hunter, nullptr);
  EXPECT_EQ(hunter->kind(), CompetitorStrategyKind::kHunter);

  const auto* predator = registry.find(CompetitorStrategyKind::kPredator);
  ASSERT_NE(predator, nullptr);
  EXPECT_EQ(predator->stable_id(), "predator");
}

TEST(Initialization, UserSpecifiedOneDimensionalPositionsArePreserved) {
  CompetitionInitializationConfig config;
  config.bounds = {point({-5.0}), point({5.0})};
  config.position_mode = PositionInitializationMode::kUserSpecified;
  config.heading_mode = HeadingInitializationMode::kZero;
  config.step_policy = StepPolicyConfig{StepPolicyKind::kFixed, 0.5};
  config.competitor_specs = {
    {CompetitorStrategyKind::kSticker, point({-1.5}), std::nullopt, true},
    {CompetitorStrategyKind::kHunter, point({2.25}), std::nullopt, true},
  };

  StreamManager streams(1234);
  streams.register_streams({kInitPositionsStreamName, kInitHeadingsStreamName,
                            kInitStepSizesStreamName});

  const auto competitors = initialize_competitors(config, streams);
  ASSERT_EQ(competitors.size(), 2u);
  EXPECT_DOUBLE_EQ(competitors[0].position[0], -1.5);
  EXPECT_DOUBLE_EQ(competitors[1].position[0], 2.25);
  EXPECT_EQ(competitors[0].heading.norm(), 0.0);
  EXPECT_EQ(competitors[0].current_step_size, 0.5);
}

TEST(Initialization, RandomOneDimensionalHeadingsAreUnitSignedDirections) {
  auto config = one_dimensional_random_config();
  StreamManager streams(77);
  streams.register_streams({kInitPositionsStreamName, kInitHeadingsStreamName,
                            kInitStepSizesStreamName});

  const auto competitors = initialize_competitors(config, streams);
  ASSERT_EQ(competitors.size(), 3u);
  for (const auto& competitor : competitors) {
    EXPECT_TRUE(competitor.heading[0] == 1.0 || competitor.heading[0] == -1.0);
    EXPECT_DOUBLE_EQ(competitor.heading.norm(), 1.0);
    EXPECT_GE(competitor.position[0], -2.0);
    EXPECT_LE(competitor.position[0], 3.0);
  }
}

TEST(Initialization, RandomInitializationIsReproducibleAcrossStreamManagers) {
  CompetitionInitializationConfig config;
  config.bounds = {point({-1.0, -2.0}), point({1.0, 2.0})};
  config.position_mode = PositionInitializationMode::kUniformRandom;
  config.heading_mode = HeadingInitializationMode::kRandomUnit;
  config.step_policy =
      StepPolicyConfig{StepPolicyKind::kRandomUniform, 0.0, 0.1, 0.4};
  config.competitor_specs = {
    {CompetitorStrategyKind::kSticker, std::nullopt, std::nullopt, true},
    {CompetitorStrategyKind::kPredator, std::nullopt, std::nullopt, true},
  };

  StreamManager streams_a(2026);
  StreamManager streams_b(2026);
  streams_a.register_streams({kInitPositionsStreamName, kInitHeadingsStreamName,
                              kInitStepSizesStreamName});
  streams_b.register_streams({kInitPositionsStreamName, kInitHeadingsStreamName,
                              kInitStepSizesStreamName});

  const auto competitors_a = initialize_competitors(config, streams_a);
  const auto competitors_b = initialize_competitors(config, streams_b);

  ASSERT_EQ(competitors_a.size(), competitors_b.size());
  for (size_t i = 0; i < competitors_a.size(); ++i) {
    EXPECT_TRUE(competitors_a[i].position.isApprox(competitors_b[i].position));
    EXPECT_TRUE(competitors_a[i].heading.isApprox(competitors_b[i].heading));
    EXPECT_DOUBLE_EQ(competitors_a[i].current_step_size,
                     competitors_b[i].current_step_size);
  }
}

TEST(Initialization, UserSpecifiedHeadingsAreNormalized) {
  CompetitionInitializationConfig config;
  config.bounds = {point({-1.0, -1.0}), point({1.0, 1.0})};
  config.position_mode = PositionInitializationMode::kUserSpecified;
  config.heading_mode = HeadingInitializationMode::kUserSpecified;
  config.step_policy = StepPolicyConfig{StepPolicyKind::kFixed, 0.2};
  config.competitor_specs = {
    {CompetitorStrategyKind::kHunter, point({0.0, 0.0}), point({3.0, 4.0}),
     true},
  };

  StreamManager streams(9);
  streams.register_streams({kInitPositionsStreamName, kInitHeadingsStreamName,
                            kInitStepSizesStreamName});

  const auto competitors = initialize_competitors(config, streams);
  ASSERT_EQ(competitors.size(), 1u);
  EXPECT_NEAR(competitors[0].heading[0], 0.6, 1e-12);
  EXPECT_NEAR(competitors[0].heading[1], 0.8, 1e-12);
}

TEST(Initialization, MissingUserSpecifiedPositionThrows) {
  CompetitionInitializationConfig config;
  config.bounds = {point({-1.0}), point({1.0})};
  config.position_mode = PositionInitializationMode::kUserSpecified;
  config.heading_mode = HeadingInitializationMode::kZero;
  config.step_policy = StepPolicyConfig{StepPolicyKind::kFixed, 0.1};
  config.competitor_specs = {
    {CompetitorStrategyKind::kSticker, std::nullopt, std::nullopt, true},
  };

  EXPECT_THROW(validate_initialization_config(config), std::invalid_argument);
}
