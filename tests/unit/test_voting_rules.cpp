// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

// Tests for Phases V1–V5: plurality, Borda, approval, anti-plurality, and
// generic positional scoring rule.

#include <gtest/gtest.h>
#include <socialchoicelab/aggregation/antiplurality.h>
#include <socialchoicelab/aggregation/approval.h>
#include <socialchoicelab/aggregation/borda.h>
#include <socialchoicelab/aggregation/plurality.h>
#include <socialchoicelab/aggregation/scoring_rule.h>

#include <algorithm>
#include <map>
#include <numeric>
#include <vector>

using socialchoicelab::aggregation::antiplurality_all_winners;
using socialchoicelab::aggregation::antiplurality_one_winner;
using socialchoicelab::aggregation::antiplurality_scores;
using socialchoicelab::aggregation::approval_all_winners_spatial;
using socialchoicelab::aggregation::approval_all_winners_topk;
using socialchoicelab::aggregation::approval_scores_spatial;
using socialchoicelab::aggregation::approval_scores_topk;
using socialchoicelab::aggregation::borda_all_winners;
using socialchoicelab::aggregation::borda_one_winner;
using socialchoicelab::aggregation::borda_ranking;
using socialchoicelab::aggregation::borda_scores;
using socialchoicelab::aggregation::DistConfig;
using socialchoicelab::aggregation::plurality_all_winners;
using socialchoicelab::aggregation::plurality_one_winner;
using socialchoicelab::aggregation::plurality_scores;
using socialchoicelab::aggregation::Profile;
using socialchoicelab::aggregation::scoring_rule_all_winners;
using socialchoicelab::aggregation::scoring_rule_one_winner;
using socialchoicelab::aggregation::scoring_rule_scores;
using socialchoicelab::aggregation::TieBreak;
using socialchoicelab::core::rng::PRNG;
using socialchoicelab::core::types::Point2d;

static const DistConfig kEuc{};

// ---------------------------------------------------------------------------
// Shared test profiles
// ---------------------------------------------------------------------------

// 3 voters, 3 alternatives; A(0) is the Condorcet winner.
//   V1: A > B > C   V2: A > C > B   V3: B > A > C
//   Majority: A beats B (2-1), A beats C (3-0), B beats C (3-0). A is CW.
static Profile make_condorcet_profile() {
  Profile p;
  p.n_voters = 3;
  p.n_alts = 3;
  p.rankings = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}};
  return p;
}

// Classic Condorcet cycle: A > B > C > A (3 voters, 3 alternatives).
//   V1: A > B > C   V2: B > C > A   V3: C > A > B
static Profile make_cycle_profile() {
  Profile p;
  p.n_voters = 3;
  p.n_alts = 3;
  p.rankings = {{0, 1, 2}, {1, 2, 0}, {2, 0, 1}};
  return p;
}

// Single voter, 3 alternatives; ranking A > B > C.
static Profile make_single_voter() {
  Profile p;
  p.n_voters = 1;
  p.n_alts = 3;
  p.rankings = {{0, 1, 2}};
  return p;
}

// Single alternative.
static Profile make_single_alt() {
  Profile p;
  p.n_voters = 3;
  p.n_alts = 1;
  p.rankings = {{0}, {0}, {0}};
  return p;
}

// Plurality non-winner is Condorcet winner counterexample.
//   5 voters, 3 alternatives.
//   V1,V2: A > B > C   (A gets 2 plurality votes)
//   V3,V4: B > C > A   (B gets 2 plurality votes)
//   V5:    C > B > A   (C gets 1 plurality vote)
//   Plurality: A=2, B=2, C=1 — tied between A and B.
//   Majority: B beats C (3-2), B beats A (3-2) → B is the Condorcet winner.
static Profile make_plurality_vs_condorcet() {
  Profile p;
  p.n_voters = 5;
  p.n_alts = 3;
  p.rankings = {{0, 1, 2}, {0, 1, 2}, {1, 2, 0}, {1, 2, 0}, {2, 1, 0}};
  return p;
}

// ---------------------------------------------------------------------------
// V1 — Plurality
// ---------------------------------------------------------------------------

TEST(Plurality, Scores_CondorcetProfile) {
  const auto p = make_condorcet_profile();
  const auto scores = plurality_scores(p);
  // V1: A first, V2: A first, V3: B first
  EXPECT_EQ(scores[0], 2);  // A
  EXPECT_EQ(scores[1], 1);  // B
  EXPECT_EQ(scores[2], 0);  // C
}

TEST(Plurality, AllWinners_UniqueWinner) {
  const auto p = make_condorcet_profile();
  const auto winners = plurality_all_winners(p);
  EXPECT_EQ(winners, std::vector<int>({0}));
}

TEST(Plurality, AllWinners_Cycle_Tie) {
  // Cycle: each alternative gets 1 first-place vote.
  const auto p = make_cycle_profile();
  const auto winners = plurality_all_winners(p);
  EXPECT_EQ(winners, (std::vector<int>{0, 1, 2}));
}

TEST(Plurality, OneWinner_SmallestIndex_Deterministic) {
  const auto p = make_cycle_profile();
  PRNG prng(1);
  const int w1 = plurality_one_winner(p, TieBreak::kSmallestIndex);
  const int w2 = plurality_one_winner(p, TieBreak::kSmallestIndex);
  EXPECT_EQ(w1, 0);
  EXPECT_EQ(w2, 0);
}

TEST(Plurality, OneWinner_Random_NeedsProng) {
  const auto p = make_cycle_profile();
  EXPECT_THROW(plurality_one_winner(p, TieBreak::kRandom, nullptr),
               std::invalid_argument);
}

TEST(Plurality, OneWinner_Random_EqualProbability) {
  // Over n=3000 draws each of the 3 tied alternatives should win ~1000 times.
  const auto p = make_cycle_profile();
  PRNG prng(2026);
  std::map<int, int> counts;
  for (int i = 0; i < 3000; ++i)
    ++counts[plurality_one_winner(p, TieBreak::kRandom, &prng)];
  for (int j = 0; j < 3; ++j) {
    EXPECT_GT(counts[j], 700);
    EXPECT_LT(counts[j], 1300);
  }
}

TEST(Plurality, SingleVoter) {
  const auto p = make_single_voter();
  EXPECT_EQ(plurality_one_winner(p, TieBreak::kSmallestIndex), 0);
}

TEST(Plurality, SingleAlternative) {
  const auto p = make_single_alt();
  const auto w = plurality_all_winners(p);
  EXPECT_EQ(w, std::vector<int>({0}));
}

TEST(Plurality, CondorcetNeedNotBePluralityWinner) {
  // B is Condorcet winner but plurality is tied A,B.
  const auto p = make_plurality_vs_condorcet();
  const auto all = plurality_all_winners(p);
  // A and B tied with 2 votes each.
  EXPECT_EQ(all, (std::vector<int>{0, 1}));
  // Condorcet winner (1=B) is among the plurality winners but not guaranteed
  // to be the unique plurality winner.
  EXPECT_TRUE(std::find(all.begin(), all.end(), 1) != all.end());
}

// ---------------------------------------------------------------------------
// V2 — Borda
// ---------------------------------------------------------------------------

TEST(Borda, Scores_CondorcetProfile) {
  const auto p = make_condorcet_profile();
  const auto scores = borda_scores(p);
  // m=3; score = (2-rank) per voter.
  // V1(A>B>C): A+=2, B+=1, C+=0
  // V2(A>C>B): A+=2, B+=0, C+=1
  // V3(B>A>C): A+=1, B+=2, C+=0
  EXPECT_EQ(scores[0], 5);  // A
  EXPECT_EQ(scores[1], 3);  // B
  EXPECT_EQ(scores[2], 1);  // C
}

TEST(Borda, ScoreSum_EqualsFormula) {
  // Sum of all Borda scores = n × m(m-1)/2.
  const auto p = make_condorcet_profile();
  const auto scores = borda_scores(p);
  const int expected = p.n_voters * p.n_alts * (p.n_alts - 1) / 2;
  const int total = std::accumulate(scores.begin(), scores.end(), 0);
  EXPECT_EQ(total, expected);
}

TEST(Borda, CondorcetWinner_IsBordaWinner_ThreeAlts) {
  // Fishburn (1977): for m=3, Condorcet winner is always the Borda winner.
  const auto p = make_condorcet_profile();
  const auto bw = borda_all_winners(p);
  EXPECT_EQ(bw, std::vector<int>({0}));  // A is CW and Borda winner
}

TEST(Borda, Cycle_ProducesCompleteRanking) {
  // Condorcet cycle: Borda still produces a unique social ranking.
  const auto p = make_cycle_profile();
  const auto scores = borda_scores(p);
  // Cycle: each alternative is ranked first/second/third once each.
  // scores[j] = 2+1+0 = 3 for all j (perfect symmetry).
  for (int j = 0; j < 3; ++j) EXPECT_EQ(scores[j], 3);
  // All three are tied; ranking can still be produced.
  const auto ranking = borda_ranking(p, TieBreak::kSmallestIndex);
  EXPECT_EQ(static_cast<int>(ranking.size()), 3);
}

TEST(Borda, SingleVoter) {
  const auto p = make_single_voter();
  EXPECT_EQ(borda_one_winner(p, TieBreak::kSmallestIndex), 0);
}

TEST(Borda, SingleAlternative) {
  const auto p = make_single_alt();
  const auto w = borda_all_winners(p);
  EXPECT_EQ(w, std::vector<int>({0}));
}

TEST(Borda, Ranking_SmallestIndex_Deterministic) {
  const auto p = make_condorcet_profile();
  const auto r = borda_ranking(p, TieBreak::kSmallestIndex);
  EXPECT_EQ(r[0], 0);  // A has highest Borda score
  EXPECT_EQ(r[1], 1);  // B second
  EXPECT_EQ(r[2], 2);  // C last
}

// ---------------------------------------------------------------------------
// V3 — Approval voting
// ---------------------------------------------------------------------------

TEST(Approval, Spatial_ZeroThreshold_NoApprovals) {
  // Threshold=0 means only alternatives exactly at voter position are approved.
  // Use distinct positions so no approvals.
  const std::vector<Point2d> alts = {{0.0, 0.0}, {5.0, 0.0}};
  const std::vector<Point2d> voters = {{2.5, 0.0}};
  const auto winners = approval_all_winners_spatial(alts, voters, kEuc, 0.0);
  EXPECT_TRUE(winners.empty());
}

TEST(Approval, Spatial_LargeThreshold_AllApproved) {
  const std::vector<Point2d> alts = {{0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}};
  const std::vector<Point2d> voters = {{1.0, 0.0}};
  const auto scores = approval_scores_spatial(alts, voters, kEuc, 100.0);
  for (int s : scores) EXPECT_EQ(s, 1);
  const auto winners = approval_all_winners_spatial(alts, voters, kEuc, 100.0);
  EXPECT_EQ(static_cast<int>(winners.size()), 3);
}

TEST(Approval, Spatial_OneDimensionalEmbeddedThreshold_IsInclusive) {
  // Explicit 1D compatibility check using x-axis embedding in Point2d.
  // Voter at x=1 is exactly distance 1 from alts at x=0 and x=2, and distance 3
  // from alt at x=4. Threshold comparison should be inclusive.
  const std::vector<Point2d> alts = {{0.0, 0.0}, {2.0, 0.0}, {4.0, 0.0}};
  const std::vector<Point2d> voters = {{1.0, 0.0}};

  const auto scores = approval_scores_spatial(alts, voters, kEuc, 1.0);
  ASSERT_EQ(scores.size(), 3u);
  EXPECT_EQ(scores[0], 1);
  EXPECT_EQ(scores[1], 1);
  EXPECT_EQ(scores[2], 0);

  const auto winners = approval_all_winners_spatial(alts, voters, kEuc, 1.0);
  EXPECT_EQ(winners, (std::vector<int>{0, 1}));
}

TEST(Approval, Spatial_OneDimensionalEmbeddedMidpointApprovesSymmetrically) {
  // A midpoint voter on the line should approve both equidistant alternatives
  // when the threshold matches that shared distance exactly.
  const std::vector<Point2d> alts = {{-2.0, 0.0}, {2.0, 0.0}, {5.0, 0.0}};
  const std::vector<Point2d> voters = {{0.0, 0.0}};

  const auto scores = approval_scores_spatial(alts, voters, kEuc, 2.0);
  ASSERT_EQ(scores.size(), 3u);
  EXPECT_EQ(scores[0], 1);
  EXPECT_EQ(scores[1], 1);
  EXPECT_EQ(scores[2], 0);
}

TEST(Approval, Spatial_DistanceThreshold_CorrectSubset) {
  // Alternatives at (0,0) and (10,0); voter at (1,0) with threshold 2.
  // dist to A = 1 ≤ 2 → approved.  dist to B = 9 > 2 → not approved.
  const std::vector<Point2d> alts = {{0.0, 0.0}, {10.0, 0.0}};
  const std::vector<Point2d> voters = {{1.0, 0.0}};
  const auto scores = approval_scores_spatial(alts, voters, kEuc, 2.0);
  EXPECT_EQ(scores[0], 1);
  EXPECT_EQ(scores[1], 0);
}

TEST(Approval, Spatial_NegativeThreshold_Throws) {
  const std::vector<Point2d> alts = {{0.0, 0.0}};
  const std::vector<Point2d> voters = {{1.0, 0.0}};
  EXPECT_THROW(approval_scores_spatial(alts, voters, kEuc, -1.0),
               std::invalid_argument);
}

TEST(Approval, Topk_kEqualm_SameAsAllApproved) {
  // k = n_alts: every alternative is approved, so all tied.
  const auto p = make_condorcet_profile();
  const auto scores = approval_scores_topk(p, p.n_alts);
  for (int s : scores) EXPECT_EQ(s, p.n_voters);
  const auto winners = approval_all_winners_topk(p, p.n_alts);
  EXPECT_EQ(static_cast<int>(winners.size()), p.n_alts);
}

TEST(Approval, Topk_kEqualOne_MatchesPlurality) {
  // Top-1 approval = plurality.
  const auto p = make_condorcet_profile();
  const auto ap_scores = approval_scores_topk(p, 1);
  const auto pl_scores = plurality_scores(p);
  EXPECT_EQ(ap_scores, pl_scores);
  EXPECT_EQ(approval_all_winners_topk(p, 1), plurality_all_winners(p));
}

TEST(Approval, Topk_InvalidK_Throws) {
  const auto p = make_condorcet_profile();
  EXPECT_THROW(approval_scores_topk(p, 0), std::invalid_argument);
  EXPECT_THROW(approval_scores_topk(p, p.n_alts + 1), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// V4 — Anti-plurality
// ---------------------------------------------------------------------------

TEST(Antiplurality, Scores_ScoreSum) {
  // scores[j] = n_voters × (n_alts-1) - last_place_votes[j].
  // Sum over all j = n_voters × (n_alts-1).
  const auto p = make_condorcet_profile();
  const auto scores = antiplurality_scores(p);
  const int expected_sum = p.n_voters * (p.n_alts - 1);
  const int total = std::accumulate(scores.begin(), scores.end(), 0);
  EXPECT_EQ(total, expected_sum);
}

TEST(Antiplurality, SingleAlternative_TrivialWinner) {
  const auto p = make_single_alt();
  // With 1 alternative, m-1=0 so all scores are 0.
  const auto winners = antiplurality_all_winners(p);
  EXPECT_EQ(winners, std::vector<int>({0}));
}

TEST(Antiplurality, PluralityWinner_CanLoseAntiplurality) {
  // Build a profile where the plurality winner has the lowest anti-plurality
  // score.
  //   3 voters, 3 alternatives.
  //   V1,V2: A > B > C   (A gets 2 plurality votes; C gets 2 last-place)
  //   V3:    B > C > A   (A gets 1 last-place)
  // Antiplurality: A gets 3*(3-1) - 3 = 3 non-last = n-last_count
  //   Last-place: A=1 from V3, C=2 from V1,V2.
  //   anti scores: A = 3-1=2, B = 3-0=3, C = 3-2=1.
  // Plurality winner = A (2 first-place votes).
  // Anti-plurality winner = B. A does not win anti-plurality.
  Profile p;
  p.n_voters = 3;
  p.n_alts = 3;
  p.rankings = {{0, 1, 2}, {0, 1, 2}, {1, 2, 0}};

  const int plur_w = plurality_one_winner(p, TieBreak::kSmallestIndex);
  const int anti_w = antiplurality_one_winner(p, TieBreak::kSmallestIndex);
  EXPECT_EQ(plur_w, 0);       // A is plurality winner
  EXPECT_NE(plur_w, anti_w);  // A is not anti-plurality winner
}

TEST(Antiplurality, OneWinner_SmallestIndex_Deterministic) {
  const auto p = make_cycle_profile();
  // Cycle: each alternative is last once, so all scores equal.
  const int w = antiplurality_one_winner(p, TieBreak::kSmallestIndex);
  EXPECT_EQ(w, 0);
}

// ---------------------------------------------------------------------------
// V5 — Generic positional scoring rule
// ---------------------------------------------------------------------------

TEST(ScoringRule, RecoverPlurality) {
  const auto p = make_condorcet_profile();
  const std::vector<double> sv = {1.0, 0.0, 0.0};  // plurality
  const auto scores = scoring_rule_scores(p, sv);
  const auto pl = plurality_scores(p);
  for (int j = 0; j < 3; ++j)
    EXPECT_DOUBLE_EQ(scores[j], static_cast<double>(pl[j]));
  EXPECT_EQ(scoring_rule_all_winners(p, sv), plurality_all_winners(p));
}

TEST(ScoringRule, RecoverBorda) {
  const auto p = make_condorcet_profile();
  const std::vector<double> sv = {2.0, 1.0, 0.0};  // Borda for m=3
  const auto scores = scoring_rule_scores(p, sv);
  const auto borda = borda_scores(p);
  for (int j = 0; j < 3; ++j)
    EXPECT_DOUBLE_EQ(scores[j], static_cast<double>(borda[j]));
}

TEST(ScoringRule, RecoverAntiplurality) {
  const auto p = make_condorcet_profile();
  const std::vector<double> sv = {1.0, 1.0, 0.0};  // anti-plurality for m=3
  const auto scores = scoring_rule_scores(p, sv);
  const auto anti = antiplurality_scores(p);
  for (int j = 0; j < 3; ++j)
    EXPECT_DOUBLE_EQ(scores[j], static_cast<double>(anti[j]));
}

TEST(ScoringRule, WrongLength_Throws) {
  const auto p = make_condorcet_profile();
  EXPECT_THROW(scoring_rule_scores(p, {1.0, 0.0}), std::invalid_argument);
}

TEST(ScoringRule, NotNonIncreasing_Throws) {
  const auto p = make_condorcet_profile();
  EXPECT_THROW(scoring_rule_scores(p, {0.0, 1.0, 0.0}), std::invalid_argument);
}

TEST(ScoringRule, SingleVoter) {
  const auto p = make_single_voter();
  const int w =
      scoring_rule_one_winner(p, {2.0, 1.0, 0.0}, TieBreak::kSmallestIndex);
  EXPECT_EQ(w, 0);
}

TEST(ScoringRule, SingleAlternative) {
  const auto p = make_single_alt();
  const auto w = scoring_rule_all_winners(p, {1.0});
  EXPECT_EQ(w, std::vector<int>({0}));
}
