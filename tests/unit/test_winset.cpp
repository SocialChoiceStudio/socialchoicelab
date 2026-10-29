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
#include <limits>
#include <string>
#include <vector>

using socialchoicelab::core::types::Point2d;
using socialchoicelab::geometry::DistConfig;
using socialchoicelab::geometry::majority_prefers;
using socialchoicelab::geometry::weighted_winset_2d;
using socialchoicelab::geometry::winset_2d;
using socialchoicelab::geometry::winset_2d_petal;
using socialchoicelab::geometry::winset_contains;
using socialchoicelab::geometry::winset_is_empty;
using socialchoicelab::geometry::winset_with_veto_2d;
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

// ============================================================
// Salience weights (review findings S02b-001 and S02b-003)
// ============================================================

namespace {
namespace dist = socialchoicelab::preference::distance;

DistConfig make_cfg(dist::DistanceType type, double p, double w0, double w1) {
  DistConfig cfg;
  cfg.type = type;
  cfg.order_p = p;
  cfg.salience_weights = {w0, w1};
  return cfg;
}
}  // namespace

// S02b-001 reproduction: one voter at (0,0), sq (1,0), k = 1. The voter's
// preferred-to set is the open unit disc for any equal weights, because the
// weights scale d(v, x) and d(v, sq) alike. Before the fix, weights (2,2)
// gave a disc of radius 2 and weights (0.5,0.5) one of radius 0.5.
TEST(WinsetWeights, EqualNonUnitWeights_OneVoter_MatchesMajorityPrefers) {
  const std::vector<Point2d> voters = {Point2d(0.0, 0.0)};
  const Point2d sq(1.0, 0.0);
  for (double w : {2.0, 0.5}) {
    const DistConfig cfg = make_cfg(dist::DistanceType::EUCLIDEAN, 2.0, w, w);
    const auto ws = winset_2d(sq, voters, cfg, 1);
    for (const Point2d& x : {Point2d(1.5, 0.0), Point2d(0.9, 0.0),
                             Point2d(0.5, 0.0), Point2d(0.0, 0.3)}) {
      EXPECT_EQ(winset_contains(ws, x), majority_prefers(x, sq, voters, cfg, 1))
          << "w = " << w << ", x = (" << x.x() << ", " << x.y() << ")";
    }
    EXPECT_FALSE(winset_contains(ws, Point2d(1.5, 0.0))) << "w = " << w;
    EXPECT_TRUE(winset_contains(ws, Point2d(0.9, 0.0))) << "w = " << w;
  }
}

// For every distance type and equal non-unit / unequal weights, winset
// membership agrees with majority_prefers at grid points that are not near a
// preferred-to boundary (curved boundaries are sampled at 64 points).
TEST(WinsetWeights, AllMetrics_AgreeWithMajorityPrefersOnGrid) {
  struct Metric {
    dist::DistanceType type;
    double p;
  };
  const Metric metrics[] = {{dist::DistanceType::EUCLIDEAN, 2.0},
                            {dist::DistanceType::MANHATTAN, 1.0},
                            {dist::DistanceType::CHEBYSHEV, 1.0},
                            {dist::DistanceType::MINKOWSKI, 3.0}};
  const double weight_sets[][2] = {{2.0, 2.0}, {0.5, 0.5}, {2.0, 0.5}};
  const std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(2.0, 0.2),
                                       Point2d(0.8, 1.6)};
  const Point2d sq(1.4, 1.0);
  for (const auto& m : metrics) {
    for (const auto& w : weight_sets) {
      const DistConfig cfg = make_cfg(m.type, m.p, w[0], w[1]);
      const auto ws = winset_2d(sq, voters, cfg);
      int checked = 0;
      for (int i = 0; i <= 24; ++i) {
        for (int j = 0; j <= 24; ++j) {
          const Point2d x(-0.5 + 0.125 * i, -0.5 + 0.125 * j);
          bool near_boundary = false;
          for (const auto& v : voters) {
            const double dx = dist::calculate_distance(v, x, m.type, m.p,
                                                       cfg.salience_weights);
            const double ds = dist::calculate_distance(v, sq, m.type, m.p,
                                                       cfg.salience_weights);
            if (std::abs(dx - ds) < 0.02 * ds) near_boundary = true;
          }
          if (near_boundary) continue;
          ++checked;
          EXPECT_EQ(winset_contains(ws, x),
                    majority_prefers(x, sq, voters, cfg))
              << "type " << static_cast<int>(m.type) << ", w = (" << w[0]
              << ", " << w[1] << "), x = (" << x.x() << ", " << x.y() << ")";
        }
      }
      EXPECT_GT(checked, 400);
    }
  }
}

// S02b-003: a weight of 0 (or a negative or non-finite weight) used to give
// Inf/NaN polygon vertices that aborted the process inside CGAL. Every
// function that builds preferred-to polygons now throws instead.
TEST(WinsetWeights, NonPositiveOrNonFiniteWeightThrows) {
  const std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(1.0, 0.0),
                                       Point2d(0.5, 1.0)};
  const Point2d sq(0.3, 0.5);
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();
  const double bad[][2] = {
    {0.0, 1.0}, {1.0, 0.0}, {-1.0, 1.0}, {nan, 1.0}, {1.0, inf}};
  for (const auto& m :
       {dist::DistanceType::EUCLIDEAN, dist::DistanceType::MANHATTAN,
        dist::DistanceType::CHEBYSHEV, dist::DistanceType::MINKOWSKI}) {
    for (const auto& w : bad) {
      const DistConfig cfg = make_cfg(m, 3.0, w[0], w[1]);
      try {
        (void)winset_2d(sq, voters, cfg);
        ADD_FAILURE() << "winset_2d accepted weights (" << w[0] << ", " << w[1]
                      << ")";
      } catch (const std::invalid_argument& e) {
        const std::string msg = e.what();
        EXPECT_NE(msg.find("winset_2d: salience_weights["), std::string::npos)
            << msg;
        EXPECT_NE(msg.find("finite and > 0"), std::string::npos) << msg;
      }
      EXPECT_THROW((void)winset_2d_petal(sq, voters, cfg),
                   std::invalid_argument);
      EXPECT_THROW((void)winset_with_veto_2d(sq, voters, cfg, {}),
                   std::invalid_argument);
      EXPECT_THROW((void)weighted_winset_2d(sq, voters, {1.0, 1.0, 1.0}, cfg),
                   std::invalid_argument);
    }
  }
}

// The check runs before any shape is built, even when every voter is at sq.
TEST(WinsetWeights, ZeroWeightThrowsEvenWhenVotersAtSQ) {
  const std::vector<Point2d> voters = {Point2d(0.3, 0.5)};
  const DistConfig cfg = make_cfg(dist::DistanceType::EUCLIDEAN, 2.0, 0.0, 1.0);
  EXPECT_THROW((void)winset_2d(Point2d(0.3, 0.5), voters, cfg),
               std::invalid_argument);
}

TEST(WinsetWeights, SemiAxisOverflowThrows) {
  const std::vector<Point2d> voters = {Point2d(0.0, 0.0)};
  const DistConfig cfg =
      make_cfg(dist::DistanceType::EUCLIDEAN, 2.0, 1e-310, 1.0);
  EXPECT_THROW((void)winset_2d(Point2d(0.0, 1.0), voters, cfg),
               std::invalid_argument);
}

// Convention B: a weight of 0 still masks a dimension in distance-only
// computations such as majority_prefers.
TEST(WinsetWeights, ZeroWeightStillAllowedForMajorityPrefers) {
  const std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(0.0, 5.0),
                                       Point2d(1.0, -5.0)};
  const DistConfig cfg = make_cfg(dist::DistanceType::EUCLIDEAN, 2.0, 1.0, 0.0);
  // Only x matters: x = 0.1 is closer than x = 2.0 for all three voters.
  EXPECT_TRUE(
      majority_prefers(Point2d(0.1, 100.0), Point2d(2.0, 0.0), voters, cfg, 3));
}
