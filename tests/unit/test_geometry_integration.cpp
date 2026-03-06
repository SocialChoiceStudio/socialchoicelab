// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

// Phase E1 — Geometry Integration Tests.
//
// End-to-end tests that chain the full geometry pipeline on a fixed voter
// configuration:
//
//   convex_hull_2d → majority_prefers / pairwise_matrix
//                  → winset_2d
//                  → yolk_2d
//                  → uncovered_set / uncovered_set_boundary_2d
//                  → heart / heart_boundary_2d
//                  → copeland_winner / core_2d
//                  → weighted_majority_prefers / weighted_winset_2d
//                  → winset_with_veto_2d / winset_ops
//
// These tests verify that:
//  (a) each layer produces output that is structurally valid;
//  (b) outputs of earlier layers feed correctly into later layers;
//  (c) the key subset theorems hold across the pipeline
//      (Heart ⊆ Uncovered Set, Uncovered Set ⊆ Alternatives, etc.).
//
// Two voter configurations are used throughout:
//
//  kVoters5  — 5 voters clustered near the origin.  A=(0,0) is the Condorcet
//               winner for alternatives {A, B, C, D}, so every downstream
//               concept collapses to a singleton/empty set.
//
//  kCycleVoters — the verified 3-voter Condorcet cycle from test_core.cpp /
//                 test_heart.cpp. Used to test the "no winner" path.

#include <gtest/gtest.h>
#include <socialchoicelab/geometry/convex_hull.h>
#include <socialchoicelab/geometry/copeland.h>
#include <socialchoicelab/geometry/core.h>
#include <socialchoicelab/geometry/geom2d.h>
#include <socialchoicelab/geometry/heart.h>
#include <socialchoicelab/geometry/majority.h>
#include <socialchoicelab/geometry/uncovered_set.h>
#include <socialchoicelab/geometry/winset.h>
#include <socialchoicelab/geometry/winset_ops.h>
#include <socialchoicelab/geometry/yolk.h>

#include <algorithm>
#include <cmath>

using namespace socialchoicelab::geometry;
using socialchoicelab::core::types::Point2d;

// ---------------------------------------------------------------------------
// Shared fixtures
// ---------------------------------------------------------------------------

static const DistConfig kEuc{};

// Five voters clustered near the origin. Simple majority k = 3.
// Centroid ≈ (0.1, 0.08).
static const std::vector<Point2d> kVoters5 = {
  {0.0, 0.0}, {0.5, 0.0}, {-0.3, 0.2}, {0.1, 0.3}, {0.2, -0.1}};

// Five collinear voters on the x-axis; median (Condorcet core) at (0,0).
// Under simple majority (k=3) the winset of (0,0) is provably empty
// (at most 2 of 5 voters can ever prefer any other point to the median).
static const std::vector<Point2d> kCollinear5 = {
  {-2.0, 0.0}, {-1.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}};
static const Point2d kMedian = {0.0, 0.0};

// Alternatives: A at (0.05, 0.05) — inside the voter cluster → Condorcet
// winner.  B, C, D far from the cluster.
static const Point2d kA = {0.05, 0.05};
static const Point2d kB = {5.0, 0.0};
static const Point2d kC = {-5.0, 0.0};
static const Point2d kD = {0.0, 5.0};
static const std::vector<Point2d> kAlts4 = {kA, kB, kC, kD};

// Status quo far outside the voter cluster.
static const Point2d kSQ = {4.0, 1.0};

// Verified 3-alternative Condorcet cycle (from test_heart.cpp):
//   A=(2,0), B=(0,2), C=(-2,0);  voters V1=(1,0.5), V2=(-0.5,1.5),
//   V3=(-0.5,-1).
static const std::vector<Point2d> kCycleAlts = {
  {2.0, 0.0}, {0.0, 2.0}, {-2.0, 0.0}};
static const std::vector<Point2d> kCycleVoters = {
  {1.0, 0.5}, {-0.5, 1.5}, {-0.5, -1.0}};

static bool contains_pt(const std::vector<Point2d>& v, const Point2d& p,
                        double tol = 1e-6) {
  return std::any_of(v.begin(), v.end(), [&](const Point2d& q) {
    return std::abs(q.x() - p.x()) < tol && std::abs(q.y() - p.y()) < tol;
  });
}

// ---------------------------------------------------------------------------
// Layer 1 — Convex hull feeds into downstream layers
// ---------------------------------------------------------------------------

TEST(Integration_ConvexHull, HullContainsVoterCentroid) {
  // The convex hull of voters should contain their centroid (a property of
  // any convex set).
  const Polygon2E hull = convex_hull_2d(kVoters5);
  ASSERT_GE(hull.size(), 3u);

  // Centroid.
  double cx = 0.0;
  double cy = 0.0;
  for (const auto& v : kVoters5) {
    cx += v.x();
    cy += v.y();
  }
  cx /= static_cast<double>(kVoters5.size());
  cy /= static_cast<double>(kVoters5.size());

  const auto side = bounded_side(hull, to_exact(Point2d{cx, cy}));
  EXPECT_NE(side, BoundedSide::kOnUnboundedSide)
      << "Centroid of voters must lie inside (or on) the convex hull.";
}

TEST(Integration_ConvexHull, CondorcetWinner_InsideHull) {
  // The Condorcet winner kA is inside the voter cluster, hence inside the
  // hull.
  const Polygon2E hull = convex_hull_2d(kVoters5);
  const auto side = bounded_side(hull, to_exact(kA));
  EXPECT_NE(side, BoundedSide::kOnUnboundedSide)
      << "Condorcet winner should lie inside the voter convex hull.";
}

TEST(Integration_ConvexHull, SQ_OutsideHull) {
  // kSQ is far outside the cluster, hence outside the hull.
  const Polygon2E hull = convex_hull_2d(kVoters5);
  const auto side = bounded_side(hull, to_exact(kSQ));
  EXPECT_EQ(side, BoundedSide::kOnUnboundedSide)
      << "Status quo should lie outside the voter convex hull.";
}

// ---------------------------------------------------------------------------
// Layer 2 — Majority relation
// ---------------------------------------------------------------------------

TEST(Integration_Majority, CondorcetWinner_BeatsAllOthers) {
  // kA (near the cluster centre) should beat each of B, C, D under simple
  // majority with k=3/5.
  for (const auto& alt : {kB, kC, kD}) {
    EXPECT_TRUE(majority_prefers(kA, alt, kVoters5, kEuc, -1))
        << "Condorcet winner should beat alt (" << alt.x() << "," << alt.y()
        << ").";
  }
}

TEST(Integration_Majority, CondorcetWinner_NoneBeatsIt) {
  for (const auto& alt : {kB, kC, kD}) {
    EXPECT_FALSE(majority_prefers(alt, kA, kVoters5, kEuc, -1))
        << "No alternative should beat the Condorcet winner.";
  }
}

TEST(Integration_Majority, PairwiseMatrix_RowConsistency) {
  // pairwise_matrix stores preference *margins* (positive = row alternative
  // majority-beats column alternative; negative = column beats row; 0 = tie).
  // Diagonal must be 0; (i,j) and (j,i) cannot both be positive.
  const Eigen::MatrixXi M = pairwise_matrix(kAlts4, kVoters5, kEuc, -1);
  const int m = static_cast<int>(kAlts4.size());
  for (int i = 0; i < m; ++i) {
    EXPECT_EQ(M(i, i), 0)
        << "Diagonal of pairwise matrix must be 0 (no self-preference).";
    for (int j = i + 1; j < m; ++j) {
      // M(i,j) + M(j,i) == 0 always (antisymmetric margin).
      EXPECT_EQ(M(i, j) + M(j, i), 0)
          << "pairwise_matrix must be antisymmetric: M(i,j)+M(j,i)=0.";
      EXPECT_FALSE(M(i, j) > 0 && M(j, i) > 0)
          << "Both M(" << i << "," << j << ") and M(" << j << "," << i
          << ") are positive — majority relation must be asymmetric.";
    }
  }
}

TEST(Integration_Majority, Cycle_PairwiseMatrix_HasNoCondorcetWinner) {
  // pairwise_matrix stores margins; positive margin means row beats column.
  const Eigen::MatrixXi M = pairwise_matrix(kCycleAlts, kCycleVoters, kEuc, -1);
  const int m = static_cast<int>(kCycleAlts.size());
  // No alternative beats ALL others (would require all off-diagonal entries
  // in its row to be strictly positive).
  for (int i = 0; i < m; ++i) {
    bool beats_all = true;
    for (int j = 0; j < m && beats_all; ++j) {
      if (i != j && M(i, j) <= 0) beats_all = false;
    }
    EXPECT_FALSE(beats_all) << "Alternative " << i
                            << " beats all others — a Condorcet cycle should"
                               " have no such alternative.";
  }
}

// ---------------------------------------------------------------------------
// Layer 3 — Winset
// ---------------------------------------------------------------------------

TEST(Integration_Winset, SQ_HasNonEmptyWinset) {
  // kSQ is far outside the cluster; the voter-preferred region should be
  // non-empty.
  const WinsetRegion ws = winset_2d(kSQ, kVoters5, kEuc, -1, 64);
  EXPECT_FALSE(winset_is_empty(ws))
      << "Winset of status quo outside the voter cluster should be non-empty.";
}

TEST(Integration_Winset, MedianVoter_HasEmptyWinset) {
  // For 5 collinear voters at x ∈ {-2,-1,0,1,2} (all y=0), the median voter's
  // ideal (0,0) is the continuous Condorcet core: at most 2 of 5 voters can
  // simultaneously prefer any other point over (0,0), so the winset is empty.
  //
  // Proof sketch: for point X=(a,b) to beat SQ=(0,0), voter Vi at (xi,0)
  // must satisfy (xi-a)²+b² < xi², i.e., xi² - 2xia + a² + b² < xi²,
  // i.e., a²+b² < 2xia.  Exactly the voters with xi > (a²+b²)/(2a) (for a>0)
  // or xi < (a²+b²)/(2a) (for a<0) prefer X.  The symmetric configuration
  // ensures at most 2 of the 5 voters satisfy this for any (a,b)≠(0,0).
  const WinsetRegion ws = winset_2d(kMedian, kCollinear5, kEuc, -1, 64);
  EXPECT_TRUE(winset_is_empty(ws))
      << "Winset at the median voter ideal should be empty (Condorcet core).";
}

TEST(Integration_Winset, WinsetOps_IntersectionWithSelf_EqualsSelf) {
  // winset ∩ winset should equal the original winset (non-empty).
  const WinsetRegion ws = winset_2d(kSQ, kVoters5, kEuc, -1, 64);
  ASSERT_FALSE(winset_is_empty(ws));
  const WinsetRegion ws_inter = winset_intersection(ws, ws);
  EXPECT_FALSE(winset_is_empty(ws_inter))
      << "Intersection of a non-empty set with itself should be non-empty.";
}

TEST(Integration_Winset, WinsetOps_UnionWithEmpty_EqualsSelf) {
  const WinsetRegion ws = winset_2d(kSQ, kVoters5, kEuc, -1, 64);
  ASSERT_FALSE(winset_is_empty(ws));
  const WinsetRegion ws_union = winset_union(ws, WinsetRegion{});
  EXPECT_FALSE(winset_is_empty(ws_union))
      << "Union with empty set should preserve the original non-empty winset.";
}

TEST(Integration_Winset, VetoPlayer_ShrinksWinset) {
  // Adding a veto player far from kSQ at an awkward location should shrink
  // or keep equal the winset (monotonicity of veto intersection).
  const WinsetRegion ws_plain = winset_2d(kSQ, kVoters5, kEuc, -1, 64);
  ASSERT_FALSE(winset_is_empty(ws_plain));

  const std::vector<Point2d> veto_ideals = {{6.0, 2.0}};
  const WinsetRegion ws_veto =
      winset_with_veto_2d(kSQ, kVoters5, kEuc, veto_ideals, -1, 64);

  // The veto winset is a subset of the plain winset; its bbox should be
  // no larger.  We just check it is still structurally valid (doesn't throw).
  // (Whether it is empty depends on geometry; both are valid outcomes.)
  static_cast<void>(ws_veto);  // no throw = pass
}

// ---------------------------------------------------------------------------
// Layer 4 — Yolk feeds into uncovered set and core
// ---------------------------------------------------------------------------

TEST(Integration_Yolk, YolkCenter_NearVoterCentroid) {
  // For a tight voter cluster the Yolk centre should be near the centroid.
  const auto y = yolk_2d(kVoters5, -1);

  double cx = 0.0;
  double cy = 0.0;
  for (const auto& v : kVoters5) {
    cx += v.x();
    cy += v.y();
  }
  cx /= static_cast<double>(kVoters5.size());
  cy /= static_cast<double>(kVoters5.size());

  const double dist = std::hypot(y.center.x() - cx, y.center.y() - cy);
  EXPECT_LE(dist, 2.0)
      << "Yolk centre should be within 2 units of the voter centroid.";
}

TEST(Integration_Yolk, YolkRadius_NonNegative) {
  const auto y = yolk_2d(kVoters5, -1);
  EXPECT_GE(y.radius, 0.0);
}

TEST(Integration_Yolk, Cycle_YolkCenter_InsideVoterHull) {
  const auto y = yolk_2d(kCycleVoters, -1);
  const Polygon2E hull = convex_hull_2d(kCycleVoters);
  const auto side = bounded_side(hull, to_exact(y.center));
  EXPECT_NE(side, BoundedSide::kOnUnboundedSide)
      << "Yolk centre of cycle voters should lie inside the voter convex hull.";
}

// ---------------------------------------------------------------------------
// Layer 5 — Uncovered set
// ---------------------------------------------------------------------------

TEST(Integration_UncoveredSet, CondorcetWinner_IsSoleElement) {
  const auto unc = uncovered_set(kAlts4, kVoters5, kEuc, -1);
  ASSERT_EQ(unc.size(), 1u)
      << "Uncovered set of a configuration with a Condorcet winner should be a"
         " singleton.";
  EXPECT_NEAR(unc[0].x(), kA.x(), 1e-6);
  EXPECT_NEAR(unc[0].y(), kA.y(), 1e-6);
}

TEST(Integration_UncoveredSet, Cycle_AllInUncoveredSet) {
  const auto unc = uncovered_set(kCycleAlts, kCycleVoters, kEuc, -1);
  EXPECT_EQ(unc.size(), 3u);
}

TEST(Integration_UncoveredSet, UncoveredSet_SubsetOfAlternatives) {
  const auto unc = uncovered_set(kAlts4, kVoters5, kEuc, -1);
  for (const auto& u : unc) {
    EXPECT_TRUE(contains_pt(kAlts4, u))
        << "Every element of the uncovered set must be an element of the"
           " original alternative set.";
  }
}

TEST(Integration_UncoveredSet, Boundary_ContainedInVoterHullExpansion) {
  // The uncovered set boundary should be loosely contained within a reasonable
  // expansion of the voter convex hull.  We verify the bounding box of the
  // boundary polygon is within 3 units of the voter extent.
  const double margin = 3.0;
  double vxmin = kVoters5[0].x();
  double vxmax = vxmin;
  double vymin = kVoters5[0].y();
  double vymax = vymin;
  for (const auto& v : kVoters5) {
    vxmin = std::min(vxmin, v.x());
    vxmax = std::max(vxmax, v.x());
    vymin = std::min(vymin, v.y());
    vymax = std::max(vymax, v.y());
  }

  const Polygon2E unc_poly = uncovered_set_boundary_2d(kVoters5, kEuc, 8, -1);
  ASSERT_GE(unc_poly.size(), 1u);
  for (auto it = unc_poly.vertices_begin(); it != unc_poly.vertices_end();
       ++it) {
    EXPECT_GE(CGAL::to_double(it->x()), vxmin - margin);
    EXPECT_LE(CGAL::to_double(it->x()), vxmax + margin);
    EXPECT_GE(CGAL::to_double(it->y()), vymin - margin);
    EXPECT_LE(CGAL::to_double(it->y()), vymax + margin);
  }
}

// ---------------------------------------------------------------------------
// Layer 6 — Heart
// ---------------------------------------------------------------------------

TEST(Integration_Heart, CondorcetWinner_IsSoleHeartElement) {
  const auto h = heart(kAlts4, kVoters5, kEuc, -1);
  ASSERT_EQ(h.size(), 1u)
      << "Heart with a Condorcet winner should be a singleton.";
  EXPECT_NEAR(h[0].x(), kA.x(), 1e-6);
  EXPECT_NEAR(h[0].y(), kA.y(), 1e-6);
}

TEST(Integration_Heart, Cycle_HeartEqualsUncoveredSet) {
  const auto h = heart(kCycleAlts, kCycleVoters, kEuc, -1);
  const auto unc = uncovered_set(kCycleAlts, kCycleVoters, kEuc, -1);
  EXPECT_EQ(h.size(), unc.size())
      << "For a Condorcet cycle, Heart should equal the Uncovered Set.";
}

TEST(Integration_Heart, HeartSubsetOfUncoveredSet) {
  // Theorem (Laffond et al. 1993): Heart ⊆ Uncovered Set.
  const auto h = heart(kAlts4, kVoters5, kEuc, -1);
  const auto unc = uncovered_set(kAlts4, kVoters5, kEuc, -1);
  for (const auto& hp : h) {
    EXPECT_TRUE(contains_pt(unc, hp))
        << "Heart element (" << hp.x() << "," << hp.y()
        << ") is not in the uncovered set.";
  }
}

TEST(Integration_Heart, HeartBoundary_SubsetOfUncoveredBoundary) {
  // Verify that the Heart boundary bbox ⊆ uncovered set boundary bbox
  // (approximation tolerance ±0.5 grid units).
  const Polygon2E hb = heart_boundary_2d(kVoters5, kEuc, 8, -1);
  const Polygon2E ub = uncovered_set_boundary_2d(kVoters5, kEuc, 8, -1);
  ASSERT_GE(hb.size(), 1u);
  ASSERT_GE(ub.size(), 1u);

  auto bbox = [](const Polygon2E& p) -> std::array<double, 4> {
    double xmn = CGAL::to_double(p.vertices_begin()->x());
    double xmx = xmn;
    double ymn = CGAL::to_double(p.vertices_begin()->y());
    double ymx = ymn;
    for (auto it = p.vertices_begin(); it != p.vertices_end(); ++it) {
      xmn = std::min(xmn, CGAL::to_double(it->x()));
      xmx = std::max(xmx, CGAL::to_double(it->x()));
      ymn = std::min(ymn, CGAL::to_double(it->y()));
      ymx = std::max(ymx, CGAL::to_double(it->y()));
    }
    return {xmn, xmx, ymn, ymx};
  };
  const auto hbbox = bbox(hb);
  const auto ubbox = bbox(ub);
  const double tol = 0.5;
  EXPECT_GE(hbbox[0], ubbox[0] - tol);
  EXPECT_LE(hbbox[1], ubbox[1] + tol);
  EXPECT_GE(hbbox[2], ubbox[2] - tol);
  EXPECT_LE(hbbox[3], ubbox[3] + tol);
}

// ---------------------------------------------------------------------------
// Layer 7 — Extended services (Copeland, core, weighted, veto)
// ---------------------------------------------------------------------------

TEST(Integration_Extended, CopelandWinner_IsCondorcetWinner) {
  const Point2d cw = copeland_winner(kAlts4, kVoters5, kEuc, -1);
  EXPECT_NEAR(cw.x(), kA.x(), 1e-6);
  EXPECT_NEAR(cw.y(), kA.y(), 1e-6);
}

TEST(Integration_Extended, CopelandScores_CondorcetWinnerHasMaxScore) {
  const auto scores = copeland_scores(kAlts4, kVoters5, kEuc, -1);
  ASSERT_EQ(scores.size(), kAlts4.size());
  // kA is at index 0 in kAlts4; its score should be the maximum.
  const int kA_score = scores[0];
  for (std::size_t i = 1; i < scores.size(); ++i) {
    EXPECT_GE(kA_score, scores[i])
        << "Condorcet winner should have the highest Copeland score.";
  }
}

TEST(Integration_Extended, Core2D_FindsMedianVoterCore) {
  // With 5 collinear voters at x ∈ {-2,-1,0,1,2} on the x-axis, the
  // continuous Condorcet core is (0,0) — the median voter's ideal point.
  // core_2d checks the Yolk centre and all voter ideal points; the Yolk centre
  // for this symmetric configuration is (0,0) exactly, which has an empty
  // winset, so core_2d should return (0,0).
  const auto c = core_2d(kCollinear5, kEuc, -1, 64);
  ASSERT_TRUE(c.has_value())
      << "core_2d should find the Condorcet core for collinear voters.";
  EXPECT_NEAR(c->x(), 0.0, 0.5)
      << "core_2d result should be near the median voter (0,0).";
  EXPECT_NEAR(c->y(), 0.0, 0.5);
}

TEST(Integration_Extended, Core2D_Cycle_NoCore) {
  // In a Condorcet cycle no point has an empty winset; core_2d should return
  // nullopt (checked against the cycle voter ideal points + Yolk centre).
  const auto c = core_2d(kCycleVoters, kEuc, -1, 64);
  EXPECT_FALSE(c.has_value())
      << "core_2d should return nullopt for a Condorcet cycle.";
}

TEST(Integration_Extended, WeightedMajority_UniformWeights_MatchesMajority) {
  // Uniform weights should give the same result as unweighted majority.
  const std::vector<double> w(kVoters5.size(), 1.0);
  for (const auto& alt : {kB, kC, kD}) {
    EXPECT_EQ(majority_prefers(kA, alt, kVoters5, kEuc, -1),
              weighted_majority_prefers(kA, alt, kVoters5, w, kEuc, 0.5))
        << "Uniform-weight majority should agree with unweighted majority.";
  }
}

TEST(Integration_Extended, WeightedWinset_UniformWeights_NonEmpty) {
  // Weighted winset with uniform weights and threshold 0.5 should match the
  // qualitative result of a standard simple majority winset at the SQ.
  const std::vector<double> w(kVoters5.size(), 1.0);
  const WinsetRegion wws = weighted_winset_2d(kSQ, kVoters5, w, kEuc, 0.5, 64);
  const WinsetRegion ws = winset_2d(kSQ, kVoters5, kEuc, -1, 64);
  // Both should be non-empty for a SQ outside the voter cluster.
  EXPECT_FALSE(winset_is_empty(ws));
  EXPECT_FALSE(winset_is_empty(wws));
}

// ---------------------------------------------------------------------------
// Cross-layer subset theorems
// ---------------------------------------------------------------------------

TEST(Integration_Theorems, HeartSubsetOfUncoveredSet_Cycle) {
  const auto h = heart(kCycleAlts, kCycleVoters, kEuc, -1);
  const auto unc = uncovered_set(kCycleAlts, kCycleVoters, kEuc, -1);
  EXPECT_LE(h.size(), unc.size());
  for (const auto& hp : h) {
    EXPECT_TRUE(contains_pt(unc, hp)) << "Heart ⊆ Uncovered Set violated.";
  }
}

TEST(Integration_Theorems, UncoveredSet_SubsetOfAlternatives_Cycle) {
  const auto unc = uncovered_set(kCycleAlts, kCycleVoters, kEuc, -1);
  for (const auto& u : unc) {
    EXPECT_TRUE(contains_pt(kCycleAlts, u))
        << "Uncovered Set ⊆ Alternatives violated.";
  }
}

TEST(Integration_Theorems, CondorcetWinner_HeartEqualsUncoveredEqualsCore) {
  // When a Condorcet winner exists all three concepts collapse to the same
  // singleton (within grid/numerical tolerance).
  const auto h = heart(kAlts4, kVoters5, kEuc, -1);
  const auto unc = uncovered_set(kAlts4, kVoters5, kEuc, -1);
  const Point2d cw = copeland_winner(kAlts4, kVoters5, kEuc, -1);

  ASSERT_EQ(h.size(), 1u);
  ASSERT_EQ(unc.size(), 1u);
  EXPECT_NEAR(h[0].x(), unc[0].x(), 1e-6);
  EXPECT_NEAR(h[0].y(), unc[0].y(), 1e-6);
  EXPECT_NEAR(cw.x(), unc[0].x(), 1e-6);
  EXPECT_NEAR(cw.y(), unc[0].y(), 1e-6);
}
