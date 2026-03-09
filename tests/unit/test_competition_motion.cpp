// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <socialchoicelab/competition/adaptation.h>
#include <socialchoicelab/competition/boundary_policy.h>
#include <socialchoicelab/competition/competition_types.h>
#include <socialchoicelab/competition/motion.h>
#include <socialchoicelab/competition/step_policy.h>
#include <socialchoicelab/core/rng/stream_manager.h>

#include <utility>
#include <vector>

using socialchoicelab::competition::advance_competitor;
using socialchoicelab::competition::advance_competitor_state;
using socialchoicelab::competition::apply_boundary_policy;
using socialchoicelab::competition::BoundaryPolicyKind;
using socialchoicelab::competition::CompetitionBounds;
using socialchoicelab::competition::CompetitionObjectiveKind;
using socialchoicelab::competition::CompetitorRoundMetrics;
using socialchoicelab::competition::CompetitorState;
using socialchoicelab::competition::CompetitorStrategyKind;
using socialchoicelab::competition::kMotionStepSizesStreamName;
using socialchoicelab::competition::resolve_step_size;
using socialchoicelab::competition::StepPolicyConfig;
using socialchoicelab::competition::StepPolicyKind;
using socialchoicelab::competition::StrategyDecision;
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

CompetitorState competitor(PointNd position, PointNd heading = point({0.0})) {
  CompetitorState state;
  state.id = 0;
  state.strategy_kind = CompetitorStrategyKind::kHunter;
  state.position = std::move(position);
  state.heading = std::move(heading);
  return state;
}

CompetitorRoundMetrics metrics(double vote_share) {
  CompetitorRoundMetrics result;
  result.vote_share = vote_share;
  return result;
}

}  // namespace

TEST(CompetitionMotion, FixedStepMovesAlongHeadingInOneDimension) {
  const CompetitionBounds bounds{point({-5.0}), point({5.0})};
  const CompetitorState state = competitor(point({0.0}), point({1.0}));
  const StrategyDecision decision{point({1.0})};
  const StepPolicyConfig policy{StepPolicyKind::kFixed, 1.5};

  const auto motion = advance_competitor(
      state, decision, bounds, policy, BoundaryPolicyKind::kProjectToBox,
      CompetitionObjectiveKind::kVoteShare, nullptr);

  EXPECT_DOUBLE_EQ(motion.next_position[0], 1.5);
  EXPECT_DOUBLE_EQ(motion.applied_step_size, 1.5);
}

TEST(CompetitionMotion, RandomUniformStepIsSeededAndReproducible) {
  CompetitorState state = competitor(point({0.0, 0.0}), point({1.0, 0.0}));
  const StepPolicyConfig policy{StepPolicyKind::kRandomUniform, 0.0, 0.25,
                                0.75};
  StreamManager streams_a(2026);
  StreamManager streams_b(2026);
  streams_a.register_streams({kMotionStepSizesStreamName});
  streams_b.register_streams({kMotionStepSizesStreamName});

  const double step_a = resolve_step_size(
      state, policy, CompetitionObjectiveKind::kVoteShare, &streams_a);
  const double step_b = resolve_step_size(
      state, policy, CompetitionObjectiveKind::kVoteShare, &streams_b);

  EXPECT_DOUBLE_EQ(step_a, step_b);
  EXPECT_GE(step_a, 0.25);
  EXPECT_LE(step_a, 0.75);
}

TEST(CompetitionMotion, ProportionalStepUsesObjectiveDelta) {
  CompetitorState state = competitor(point({0.0}), point({1.0}));
  state.previous_round_metrics = metrics(0.2);
  state.current_round_metrics = metrics(0.5);
  const StepPolicyConfig policy{StepPolicyKind::kShareDeltaProportional, 0.0,
                                0.0, 0.0, 2.0};

  const double step = resolve_step_size(
      state, policy, CompetitionObjectiveKind::kVoteShare, nullptr);
  EXPECT_DOUBLE_EQ(step, 0.6);
}

TEST(CompetitionMotion, ProjectToBoxClampsOutOfBoundsMove) {
  const CompetitionBounds bounds{point({-1.0}), point({1.0})};
  const auto bounded = apply_boundary_policy(point({0.5}), point({2.5}), bounds,
                                             BoundaryPolicyKind::kProjectToBox);
  EXPECT_DOUBLE_EQ(bounded[0], 1.0);
}

TEST(CompetitionMotion, StayPutRejectsOutOfBoundsMove) {
  const CompetitionBounds bounds{point({-1.0}), point({1.0})};
  const auto bounded = apply_boundary_policy(point({0.5}), point({2.5}), bounds,
                                             BoundaryPolicyKind::kStayPut);
  EXPECT_DOUBLE_EQ(bounded[0], 0.5);
}

TEST(CompetitionMotion, ReflectBouncesBackIntoBounds) {
  const CompetitionBounds bounds{point({-1.0}), point({1.0})};
  const auto bounded = apply_boundary_policy(point({0.5}), point({2.5}), bounds,
                                             BoundaryPolicyKind::kReflect);
  EXPECT_DOUBLE_EQ(bounded[0], -0.5);
}

TEST(CompetitionMotion, AdvanceCompetitorStateNormalizesHeading) {
  const CompetitionBounds bounds{point({-5.0, -5.0}), point({5.0, 5.0})};
  const CompetitorState state =
      competitor(point({0.0, 0.0}), point({0.0, 0.0}));
  const StrategyDecision decision{point({3.0, 4.0})};
  const StepPolicyConfig policy{StepPolicyKind::kFixed, 2.0};

  const auto updated = advance_competitor_state(
      state, decision, bounds, policy, BoundaryPolicyKind::kProjectToBox,
      CompetitionObjectiveKind::kVoteShare, nullptr);

  EXPECT_NEAR(updated.heading[0], 0.6, 1e-12);
  EXPECT_NEAR(updated.heading[1], 0.8, 1e-12);
  EXPECT_NEAR(updated.position[0], 1.2, 1e-12);
  EXPECT_NEAR(updated.position[1], 1.6, 1e-12);
  EXPECT_DOUBLE_EQ(updated.current_step_size, 2.0);
}
