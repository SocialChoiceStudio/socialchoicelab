// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

// Phase I1: end-to-end integration tests chaining the aggregation layer with
// the geometry layer.
//
// Test chains:
//   (a) Condorcet winner configuration → all positional rules agree on winner
//       → is_condorcet_consistent = true for all.
//   (b) Condorcet cycle → no majority winner → is_condorcet_consistent = true
//       for any winner (vacuously).
//   (c) Spatial profile → Borda winner matches Copeland winner for 3 alts
//       (Fishburn 1977 result).
//   (d) Impartial culture → all alternatives have equal expected Borda score
//       (statistical test).
//   (e) Pareto set from profile ⊆ convex hull of alternatives (spatial Pareto).
//   (f) Condorcet winner in profile ↔ Condorcet winner in geometry layer agree.
//   (g) pairwise_ranking places Copeland winner first.

#include <gtest/gtest.h>
#include <socialchoicelab/aggregation/antiplurality.h>
#include <socialchoicelab/aggregation/borda.h>
#include <socialchoicelab/aggregation/condorcet_consistency.h>
#include <socialchoicelab/aggregation/pareto.h>
#include <socialchoicelab/aggregation/plurality.h>
#include <socialchoicelab/aggregation/profile.h>
#include <socialchoicelab/aggregation/profile_generators.h>
#include <socialchoicelab/aggregation/social_ranking.h>
#include <socialchoicelab/geometry/copeland.h>
#include <socialchoicelab/geometry/core.h>
#include <socialchoicelab/geometry/majority.h>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

using socialchoicelab::aggregation::antiplurality_all_winners;
using socialchoicelab::aggregation::antiplurality_one_winner;
using socialchoicelab::aggregation::borda_all_winners;
using socialchoicelab::aggregation::borda_one_winner;
using socialchoicelab::aggregation::borda_scores;
using socialchoicelab::aggregation::build_spatial_profile;
using socialchoicelab::aggregation::condorcet_winner_profile;
using socialchoicelab::aggregation::has_condorcet_winner_profile;
using socialchoicelab::aggregation::impartial_culture;
using socialchoicelab::aggregation::is_condorcet_consistent;
using socialchoicelab::aggregation::is_majority_consistent;
using socialchoicelab::aggregation::is_well_formed;
using socialchoicelab::aggregation::pairwise_ranking;
using socialchoicelab::aggregation::pareto_set;
using socialchoicelab::aggregation::plurality_all_winners;
using socialchoicelab::aggregation::plurality_one_winner;
using socialchoicelab::aggregation::Profile;
using socialchoicelab::aggregation::TieBreak;
using socialchoicelab::aggregation::uniform_spatial_profile;
using socialchoicelab::core::rng::PRNG;
using socialchoicelab::core::types::Point2d;
using socialchoicelab::geometry::condorcet_winner;
using socialchoicelab::geometry::DistConfig;
using socialchoicelab::geometry::has_condorcet_winner;
using socialchoicelab::geometry::pairwise_matrix;
namespace geometry = socialchoicelab::geometry;

static const DistConfig kEuc{};

// ---------------------------------------------------------------------------
// Shared spatial configuration: Condorcet winner setup.
//
// Alternatives: A=(0,0), B=(5,5), C=(5,-5).
// Voters: 3 voters all near A, so A beats all by majority.
// V1=(0.5,0.5), V2=(0.5,-0.5), V3=(1.0,0.0).
// ---------------------------------------------------------------------------
static const std::vector<Point2d> kCondorcetAlts = {
  {0.0, 0.0}, {5.0, 5.0}, {5.0, -5.0}};
static const std::vector<Point2d> kCondorcetVoters = {
  {0.5, 0.5}, {0.5, -0.5}, {1.0, 0.0}};

// Condorcet cycle configuration (verified in geometry tests):
//   A=(2,0), B=(0,2), C=(-2,0); V1=(1,0.5), V2=(-0.5,1.5), V3=(-0.5,-1).
static const std::vector<Point2d> kCycleAlts = {
  {2.0, 0.0}, {0.0, 2.0}, {-2.0, 0.0}};
static const std::vector<Point2d> kCycleVoters = {
  {1.0, 0.5}, {-0.5, 1.5}, {-0.5, -1.0}};

// Explicit 1D compatibility fixture using x-axis embedding in Point2d.
// Alternatives A=(0,0), B=(2,0), C=(5,0).
// Voters at x={1.6, 2.1, 2.4} all rank B first, so B should be the
// Condorcet winner in both the profile and geometry layers.
static const std::vector<Point2d> kLineAlts = {
  {0.0, 0.0}, {2.0, 0.0}, {5.0, 0.0}};
static const std::vector<Point2d> kLineVoters = {
  {1.6, 0.0}, {2.1, 0.0}, {2.4, 0.0}};

// ---------------------------------------------------------------------------
// (a) Condorcet winner: all positional rules agree, consistency checks pass.
// ---------------------------------------------------------------------------

TEST(Integration_CondorcetWinner, AllPositionalRulesSelectA) {
  const auto profile =
      build_spatial_profile(kCondorcetAlts, kCondorcetVoters, kEuc);

  // All three voters are nearest A, so A must be first in all rankings.
  const int pl_w = plurality_one_winner(profile, TieBreak::kSmallestIndex);
  const int bd_w = borda_one_winner(profile, TieBreak::kSmallestIndex);
  const int ap_w = antiplurality_one_winner(profile, TieBreak::kSmallestIndex);
  EXPECT_EQ(pl_w, 0);
  EXPECT_EQ(bd_w, 0);
  EXPECT_EQ(ap_w, 0);
}

TEST(Integration_CondorcetWinner, IsCondorcetConsistent_AllRules) {
  const auto profile =
      build_spatial_profile(kCondorcetAlts, kCondorcetVoters, kEuc);
  EXPECT_TRUE(is_condorcet_consistent(profile, 0));
  EXPECT_TRUE(is_majority_consistent(profile, 0));
}

TEST(Integration_CondorcetWinner, HasCondorcetWinnerProfile_True) {
  const auto profile =
      build_spatial_profile(kCondorcetAlts, kCondorcetVoters, kEuc);
  EXPECT_TRUE(has_condorcet_winner_profile(profile));
  const auto cw = condorcet_winner_profile(profile);
  ASSERT_TRUE(cw.has_value());
  EXPECT_EQ(*cw, 0);
}

TEST(Integration_CondorcetWinner, GeometryAndProfileCondorcetWinnerAgree) {
  // Geometry layer: has_condorcet_winner over Point2d alternatives.
  EXPECT_TRUE(has_condorcet_winner(kCondorcetAlts, kCondorcetVoters, kEuc));
  const auto geo_cw = condorcet_winner(kCondorcetAlts, kCondorcetVoters, kEuc);
  ASSERT_TRUE(geo_cw.has_value());

  // Profile layer: condorcet_winner_profile returns the same index.
  const auto profile =
      build_spatial_profile(kCondorcetAlts, kCondorcetVoters, kEuc);
  const auto prof_cw = condorcet_winner_profile(profile);
  ASSERT_TRUE(prof_cw.has_value());
  EXPECT_TRUE(geo_cw->isApprox(kCondorcetAlts[*prof_cw]));
}

// ---------------------------------------------------------------------------
// (b) Condorcet cycle: vacuous Condorcet consistency.
// ---------------------------------------------------------------------------

TEST(Integration_Cycle, NoCondorcetWinner_VacuouslyConsistent) {
  const auto profile = build_spatial_profile(kCycleAlts, kCycleVoters, kEuc);
  EXPECT_FALSE(has_condorcet_winner_profile(profile));
  // Any winner is vacuously Condorcet consistent when no CW exists.
  for (int w = 0; w < profile.n_alts; ++w) {
    EXPECT_TRUE(is_condorcet_consistent(profile, w));
  }
}

TEST(Integration_Cycle, GeometryAndProfileCycleAgree) {
  EXPECT_FALSE(has_condorcet_winner(kCycleAlts, kCycleVoters, kEuc));
  const auto profile = build_spatial_profile(kCycleAlts, kCycleVoters, kEuc);
  EXPECT_FALSE(has_condorcet_winner_profile(profile));
}

TEST(Integration_OneDimensionalEmbeddedFixture, GeometryAndProfileAgree) {
  const auto profile = build_spatial_profile(kLineAlts, kLineVoters, kEuc);
  ASSERT_TRUE(is_well_formed(profile));

  EXPECT_TRUE(has_condorcet_winner(kLineAlts, kLineVoters, kEuc));
  EXPECT_TRUE(has_condorcet_winner_profile(profile));

  const auto prof_cw = condorcet_winner_profile(profile);
  ASSERT_TRUE(prof_cw.has_value());
  EXPECT_EQ(*prof_cw, 1);  // B

  const auto geo_cw = condorcet_winner(kLineAlts, kLineVoters, kEuc);
  ASSERT_TRUE(geo_cw.has_value());
  EXPECT_TRUE(geo_cw->isApprox(kLineAlts[1]));

  EXPECT_EQ(plurality_one_winner(profile, TieBreak::kSmallestIndex), 1);
  EXPECT_EQ(borda_one_winner(profile, TieBreak::kSmallestIndex), 1);
  EXPECT_TRUE(is_condorcet_consistent(profile, 1));
  EXPECT_TRUE(is_majority_consistent(profile, 1));
}

TEST(Integration_OneDimensionalEmbeddedFixture, PositionalRulesMatchOnLine) {
  const auto profile = build_spatial_profile(kLineAlts, kLineVoters, kEuc);
  ASSERT_TRUE(is_well_formed(profile));

  EXPECT_EQ(plurality_all_winners(profile), (std::vector<int>{1}));
  EXPECT_EQ(borda_all_winners(profile), (std::vector<int>{1}));
  EXPECT_EQ(antiplurality_all_winners(profile), (std::vector<int>{0, 1}));
}

// ---------------------------------------------------------------------------
// (c) Borda winner = Copeland winner for 3-alternative Condorcet winner case
//     (Fishburn 1977).
// ---------------------------------------------------------------------------

TEST(Integration_Fishburn, BordaWinner_MatchesCopelandWinner_ThreeAlts) {
  const auto profile =
      build_spatial_profile(kCondorcetAlts, kCondorcetVoters, kEuc);

  // Copeland winner from geometry layer (returns Point2d, not index).
  const auto cop_pt =
      geometry::copeland_winner(kCondorcetAlts, kCondorcetVoters, kEuc);
  // Find its index in kCondorcetAlts.
  int cop_idx = -1;
  for (int i = 0; i < static_cast<int>(kCondorcetAlts.size()); ++i) {
    if (kCondorcetAlts[i].isApprox(cop_pt)) {
      cop_idx = i;
      break;
    }
  }
  ASSERT_NE(cop_idx, -1);

  // Borda winner from profile.
  const int borda_w = borda_one_winner(profile, TieBreak::kSmallestIndex);

  // Both should select A (index 0).
  EXPECT_EQ(cop_idx, 0);
  EXPECT_EQ(borda_w, 0);
  EXPECT_EQ(cop_idx, borda_w);
}

// ---------------------------------------------------------------------------
// (d) Impartial culture → equal expected Borda scores (statistical test).
// ---------------------------------------------------------------------------

TEST(Integration_ImpartialCulture, EqualExpectedBordaScores) {
  // Under IC, by symmetry, all alternatives have the same expected Borda score.
  // With n=3000 voters and m=4 alternatives, each alternative's Borda score
  // should be approximately n × (m-1)/2 = 3000 × 1.5 = 4500.
  // Allow 5% relative tolerance.
  PRNG prng(2026);
  const int n = 3000;
  const int m = 4;
  const auto profile = impartial_culture(n, m, prng);
  const auto scores = borda_scores(profile);
  const double expected = static_cast<double>(n) * (m - 1) / 2.0;
  for (int j = 0; j < m; ++j) {
    EXPECT_NEAR(static_cast<double>(scores[j]), expected, expected * 0.05);
  }
}

// ---------------------------------------------------------------------------
// (e) Spatial Pareto: profile Pareto set ⊆ convex hull of alternatives.
// ---------------------------------------------------------------------------
// For collinear alternatives, the two endpoints dominate the interior in
// spatial utility only if all voters are perfectly aligned.  A more robust
// test: the Pareto set never includes an alternative that ALL voters rank last.

TEST(Integration_SpatialPareto, ParetoSet_ExcludesUniversallyLast) {
  // Place alternatives at A=(0,0), B=(5,0), C=(50,0) and voters all near
  // A so that C is unambiguously farthest (last) for every voter.
  const std::vector<Point2d> alts = {{0.0, 0.0}, {5.0, 0.0}, {50.0, 0.0}};
  const std::vector<Point2d> voters = {{0.5, 0.0}, {1.0, 0.0}, {0.0, 0.5}};
  const auto profile = build_spatial_profile(alts, voters, kEuc);

  // All voters are close to A and far from C: verify C is last.
  for (const auto& r : profile.rankings) {
    EXPECT_EQ(r[profile.n_alts - 1], 2);
  }
  // C must not be in the Pareto set (A dominates C for every voter).
  const auto ps = pareto_set(profile);
  EXPECT_TRUE(std::find(ps.begin(), ps.end(), 2) == ps.end());
}

// ---------------------------------------------------------------------------
// (f) pairwise_ranking places Copeland winner first.
// ---------------------------------------------------------------------------

TEST(Integration_PairwiseRanking, CopelandWinner_RankedFirst) {
  const auto pm = pairwise_matrix(kCondorcetAlts, kCondorcetVoters, kEuc);
  const auto ranking = pairwise_ranking(pm, TieBreak::kSmallestIndex);
  // A (index 0) has the highest Copeland score; should be ranked first.
  EXPECT_EQ(ranking[0], 0);
}

TEST(Integration_PairwiseRanking, ConsistentWithCopelandWinner) {
  const auto pm = pairwise_matrix(kCondorcetAlts, kCondorcetVoters, kEuc);
  const auto ranking = pairwise_ranking(pm, TieBreak::kSmallestIndex);
  // Geometry layer copeland_winner returns the winning Point2d; verify
  // it matches the first element of the ranking.
  const auto cop_pt =
      geometry::copeland_winner(kCondorcetAlts, kCondorcetVoters, kEuc);
  EXPECT_TRUE(cop_pt.isApprox(kCondorcetAlts[ranking[0]]));
}

// ---------------------------------------------------------------------------
// (g) Uniform spatial profile is well-formed; positional rules run cleanly.
// ---------------------------------------------------------------------------

TEST(Integration_UniformProfile, RunsWithoutError) {
  const std::vector<Point2d> alts = {{0.0, 0.0}, {5.0, 0.0}, {10.0, 0.0}};
  PRNG prng(77);
  const auto profile =
      uniform_spatial_profile(100, alts, 0.0, 10.0, -1.0, 1.0, kEuc, prng);
  EXPECT_TRUE(is_well_formed(profile));
  // All four rules should execute without throwing.
  EXPECT_NO_THROW(plurality_all_winners(profile));
  EXPECT_NO_THROW(borda_all_winners(profile));
  EXPECT_NO_THROW(antiplurality_all_winners(profile));
  EXPECT_NO_THROW(pareto_set(profile));
}
