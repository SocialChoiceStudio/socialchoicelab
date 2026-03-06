// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

// Tests for Phase F3: copeland_scores, copeland_winner (copeland.h).
// Also covers weighted_majority_prefers (majority.h, F5).

#include <gtest/gtest.h>
#include <socialchoicelab/geometry/copeland.h>
#include <socialchoicelab/geometry/majority.h>

#include <numeric>

using namespace socialchoicelab::geometry;
using socialchoicelab::core::types::Point2d;

static const DistConfig kEuc{};

// ---------------------------------------------------------------------------
// copeland_scores
// ---------------------------------------------------------------------------

TEST(CopelandScores, EmptyAlternatives_Throws) {
  const std::vector<Point2d> voters = {{0.0, 0.0}, {1.0, 0.0}};
  EXPECT_THROW(copeland_scores({}, voters), std::invalid_argument);
}

TEST(CopelandScores, EmptyVoters_Throws) {
  const std::vector<Point2d> alts = {{0.0, 0.0}};
  EXPECT_THROW(copeland_scores(alts, {}), std::invalid_argument);
}

TEST(CopelandScores, SingleAlternative_ScoreIsZero) {
  const std::vector<Point2d> alts = {{0.5, 0.5}};
  const std::vector<Point2d> voters = {{0.0, 0.0}, {1.0, 0.0}};
  auto scores = copeland_scores(alts, voters);
  ASSERT_EQ(scores.size(), 1u);
  EXPECT_EQ(scores[0], 0);
}

TEST(CopelandScores, CondorcetCycle_AllScoresZero) {
  // Genuine spatial Condorcet cycle (same configuration as
  // CondorcetCycle_NoWinner in test_core.cpp):
  //   A=(2,0), B=(0,2), C=(-2,0);  V1=(1,0.5), V2=(-0.5,1.5), V3=(-0.5,-1)
  //   A beats B (V1+V3), B beats C (V1+V2), C beats A (V2+V3) → cycle.
  //   Copeland score for each: +1 (beat) - 1 (beaten) = 0.
  const std::vector<Point2d> alts = {{2.0, 0.0}, {0.0, 2.0}, {-2.0, 0.0}};
  const std::vector<Point2d> voters = {{1.0, 0.5}, {-0.5, 1.5}, {-0.5, -1.0}};
  auto scores = copeland_scores(alts, voters);
  ASSERT_EQ(scores.size(), 3u);
  for (int s : scores) {
    EXPECT_EQ(s, 0) << "Expected zero Copeland score in a Condorcet cycle.";
  }
}

TEST(CopelandScores, CondorcetWinner_HasMaxScore) {
  // Two voters clustered near A=(0,0); one near B=(5,5).
  // A beats B and C under simple majority → score(A) = 2, others ≤ 0.
  const std::vector<Point2d> alts = {{0.0, 0.0}, {5.0, 5.0}, {5.0, -5.0}};
  const std::vector<Point2d> voters = {{0.1, 0.0}, {-0.1, 0.0}, {5.1, 5.0}};
  auto scores = copeland_scores(alts, voters);
  ASSERT_EQ(scores.size(), 3u);
  // A beats B and C.
  EXPECT_EQ(scores[0], 2);   // A
  EXPECT_LE(scores[1], 0);   // B
  EXPECT_LE(scores[2], -1);  // C
}

TEST(CopelandScores, ScoresSumIsZero) {
  // For any configuration, the total Copeland score is zero
  // (each pairwise comparison contributes +1 to one and −1 to the other).
  const std::vector<Point2d> alts = {
    {0.0, 0.0}, {1.0, 0.0}, {0.5, 0.866}, {0.5, 0.2}};
  const std::vector<Point2d> voters = {{0.0, 0.0}, {1.0, 0.0}, {0.5, 0.5}};
  auto scores = copeland_scores(alts, voters, kEuc, -1);
  const int total = std::accumulate(scores.begin(), scores.end(), 0);
  EXPECT_EQ(total, 0);
}

TEST(CopelandScores, Supermajority_ChangesScores) {
  // Under unanimity (k = n = 3), an alternative can be hard to beat.
  // With all 3 voters near (0,0), the alternative (0,0) unanimously beats
  // (5,5) and (5,-5).
  const std::vector<Point2d> alts = {{0.0, 0.0}, {5.0, 5.0}, {5.0, -5.0}};
  const std::vector<Point2d> voters = {{0.0, 0.0}, {0.1, 0.0}, {-0.1, 0.0}};
  const int k_unanimity = 3;
  auto scores = copeland_scores(alts, voters, kEuc, k_unanimity);
  // (0,0) unanimously beats both others.
  EXPECT_EQ(scores[0], 2);
}

// ---------------------------------------------------------------------------
// copeland_winner
// ---------------------------------------------------------------------------

TEST(CopelandWinner, CondorcetWinnerIsReturned) {
  const std::vector<Point2d> alts = {{0.0, 0.0}, {5.0, 5.0}, {5.0, -5.0}};
  const std::vector<Point2d> voters = {{0.1, 0.0}, {-0.1, 0.0}, {5.1, 5.0}};
  const Point2d winner = copeland_winner(alts, voters);
  EXPECT_NEAR(winner.x(), 0.0, 1e-9);
  EXPECT_NEAR(winner.y(), 0.0, 1e-9);
}

TEST(CopelandWinner, TieBreaking_ReturnsFirstInVector) {
  // In a Condorcet cycle all scores are 0; first alternative is returned.
  const std::vector<Point2d> alts = {{2.0, 0.0}, {0.0, 2.0}, {-2.0, 0.0}};
  const std::vector<Point2d> voters = {{1.0, 0.5}, {-0.5, 1.5}, {-0.5, -1.0}};
  const Point2d winner = copeland_winner(alts, voters);
  // All scores are 0 in a cycle → first element (A=(2,0)) is returned.
  EXPECT_NEAR(winner.x(), 2.0, 1e-9);
  EXPECT_NEAR(winner.y(), 0.0, 1e-9);
}

// ---------------------------------------------------------------------------
// F5 — weighted_majority_prefers
// ---------------------------------------------------------------------------

TEST(WeightedMajority, UniformWeights_MatchesMajorityPrefers) {
  // 3 voters at (0,0), (2,0), (1,2).
  // x=(0.5, 0) vs y=(1.5, 0).  V1 prefers x (closer), V2 prefers y,
  // V3 is equidistant (d=sqrt(0.25+4) ≈ 2.06 vs d=sqrt(0.25+4) ≈ 2.06 —
  // actually V3=(1,2): d(V3,x)=sqrt(0.25+4)=2.06, d(V3,y)=sqrt(0.25+4)=2.06.
  // Tie for V3.  Majority: 1 for x, 1 for y, 1 tie → no majority.
  // Use x=(0.1,0), y=(1.0,0):
  //   d(V1,x)=0.1, d(V1,y)=1.0 → V1 prefers x.
  //   d(V2,x)=1.9, d(V2,y)=1.0 → V2 prefers y.
  //   d(V3,x)=sqrt(0.81+4)≈2.19, d(V3,y)=sqrt(0+4)=2 → V3 prefers y.
  // Majority: 1 for x, 2 for y → simple majority prefers y.
  const std::vector<Point2d> voters = {{0.0, 0.0}, {2.0, 0.0}, {1.0, 2.0}};
  const Point2d x{0.1, 0.0};
  const Point2d y{1.0, 0.0};
  const std::vector<double> equal_w = {1.0, 1.0, 1.0};

  bool std_result = majority_prefers(x, y, voters, kEuc);
  bool wt_result = weighted_majority_prefers(x, y, voters, equal_w);
  EXPECT_EQ(std_result, wt_result);

  // Also check the reverse direction.
  bool std_rev = majority_prefers(y, x, voters, kEuc);
  bool wt_rev = weighted_majority_prefers(y, x, voters, equal_w);
  EXPECT_EQ(std_rev, wt_rev);
}

TEST(WeightedMajority, DoubleWeightChangesOutcome) {
  // V1=(0,0), V2=(0,0.1), V3=(3,0).  x=(0.5,0), y=(2.5,0).
  //   d(V1,x)=0.5, d(V1,y)=2.5 → V1 prefers x.  (weight 1)
  //   d(V2,x)≈0.51, d(V2,y)≈2.50 → V2 prefers x.  (weight 1)
  //   d(V3,x)=2.5, d(V3,y)=0.5 → V3 prefers y.
  //
  // Equal weights {1,1,1}: x_weight=2, total=3, threshold=1.5 → x wins.
  // Triple-weight V3 {1,1,3}: x_weight=2, y_weight=3, total=5, threshold=2.5
  //   → x_weight=2 < 2.5 (x loses), y_weight=3 >= 2.5 (y wins).
  const std::vector<Point2d> voters = {{0.0, 0.0}, {0.0, 0.1}, {3.0, 0.0}};
  const Point2d x{0.5, 0.0};
  const Point2d y{2.5, 0.0};
  const std::vector<double> equal_w = {1.0, 1.0, 1.0};
  const std::vector<double> heavy_v3 = {1.0, 1.0, 3.0};

  // Equal weights: majority prefers x to y.
  EXPECT_TRUE(weighted_majority_prefers(x, y, voters, equal_w));

  // Triple-weight V3: y wins, x loses.
  EXPECT_FALSE(weighted_majority_prefers(x, y, voters, heavy_v3));
  EXPECT_TRUE(weighted_majority_prefers(y, x, voters, heavy_v3));
}

TEST(WeightedMajority, ThresholdAtExactly50Percent) {
  // 2 voters: V1=(0,0), V2=(2,0), equal weights = 1.
  // x=(0.5,0), y=(1.5,0): V1 prefers x (d=0.5 < d=1.5), V2 prefers y.
  // total=2, x_weight=1.  threshold=0.5 → need x_weight >= 0.5*2 = 1.0.
  // 1.0 >= 1.0 → true.  (Tie is resolved in favour of x at exactly 50%.)
  const std::vector<Point2d> voters = {{0.0, 0.0}, {2.0, 0.0}};
  const std::vector<double> equal_w = {1.0, 1.0};
  const Point2d x{0.5, 0.0};
  const Point2d y{1.5, 0.0};

  EXPECT_TRUE(weighted_majority_prefers(x, y, voters, equal_w, kEuc, 0.5));
  // Both sides have exactly 50% → x returns true, y returns true too.
  EXPECT_TRUE(weighted_majority_prefers(y, x, voters, equal_w, kEuc, 0.5));
}

TEST(WeightedMajority, InvalidInputs_Throw) {
  const std::vector<Point2d> voters = {{0.0, 0.0}, {1.0, 0.0}};
  const Point2d x{0.0, 0.0};
  const Point2d y{1.0, 0.0};

  EXPECT_THROW(weighted_majority_prefers(x, y, voters, {1.0}),
               std::invalid_argument);  // wrong length
  EXPECT_THROW(weighted_majority_prefers(x, y, voters, {-1.0, 1.0}),
               std::invalid_argument);  // non-positive weight
  EXPECT_THROW(weighted_majority_prefers(x, y, voters, {1.0, 1.0}, kEuc, 0.0),
               std::invalid_argument);  // zero threshold
  EXPECT_THROW(weighted_majority_prefers(x, y, voters, {1.0, 1.0}, kEuc, 1.5),
               std::invalid_argument);  // threshold > 1
}
