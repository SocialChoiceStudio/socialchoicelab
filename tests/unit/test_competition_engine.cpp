// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <socialchoicelab/competition/engine.h>
#include <socialchoicelab/core/rng/stream_manager.h>

#include <utility>
#include <vector>

using socialchoicelab::competition::BoundaryPolicyKind;
using socialchoicelab::competition::CompetitionBounds;
using socialchoicelab::competition::CompetitionEngineConfig;
using socialchoicelab::competition::CompetitionObjectiveKind;
using socialchoicelab::competition::CompetitorState;
using socialchoicelab::competition::CompetitorStrategyKind;
using socialchoicelab::competition::ElectionFeedbackConfig;
using socialchoicelab::competition::HeadingInitializationMode;
using socialchoicelab::competition::kHunterAdaptationStreamName;
using socialchoicelab::competition::kMotionStepSizesStreamName;
using socialchoicelab::competition::run_competition_round;
using socialchoicelab::competition::run_fixed_round_competition;
using socialchoicelab::competition::SeatRuleKind;
using socialchoicelab::competition::StepPolicyConfig;
using socialchoicelab::competition::StepPolicyKind;
using socialchoicelab::core::rng::StreamManager;
using socialchoicelab::core::types::PointNd;
using socialchoicelab::geometry::DistConfig;

namespace {

PointNd point(std::initializer_list<double> values) {
  PointNd p(static_cast<int>(values.size()));
  int i = 0;
  for (double value : values) {
    p[i++] = value;
  }
  return p;
}

CompetitorState competitor(int id, CompetitorStrategyKind kind,
                           PointNd position, PointNd heading = point({0.0})) {
  CompetitorState state;
  state.id = id;
  state.strategy_kind = kind;
  state.position = std::move(position);
  state.heading = std::move(heading);
  return state;
}

CompetitionEngineConfig one_dimensional_engine_config() {
  CompetitionEngineConfig config;
  config.bounds = {point({-5.0}), point({5.0})};
  config.election_feedback = ElectionFeedbackConfig{
    DistConfig{socialchoicelab::preference::distance::DistanceType::EUCLIDEAN,
               2.0,
               {1.0}},
    1, SeatRuleKind::kPluralityTopK};
  config.step_policy = StepPolicyConfig{StepPolicyKind::kFixed, 1.0};
  config.boundary_policy = BoundaryPolicyKind::kProjectToBox;
  config.objective_kind = CompetitionObjectiveKind::kVoteShare;
  config.max_rounds = 1;
  return config;
}

}  // namespace

TEST(CompetitionEngine, SingleRoundAllStickersKeepPositionsAndGetMetrics) {
  auto config = one_dimensional_engine_config();
  const std::vector<CompetitorState> competitors = {
    competitor(0, CompetitorStrategyKind::kSticker, point({0.0})),
    competitor(1, CompetitorStrategyKind::kSticker, point({3.0}))};
  const std::vector<PointNd> voters = {point({0.1}), point({0.2}),
                                       point({2.9})};

  const auto step = run_competition_round(config, competitors, voters, nullptr);

  ASSERT_EQ(step.record.evaluated_competitors.size(), 2u);
  EXPECT_EQ(step.record.feedback.vote_totals, (std::vector<int>{2, 1}));
  EXPECT_DOUBLE_EQ(step.next_competitors[0].position[0], 0.0);
  EXPECT_DOUBLE_EQ(step.next_competitors[1].position[0], 3.0);
  ASSERT_TRUE(
      step.record.evaluated_competitors[0].current_round_metrics.has_value());
  EXPECT_EQ(
      step.record.evaluated_competitors[0].current_round_metrics->vote_total,
      2);
}

TEST(CompetitionEngine, SynchronousAggregatorUsesCurrentRoundSupporters) {
  auto config = one_dimensional_engine_config();
  config.step_policy = StepPolicyConfig{StepPolicyKind::kFixed, 0.5};

  const std::vector<CompetitorState> competitors = {
    competitor(0, CompetitorStrategyKind::kAggregator, point({0.0})),
    competitor(1, CompetitorStrategyKind::kSticker, point({4.0}))};
  const std::vector<PointNd> voters = {point({0.1}), point({1.0}), point({1.5}),
                                       point({3.9})};

  const auto step = run_competition_round(config, competitors, voters, nullptr);

  EXPECT_EQ(step.record.feedback.supporter_indices[0],
            (std::vector<int>{0, 1, 2}));
  EXPECT_DOUBLE_EQ(step.next_competitors[0].position[0], 0.5);
  EXPECT_DOUBLE_EQ(step.next_competitors[1].position[0], 4.0);
}

TEST(CompetitionEngine, FixedRoundTraceCarriesPreviousMetricsForward) {
  auto config = one_dimensional_engine_config();
  config.max_rounds = 2;

  const std::vector<CompetitorState> competitors = {
    competitor(0, CompetitorStrategyKind::kSticker, point({0.0})),
    competitor(1, CompetitorStrategyKind::kSticker, point({3.0}))};
  const std::vector<PointNd> voters = {point({0.1}), point({0.2}),
                                       point({2.9})};

  const auto trace =
      run_fixed_round_competition(config, competitors, voters, nullptr);

  ASSERT_EQ(trace.rounds.size(), 2u);
  ASSERT_EQ(trace.final_competitors.size(), 2u);
  ASSERT_TRUE(trace.final_competitors[0].previous_round_metrics.has_value());
  ASSERT_TRUE(trace.final_competitors[0].current_round_metrics.has_value());
  EXPECT_EQ(trace.final_competitors[0].previous_round_metrics->vote_total, 2);
  EXPECT_EQ(trace.final_competitors[0].current_round_metrics->vote_total, 2);
}

TEST(CompetitionEngine, HunterRoundIsReproducibleWithNamedStream) {
  auto config = one_dimensional_engine_config();
  config.step_policy = StepPolicyConfig{StepPolicyKind::kFixed, 1.0};

  std::vector<CompetitorState> competitors = {
    competitor(0, CompetitorStrategyKind::kHunter, point({0.0}), point({1.0})),
    competitor(1, CompetitorStrategyKind::kSticker, point({3.0}))};
  competitors[0].previous_round_metrics =
      socialchoicelab::competition::CompetitorRoundMetrics{0.6, 0.0, 0, 0};
  competitors[0].current_round_metrics =
      socialchoicelab::competition::CompetitorRoundMetrics{0.2, 0.0, 0, 0};

  const std::vector<PointNd> voters = {point({2.8}), point({2.9}),
                                       point({3.0})};

  StreamManager streams_a(2026);
  StreamManager streams_b(2026);
  streams_a.register_streams(
      {kHunterAdaptationStreamName, kMotionStepSizesStreamName});
  streams_b.register_streams(
      {kHunterAdaptationStreamName, kMotionStepSizesStreamName});

  const auto step_a =
      run_competition_round(config, competitors, voters, &streams_a);
  const auto step_b =
      run_competition_round(config, competitors, voters, &streams_b);

  EXPECT_TRUE(step_a.next_competitors[0].heading.isApprox(
      step_b.next_competitors[0].heading));
  EXPECT_TRUE(step_a.next_competitors[0].position.isApprox(
      step_b.next_competitors[0].position));
}

// --- Inactive competitor tests ---
//
// Inactive competitors are fully excluded from voting and motion. They do not
// appear as alternatives that voters rank, they receive zero metrics, and their
// position is frozen every round. This models a candidate who has dropped out.
// See the design comment on `detail::active_competitor_indices` in engine.h
// for the full rationale.

TEST(CompetitionEngine, InactiveCompetitorIsExcludedFromElectionAndFrozen) {
  auto config = one_dimensional_engine_config();
  config.step_policy = StepPolicyConfig{StepPolicyKind::kFixed, 1.0};

  // Competitor 0 is inactive. Competitor 1 is an Aggregator at 4.0.
  // Three voters at 0.1, 0.2, 3.9 — all closer to competitor 1 if comp 0
  // is excluded, so comp 1 should get all votes.
  std::vector<CompetitorState> competitors = {
    competitor(0, CompetitorStrategyKind::kSticker, point({0.0})),
    competitor(1, CompetitorStrategyKind::kAggregator, point({4.0}))};
  competitors[0].active = false;

  const std::vector<PointNd> voters = {point({0.1}), point({0.2}),
                                       point({3.9})};

  const auto step = run_competition_round(config, competitors, voters, nullptr);

  // Inactive competitor 0: zero metrics in every slot.
  ASSERT_TRUE(
      step.record.evaluated_competitors[0].current_round_metrics.has_value());
  EXPECT_EQ(
      step.record.evaluated_competitors[0].current_round_metrics->vote_total,
      0);
  EXPECT_DOUBLE_EQ(
      step.record.evaluated_competitors[0].current_round_metrics->vote_share,
      0.0);

  // Active competitor 1: all three voters, since comp 0 is excluded.
  ASSERT_TRUE(
      step.record.evaluated_competitors[1].current_round_metrics.has_value());
  EXPECT_EQ(
      step.record.evaluated_competitors[1].current_round_metrics->vote_total,
      3);

  // Inactive competitor 0 must not move.
  EXPECT_DOUBLE_EQ(step.next_competitors[0].position[0], 0.0);

  // The round feedback vote_totals are indexed by total competitor count.
  EXPECT_EQ(step.record.feedback.vote_totals[0], 0);
  EXPECT_EQ(step.record.feedback.vote_totals[1], 3);
}

TEST(CompetitionEngine, AllInactiveCompetitorsIsRejected) {
  auto config = one_dimensional_engine_config();
  std::vector<CompetitorState> competitors = {
    competitor(0, CompetitorStrategyKind::kSticker, point({0.0})),
    competitor(1, CompetitorStrategyKind::kSticker, point({3.0}))};
  competitors[0].active = false;
  competitors[1].active = false;

  const std::vector<PointNd> voters = {point({1.0})};

  EXPECT_THROW(
      (void)run_competition_round(config, competitors, voters, nullptr),
      std::invalid_argument);
}

TEST(CompetitionEngine, InactiveCompetitorPositionFrozenAcrossMultipleRounds) {
  auto config = one_dimensional_engine_config();
  config.max_rounds = 5;
  config.step_policy = StepPolicyConfig{StepPolicyKind::kFixed, 1.0};

  // Competitor 0 inactive at 0.0. Competitor 1 active (Aggregator) at 2.0.
  // Voters near 3.0 — Aggregator will converge toward them.
  std::vector<CompetitorState> competitors = {
    competitor(0, CompetitorStrategyKind::kSticker, point({0.0})),
    competitor(1, CompetitorStrategyKind::kAggregator, point({2.0}))};
  competitors[0].active = false;

  const std::vector<PointNd> voters = {point({2.9}), point({3.0}),
                                       point({3.1})};

  const auto trace =
      run_fixed_round_competition(config, competitors, voters, nullptr);

  // Inactive competitor must be frozen at 0.0 in every recorded round.
  for (const auto& round : trace.rounds) {
    EXPECT_DOUBLE_EQ(round.evaluated_competitors[0].position[0], 0.0)
        << "Inactive competitor moved in round " << round.round_index;
  }
  EXPECT_DOUBLE_EQ(trace.final_competitors[0].position[0], 0.0);
}
