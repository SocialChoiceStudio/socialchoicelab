// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <socialchoicelab/competition/experiment_runner.h>

#include <utility>
#include <vector>

using socialchoicelab::competition::BoundaryPolicyKind;
using socialchoicelab::competition::CompetitionEngineConfig;
using socialchoicelab::competition::CompetitionExperimentConfig;
using socialchoicelab::competition::CompetitionObjectiveKind;
using socialchoicelab::competition::CompetitionScenario;
using socialchoicelab::competition::CompetitorState;
using socialchoicelab::competition::CompetitorStrategyKind;
using socialchoicelab::competition::ElectionFeedbackConfig;
using socialchoicelab::competition::run_competition_experiment;
using socialchoicelab::competition::run_competition_sweep;
using socialchoicelab::competition::SeatRuleKind;
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

CompetitorState competitor(int id, CompetitorStrategyKind kind,
                           PointNd position) {
  CompetitorState state;
  state.id = id;
  state.strategy_kind = kind;
  state.position = std::move(position);
  state.heading = PointNd::Zero(state.position.size());
  return state;
}

CompetitionExperimentConfig experiment_config() {
  CompetitionExperimentConfig config;
  config.engine_config.bounds = {point({-5.0}), point({5.0})};
  config.engine_config.election_feedback = ElectionFeedbackConfig{
    DistConfig{socialchoicelab::preference::distance::DistanceType::EUCLIDEAN,
               2.0,
               {1.0}},
    1, SeatRuleKind::kPluralityTopK};
  config.engine_config.step_policy =
      StepPolicyConfig{StepPolicyKind::kFixed, 1.0};
  config.engine_config.boundary_policy = BoundaryPolicyKind::kProjectToBox;
  config.engine_config.objective_kind = CompetitionObjectiveKind::kVoteShare;
  config.engine_config.max_rounds = 3;
  config.initial_competitors = {
    competitor(0, CompetitorStrategyKind::kSticker, point({0.0})),
    competitor(1, CompetitorStrategyKind::kSticker, point({3.0}))};
  config.voter_ideals = {point({0.1}), point({0.2}), point({2.9})};
  config.master_seed = 2026;
  config.num_runs = 3;
  config.retain_traces = false;
  return config;
}

}  // namespace

TEST(CompetitionExperimentRunner, SerialExperimentReturnsRunSummariesAndMeans) {
  const auto config = experiment_config();
  const auto result = run_competition_experiment(config);

  ASSERT_EQ(result.run_summaries.size(), 3u);
  EXPECT_TRUE(result.traces.empty());
  EXPECT_EQ(result.summary.num_runs, 3);
  EXPECT_DOUBLE_EQ(result.summary.mean_rounds_completed, 3.0);
  EXPECT_DOUBLE_EQ(result.summary.mean_final_vote_shares[0], 2.0 / 3.0);
  EXPECT_DOUBLE_EQ(result.summary.mean_final_seat_shares[0], 1.0);
}

TEST(CompetitionExperimentRunner, TraceRetentionKeepsPerRunTraces) {
  auto config = experiment_config();
  config.retain_traces = true;
  config.num_runs = 2;

  const auto result = run_competition_experiment(config);
  ASSERT_EQ(result.traces.size(), 2u);
  EXPECT_EQ(result.traces[0].rounds.size(), 3u);
}

TEST(CompetitionExperimentRunner,
     RepeatedExperimentIsReproducibleFromMasterSeed) {
  const auto config = experiment_config();

  const auto result_a = run_competition_experiment(config);
  const auto result_b = run_competition_experiment(config);

  ASSERT_EQ(result_a.run_summaries.size(), result_b.run_summaries.size());
  for (size_t i = 0; i < result_a.run_summaries.size(); ++i) {
    EXPECT_EQ(result_a.run_summaries[i].run_seed,
              result_b.run_summaries[i].run_seed);
    EXPECT_EQ(result_a.run_summaries[i].rounds_completed,
              result_b.run_summaries[i].rounds_completed);
    EXPECT_EQ(result_a.run_summaries[i].termination_reason,
              result_b.run_summaries[i].termination_reason);
    EXPECT_DOUBLE_EQ(result_a.run_summaries[i].final_vote_shares[0],
                     result_b.run_summaries[i].final_vote_shares[0]);
  }
}

TEST(CompetitionExperimentRunner, SweepRunsMultipleScenarios) {
  auto config_a = experiment_config();
  auto config_b = experiment_config();
  config_b.engine_config.election_feedback.seat_rule =
      SeatRuleKind::kHareLargestRemainder;
  config_b.engine_config.election_feedback.seat_count = 2;

  const auto sweep = run_competition_sweep({
    {"plurality", config_a},
    {"hare", config_b},
  });

  ASSERT_EQ(sweep.results.size(), 2u);
  EXPECT_EQ(sweep.scenarios[0].label, "plurality");
  EXPECT_EQ(sweep.scenarios[1].label, "hare");
  EXPECT_EQ(sweep.results[0].summary.num_runs, 3);
  EXPECT_EQ(sweep.results[1].summary.num_runs, 3);
}

TEST(CompetitionExperimentRunner, SummaryTracksEarlyTerminationRate) {
  auto config = experiment_config();
  config.engine_config.termination.stop_on_convergence = true;
  config.engine_config.termination.position_epsilon = 1e-12;

  const auto result = run_competition_experiment(config);

  EXPECT_DOUBLE_EQ(result.summary.early_termination_rate, 1.0);
  EXPECT_EQ(result.summary.termination_reason_counts[static_cast<int>(
                TerminationReason::kConverged)],
            3);
}
