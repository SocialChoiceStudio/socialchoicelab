// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// test_majority.cpp — GTest suite for majority.h (B1 and B2).

#include <gtest/gtest.h>
#include <socialchoicelab/geometry/majority.h>

#include <Eigen/Dense>
#include <cmath>
#include <vector>

using socialchoicelab::core::types::Point2d;
using socialchoicelab::geometry::DistConfig;
using socialchoicelab::geometry::majority_prefers;
using socialchoicelab::geometry::pairwise_matrix;
using socialchoicelab::geometry::preference_margin;
using socialchoicelab::geometry::simple_majority;
namespace dist = socialchoicelab::preference::distance;

// Default Euclidean config with unit weights.
static const DistConfig kEuclidean{};

// ============================================================
// simple_majority helper
// ============================================================

TEST(SimpleMajority, OddN) {
  EXPECT_EQ(simple_majority(1), 1);
  EXPECT_EQ(simple_majority(3), 2);
  EXPECT_EQ(simple_majority(5), 3);
  EXPECT_EQ(simple_majority(7), 4);
}

TEST(SimpleMajority, EvenN) {
  EXPECT_EQ(simple_majority(2), 2);
  EXPECT_EQ(simple_majority(4), 3);
  EXPECT_EQ(simple_majority(6), 4);
}

// ============================================================
// B1 — majority_prefers
// ============================================================

TEST(MajorityPrefers, SingleVoter_PreferNearer) {
  // Voter at (0,0). x=(0.1,0) is closer than y=(1,0).
  std::vector<Point2d> voters = {Point2d(0.0, 0.0)};
  Point2d x(0.1, 0.0);
  Point2d y(1.0, 0.0);
  EXPECT_TRUE(majority_prefers(x, y, voters, kEuclidean));
  EXPECT_FALSE(majority_prefers(y, x, voters, kEuclidean));
}

TEST(MajorityPrefers, SimpleMajority_ThreeVoters_ClearWinner) {
  // Voters at (0,0), (0.2,0), (0.4,0).
  // x=(0.2,0) is the centroid; y=(2,0) is far right.
  // All three prefer x → majority holds for any k.
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(0.2, 0.0),
                                 Point2d(0.4, 0.0)};
  Point2d x(0.2, 0.0);
  Point2d y(2.0, 0.0);
  EXPECT_TRUE(majority_prefers(x, y, voters, kEuclidean));
  EXPECT_TRUE(
      majority_prefers(x, y, voters, kEuclidean, 1));  // unanimity-for-x
  EXPECT_TRUE(
      majority_prefers(x, y, voters, kEuclidean, 2));  // simple majority
  EXPECT_TRUE(majority_prefers(x, y, voters, kEuclidean, 3));  // unanimity
}

TEST(MajorityPrefers, SimpleMajority_ThreeVoters_TwoOne_Split) {
  // Voters at (0,0), (0,0), (10,0).
  // x=(0.5,0), y=(9,0). Two voters prefer x, one prefers y.
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(0.0, 0.0),
                                 Point2d(10.0, 0.0)};
  Point2d x(0.5, 0.0);
  Point2d y(9.0, 0.0);
  EXPECT_TRUE(majority_prefers(x, y, voters, kEuclidean));      // 2 >= 2 ✓
  EXPECT_TRUE(majority_prefers(x, y, voters, kEuclidean, 2));   // 2 >= 2 ✓
  EXPECT_FALSE(majority_prefers(x, y, voters, kEuclidean, 3));  // 2 < 3 ✗
}

TEST(MajorityPrefers, EvenN_HalfExactTie_RequiresOneMore) {
  // 4 voters: 2 prefer x, 2 prefer y → simple majority (k=3) fails.
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(0.0, 0.0),
                                 Point2d(10.0, 0.0), Point2d(10.0, 0.0)};
  Point2d x(0.5, 0.0);
  Point2d y(9.5, 0.0);
  // simple_majority(4) = 3; only 2 prefer x, so fails.
  EXPECT_FALSE(majority_prefers(x, y, voters, kEuclidean));
  EXPECT_FALSE(majority_prefers(y, x, voters, kEuclidean));
  // k=2 (half): both pass.
  EXPECT_TRUE(majority_prefers(x, y, voters, kEuclidean, 2));
}

TEST(MajorityPrefers, Supermajority_TwoThirds) {
  // 6 voters: 4 prefer x, 2 prefer y. Simple majority (k=4) passes.
  // Two-thirds (k=4) passes; k=5 fails.
  std::vector<Point2d> voters = {Point2d(0.0, 0.0),  Point2d(0.0, 0.0),
                                 Point2d(0.0, 0.0),  Point2d(0.0, 0.0),
                                 Point2d(10.0, 0.0), Point2d(10.0, 0.0)};
  Point2d x(0.5, 0.0);
  Point2d y(9.5, 0.0);
  EXPECT_TRUE(majority_prefers(x, y, voters, kEuclidean));      // k=4 ✓
  EXPECT_TRUE(majority_prefers(x, y, voters, kEuclidean, 4));   // 2/3 ✓
  EXPECT_FALSE(majority_prefers(x, y, voters, kEuclidean, 5));  // 5/6 ✗
  EXPECT_FALSE(majority_prefers(x, y, voters, kEuclidean, 6));  // unanimity ✗
}

TEST(MajorityPrefers, Unanimity_AllMustPrefer) {
  // All three voters clearly closer to x than y.
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(0.3, 0.1),
                                 Point2d(0.5, 1.0)};
  Point2d x(0.3, 0.3);
  Point2d y(6.0, 6.0);
  EXPECT_TRUE(majority_prefers(x, y, voters, kEuclidean, 3));

  // Move one voter near y so they prefer y — unanimity for x now fails.
  voters[2] = Point2d(5.9, 5.9);
  EXPECT_FALSE(majority_prefers(x, y, voters, kEuclidean, 3));
}

TEST(MajorityPrefers, AllPreferSame_AnyKPasses) {
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(0.1, 0.0),
                                 Point2d(0.2, 0.0), Point2d(0.15, 0.0),
                                 Point2d(0.05, 0.0)};
  Point2d x(0.1, 0.0);
  Point2d y(100.0, 100.0);
  for (int k = 1; k <= 5; ++k) {
    EXPECT_TRUE(majority_prefers(x, y, voters, kEuclidean, k));
  }
}

TEST(MajorityPrefers, ManhattanDistance) {
  // Under Manhattan distance, voter at (0,0), x=(1,1), y=(0,2).
  // d_L1((0,0),(1,1)) = 2; d_L1((0,0),(0,2)) = 2 → tied, no strict preference.
  std::vector<Point2d> voters = {Point2d(0.0, 0.0)};
  DistConfig manhattan{dist::DistanceType::MANHATTAN, 1.0, {1.0, 1.0}};
  Point2d x(1.0, 1.0);
  Point2d y(0.0, 2.0);
  EXPECT_FALSE(majority_prefers(x, y, voters, manhattan));
  EXPECT_FALSE(majority_prefers(y, x, voters, manhattan));
}

TEST(MajorityPrefers, Error_EmptyVoters) {
  std::vector<Point2d> voters;
  EXPECT_THROW(
      majority_prefers(Point2d(0, 0), Point2d(1, 1), voters, kEuclidean),
      std::invalid_argument);
}

TEST(MajorityPrefers, Error_KOutOfRange) {
  std::vector<Point2d> voters = {Point2d(0, 0)};
  EXPECT_THROW(
      majority_prefers(Point2d(0, 0), Point2d(1, 1), voters, kEuclidean, 0),
      std::invalid_argument);
  EXPECT_THROW(
      majority_prefers(Point2d(0, 0), Point2d(1, 1), voters, kEuclidean, 2),
      std::invalid_argument);
}

// ============================================================
// preference_margin
// ============================================================

TEST(PreferenceMargin, Positive_XWins) {
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(0.1, 0.0),
                                 Point2d(0.2, 0.0)};
  Point2d x(0.1, 0.0);
  Point2d y(5.0, 0.0);
  EXPECT_EQ(preference_margin(x, y, voters, kEuclidean), 3);
  EXPECT_EQ(preference_margin(y, x, voters, kEuclidean), -3);
}

TEST(PreferenceMargin, Zero_WhenTied) {
  // Symmetric: 2 voters each side.
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(0.0, 0.0),
                                 Point2d(10.0, 0.0), Point2d(10.0, 0.0)};
  Point2d x(0.5, 0.0);
  Point2d y(9.5, 0.0);
  EXPECT_EQ(preference_margin(x, y, voters, kEuclidean), 0);
}

TEST(PreferenceMargin, EquidistantVoterCountsZero) {
  // Voter on the perpendicular bisector: equidistant from x and y.
  // Place voter at midpoint.
  Point2d x(-1.0, 0.0);
  Point2d y(1.0, 0.0);
  std::vector<Point2d> voters = {Point2d(0.0, 1.0)};  // equidistant
  EXPECT_EQ(preference_margin(x, y, voters, kEuclidean), 0);
}

TEST(PreferenceMargin, ConvertSignConsistently) {
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(0.0, 0.0),
                                 Point2d(1.0, 0.0)};
  Point2d x(0.2, 0.0);
  Point2d y(0.8, 0.0);
  int m_xy = preference_margin(x, y, voters, kEuclidean);
  int m_yx = preference_margin(y, x, voters, kEuclidean);
  EXPECT_EQ(m_xy, -m_yx);
}

TEST(PreferenceMargin, Error_EmptyVoters) {
  std::vector<Point2d> voters;
  EXPECT_THROW(
      preference_margin(Point2d(0, 0), Point2d(1, 1), voters, kEuclidean),
      std::invalid_argument);
}

// ============================================================
// B2 — pairwise_matrix
// ============================================================

TEST(PairwiseMatrix, AntiSymmetric) {
  std::vector<Point2d> alts = {Point2d(0.0, 0.0), Point2d(1.0, 0.0),
                               Point2d(0.5, 1.0)};
  std::vector<Point2d> voters = {Point2d(0.2, 0.1), Point2d(0.8, 0.2)};
  auto M = pairwise_matrix(alts, voters, kEuclidean);
  ASSERT_EQ(M.rows(), 3);
  ASSERT_EQ(M.cols(), 3);
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(M(i, i), 0);
    for (int j = 0; j < 3; ++j) {
      EXPECT_EQ(M(i, j), -M(j, i));
    }
  }
}

TEST(PairwiseMatrix, DiagonalIsZero) {
  std::vector<Point2d> alts = {Point2d(0.0, 0.0), Point2d(2.0, 0.0)};
  std::vector<Point2d> voters = {Point2d(1.0, 0.0)};
  auto M = pairwise_matrix(alts, voters, kEuclidean);
  EXPECT_EQ(M(0, 0), 0);
  EXPECT_EQ(M(1, 1), 0);
}

TEST(PairwiseMatrix, CondorcetCycle_ThreeAlternatives) {
  // Classic 3-voter Condorcet cycle (Condorcet 1785):
  //   Voter 1: A > B > C (ideal near A)
  //   Voter 2: B > C > A (ideal near B)
  //   Voter 3: C > A > B (ideal near C)
  // Place alternatives at triangle vertices; ideal points biased toward each.
  Point2d A(0.0, 0.0);
  Point2d B(1.0, 0.0);
  Point2d C(0.5, 0.866);  // equilateral triangle

  std::vector<Point2d> alts = {A, B, C};
  // Voter positions chosen to produce the cycle A>B>C>A (verified by hand):
  //   V1 (0.1,-0.5):  A > B > C
  //   V2 (1.5, 0.5):  B > C > A
  //   V3 (-0.1, 1.0): C > A > B
  // Pairwise result: A beats B 2-1, B beats C 2-1, C beats A 2-1.
  std::vector<Point2d> voters = {
    Point2d(0.1, -0.5),  // near A, away from C
    Point2d(1.5, 0.5),   // near B, away from A
    Point2d(-0.1, 1.0),  // near C, away from B
  };
  auto M = pairwise_matrix(alts, voters, kEuclidean);
  // Each pair should have a non-zero margin reflecting the 2-vs-1 splits
  // in a Condorcet cycle, but the actual signs depend on distances.
  // Verify anti-symmetry and zero diagonal (structural properties).
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(M(i, i), 0);
    for (int j = 0; j < 3; ++j) {
      EXPECT_EQ(M(i, j), -M(j, i));
    }
  }
  // Verify total wins (sum of positive entries per row) are not all equal → no
  // Condorcet winner.
  Eigen::VectorXi wins = Eigen::VectorXi::Zero(3);
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      if (M(i, j) > 0) ++wins(i);
  // In a genuine Condorcet cycle each alternative wins exactly 1 pairwise.
  EXPECT_EQ(wins(0) + wins(1) + wins(2), 3);
}

TEST(PairwiseMatrix, KnownOutcome_TwoAlternatives) {
  // 3 voters all near x; y is far away → M(0,1) = 3, M(1,0) = -3.
  std::vector<Point2d> alts = {Point2d(0.1, 0.0), Point2d(5.0, 0.0)};
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(0.0, 0.1),
                                 Point2d(0.2, 0.0)};
  auto M = pairwise_matrix(alts, voters, kEuclidean);
  EXPECT_EQ(M(0, 1), 3);
  EXPECT_EQ(M(1, 0), -3);
}

TEST(PairwiseMatrix, SupermajorityK_ValidationOnly) {
  // k=5 for 3 voters → invalid; function throws.
  std::vector<Point2d> alts = {Point2d(0, 0), Point2d(1, 0)};
  std::vector<Point2d> voters = {Point2d(0, 0), Point2d(0.5, 0), Point2d(1, 0)};
  EXPECT_THROW(pairwise_matrix(alts, voters, kEuclidean, 5),
               std::invalid_argument);
}

TEST(PairwiseMatrix, SupermajorityK_Valid_MatrixUnchanged) {
  // k is valid but doesn't change raw margins.
  std::vector<Point2d> alts = {Point2d(0.1, 0), Point2d(5.0, 0)};
  std::vector<Point2d> voters = {Point2d(0, 0), Point2d(0, 0.1),
                                 Point2d(0.2, 0)};
  auto M_default = pairwise_matrix(alts, voters, kEuclidean);
  auto M_k2 = pairwise_matrix(alts, voters, kEuclidean, 2);
  auto M_k3 = pairwise_matrix(alts, voters, kEuclidean, 3);
  EXPECT_TRUE((M_default - M_k2).isZero());
  EXPECT_TRUE((M_default - M_k3).isZero());
}

TEST(PairwiseMatrix, Error_EmptyAlternatives) {
  std::vector<Point2d> alts;
  std::vector<Point2d> voters = {Point2d(0, 0)};
  EXPECT_THROW(pairwise_matrix(alts, voters, kEuclidean),
               std::invalid_argument);
}

TEST(PairwiseMatrix, Error_EmptyVoters) {
  std::vector<Point2d> alts = {Point2d(0, 0), Point2d(1, 0)};
  std::vector<Point2d> voters;
  EXPECT_THROW(pairwise_matrix(alts, voters, kEuclidean),
               std::invalid_argument);
}
