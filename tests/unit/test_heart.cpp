// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

// Tests for Phase G1 (heart) and G2 (heart_boundary_2d).

#include <gtest/gtest.h>
#include <socialchoicelab/geometry/heart.h>

#include <algorithm>
#include <cmath>
#include <vector>

using socialchoicelab::core::types::Point2d;
using socialchoicelab::geometry::DistConfig;
using socialchoicelab::geometry::heart_boundary_2d;
using socialchoicelab::geometry::uncovered_set_boundary_2d;

static const DistConfig kEuc{};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// The verified spatial Condorcet cycle from test_core.cpp:
//   A=(2,0), B=(0,2), C=(-2,0);  V1=(1,0.5), V2=(-0.5,1.5), V3=(-0.5,-1)
//   A beats B, B beats C, C beats A.
static const std::vector<Point2d> kCycleAlts = {
  {2.0, 0.0}, {0.0, 2.0}, {-2.0, 0.0}};
static const std::vector<Point2d> kCycleVoters = {
  {1.0, 0.5}, {-0.5, 1.5}, {-0.5, -1.0}};

// Condorcet winner configuration (from test_core.cpp):
//   A=(0,0) beats B=(5,5) and C=(5,-5) under simple majority.
static const std::vector<Point2d> kWinnerAlts = {
  {0.0, 0.0}, {5.0, 5.0}, {5.0, -5.0}};
static const std::vector<Point2d> kWinnerVoters = {
  {0.1, 0.0}, {-0.1, 0.0}, {5.1, 5.0}};

static bool contains(const std::vector<Point2d>& v, const Point2d& p,
                     double tol = 1e-9) {
  return std::any_of(v.begin(), v.end(), [&](const Point2d& q) {
    return std::abs(q.x() - p.x()) < tol && std::abs(q.y() - p.y()) < tol;
  });
}

// ---------------------------------------------------------------------------
// G1 — heart() over finite alternatives
// ---------------------------------------------------------------------------

TEST(Heart, EmptyAlternatives_ReturnsEmpty) {
  EXPECT_TRUE(heart({}, kCycleVoters).empty());
}

TEST(Heart, EmptyVoters_Throws) {
  EXPECT_THROW(heart(kCycleAlts, {}), std::invalid_argument);
}

TEST(Heart, SingleAlternative_IsItsOwnHeart) {
  const std::vector<Point2d> one_alt = {{1.0, 1.0}};
  const auto h = heart(one_alt, kCycleVoters);
  ASSERT_EQ(h.size(), 1u);
  EXPECT_NEAR(h[0].x(), 1.0, 1e-9);
  EXPECT_NEAR(h[0].y(), 1.0, 1e-9);
}

TEST(Heart, CondorcetWinner_IsOnlyElement) {
  // With a Condorcet winner, Heart = {winner} (Property 3 from module header).
  // A=(0,0) is the Condorcet winner.
  const auto h = heart(kWinnerAlts, kWinnerVoters, kEuc, -1);
  ASSERT_EQ(h.size(), 1u)
      << "Heart should be a singleton when Condorcet winner exists.";
  EXPECT_NEAR(h[0].x(), 0.0, 1e-9);
  EXPECT_NEAR(h[0].y(), 0.0, 1e-9);
}

TEST(Heart, CondorcetCycle_EqualsUncoveredSet) {
  // Pure 3-alternative Condorcet cycle → Heart = Uncovered Set = {A,B,C}.
  // (Property 4 from module header: each alternative is defended by the other
  // two.)
  const auto h = heart(kCycleAlts, kCycleVoters, kEuc, -1);
  EXPECT_EQ(h.size(), 3u)
      << "Heart of a Condorcet cycle should equal the full alternative set.";
  for (const auto& alt : kCycleAlts) {
    EXPECT_TRUE(contains(h, alt)) << "Alternative (" << alt.x() << ","
                                  << alt.y() << ") should be in the Heart.";
  }
}

TEST(Heart, HeartSubsetOfUncoveredSet) {
  // Theorem: Heart ⊆ Uncovered Set for any configuration.
  // Verify with the cycle configuration.
  const auto h = heart(kCycleAlts, kCycleVoters, kEuc, -1);
  const auto unc = uncovered_set(kCycleAlts, kCycleVoters, kEuc, -1);
  for (const auto& hp : h) {
    EXPECT_TRUE(contains(unc, hp))
        << "Heart element (" << hp.x() << "," << hp.y()
        << ") is not in the uncovered set.";
  }
}

TEST(Heart, HeartSubsetOfUncoveredSet_WinnerCase) {
  const auto h = heart(kWinnerAlts, kWinnerVoters, kEuc, -1);
  const auto unc = uncovered_set(kWinnerAlts, kWinnerVoters, kEuc, -1);
  for (const auto& hp : h) {
    EXPECT_TRUE(contains(unc, hp))
        << "Heart element (" << hp.x() << "," << hp.y()
        << ") is not in the uncovered set.";
  }
}

TEST(Heart, HeartIsNonEmpty_ForNonEmptyAlternatives) {
  // Theorem: the Heart is always non-empty (Laffond et al. 1993).
  const auto h = heart(kCycleAlts, kCycleVoters, kEuc, -1);
  EXPECT_FALSE(h.empty());

  const auto h2 = heart(kWinnerAlts, kWinnerVoters, kEuc, -1);
  EXPECT_FALSE(h2.empty());
}

TEST(Heart, Supermajority_CanExpandHeart) {
  // Under unanimity (k = n = 3) few alternatives beat others, so the
  // Heart may expand relative to simple majority.  At minimum, the Heart
  // with a higher k is a superset of the Heart under simple majority.
  // (Harder to beat → harder to exclude → Heart grows or stays the same.)
  const auto h_simple = heart(kWinnerAlts, kWinnerVoters, kEuc, -1);
  const auto h_unanimity = heart(kWinnerAlts, kWinnerVoters, kEuc, 3);

  // Simple majority Heart ⊆ unanimity Heart is guaranteed since fewer
  // beats under unanimity means less can be excluded.
  for (const auto& hp : h_simple) {
    EXPECT_TRUE(contains(h_unanimity, hp))
        << "Simple-majority Heart element not in unanimity Heart.";
  }
}

TEST(Heart, FourAlternatives_HeartSubsetOfUncoveredSet) {
  // 4 alternatives: the verified 3-way cycle {A,B,C} plus a dominated D.
  // Alternatives placed far from centre so D at origin is dominated.
  //
  // Alternatives: A=(5,0), B=(0,5), C=(-5,0), D=(0,0) [dominated centre].
  // Voters: 3 voters that produce A>B>C>A, all far from D so they also
  // prefer any cycle member to D.
  //
  // Cycle voters (scaled up so D at origin is clearly dominated):
  //   V1=(3, 1): near A (5,0)
  //   V2=(-1, 3): near B (0,5)
  //   V3=(-1,-3): near C (-5,0)
  // D=(0,0): distance from each voter ≈ 3.2; distance to each alt ≈ 5.
  // All voters prefer D over any alt? No: d(V1,D)=sqrt(9+1)≈3.16,
  // d(V1,A)=sqrt(4+1)≈2.24. V1 prefers A over D. ✓
  //
  // Check cycle: majority(A,B)? V1 prefers A (d≈2.24 vs d≈sqrt(9+16)=5).
  // V2 prefers B (d≈sqrt(1+4)≈2.24 vs d≈sqrt(25)=5). V3 prefers...
  // d(V3,A)=sqrt(36+9)≈6.7, d(V3,B)=sqrt(1+64)≈8.1. V3 prefers A.
  // Count: 2 prefer A, 1 prefers B. A beats B under k=2 of 3. ✓
  //
  // majority(B,C)? V1: d(V1,B)=5, d(V1,C)≈6.7. V1 prefers B.
  // V2: d(V2,B)≈2.24, d(V2,C)=sqrt(16+9)≈5. V2 prefers B.
  // V3: d(V3,B)≈8.1, d(V3,C)=sqrt(16+9)≈4.47. V3 prefers C.
  // 2 prefer B. B beats C ✓.
  //
  // majority(C,A)? V1: d(V1,C)≈6.7, d(V1,A)≈2.24. V1 prefers A.
  // V2: d(V2,C)≈5, d(V2,A)=sqrt(25+9)≈5.83. V2 prefers C.
  // V3: d(V3,C)≈4.47, d(V3,A)≈6.7. V3 prefers C.
  // 2 prefer C. C beats A ✓. → Confirmed cycle.
  //
  // D is dominated: all 3 voters prefer each cycle member to D (check V1
  // above; V2 prefers B over D: d(V2,D)=sqrt(1+9)≈3.16 < d(V2,B)≈2.24?
  // Wait: d(V2,B)=sqrt(1+4)≈2.24 < d(V2,D)≈3.16 → V2 prefers B.
  // V2 prefers C over D: d(V2,C)=5, d(V2,D)≈3.16 → V2 prefers D to C!
  // So D is NOT dominated by C from V2's perspective.
  //
  // This shows that constructing a clean "D dominated by all" spatial config
  // is non-trivial.  The test verifies the containment theorem only.

  const std::vector<Point2d> alts = {
    {5.0, 0.0},   // A
    {0.0, 5.0},   // B
    {-5.0, 0.0},  // C
    {0.0, 0.0},   // D — centre
  };
  const std::vector<Point2d> voters = {{3.0, 1.0}, {-1.0, 3.0}, {-1.0, -3.0}};

  const auto h = heart(alts, voters, kEuc, -1);
  const auto unc = uncovered_set(alts, voters, kEuc, -1);

  // Theorem: Heart ⊆ Uncovered Set (always).
  EXPECT_FALSE(h.empty()) << "Heart must be non-empty.";
  for (const auto& hp : h) {
    EXPECT_TRUE(contains(unc, hp, 1e-9))
        << "Heart element (" << hp.x() << "," << hp.y()
        << ") is not in the uncovered set (4-alternative case).";
  }
  // Heart size ≤ uncovered set size.
  EXPECT_LE(h.size(), unc.size());
}

// ---------------------------------------------------------------------------
// G2 — heart_boundary_2d() — continuous approximation
// ---------------------------------------------------------------------------

TEST(HeartBoundary, TooFewVoters_Throws) {
  EXPECT_THROW(heart_boundary_2d({{0.0, 0.0}}), std::invalid_argument);
  EXPECT_THROW(heart_boundary_2d({}), std::invalid_argument);
}

TEST(HeartBoundary, InvalidResolution_Throws) {
  const std::vector<Point2d> voters = {{0.0, 0.0}, {1.0, 0.0}, {0.5, 0.866}};
  EXPECT_THROW(heart_boundary_2d(voters, kEuc, 1), std::invalid_argument);
}

TEST(HeartBoundary, EquilateralTriangle_ProducesNonEmptyPolygon) {
  // Symmetric triangle: McKelvey's chaos theorem → no Condorcet winner.
  // The Heart boundary should be a non-trivial polygon covering the centre.
  const std::vector<Point2d> voters = {{0.0, 0.0}, {1.0, 0.0}, {0.5, 0.866}};
  const Polygon2E poly = heart_boundary_2d(voters, kEuc, 10, -1);
  EXPECT_GE(poly.size(), 3u)
      << "Heart boundary should be a non-trivial polygon.";
}

TEST(HeartBoundary, CollinearVoters_HeartNearMedian) {
  // 3 collinear voters at (0,0), (1,0), (2,0). The Condorcet winner is at
  // (1,0) (median voter theorem).  The Heart boundary should be a polygon
  // whose centroid is near (1, 0).
  const std::vector<Point2d> voters = {{0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}};
  const Polygon2E poly = heart_boundary_2d(voters, kEuc, 10, -1);
  ASSERT_GE(poly.size(), 1u);

  // Compute centroid of the polygon.
  double cx = 0.0;
  double cy = 0.0;
  for (auto it = poly.vertices_begin(); it != poly.vertices_end(); ++it) {
    cx += CGAL::to_double(it->x());
    cy += CGAL::to_double(it->y());
  }
  cx /= static_cast<double>(poly.size());
  cy /= static_cast<double>(poly.size());

  EXPECT_NEAR(cx, 1.0, 0.5)
      << "Heart centroid x should be near the median (1).";
  EXPECT_NEAR(cy, 0.0, 0.5) << "Heart centroid y should be near 0.";
}

TEST(HeartBoundary, HeartSubsetOfUncoveredSetBoundary) {
  // The Heart boundary bounding box should be contained within the uncovered
  // set boundary bounding box (Heart ⊆ Uncovered Set).
  const std::vector<Point2d> voters = {{0.0, 0.0}, {2.0, 0.0}, {1.0, 2.0}};

  const Polygon2E heart_poly = heart_boundary_2d(voters, kEuc, 10, -1);
  const Polygon2E unc_poly = uncovered_set_boundary_2d(voters, kEuc, 10, -1);

  ASSERT_GE(heart_poly.size(), 1u);
  ASSERT_GE(unc_poly.size(), 1u);

  // Compute bounding boxes.
  auto bbox = [](const Polygon2E& p) -> std::array<double, 4> {
    double xmin = CGAL::to_double(p.vertices_begin()->x());
    double xmax = xmin;
    double ymin = CGAL::to_double(p.vertices_begin()->y());
    double ymax = ymin;
    for (auto it = p.vertices_begin(); it != p.vertices_end(); ++it) {
      xmin = std::min(xmin, CGAL::to_double(it->x()));
      xmax = std::max(xmax, CGAL::to_double(it->x()));
      ymin = std::min(ymin, CGAL::to_double(it->y()));
      ymax = std::max(ymax, CGAL::to_double(it->y()));
    }
    return {xmin, xmax, ymin, ymax};
  };

  auto hb = bbox(heart_poly);
  auto ub = bbox(unc_poly);
  const double tol = 0.5;  // generous tolerance for grid approximation

  EXPECT_GE(hb[0], ub[0] - tol)
      << "Heart bbox xmin exceeds uncovered set bbox.";
  EXPECT_LE(hb[1], ub[1] + tol)
      << "Heart bbox xmax exceeds uncovered set bbox.";
  EXPECT_GE(hb[2], ub[2] - tol)
      << "Heart bbox ymin exceeds uncovered set bbox.";
  EXPECT_LE(hb[3], ub[3] + tol)
      << "Heart bbox ymax exceeds uncovered set bbox.";
}

TEST(HeartBoundary, AllVotersSameIdeal_NearThatIdeal) {
  // All voters at (1,1): (1,1) is the Condorcet winner, Heart = {(1,1)}.
  // The heart_boundary polygon should be near (1,1).
  const std::vector<Point2d> voters = {{1.0, 1.0}, {1.0, 1.0}, {1.0, 1.0}};
  const Polygon2E poly = heart_boundary_2d(voters, kEuc, 10, -1);
  ASSERT_GE(poly.size(), 1u);

  // All vertices should be near (1,1).
  for (auto it = poly.vertices_begin(); it != poly.vertices_end(); ++it) {
    EXPECT_NEAR(CGAL::to_double(it->x()), 1.0, 0.5);
    EXPECT_NEAR(CGAL::to_double(it->y()), 1.0, 0.5);
  }
}
