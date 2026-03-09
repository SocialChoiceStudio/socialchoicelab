// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

// Tests for Phase P1 (Profile, build_spatial_profile,
// profile_from_utility_matrix, is_well_formed) and Phase P2
// (uniform_spatial_profile, gaussian_spatial_profile, impartial_culture).

#include <gtest/gtest.h>
#include <socialchoicelab/aggregation/profile.h>
#include <socialchoicelab/aggregation/profile_generators.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <vector>

using socialchoicelab::aggregation::build_spatial_profile;
using socialchoicelab::aggregation::gaussian_spatial_profile;
using socialchoicelab::aggregation::impartial_culture;
using socialchoicelab::aggregation::is_well_formed;
using socialchoicelab::aggregation::Profile;
using socialchoicelab::aggregation::profile_from_utility_matrix;
using socialchoicelab::aggregation::uniform_spatial_profile;
using socialchoicelab::core::rng::PRNG;
using socialchoicelab::core::types::Point2d;
using socialchoicelab::geometry::DistConfig;

static const DistConfig kEuc{};

// ---------------------------------------------------------------------------
// P1 — is_well_formed
// ---------------------------------------------------------------------------

TEST(IsWellFormed, EmptyProfile_IsWellFormed) {
  Profile p;
  p.n_voters = 0;
  p.n_alts = 0;
  EXPECT_TRUE(is_well_formed(p));
}

TEST(IsWellFormed, ValidProfile_IsWellFormed) {
  Profile p;
  p.n_voters = 2;
  p.n_alts = 3;
  p.rankings = {{1, 2, 0}, {0, 1, 2}};
  EXPECT_TRUE(is_well_formed(p));
}

TEST(IsWellFormed, WrongRankingSize_NotWellFormed) {
  Profile p;
  p.n_voters = 1;
  p.n_alts = 3;
  p.rankings = {{0, 1}};  // missing one alternative
  EXPECT_FALSE(is_well_formed(p));
}

TEST(IsWellFormed, DuplicateIndex_NotWellFormed) {
  Profile p;
  p.n_voters = 1;
  p.n_alts = 3;
  p.rankings = {{0, 0, 1}};  // 0 duplicated, 2 missing
  EXPECT_FALSE(is_well_formed(p));
}

TEST(IsWellFormed, OutOfRangeIndex_NotWellFormed) {
  Profile p;
  p.n_voters = 1;
  p.n_alts = 2;
  p.rankings = {{0, 5}};
  EXPECT_FALSE(is_well_formed(p));
}

// ---------------------------------------------------------------------------
// P1 — build_spatial_profile
// ---------------------------------------------------------------------------

TEST(BuildSpatialProfile, EmptyAlternatives_EmptyRankings) {
  const std::vector<Point2d> alts;
  const std::vector<Point2d> voters = {{0.0, 0.0}};
  const auto p = build_spatial_profile(alts, voters, kEuc);
  EXPECT_EQ(p.n_voters, 1);
  EXPECT_EQ(p.n_alts, 0);
  EXPECT_TRUE(is_well_formed(p));
}

TEST(BuildSpatialProfile, SingleAlternative_SingleVoter) {
  const std::vector<Point2d> alts = {{1.0, 1.0}};
  const std::vector<Point2d> voters = {{0.0, 0.0}};
  const auto p = build_spatial_profile(alts, voters, kEuc);
  EXPECT_EQ(p.n_voters, 1);
  EXPECT_EQ(p.n_alts, 1);
  EXPECT_EQ(p.rankings[0][0], 0);
  EXPECT_TRUE(is_well_formed(p));
}

TEST(BuildSpatialProfile, EmptyVoters_Throws) {
  const std::vector<Point2d> alts = {{0.0, 0.0}};
  EXPECT_THROW(build_spatial_profile(alts, {}, kEuc), std::invalid_argument);
}

TEST(BuildSpatialProfile, CondorcetWinnerNearest_RankedFirst) {
  // A=(0,0), B=(5,5), C=(5,-5).  Voter at (0.5,0) is nearest to A.
  const std::vector<Point2d> alts = {{0.0, 0.0}, {5.0, 5.0}, {5.0, -5.0}};
  const std::vector<Point2d> voters = {{0.5, 0.0}};
  const auto p = build_spatial_profile(alts, voters, kEuc);
  EXPECT_EQ(p.rankings[0][0], 0);  // A is first choice
  EXPECT_TRUE(is_well_formed(p));
}

TEST(BuildSpatialProfile, CollinearVoters_MedianPreference) {
  // Alternatives at x=0 and x=10; voter at x=3 prefers A(0).
  const std::vector<Point2d> alts = {{0.0, 0.0}, {10.0, 0.0}};
  const std::vector<Point2d> voters = {{3.0, 0.0}};
  const auto p = build_spatial_profile(alts, voters, kEuc);
  EXPECT_EQ(p.rankings[0][0], 0);
  EXPECT_TRUE(is_well_formed(p));
}

TEST(BuildSpatialProfile, TiesBrokenByIndex) {
  // Voter at origin; A=(-1,0) and B=(1,0) equidistant → A (index 0) ranked
  // first.
  const std::vector<Point2d> alts = {{-1.0, 0.0}, {1.0, 0.0}};
  const std::vector<Point2d> voters = {{0.0, 0.0}};
  const auto p = build_spatial_profile(alts, voters, kEuc);
  EXPECT_EQ(p.rankings[0][0], 0);  // A has lower index → ranked first
}

TEST(BuildSpatialProfile, MultipleVoters_WellFormed) {
  const std::vector<Point2d> alts = {{0.0, 0.0}, {5.0, 0.0}, {10.0, 0.0}};
  const std::vector<Point2d> voters = {{1.0, 0.0}, {9.0, 0.0}, {5.0, 0.0}};
  const auto p = build_spatial_profile(alts, voters, kEuc);
  EXPECT_EQ(p.n_voters, 3);
  EXPECT_EQ(p.n_alts, 3);
  EXPECT_TRUE(is_well_formed(p));
  // Voter 0 at x=1: nearest A(0), then B(5), then C(10)
  EXPECT_EQ(p.rankings[0][0], 0);
  // Voter 1 at x=9: nearest C(10), then B(5), then A(0)
  EXPECT_EQ(p.rankings[1][0], 2);
  // Voter 2 at x=5: equidistant A and C; B is nearest (distance 0)
  EXPECT_EQ(p.rankings[2][0], 1);
}

TEST(BuildSpatialProfile, OneDimensionalEmbeddedFixture_BehavesAsExpected) {
  // Explicit 1D compatibility check using x-axis embedding in Point2d.
  // Alternatives at x = {0, 2, 4}; voters at x = {0.25, 1.0, 3.0}.
  const std::vector<Point2d> alts = {{0.0, 0.0}, {2.0, 0.0}, {4.0, 0.0}};
  const std::vector<Point2d> voters = {{0.25, 0.0}, {1.0, 0.0}, {3.0, 0.0}};
  const auto p = build_spatial_profile(alts, voters, kEuc);

  ASSERT_TRUE(is_well_formed(p));
  ASSERT_EQ(p.n_voters, 3);
  ASSERT_EQ(p.n_alts, 3);

  // Voter at 0.25: 0 is closest, then 2, then 4.
  EXPECT_EQ(p.rankings[0], (std::vector<int>{0, 1, 2}));

  // Voter at 1.0: tie between 0 and 2; index 0 should win the tie.
  EXPECT_EQ(p.rankings[1], (std::vector<int>{0, 1, 2}));

  // Voter at 3.0: tie between 2 and 4; index 1 should win the tie.
  EXPECT_EQ(p.rankings[2], (std::vector<int>{1, 2, 0}));
}

TEST(BuildSpatialProfile, OneDimensionalEmbeddedMidpointTie_BreaksByIndex) {
  // On the line, a midpoint voter should rank the lower-index tied
  // alternative first.
  const std::vector<Point2d> alts = {{-1.0, 0.0}, {1.0, 0.0}, {3.0, 0.0}};
  const std::vector<Point2d> voters = {{0.0, 0.0}};
  const auto p = build_spatial_profile(alts, voters, kEuc);

  ASSERT_TRUE(is_well_formed(p));
  EXPECT_EQ(p.rankings[0], (std::vector<int>{0, 1, 2}));
}

// ---------------------------------------------------------------------------
// P1 — profile_from_utility_matrix
// ---------------------------------------------------------------------------

TEST(ProfileFromUtility, BasicOrdering) {
  // Voter 0: u(A)=3, u(B)=1, u(C)=2  → ranking: A > C > B  → [0, 2, 1]
  // Voter 1: u(A)=0, u(B)=5, u(C)=2  → ranking: B > C > A  → [1, 2, 0]
  Eigen::MatrixXd U(2, 3);
  U << 3.0, 1.0, 2.0, 0.0, 5.0, 2.0;
  const auto p = profile_from_utility_matrix(U);
  EXPECT_EQ(p.n_voters, 2);
  EXPECT_EQ(p.n_alts, 3);
  EXPECT_EQ(p.rankings[0][0], 0);  // A first
  EXPECT_EQ(p.rankings[0][1], 2);  // C second
  EXPECT_EQ(p.rankings[0][2], 1);  // B last
  EXPECT_EQ(p.rankings[1][0], 1);  // B first
  EXPECT_TRUE(is_well_formed(p));
}

TEST(ProfileFromUtility, TiesBrokenBySmallestIndex) {
  // Voter 0: equal utility for A and B; A (index 0) should be ranked first.
  Eigen::MatrixXd U(1, 2);
  U << 1.0, 1.0;
  const auto p = profile_from_utility_matrix(U);
  EXPECT_EQ(p.rankings[0][0], 0);
}

TEST(ProfileFromUtility, EmptyVoters_Throws) {
  Eigen::MatrixXd U(0, 3);
  EXPECT_THROW(profile_from_utility_matrix(U), std::invalid_argument);
}

TEST(ProfileFromUtility, SingleAlternative) {
  Eigen::MatrixXd U(2, 1);
  U << 5.0, 3.0;
  const auto p = profile_from_utility_matrix(U);
  EXPECT_TRUE(is_well_formed(p));
  EXPECT_EQ(p.rankings[0][0], 0);
  EXPECT_EQ(p.rankings[1][0], 0);
}

// ---------------------------------------------------------------------------
// P2 — impartial_culture
// ---------------------------------------------------------------------------

TEST(ImpartialCulture, WellFormed) {
  PRNG prng(42);
  const auto p = impartial_culture(50, 4, prng);
  EXPECT_EQ(p.n_voters, 50);
  EXPECT_EQ(p.n_alts, 4);
  EXPECT_TRUE(is_well_formed(p));
}

TEST(ImpartialCulture, Reproducible_SameSeed) {
  PRNG prng1(99), prng2(99);
  const auto p1 = impartial_culture(10, 3, prng1);
  const auto p2 = impartial_culture(10, 3, prng2);
  EXPECT_EQ(p1.rankings, p2.rankings);
}

TEST(ImpartialCulture, DifferentSeeds_DifferentProfiles) {
  PRNG prng1(1), prng2(2);
  const auto p1 = impartial_culture(100, 4, prng1);
  const auto p2 = impartial_culture(100, 4, prng2);
  EXPECT_NE(p1.rankings, p2.rankings);
}

TEST(ImpartialCulture, EqualOrderingFrequency_ChiSquare) {
  // For m=3 there are 3!=6 orderings.  With n=6000 draws each should
  // appear ~1000 times.  Chi-square with 5 d.f., α=1%: critical value ≈ 15.1.
  PRNG prng(2026);
  const int n = 6000;
  const auto p = impartial_culture(n, 3, prng);

  std::map<std::vector<int>, int> freq;
  for (const auto& r : p.rankings) ++freq[r];
  EXPECT_EQ(static_cast<int>(freq.size()), 6);  // all 6 orderings appear

  double chi2 = 0.0;
  const double expected = n / 6.0;
  for (const auto& [ordering, count] : freq) {
    const double diff = count - expected;
    chi2 += diff * diff / expected;
  }
  EXPECT_LT(chi2, 15.1);  // fail to reject at α=1%
}

TEST(ImpartialCulture, ZeroVoters_Throws) {
  PRNG prng(1);
  EXPECT_THROW(impartial_culture(0, 3, prng), std::invalid_argument);
}

TEST(ImpartialCulture, ZeroAlternatives_Throws) {
  PRNG prng(1);
  EXPECT_THROW(impartial_culture(5, 0, prng), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// P2 — uniform_spatial_profile
// ---------------------------------------------------------------------------

TEST(UniformSpatialProfile, WellFormed) {
  const std::vector<Point2d> alts = {{0.0, 0.0}, {5.0, 0.0}, {10.0, 0.0}};
  PRNG prng(7);
  const auto p =
      uniform_spatial_profile(20, alts, 0.0, 10.0, 0.0, 10.0, kEuc, prng);
  EXPECT_EQ(p.n_voters, 20);
  EXPECT_EQ(p.n_alts, 3);
  EXPECT_TRUE(is_well_formed(p));
}

TEST(UniformSpatialProfile, Reproducible_SameSeed) {
  const std::vector<Point2d> alts = {{0.0, 0.0}, {5.0, 5.0}};
  PRNG prng1(55), prng2(55);
  const auto p1 =
      uniform_spatial_profile(10, alts, 0.0, 10.0, 0.0, 10.0, kEuc, prng1);
  const auto p2 =
      uniform_spatial_profile(10, alts, 0.0, 10.0, 0.0, 10.0, kEuc, prng2);
  EXPECT_EQ(p1.rankings, p2.rankings);
}

TEST(UniformSpatialProfile, InvalidBounds_Throws) {
  const std::vector<Point2d> alts = {{0.0, 0.0}};
  PRNG prng(1);
  EXPECT_THROW(uniform_spatial_profile(5, alts, 5.0, 1.0, 0.0, 1.0, kEuc, prng),
               std::invalid_argument);
  EXPECT_THROW(uniform_spatial_profile(5, alts, 0.0, 1.0, 5.0, 1.0, kEuc, prng),
               std::invalid_argument);
}

// ---------------------------------------------------------------------------
// P2 — gaussian_spatial_profile
// ---------------------------------------------------------------------------

TEST(GaussianSpatialProfile, WellFormed) {
  const std::vector<Point2d> alts = {{-2.0, 0.0}, {0.0, 0.0}, {2.0, 0.0}};
  PRNG prng(13);
  const auto p =
      gaussian_spatial_profile(30, alts, Point2d(0.0, 0.0), 1.0, kEuc, prng);
  EXPECT_EQ(p.n_voters, 30);
  EXPECT_EQ(p.n_alts, 3);
  EXPECT_TRUE(is_well_formed(p));
}

TEST(GaussianSpatialProfile, InvalidStdDev_Throws) {
  const std::vector<Point2d> alts = {{0.0, 0.0}};
  PRNG prng(1);
  EXPECT_THROW(
      gaussian_spatial_profile(5, alts, Point2d(0.0, 0.0), 0.0, kEuc, prng),
      std::invalid_argument);
  EXPECT_THROW(
      gaussian_spatial_profile(5, alts, Point2d(0.0, 0.0), -1.0, kEuc, prng),
      std::invalid_argument);
}
