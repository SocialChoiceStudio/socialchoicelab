// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <socialchoicelab/competition/adaptation.h>
#include <socialchoicelab/competition/competition_types.h>
#include <socialchoicelab/competition/strategy_registry.h>
#include <socialchoicelab/core/rng/stream_manager.h>

#include <optional>
#include <utility>
#include <vector>

using socialchoicelab::competition::AdaptationContext;
using socialchoicelab::competition::CompetitionBounds;
using socialchoicelab::competition::CompetitionObjectiveKind;
using socialchoicelab::competition::CompetitorRoundMetrics;
using socialchoicelab::competition::CompetitorState;
using socialchoicelab::competition::CompetitorStrategyKind;
using socialchoicelab::competition::kHunterAdaptationStreamName;
using socialchoicelab::competition::StrategyRegistry;
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

CompetitorState competitor(int id, CompetitorStrategyKind kind,
                           PointNd position, PointNd heading) {
  CompetitorState state;
  state.id = id;
  state.strategy_kind = kind;
  state.position = std::move(position);
  state.heading = std::move(heading);
  return state;
}

CompetitorRoundMetrics metrics(double vote_share, double seat_share = 0.0) {
  CompetitorRoundMetrics result;
  result.vote_share = vote_share;
  result.seat_share = seat_share;
  return result;
}

}  // namespace

TEST(CompetitionStrategies, StickerReturnsZeroHeading) {
  const auto& registry = StrategyRegistry::built_in();
  const auto* sticker = registry.find(CompetitorStrategyKind::kSticker);
  ASSERT_NE(sticker, nullptr);

  CompetitionBounds bounds{point({-1.0, -1.0}), point({1.0, 1.0})};
  std::vector<CompetitorState> competitors = {
    competitor(0, CompetitorStrategyKind::kSticker, point({0.2, -0.1}),
               point({1.0, 0.0}))};
  std::vector<std::vector<PointNd>> supporters(1);
  AdaptationContext context{bounds, CompetitionObjectiveKind::kVoteShare,
                            competitors, supporters, nullptr};

  const auto decision = sticker->adapt(competitors[0], context);
  EXPECT_EQ(decision.next_heading.size(), 2);
  EXPECT_DOUBLE_EQ(decision.next_heading.norm(), 0.0);
}

TEST(CompetitionStrategies, HunterKeepsHeadingAfterImprovement) {
  const auto& registry = StrategyRegistry::built_in();
  const auto* hunter = registry.find(CompetitorStrategyKind::kHunter);
  ASSERT_NE(hunter, nullptr);

  CompetitionBounds bounds{point({-2.0}), point({2.0})};
  std::vector<CompetitorState> competitors = {
    competitor(0, CompetitorStrategyKind::kHunter, point({0.0}), point({1.0}))};
  competitors[0].previous_round_metrics = metrics(0.2);
  competitors[0].current_round_metrics = metrics(0.4);

  std::vector<std::vector<PointNd>> supporters(1);
  AdaptationContext context{bounds, CompetitionObjectiveKind::kVoteShare,
                            competitors, supporters, nullptr};

  const auto decision = hunter->adapt(competitors[0], context);
  EXPECT_DOUBLE_EQ(decision.next_heading[0], 1.0);
}

// Hunter: static support (equal vote share) must trigger reversal, not
// continuation — Laver & Sergenti 2012 ch.3: "casting around for a new
// policy direction when the previous policy move was punished with falling
// or static support".
TEST(CompetitionStrategies, HunterReversesOnStaticSupport1D) {
  const auto& registry = StrategyRegistry::built_in();
  const auto* hunter = registry.find(CompetitorStrategyKind::kHunter);
  ASSERT_NE(hunter, nullptr);

  CompetitionBounds bounds{point({-2.0}), point({2.0})};
  std::vector<CompetitorState> competitors = {
    competitor(0, CompetitorStrategyKind::kHunter, point({0.0}), point({1.0}))};
  competitors[0].previous_round_metrics = metrics(0.3);
  competitors[0].current_round_metrics = metrics(0.3);

  std::vector<std::vector<PointNd>> supporters(1);
  AdaptationContext context{bounds, CompetitionObjectiveKind::kVoteShare,
                            competitors, supporters, nullptr};

  const auto decision = hunter->adapt(competitors[0], context);
  EXPECT_DOUBLE_EQ(decision.next_heading[0], -1.0);
}

// Hunter: when performance drops the result must lie in the backward
// half-space, be a unit vector, and be reproducible across identical seeds.
TEST(CompetitionStrategies,
     HunterLossIsInBackwardHalfspaceAndReproducibleIn2D) {
  const auto& registry = StrategyRegistry::built_in();
  const auto* hunter = registry.find(CompetitorStrategyKind::kHunter);
  ASSERT_NE(hunter, nullptr);

  const PointNd forward = point({1.0, 0.0});
  CompetitionBounds bounds{point({-1.0, -1.0}), point({1.0, 1.0})};
  std::vector<CompetitorState> competitors = {
    competitor(0, CompetitorStrategyKind::kHunter, point({0.0, 0.0}), forward)};
  competitors[0].previous_round_metrics = metrics(0.5);
  competitors[0].current_round_metrics = metrics(0.2);

  std::vector<std::vector<PointNd>> supporters(1);
  StreamManager streams_a(99);
  StreamManager streams_b(99);
  streams_a.register_streams({kHunterAdaptationStreamName});
  streams_b.register_streams({kHunterAdaptationStreamName});

  AdaptationContext context_a{bounds, CompetitionObjectiveKind::kVoteShare,
                              competitors, supporters, &streams_a};
  AdaptationContext context_b{bounds, CompetitionObjectiveKind::kVoteShare,
                              competitors, supporters, &streams_b};

  const auto decision_a = hunter->adapt(competitors[0], context_a);
  const auto decision_b = hunter->adapt(competitors[0], context_b);

  EXPECT_TRUE(decision_a.next_heading.isApprox(decision_b.next_heading));
  EXPECT_NEAR(decision_a.next_heading.norm(), 1.0, 1e-12);
  // Must be in the backward half-space: dot product with forward heading <= 0.
  EXPECT_LE(decision_a.next_heading.dot(forward), 0.0);
}

// Hunter 1D: performance drop → deterministic negation, no stream needed.
TEST(CompetitionStrategies, HunterLossNegatesHeadingIn1D) {
  const auto& registry = StrategyRegistry::built_in();
  const auto* hunter = registry.find(CompetitorStrategyKind::kHunter);
  ASSERT_NE(hunter, nullptr);

  CompetitionBounds bounds{point({-2.0}), point({2.0})};
  std::vector<CompetitorState> competitors = {
    competitor(0, CompetitorStrategyKind::kHunter, point({0.5}), point({1.0}))};
  competitors[0].previous_round_metrics = metrics(0.5);
  competitors[0].current_round_metrics = metrics(0.2);

  std::vector<std::vector<PointNd>> supporters(1);
  // No stream_manager — 1D reversal must not require one.
  AdaptationContext context{bounds, CompetitionObjectiveKind::kVoteShare,
                            competitors, supporters, nullptr};

  const auto decision = hunter->adapt(competitors[0], context);
  EXPECT_DOUBLE_EQ(decision.next_heading[0], -1.0);
}

// Hunter 2D loss with no stream_manager must throw a clear error.
TEST(CompetitionStrategies, HunterLossThrowsWithoutStreamIn2D) {
  const auto& registry = StrategyRegistry::built_in();
  const auto* hunter = registry.find(CompetitorStrategyKind::kHunter);
  ASSERT_NE(hunter, nullptr);

  CompetitionBounds bounds{point({-1.0, -1.0}), point({1.0, 1.0})};
  std::vector<CompetitorState> competitors = {
    competitor(0, CompetitorStrategyKind::kHunter, point({0.0, 0.0}),
               point({1.0, 0.0}))};
  competitors[0].previous_round_metrics = metrics(0.5);
  competitors[0].current_round_metrics = metrics(0.2);

  std::vector<std::vector<PointNd>> supporters(1);
  AdaptationContext context{bounds, CompetitionObjectiveKind::kVoteShare,
                            competitors, supporters, nullptr};

  EXPECT_THROW((void)hunter->adapt(competitors[0], context),
               std::invalid_argument);
}

TEST(CompetitionStrategies, AggregatorMovesTowardSupporterMeanInOneDimension) {
  const auto& registry = StrategyRegistry::built_in();
  const auto* aggregator = registry.find(CompetitorStrategyKind::kAggregator);
  ASSERT_NE(aggregator, nullptr);

  CompetitionBounds bounds{point({-5.0}), point({5.0})};
  std::vector<CompetitorState> competitors = {competitor(
      0, CompetitorStrategyKind::kAggregator, point({0.0}), point({0.0}))};
  std::vector<std::vector<PointNd>> supporters = {
    {point({-1.0}), point({3.0})},
  };
  AdaptationContext context{bounds, CompetitionObjectiveKind::kVoteShare,
                            competitors, supporters, nullptr};

  const auto decision = aggregator->adapt(competitors[0], context);
  EXPECT_DOUBLE_EQ(decision.next_heading[0], 1.0);
}

TEST(CompetitionStrategies, AggregatorMovesTowardSupporterMeanInTwoDimensions) {
  const auto& registry = StrategyRegistry::built_in();
  const auto* aggregator = registry.find(CompetitorStrategyKind::kAggregator);
  ASSERT_NE(aggregator, nullptr);

  CompetitionBounds bounds{point({-5.0, -5.0}), point({5.0, 5.0})};
  std::vector<CompetitorState> competitors = {
    competitor(0, CompetitorStrategyKind::kAggregator, point({0.0, 0.0}),
               point({0.0, 0.0}))};
  std::vector<std::vector<PointNd>> supporters = {
    {point({2.0, 4.0}), point({4.0, 4.0})},
  };
  AdaptationContext context{bounds, CompetitionObjectiveKind::kVoteShare,
                            competitors, supporters, nullptr};

  const auto decision = aggregator->adapt(competitors[0], context);
  EXPECT_NEAR(decision.next_heading[0], 0.6, 1e-12);
  EXPECT_NEAR(decision.next_heading[1], 0.8, 1e-12);
}

TEST(CompetitionStrategies, PredatorMovesTowardLeaderByCurrentObjective) {
  const auto& registry = StrategyRegistry::built_in();
  const auto* predator = registry.find(CompetitorStrategyKind::kPredator);
  ASSERT_NE(predator, nullptr);

  CompetitionBounds bounds{point({-5.0}), point({5.0})};
  std::vector<CompetitorState> competitors = {
    competitor(0, CompetitorStrategyKind::kPredator, point({0.0}),
               point({0.0})),
    competitor(1, CompetitorStrategyKind::kSticker, point({3.0}), point({0.0})),
    competitor(2, CompetitorStrategyKind::kSticker, point({-2.0}),
               point({0.0}))};
  competitors[0].current_round_metrics = metrics(0.2);
  competitors[1].current_round_metrics = metrics(0.6);
  competitors[2].current_round_metrics = metrics(0.6);

  std::vector<std::vector<PointNd>> supporters(3);
  AdaptationContext context{bounds, CompetitionObjectiveKind::kVoteShare,
                            competitors, supporters, nullptr};

  const auto decision = predator->adapt(competitors[0], context);
  EXPECT_DOUBLE_EQ(decision.next_heading[0], 1.0);
}

// When the Predator is the current leader, the target is itself. The direction
// toward itself is the zero vector, which normalizes to zero.
// advance_competitor short-circuits on a zero heading and leaves the position
// unchanged. This is the correct behaviour: the leader has no incentive to
// chase itself.
TEST(CompetitionStrategies, PredatorAsLeaderProducesZeroHeading) {
  const auto& registry = StrategyRegistry::built_in();
  const auto* predator = registry.find(CompetitorStrategyKind::kPredator);
  ASSERT_NE(predator, nullptr);

  CompetitionBounds bounds{point({-5.0}), point({5.0})};
  std::vector<CompetitorState> competitors = {
    competitor(0, CompetitorStrategyKind::kPredator, point({1.0}),
               point({0.0})),
    competitor(1, CompetitorStrategyKind::kSticker, point({3.0}),
               point({0.0}))};
  // Competitor 0 (Predator) leads with 0.8 vote share.
  competitors[0].current_round_metrics = metrics(0.8);
  competitors[1].current_round_metrics = metrics(0.2);

  std::vector<std::vector<PointNd>> supporters(2);
  AdaptationContext context{bounds, CompetitionObjectiveKind::kVoteShare,
                            competitors, supporters, nullptr};

  const auto decision = predator->adapt(competitors[0], context);
  // Heading toward self is zero — advance_competitor will leave position
  // frozen.
  EXPECT_DOUBLE_EQ(decision.next_heading.norm(), 0.0);
}
