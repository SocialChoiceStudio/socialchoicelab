// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <socialchoicelab/competition/election_feedback.h>

#include <utility>
#include <vector>

using socialchoicelab::competition::apply_feedback_to_competitors;
using socialchoicelab::competition::CompetitorState;
using socialchoicelab::competition::CompetitorStrategyKind;
using socialchoicelab::competition::ElectionFeedbackConfig;
using socialchoicelab::competition::evaluate_election_feedback;
using socialchoicelab::competition::SeatRuleKind;
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

}  // namespace

TEST(CompetitionElectionFeedback,
     OneDimensionalPluralityFeedbackMatchesExpectations) {
  ElectionFeedbackConfig config;
  config.distance_config = DistConfig{};
  config.distance_config.salience_weights = {1.0};
  config.seat_count = 2;
  config.seat_rule = SeatRuleKind::kPluralityTopK;

  const std::vector<PointNd> competitors = {point({0.0}), point({2.0}),
                                            point({5.0})};
  const std::vector<PointNd> voters = {point({0.1}), point({1.0}), point({2.2}),
                                       point({4.5})};

  const auto feedback = evaluate_election_feedback(competitors, voters, config);

  EXPECT_EQ(feedback.vote_totals, (std::vector<int>{2, 1, 1}));
  EXPECT_EQ(feedback.seats, (std::vector<int>{1, 1, 0}));
  EXPECT_EQ(feedback.leader_ranking, (std::vector<int>{0, 1, 2}));
  ASSERT_EQ(feedback.supporter_indices.size(), 3u);
  EXPECT_EQ(feedback.supporter_indices[0], (std::vector<int>{0, 1}));
  EXPECT_EQ(feedback.supporter_indices[1], (std::vector<int>{2}));
  EXPECT_EQ(feedback.supporter_indices[2], (std::vector<int>{3}));
  EXPECT_DOUBLE_EQ(feedback.vote_shares[0], 0.5);
  EXPECT_DOUBLE_EQ(feedback.seat_shares[0], 0.5);
}

TEST(CompetitionElectionFeedback,
     TwoDimensionalHareFeedbackMatchesExpectations) {
  ElectionFeedbackConfig config;
  config.distance_config = DistConfig{};
  config.distance_config.salience_weights = {1.0, 1.0};
  config.seat_count = 4;
  config.seat_rule = SeatRuleKind::kHareLargestRemainder;

  const std::vector<PointNd> competitors = {
    point({0.0, 0.0}), point({4.0, 0.0}), point({2.0, 3.0})};
  const std::vector<PointNd> voters = {point({0.2, 0.1}), point({0.1, 0.0}),
                                       point({3.9, 0.1}), point({4.1, 0.0}),
                                       point({2.0, 2.8}), point({2.1, 2.9})};

  const auto feedback = evaluate_election_feedback(competitors, voters, config);

  EXPECT_EQ(feedback.vote_totals, (std::vector<int>{2, 2, 2}));
  EXPECT_EQ(feedback.seats, (std::vector<int>{2, 1, 1}));
  EXPECT_EQ(feedback.leader_ranking, (std::vector<int>{0, 1, 2}));
  EXPECT_EQ(feedback.supporter_indices[2], (std::vector<int>{4, 5}));
}

TEST(CompetitionElectionFeedback,
     ApplyingFeedbackUpdatesCurrentAndPreviousMetrics) {
  ElectionFeedbackConfig config;
  config.distance_config = DistConfig{};
  config.distance_config.salience_weights = {1.0};
  config.seat_count = 1;
  config.seat_rule = SeatRuleKind::kPluralityTopK;

  const std::vector<PointNd> positions = {point({0.0}), point({3.0})};
  const std::vector<PointNd> voters = {point({0.1}), point({2.9}),
                                       point({2.8})};
  const auto feedback = evaluate_election_feedback(positions, voters, config);

  std::vector<CompetitorState> competitors = {competitor(0, point({0.0})),
                                              competitor(1, point({3.0}))};

  auto updated = apply_feedback_to_competitors(competitors, feedback);
  ASSERT_TRUE(updated[0].current_round_metrics.has_value());
  EXPECT_EQ(updated[0].current_round_metrics->vote_total, 1);
  EXPECT_EQ(updated[1].current_round_metrics->vote_total, 2);

  const std::vector<PointNd> voters_round_two = {point({0.0}), point({0.2}),
                                                 point({0.1})};
  const auto feedback_two =
      evaluate_election_feedback(positions, voters_round_two, config);
  updated = apply_feedback_to_competitors(updated, feedback_two);

  ASSERT_TRUE(updated[0].previous_round_metrics.has_value());
  EXPECT_EQ(updated[0].previous_round_metrics->vote_total, 1);
  EXPECT_EQ(updated[0].current_round_metrics->vote_total, 3);
}
