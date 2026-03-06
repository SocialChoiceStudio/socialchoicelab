// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// test_winset.cpp — GTest suite for winset.h (B3).
//
// Tests both winset_2d (incremental depth) and winset_2d_petal (reference).
// For small n, both functions are tested and their results compared.

#include <gtest/gtest.h>
#include <socialchoicelab/geometry/winset.h>

#include <cmath>
#include <vector>

using socialchoicelab::core::types::Point2d;
using socialchoicelab::geometry::DistConfig;
using socialchoicelab::geometry::winset_2d;
using socialchoicelab::geometry::winset_2d_petal;
using socialchoicelab::geometry::winset_contains;
using socialchoicelab::geometry::winset_is_empty;
using socialchoicelab::geometry::WinsetRegion;

static const DistConfig kEuclidean{};

// ============================================================
// Helpers
// ============================================================

// Equilateral triangle voter configuration:
//   V1=(0,0), V2=(1,0), V3=(0.5, √3/2 ≈ 0.866).
// The centroid (1/3, √3/6 ≈ 0.289) is the Plott equilibrium
// (unique Condorcet winner) for this symmetric layout.
static std::vector<Point2d> equilateral_voters() {
  return {Point2d(0.0, 0.0), Point2d(1.0, 0.0),
          Point2d(0.5, std::sqrt(3.0) / 2.0)};
}

static Point2d equilateral_centroid() {
  return Point2d(0.5, std::sqrt(3.0) / 6.0);
}

// ============================================================
// winset_is_empty / Condorcet winner detection
// ============================================================

// With a single voter whose ideal point IS the SQ, their preferred-to set
// is empty and the winset must be empty.
TEST(WinsetEmpty, SingleVoterAtSQ_DepthMethod) {
  std::vector<Point2d> voters = {Point2d(0.5, 0.5)};
  Point2d sq(0.5, 0.5);
  auto ws = winset_2d(sq, voters, kEuclidean);
  EXPECT_TRUE(winset_is_empty(ws));
}

TEST(WinsetEmpty, SingleVoterAtSQ_PetalMethod) {
  std::vector<Point2d> voters = {Point2d(0.5, 0.5)};
  Point2d sq(0.5, 0.5);
  auto ws = winset_2d_petal(sq, voters, kEuclidean);
  EXPECT_TRUE(winset_is_empty(ws));
}

// All voters at the SQ: every preferred-to set is empty → winset empty.
TEST(WinsetEmpty, AllVotersAtSQ_DepthMethod) {
  std::vector<Point2d> voters = {Point2d(0.3, 0.7), Point2d(0.3, 0.7),
                                 Point2d(0.3, 0.7)};
  Point2d sq(0.3, 0.7);
  auto ws = winset_2d(sq, voters, kEuclidean);
  EXPECT_TRUE(winset_is_empty(ws));
}

TEST(WinsetEmpty, AllVotersAtSQ_PetalMethod) {
  std::vector<Point2d> voters = {Point2d(0.3, 0.7), Point2d(0.3, 0.7),
                                 Point2d(0.3, 0.7)};
  Point2d sq(0.3, 0.7);
  auto ws = winset_2d_petal(sq, voters, kEuclidean);
  EXPECT_TRUE(winset_is_empty(ws));
}

// Note: the centroid of an equilateral triangle is NOT a Condorcet winner.
// Two voters can always agree on a point closer to the midpoint between them,
// so the winset at the centroid is non-empty (tested separately below).

// SQ at a vertex: the other two voters can form a majority → winset non-empty.
TEST(WinsetNonEmpty, EquilateralVertex_DepthMethod) {
  auto voters = equilateral_voters();
  Point2d sq(0.0, 0.0);  // at voter 1's ideal point
  auto ws = winset_2d(sq, voters, kEuclidean);
  EXPECT_FALSE(winset_is_empty(ws));
}

TEST(WinsetNonEmpty, EquilateralVertex_PetalMethod) {
  auto voters = equilateral_voters();
  Point2d sq(0.0, 0.0);
  auto ws = winset_2d_petal(sq, voters, kEuclidean);
  EXPECT_FALSE(winset_is_empty(ws));
}

// ============================================================
// winset_contains — point-in-winset queries
// ============================================================

// With SQ at (0,0) and voters at (0,0),(1,0),(0.5,0.866):
// voter 1 is AT the SQ (contributes nothing).
// Voters 2 and 3 form the only winning coalition.
// Their preferred-to sets are disks centred at (1,0) and (0.5,0.866),
// each with radius = distance from that voter to (0,0).
// The petal is the intersection of those two disks.
// The centroid (0.5, 0.289) should lie inside that petal.
TEST(WinsetContains, CentroidInsideWinset_DepthMethod) {
  auto voters = equilateral_voters();
  Point2d sq(0.0, 0.0);
  auto ws = winset_2d(sq, voters, kEuclidean);
  EXPECT_FALSE(winset_is_empty(ws));
  // The centroid is inside the petal of voters 2 and 3.
  EXPECT_TRUE(winset_contains(ws, equilateral_centroid()));
}

TEST(WinsetContains, CentroidInsideWinset_PetalMethod) {
  auto voters = equilateral_voters();
  Point2d sq(0.0, 0.0);
  auto ws = winset_2d_petal(sq, voters, kEuclidean);
  EXPECT_TRUE(winset_contains(ws, equilateral_centroid()));
}

// A point far outside all disks should not be in the winset.
TEST(WinsetContains, FarPoint_NotInWinset_DepthMethod) {
  auto voters = equilateral_voters();
  Point2d sq(0.0, 0.0);
  auto ws = winset_2d(sq, voters, kEuclidean);
  EXPECT_FALSE(winset_contains(ws, Point2d(100.0, 100.0)));
}

TEST(WinsetContains, FarPoint_NotInWinset_PetalMethod) {
  auto voters = equilateral_voters();
  Point2d sq(0.0, 0.0);
  auto ws = winset_2d_petal(sq, voters, kEuclidean);
  EXPECT_FALSE(winset_contains(ws, Point2d(100.0, 100.0)));
}

// ============================================================
// Cross-check: depth method vs petal method (small n)
// ============================================================

// For a known non-empty winset, both methods should agree on containment
// of several test points.
TEST(WinsetCrossCheck, DepthAndPetalAgree_EquilateralVertex) {
  auto voters = equilateral_voters();
  Point2d sq(0.0, 0.0);
  auto ws_depth = winset_2d(sq, voters, kEuclidean);
  auto ws_petal = winset_2d_petal(sq, voters, kEuclidean);

  std::vector<Point2d> test_pts = {
    equilateral_centroid(),  // inside (both)
    Point2d(0.5, 0.5),       // inside (both)
    Point2d(0.5, 0.7),       // inside (near voter 3)
    Point2d(100.0, 100.0),   // outside (both)
    Point2d(-1.0, 0.0),      // outside (both)
    Point2d(0.5, -0.5),      // outside (both)
  };

  for (const auto& pt : test_pts) {
    EXPECT_EQ(winset_contains(ws_depth, pt), winset_contains(ws_petal, pt))
        << "Disagreement at (" << pt.x() << ", " << pt.y() << ")";
  }
}

TEST(WinsetCrossCheck, DepthAndPetalAgree_IsEmpty_AllAtSQ) {
  std::vector<Point2d> voters = {Point2d(0.3, 0.7), Point2d(0.3, 0.7),
                                 Point2d(0.3, 0.7)};
  Point2d sq(0.3, 0.7);
  auto ws_depth = winset_2d(sq, voters, kEuclidean);
  auto ws_petal = winset_2d_petal(sq, voters, kEuclidean);
  EXPECT_EQ(winset_is_empty(ws_depth), winset_is_empty(ws_petal));
  EXPECT_TRUE(winset_is_empty(ws_depth));
}

// ============================================================
// Unanimity (k = n) and the Pareto set
// ============================================================

// Under unanimity, the winset at sq is empty iff sq is Pareto non-dominated
// — i.e., sq is inside the convex hull of the ideal points (Euclidean case).
// The centroid of the equilateral triangle is inside its convex hull, so no
// point is simultaneously closer to all three vertices: unanimity winset empty.
TEST(WinsetUnanimity, InsideConvexHull_Empty_DepthMethod) {
  auto voters = equilateral_voters();
  auto sq = equilateral_centroid();  // inside the convex hull
  const int n = static_cast<int>(voters.size());
  auto ws = winset_2d(sq, voters, kEuclidean, n);  // k = n = unanimity
  EXPECT_TRUE(winset_is_empty(ws));
}

TEST(WinsetUnanimity, InsideConvexHull_Empty_PetalMethod) {
  auto voters = equilateral_voters();
  auto sq = equilateral_centroid();
  const int n = static_cast<int>(voters.size());
  auto ws = winset_2d_petal(sq, voters, kEuclidean, n);
  EXPECT_TRUE(winset_is_empty(ws));
}

// SQ outside the convex hull: all voters can agree on a Pareto-improving
// move. SQ=(2,0) is outside the triangle; all three voters prefer (1,0)
// (V2's ideal, which is closer to each of them than (2,0) is).
TEST(WinsetUnanimity, OutsideConvexHull_NonEmpty_DepthMethod) {
  auto voters = equilateral_voters();
  Point2d sq(2.0, 0.0);  // outside the convex hull
  const int n = static_cast<int>(voters.size());
  auto ws = winset_2d(sq, voters, kEuclidean, n);
  EXPECT_FALSE(winset_is_empty(ws));
  // The area near V2=(1,0) should be in the winset.
  EXPECT_TRUE(winset_contains(ws, Point2d(1.0, 0.05)));
}

// ============================================================
// Supermajority (k > simple majority) → smaller winset
// ============================================================

// With 5 voters and k=4 (supermajority), the winset shrinks.
// With k=5 (unanimity), only points all 5 voters prefer to SQ qualify.
TEST(WinsetSupermajority, LargerK_SmallerOrEqualWinset) {
  std::vector<Point2d> voters = {
    Point2d(0.1, 0.1), Point2d(0.9, 0.1), Point2d(0.5, 0.9),
    Point2d(0.2, 0.8), Point2d(0.8, 0.8),
  };
  Point2d sq(0.0, 0.0);

  auto ws3 = winset_2d(sq, voters, kEuclidean, 3);  // simple majority
  auto ws4 = winset_2d(sq, voters, kEuclidean, 4);  // supermajority
  auto ws5 = winset_2d(sq, voters, kEuclidean, 5);  // unanimity

  // Non-empty at simple majority.
  EXPECT_FALSE(winset_is_empty(ws3));

  // A point inside ws3 may or may not be in ws4 or ws5.
  // A point inside ws5 must also be inside ws3 and ws4.
  Point2d inside_all(0.5, 0.5);
  if (winset_contains(ws5, inside_all)) {
    EXPECT_TRUE(winset_contains(ws4, inside_all));
    EXPECT_TRUE(winset_contains(ws3, inside_all));
  }
  if (winset_contains(ws4, inside_all)) {
    EXPECT_TRUE(winset_contains(ws3, inside_all));
  }
}

// ============================================================
// Manhattan distance
// ============================================================

TEST(WinsetManhattan, NonEmpty_DepthMethod) {
  DistConfig manhattan{
    socialchoicelab::preference::distance::DistanceType::MANHATTAN,
    1.0,
    {1.0, 1.0}};
  auto voters = equilateral_voters();
  Point2d sq(0.0, 0.0);
  auto ws = winset_2d(sq, voters, manhattan);
  EXPECT_FALSE(winset_is_empty(ws));
}

TEST(WinsetManhattan, CrossCheck_DepthAndPetal) {
  DistConfig manhattan{
    socialchoicelab::preference::distance::DistanceType::MANHATTAN,
    1.0,
    {1.0, 1.0}};
  auto voters = equilateral_voters();
  Point2d sq(0.0, 0.0);
  auto ws_depth = winset_2d(sq, voters, manhattan);
  auto ws_petal = winset_2d_petal(sq, voters, manhattan);

  EXPECT_EQ(winset_is_empty(ws_depth), winset_is_empty(ws_petal));
  EXPECT_EQ(winset_contains(ws_depth, equilateral_centroid()),
            winset_contains(ws_petal, equilateral_centroid()));
}

// ============================================================
// Chebyshev distance (B4)
// ============================================================

TEST(WinsetChebyshev, NonEmpty_DepthMethod) {
  DistConfig chebyshev{
    socialchoicelab::preference::distance::DistanceType::CHEBYSHEV,
    1.0,
    {1.0, 1.0}};
  auto voters = equilateral_voters();
  Point2d sq(0.0, 0.0);
  auto ws = winset_2d(sq, voters, chebyshev);
  EXPECT_FALSE(winset_is_empty(ws));
}

TEST(WinsetChebyshev, CrossCheck_DepthAndPetal) {
  DistConfig chebyshev{
    socialchoicelab::preference::distance::DistanceType::CHEBYSHEV,
    1.0,
    {1.0, 1.0}};
  auto voters = equilateral_voters();
  Point2d sq(0.0, 0.0);
  auto ws_depth = winset_2d(sq, voters, chebyshev);
  auto ws_petal = winset_2d_petal(sq, voters, chebyshev);

  EXPECT_EQ(winset_is_empty(ws_depth), winset_is_empty(ws_petal));
  EXPECT_EQ(winset_contains(ws_depth, equilateral_centroid()),
            winset_contains(ws_petal, equilateral_centroid()));
}

// Supermajority under Manhattan: unanimity (k=n) is empty when one voter sits
// exactly at the SQ (their preferred-to set is empty so no coalition of size n
// can agree), while simple majority (k=3 of 4) is non-empty.
TEST(WinsetNonEuclidean, Supermajority_Manhattan_SmallerThanSimpleMajority) {
  DistConfig manhattan{
    socialchoicelab::preference::distance::DistanceType::MANHATTAN,
    1.0,
    {1.0, 1.0}};
  // 4 voters: V1=(1,1), V2=(2,1), V3=(1,2) all in the upper-right quadrant
  // relative to SQ, plus V4=(0,0) sitting AT the SQ.
  // Under Manhattan, point (1,1) lies inside V1/V2/V3's preferred-to sets
  // (each diamond centred at Vᵢ with radius d(Vᵢ, SQ)), so simple majority
  // winset is non-empty.  V4 has an empty preferred-to set, so unanimity
  // (k=4) produces an empty winset.
  std::vector<Point2d> voters = {
    Point2d(1.0, 1.0),
    Point2d(2.0, 1.0),
    Point2d(1.0, 2.0),
    Point2d(0.0, 0.0),  // V4 AT the SQ
  };
  Point2d sq(0.0, 0.0);

  // Simple majority k=3 of 4: winset non-empty.
  auto ws_majority = winset_2d(sq, voters, manhattan);
  // Unanimity k=4: V4 at SQ → empty preferred-to set → empty winset.
  auto ws_unanimity = winset_2d(sq, voters, manhattan, 4);

  EXPECT_FALSE(winset_is_empty(ws_majority));
  EXPECT_TRUE(winset_is_empty(ws_unanimity));
}

// ============================================================
// k=1 edge case: at least 1 voter prefers x to sq
// ============================================================

TEST(WinsetEdge, K1_AnyPreference_Nonempty) {
  // Any SQ that is not universally ideal should have a non-empty k=1 winset.
  std::vector<Point2d> voters = {Point2d(0.5, 0.5), Point2d(0.8, 0.2)};
  Point2d sq(0.0, 0.0);
  auto ws = winset_2d(sq, voters, kEuclidean, 1);
  EXPECT_FALSE(winset_is_empty(ws));
}

// ============================================================
// Error handling
// ============================================================

TEST(WinsetError, EmptyVoters_DepthMethod) {
  std::vector<Point2d> voters;
  EXPECT_THROW(winset_2d(Point2d(0, 0), voters, kEuclidean),
               std::invalid_argument);
}

TEST(WinsetError, EmptyVoters_PetalMethod) {
  std::vector<Point2d> voters;
  EXPECT_THROW(winset_2d_petal(Point2d(0, 0), voters, kEuclidean),
               std::invalid_argument);
}

TEST(WinsetError, TooManyVoters_PetalMethod) {
  std::vector<Point2d> voters(16, Point2d(0.5, 0.5));
  EXPECT_THROW(winset_2d_petal(Point2d(0, 0), voters, kEuclidean, -1, 64, 15),
               std::invalid_argument);
}

TEST(WinsetError, KOutOfRange) {
  std::vector<Point2d> voters = {Point2d(0, 0), Point2d(1, 0)};
  EXPECT_THROW(winset_2d(Point2d(0.5, 0), voters, kEuclidean, 3),
               std::invalid_argument);
}
