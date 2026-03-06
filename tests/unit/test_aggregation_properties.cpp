// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

// Tests for Phases W1–W3: social rankings, Pareto efficiency, and
// Condorcet / majority consistency.

#include <gtest/gtest.h>
#include <socialchoicelab/aggregation/borda.h>
#include <socialchoicelab/aggregation/condorcet_consistency.h>
#include <socialchoicelab/aggregation/pareto.h>
#include <socialchoicelab/aggregation/plurality.h>
#include <socialchoicelab/aggregation/social_ranking.h>

#include <algorithm>
#include <vector>

using socialchoicelab::aggregation::borda_scores;
using socialchoicelab::aggregation::condorcet_winner_profile;
using socialchoicelab::aggregation::has_condorcet_winner_profile;
using socialchoicelab::aggregation::is_condorcet_consistent;
using socialchoicelab::aggregation::is_majority_consistent;
using socialchoicelab::aggregation::is_pareto_efficient;
using socialchoicelab::aggregation::pairwise_ranking;
using socialchoicelab::aggregation::pareto_dominates;
using socialchoicelab::aggregation::pareto_set;
using socialchoicelab::aggregation::Profile;
using socialchoicelab::aggregation::rank_by_scores;
using socialchoicelab::aggregation::TieBreak;
using socialchoicelab::core::rng::PRNG;

// ---------------------------------------------------------------------------
// Shared profiles
// ---------------------------------------------------------------------------

// A(0) is the Condorcet winner.
//   V1: A > B > C   V2: A > C > B   V3: B > A > C
static Profile make_condorcet_profile() {
  Profile p;
  p.n_voters = 3;
  p.n_alts = 3;
  p.rankings = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}};
  return p;
}

// Cycle: A > B > C > A.
static Profile make_cycle_profile() {
  Profile p;
  p.n_voters = 3;
  p.n_alts = 3;
  p.rankings = {{0, 1, 2}, {1, 2, 0}, {2, 0, 1}};
  return p;
}

// Dominated alternative: C is last for all voters.
//   V1: A > B > C   V2: B > A > C
static Profile make_dominated_profile() {
  Profile p;
  p.n_voters = 2;
  p.n_alts = 3;
  p.rankings = {{0, 1, 2}, {1, 0, 2}};
  return p;
}

// ---------------------------------------------------------------------------
// W1 — rank_by_scores
// ---------------------------------------------------------------------------

TEST(RankByScores, DescendingOrder_SmallestIndex) {
  const std::vector<double> scores = {3.0, 1.0, 2.0};
  const auto ranking = rank_by_scores(scores, TieBreak::kSmallestIndex);
  EXPECT_EQ(ranking[0], 0);  // score 3 → first
  EXPECT_EQ(ranking[1], 2);  // score 2 → second
  EXPECT_EQ(ranking[2], 1);  // score 1 → last
}

TEST(RankByScores, Ties_SmallestIndex_AscendingWithinGroup) {
  // scores: A=5, B=5, C=3 → tied group {A,B} ordered by index.
  const std::vector<double> scores = {5.0, 5.0, 3.0};
  const auto ranking = rank_by_scores(scores, TieBreak::kSmallestIndex);
  EXPECT_EQ(ranking[0], 0);
  EXPECT_EQ(ranking[1], 1);
  EXPECT_EQ(ranking[2], 2);
}

TEST(RankByScores, Random_NeedsProng) {
  EXPECT_THROW(rank_by_scores({1.0, 1.0}, TieBreak::kRandom, nullptr),
               std::invalid_argument);
}

TEST(RankByScores, Empty_ReturnsEmpty) {
  EXPECT_TRUE(rank_by_scores({}, TieBreak::kSmallestIndex).empty());
}

TEST(RankByScores, ConsistentWithBordaOneWinner) {
  const auto p = make_condorcet_profile();
  const auto scores = borda_scores(p);
  const std::vector<double> dscore(scores.begin(), scores.end());
  const auto ranking = rank_by_scores(dscore, TieBreak::kSmallestIndex);
  // First element of ranking should match borda_one_winner with kSmallestIndex.
  EXPECT_EQ(ranking[0], borda_one_winner(p, TieBreak::kSmallestIndex));
}

// W1 — pairwise_ranking tested in integration tests (needs geometry layer).

// ---------------------------------------------------------------------------
// W2 — Pareto efficiency
// ---------------------------------------------------------------------------

TEST(Pareto, Dominates_ClearlyDominated) {
  const auto p = make_dominated_profile();
  // C is ranked last by both voters; A and B are not all-last.
  // A Pareto-dominates C? V1: A>C (pos 0<2), V2: A>C (pos 1<2). Yes.
  EXPECT_TRUE(pareto_dominates(p, 0, 2));
  // B Pareto-dominates C? V1: B>C (pos 1<2), V2: B>C (pos 0<2). Yes.
  EXPECT_TRUE(pareto_dominates(p, 1, 2));
  // A does not Pareto-dominate B: V2 prefers B over A (pos 0<1).
  EXPECT_FALSE(pareto_dominates(p, 0, 1));
}

TEST(Pareto, Dominates_SameAlternative_False) {
  const auto p = make_condorcet_profile();
  EXPECT_FALSE(pareto_dominates(p, 0, 0));
}

TEST(Pareto, Dominates_OutOfRange_Throws) {
  const auto p = make_condorcet_profile();
  EXPECT_THROW(pareto_dominates(p, -1, 0), std::invalid_argument);
  EXPECT_THROW(pareto_dominates(p, 0, 99), std::invalid_argument);
}

TEST(Pareto, Set_DominatedAlternativeExcluded) {
  const auto p = make_dominated_profile();
  const auto ps = pareto_set(p);
  // C (index 2) is Pareto-dominated; A and B are not.
  EXPECT_EQ(ps, (std::vector<int>{0, 1}));
}

TEST(Pareto, Set_CycleProfile_AllEfficient) {
  // In the Condorcet cycle each voter has a different most-preferred alt,
  // so no alternative is unanimously dominated.
  const auto p = make_cycle_profile();
  const auto ps = pareto_set(p);
  EXPECT_EQ(static_cast<int>(ps.size()), 3);
}

TEST(Pareto, Set_NonEmpty) {
  // Pareto set is always non-empty.
  for (const auto& p : {make_condorcet_profile(), make_cycle_profile(),
                        make_dominated_profile()}) {
    EXPECT_FALSE(pareto_set(p).empty());
  }
}

TEST(Pareto, CondorcetWinner_IsEfficient) {
  const auto p = make_condorcet_profile();
  // A (index 0) is the Condorcet winner; must be Pareto efficient.
  EXPECT_TRUE(is_pareto_efficient(p, 0));
}

TEST(Pareto, IsEfficient_OutOfRange_Throws) {
  const auto p = make_condorcet_profile();
  EXPECT_THROW(is_pareto_efficient(p, -1), std::invalid_argument);
  EXPECT_THROW(is_pareto_efficient(p, 99), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// W3 — Condorcet and majority consistency
// ---------------------------------------------------------------------------

TEST(CondorcetConsistency, HasCondorcetWinner_CondorcetProfile) {
  EXPECT_TRUE(has_condorcet_winner_profile(make_condorcet_profile()));
}

TEST(CondorcetConsistency, HasCondorcetWinner_CycleProfile_False) {
  EXPECT_FALSE(has_condorcet_winner_profile(make_cycle_profile()));
}

TEST(CondorcetConsistency, CondorcetWinnerProfile_IdentifiesCorrectWinner) {
  const auto cw = condorcet_winner_profile(make_condorcet_profile());
  ASSERT_TRUE(cw.has_value());
  EXPECT_EQ(*cw, 0);  // A is the Condorcet winner
}

TEST(CondorcetConsistency, CondorcetWinnerProfile_CycleIsNullopt) {
  EXPECT_FALSE(condorcet_winner_profile(make_cycle_profile()).has_value());
}

TEST(CondorcetConsistency, IsCondorcetConsistent_TrueForCWWinner) {
  const auto p = make_condorcet_profile();
  EXPECT_TRUE(is_condorcet_consistent(p, 0));   // A is CW
  EXPECT_FALSE(is_condorcet_consistent(p, 1));  // B is not CW
}

TEST(CondorcetConsistency, IsCondorcetConsistent_TrueForAnyone_NoCW) {
  // No Condorcet winner → vacuously consistent for any candidate.
  const auto p = make_cycle_profile();
  EXPECT_TRUE(is_condorcet_consistent(p, 0));
  EXPECT_TRUE(is_condorcet_consistent(p, 1));
  EXPECT_TRUE(is_condorcet_consistent(p, 2));
}

TEST(CondorcetConsistency, IsMajorityConsistent_TrueForCW) {
  // The Condorcet winner is majority consistent.
  EXPECT_TRUE(is_majority_consistent(make_condorcet_profile(), 0));
}

TEST(CondorcetConsistency, IsMajorityConsistent_FalseForNonCW) {
  EXPECT_FALSE(is_majority_consistent(make_condorcet_profile(), 1));
}

TEST(CondorcetConsistency, IsMajorityConsistent_FalseForAll_Cycle) {
  // No Condorcet winner in a cycle → no candidate is majority consistent.
  const auto p = make_cycle_profile();
  EXPECT_FALSE(is_majority_consistent(p, 0));
  EXPECT_FALSE(is_majority_consistent(p, 1));
  EXPECT_FALSE(is_majority_consistent(p, 2));
}

TEST(CondorcetConsistency, MajorityConsistency_ImpliesCondorcetConsistency) {
  // If is_majority_consistent(p, w) then is_condorcet_consistent(p, w).
  const auto p = make_condorcet_profile();
  for (int w = 0; w < p.n_alts; ++w) {
    if (is_majority_consistent(p, w)) {
      EXPECT_TRUE(is_condorcet_consistent(p, w));
    }
  }
}

TEST(CondorcetConsistency, PluralityWinner_NeedNotBeMajorityConsistent) {
  // Build a profile where the plurality winner is NOT majority consistent.
  //   V1,V2: A > B > C   (A has 2 first-place votes)
  //   V3:    B > C > A   (B has 1 first-place vote)
  // Plurality winner = A.  But B beats A (2 vs 1) — wait let me recalc:
  // B beats A: V3 prefers B, V1 and V2 prefer A. So A beats B 2-1.
  // A beats C: 3-0.  A is actually Condorcet winner AND majority consistent.
  // Need a different example. Use the plurality_vs_condorcet profile.
  //   V1,V2: A > B > C   V3,V4: B > C > A   V5: C > B > A
  //   Plurality: A=2, B=2, C=1.  After kSmallestIndex, plurality winner = A.
  //   Majority: B beats A (3-2), so A is NOT majority consistent.
  Profile p;
  p.n_voters = 5;
  p.n_alts = 3;
  p.rankings = {{0, 1, 2}, {0, 1, 2}, {1, 2, 0}, {1, 2, 0}, {2, 1, 0}};
  const int plur_w = plurality_one_winner(p, TieBreak::kSmallestIndex);
  EXPECT_EQ(plur_w, 0);                             // A wins plurality
  EXPECT_FALSE(is_majority_consistent(p, plur_w));  // A not majority consistent
}
