// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <socialchoicelab/competition/engine.h>
#include <socialchoicelab/competition/termination.h>

#include <locale>
#include <string>
#include <utility>
#include <vector>

using socialchoicelab::competition::BoundaryPolicyKind;
using socialchoicelab::competition::CompetitionBounds;
using socialchoicelab::competition::CompetitionEngineConfig;
using socialchoicelab::competition::CompetitionObjectiveKind;
using socialchoicelab::competition::CompetitorState;
using socialchoicelab::competition::CompetitorStrategyKind;
using socialchoicelab::competition::ElectionFeedbackConfig;
using socialchoicelab::competition::has_converged_by_position;
using socialchoicelab::competition::max_position_delta;
using socialchoicelab::competition::round_has_any_improvement;
using socialchoicelab::competition::run_fixed_round_competition;
using socialchoicelab::competition::SeatRuleKind;
using socialchoicelab::competition::state_signature;
using socialchoicelab::competition::StepPolicyConfig;
using socialchoicelab::competition::StepPolicyKind;
using socialchoicelab::competition::TerminationReason;
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

CompetitorState competitor(int id, PointNd position) {
  CompetitorState state;
  state.id = id;
  state.strategy_kind = CompetitorStrategyKind::kSticker;
  state.position = std::move(position);
  state.heading = PointNd::Zero(state.position.size());
  return state;
}

CompetitionEngineConfig base_config() {
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
  config.max_rounds = 5;
  return config;
}

}  // namespace

TEST(CompetitionTermination, PositionConvergenceDetectsZeroMovement) {
  const std::vector<CompetitorState> before = {competitor(0, point({0.0}))};
  const std::vector<CompetitorState> after = {competitor(0, point({0.0}))};
  EXPECT_DOUBLE_EQ(max_position_delta(before, after), 0.0);
  EXPECT_TRUE(has_converged_by_position(before, after, 1e-9));
}

TEST(CompetitionTermination, StateSignatureRoundsDeterministically) {
  const std::vector<CompetitorState> competitors = {
    competitor(0, point({0.0000001})), competitor(1, point({1.4999999}))};
  const auto signature = state_signature(competitors, 1e-3);
  EXPECT_NE(signature.find("0:0,"), std::string::npos);
  EXPECT_NE(signature.find("1:1.5,"), std::string::npos);
}

// state_signature must produce locale-independent output regardless of what
// the global locale is set to. Some European locales (e.g. de_DE) use ',' as
// the decimal separator, which would corrupt the signature format and break
// cycle detection. We inject a comma-decimal locale portably via a custom
// numpunct facet — this does not require any system locale to be installed.
TEST(CompetitionTermination, StateSignatureIsLocaleIndependent) {
  struct CommaDecimal : std::numpunct<char> {
    char do_decimal_point() const override { return ','; }
  };

  const std::locale comma_locale(std::locale::classic(), new CommaDecimal);
  const std::locale old_locale = std::locale::global(comma_locale);

  const std::vector<CompetitorState> competitors = {
    competitor(0, point({1.5}))};
  const auto signature = state_signature(competitors, 1e-6);

  std::locale::global(old_locale);

  // Must contain '.' (classic decimal), never ',' from the injected locale.
  EXPECT_NE(signature.find("1.5"), std::string::npos);
  EXPECT_EQ(signature.find("1,5"), std::string::npos);
}

TEST(CompetitionTermination, NoImprovementDetectionReadsRoundMetrics) {
  CompetitorState state = competitor(0, point({0.0}));
  state.previous_round_metrics =
      socialchoicelab::competition::CompetitorRoundMetrics{0.5, 0.0, 0, 0};
  state.current_round_metrics =
      socialchoicelab::competition::CompetitorRoundMetrics{0.5, 0.0, 0, 0};

  EXPECT_FALSE(round_has_any_improvement(
      {state}, CompetitionObjectiveKind::kVoteShare, 1e-12));
}

TEST(CompetitionTermination, EngineStopsEarlyOnConvergence) {
  auto config = base_config();
  config.termination.stop_on_convergence = true;
  config.termination.position_epsilon = 1e-12;

  const std::vector<CompetitorState> competitors = {
    competitor(0, point({0.0})), competitor(1, point({3.0}))};
  const std::vector<PointNd> voters = {point({0.1}), point({2.9})};

  const auto trace =
      run_fixed_round_competition(config, competitors, voters, nullptr);
  EXPECT_TRUE(trace.terminated_early);
  EXPECT_EQ(trace.termination_reason, TerminationReason::kConverged);
  EXPECT_EQ(trace.rounds.size(), 1u);
}

TEST(CompetitionTermination, EngineStopsEarlyOnCycleSignature) {
  auto config = base_config();
  config.termination.stop_on_cycle = true;
  config.termination.cycle_memory = 4;
  config.termination.signature_resolution = 1e-9;

  const std::vector<CompetitorState> competitors = {
    competitor(0, point({0.0})), competitor(1, point({3.0}))};
  const std::vector<PointNd> voters = {point({0.1}), point({2.9})};

  const auto trace =
      run_fixed_round_competition(config, competitors, voters, nullptr);
  EXPECT_TRUE(trace.terminated_early);
  EXPECT_EQ(trace.termination_reason, TerminationReason::kCycleDetected);
  EXPECT_EQ(trace.rounds.size(), 1u);
}

TEST(CompetitionTermination, EngineStopsAfterNoImprovementWindow) {
  auto config = base_config();
  config.termination.stop_on_no_improvement = true;
  config.termination.no_improvement_window = 1;
  config.termination.objective_epsilon = 1e-12;

  const std::vector<CompetitorState> competitors = {
    competitor(0, point({0.0})), competitor(1, point({3.0}))};
  const std::vector<PointNd> voters = {point({0.1}), point({2.9})};

  const auto trace =
      run_fixed_round_competition(config, competitors, voters, nullptr);
  EXPECT_TRUE(trace.terminated_early);
  EXPECT_EQ(trace.termination_reason, TerminationReason::kNoImprovementWindow);
  EXPECT_EQ(trace.rounds.size(), 2u);
}
