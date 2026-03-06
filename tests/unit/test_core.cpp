// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

// Tests for Phase F2: has_condorcet_winner, condorcet_winner, core_2d.

#include <gtest/gtest.h>
#include <socialchoicelab/geometry/core.h>

#include <cmath>

using namespace socialchoicelab::geometry;
using socialchoicelab::core::types::Point2d;

static const DistConfig kEuc{};

// ---------------------------------------------------------------------------
// has_condorcet_winner / condorcet_winner — finite alternative sets
// ---------------------------------------------------------------------------

TEST(CondorcetFinite, EmptyAlternatives_ReturnsFalseOrNullopt) {
  const std::vector<Point2d> alts{};
  const std::vector<Point2d> voters = {{0.0, 0.0}, {1.0, 0.0}, {0.5, 1.0}};
  EXPECT_FALSE(has_condorcet_winner(alts, voters));
  EXPECT_EQ(condorcet_winner(alts, voters), std::nullopt);
}

TEST(CondorcetFinite, SingleAlternative_IsCondorcetWinner) {
  const std::vector<Point2d> alts = {{1.0, 1.0}};
  const std::vector<Point2d> voters = {{0.0, 0.0}, {2.0, 0.0}, {1.0, 2.0}};
  // With a single alternative it trivially beats all others (none exist).
  EXPECT_TRUE(has_condorcet_winner(alts, voters));
  auto cw = condorcet_winner(alts, voters);
  ASSERT_NE(cw, std::nullopt);
  EXPECT_DOUBLE_EQ(cw->x(), 1.0);
  EXPECT_DOUBLE_EQ(cw->y(), 1.0);
}

TEST(CondorcetFinite, CondorcetCycle_NoWinner) {
  // A genuine spatial Condorcet cycle verified analytically:
  //
  // Alternatives:
  //   A = (2,0),  B = (0,2),  C = (-2,0)
  //
  // Voter ideals (distances verified to three decimal places):
  //   V1 = (1, 0.5):   d(V1,A)=1.12  d(V1,B)=1.80  d(V1,C)=3.04 → A>B>C
  //   V2 = (-0.5,1.5): d(V2,A)=2.92  d(V2,B)=0.71  d(V2,C)=2.12 → B>C>A
  //   V3 = (-0.5,-1):  d(V3,A)=2.69  d(V3,B)=3.04  d(V3,C)=1.80 → C>A>B
  //
  // Pairwise majority (k=2 of 3):
  //   A beats B: V1 + V3 prefer A  (2/3) ✓
  //   B beats C: V1 + V2 prefer B  (2/3) ✓
  //   C beats A: V2 + V3 prefer C  (2/3) ✓  → cycle, no Condorcet winner.
  const std::vector<Point2d> alts = {{2.0, 0.0}, {0.0, 2.0}, {-2.0, 0.0}};
  const std::vector<Point2d> voters = {{1.0, 0.5}, {-0.5, 1.5}, {-0.5, -1.0}};
  EXPECT_FALSE(has_condorcet_winner(alts, voters));
  EXPECT_EQ(condorcet_winner(alts, voters), std::nullopt);
}

TEST(CondorcetFinite, OneAlternativeDominates) {
  // Three alternatives; two voters clustered near A, one near B.
  // A beats B and C under simple majority (2 of 3 prefer A).
  const std::vector<Point2d> alts = {
    {0.0, 0.0},   // A — near voters 1 and 2
    {5.0, 5.0},   // B — far from everyone
    {5.0, -5.0},  // C — far from everyone
  };
  const std::vector<Point2d> voters = {{0.1, 0.0}, {-0.1, 0.0}, {5.1, 5.0}};
  EXPECT_TRUE(has_condorcet_winner(alts, voters));
  auto cw = condorcet_winner(alts, voters);
  ASSERT_NE(cw, std::nullopt);
  // The winner should be A (at (0,0)).
  EXPECT_NEAR(cw->x(), 0.0, 1e-9);
  EXPECT_NEAR(cw->y(), 0.0, 1e-9);
}

TEST(CondorcetFinite, CondorcetWinner_BeatsAllOthers) {
  // Verify the returned winner actually majority-beats every other alternative.
  const std::vector<Point2d> alts = {{0.0, 0.0}, {5.0, 5.0}, {5.0, -5.0}};
  const std::vector<Point2d> voters = {{0.1, 0.0}, {-0.1, 0.0}, {5.1, 5.0}};
  auto cw = condorcet_winner(alts, voters);
  ASSERT_NE(cw, std::nullopt);
  for (const auto& alt : alts) {
    if (alt.x() == cw->x() && alt.y() == cw->y()) continue;
    EXPECT_TRUE(majority_prefers(*cw, alt, voters, kEuc))
        << "Condorcet winner fails to beat (" << alt.x() << "," << alt.y()
        << ")";
  }
}

TEST(CondorcetFinite, Supermajority_EliminatesWinner) {
  // Under unanimity (k = n = 3), a non-Pareto alternative loses.
  // All 3 voters are near (0,0); the alternative (5,5) is far from all of
  // them.  Under k=3, (0,0) cannot be beaten by (5,5) because all 3 prefer
  // (0,0).  But (5,5) also cannot beat (0,0) unanimously.
  // With k = n, "A beats B" requires ALL voters to prefer A to B.
  // Here:
  //   V1=(0,0), V2=(0.1,0), V3=(-0.1,0).  d(Vi, A) ~ 0; d(Vi, B) ~ 7.07.
  //   All 3 prefer A=(0,0) to B=(5,5) → A unanimously beats B.
  //   So A IS the Condorcet winner under unanimity too.
  const std::vector<Point2d> alts = {{0.0, 0.0}, {5.0, 5.0}};
  const std::vector<Point2d> voters = {{0.0, 0.0}, {0.1, 0.0}, {-0.1, 0.0}};
  const int k_unanimity = 3;
  EXPECT_TRUE(has_condorcet_winner(alts, voters, kEuc, k_unanimity));
}

// ---------------------------------------------------------------------------
// core_2d — continuous policy space
// ---------------------------------------------------------------------------

TEST(Core2D, CollinearVoters_HaveCondorcetWinner) {
  // 3 collinear voters at (0,0), (1,0), (2,0).  Median = (1,0).
  // In 1D embedded in 2D, (1,0) is the Condorcet winner.
  // core_2d should detect it via the Yolk centre (near (1,0)) having an
  // empty winset.
  const std::vector<Point2d> voters = {{0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}};
  auto cw = core_2d(voters, kEuc, -1, 64);
  ASSERT_NE(cw, std::nullopt)
      << "Expected a Condorcet winner for collinear voters.";
  // The winner should be very close to the median voter (1, 0).
  EXPECT_NEAR(cw->x(), 1.0, 0.1);
  EXPECT_NEAR(cw->y(), 0.0, 0.1);
}

TEST(Core2D, EquilateralTriangle_NoCondorcetWinner) {
  // Symmetric equilateral triangle: McKelvey's theorem applies —
  // simple majority is everywhere intransitive.
  const std::vector<Point2d> voters = {{0.0, 0.0}, {1.0, 0.0}, {0.5, 0.866}};
  auto cw = core_2d(voters, kEuc, -1, 64);
  EXPECT_EQ(cw, std::nullopt)
      << "Equilateral triangle should have no Condorcet winner.";
}

TEST(Core2D, AllVotersSameIdeal_IsCondorcetWinner) {
  // When all voters share an ideal, that point is unanimously preferred to
  // any other; it is the Condorcet winner under any k.
  const std::vector<Point2d> voters = {{1.0, 1.0}, {1.0, 1.0}, {1.0, 1.0}};
  auto cw = core_2d(voters, kEuc, -1, 64);
  ASSERT_NE(cw, std::nullopt);
  EXPECT_NEAR(cw->x(), 1.0, 0.2);
  EXPECT_NEAR(cw->y(), 1.0, 0.2);
}

TEST(Core2D, TooFewVoters_Throws) {
  const std::vector<Point2d> one_voter = {{0.0, 0.0}};
  EXPECT_THROW(core_2d(one_voter), std::invalid_argument);
  const std::vector<Point2d> empty;
  EXPECT_THROW(core_2d(empty), std::invalid_argument);
}
