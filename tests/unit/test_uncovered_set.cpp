// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// test_uncovered_set.cpp — GTest suite for uncovered_set.h.
//   D1: covers()
//   D2: uncovered_set()
//   D3: uncovered_set_boundary_2d()
//
// Key test configurations (all verified analytically):
//
// ── Condorcet cycle ─────────────────────────────────────────────────────────
//   Voters:       V1=(0.3, 1), V2=(1, -2), V3=(-1, 0.3)
//   Alternatives: A=(0,1),     B=(1,0),    C=(-1,0)
//   Majority:     A beats B (2:1), B beats C (2:1), C beats A (2:1).
//   Covering:     No alternative covers any other.
//   Uncovered set = {A, B, C}.
//
// ── Condorcet winner ────────────────────────────────────────────────────────
//   Voters:       V1=(0,0), V2=(0.1,0.1), V3=(0.2,0.2)
//   Alternatives: A=(0.1,0.1), B=(1,0), C=(0,1)
//   Majority:     A beats B (3:0), A beats C (3:0). A is Condorcet winner.
//   Covering:     A covers B (beats B; nothing beats A → vacuously ok).
//                 A covers C (same reasoning).
//   Uncovered set = {A}.
//
// ── Supermajority expansion ──────────────────────────────────────────────────
//   Voters:       V1=(0,0), V2=(0.5,0), V3=(2,0)
//   Alternatives: A=(0.3,0), B=(1.5,0)
//   Under k=2: A beats B (V1,V2 prefer A; V3 prefers B → 2≥2 → true).
//              B doesn't beat A (only V3 prefers B → 1<2). A covers B.
//              Uncovered set = {A}.
//   Under k=3: A beats B needs all 3 → only 2 → false.
//              B beats A needs all 3 → only 1 → false.
//              No majority relations → no covers.
//              Uncovered set = {A, B}.
//
// References:
//   Miller (1980) AJPS 24(1): definition of covering and uncovered set.
//   McKelvey (1986) AJPS 30(2): Condorcet winner uniquely uncovered.
//   Feld, Grofman & Miller (1988) AJPS 32(2): Yolk center ∈ uncovered set.

#include <gtest/gtest.h>
#include <socialchoicelab/geometry/uncovered_set.h>
#include <socialchoicelab/geometry/yolk.h>

#include <algorithm>
#include <cmath>
#include <vector>

using socialchoicelab::core::types::Point2d;
using socialchoicelab::geometry::covers;
using socialchoicelab::geometry::DistConfig;
using socialchoicelab::geometry::Polygon2E;
using socialchoicelab::geometry::uncovered_set;
using socialchoicelab::geometry::uncovered_set_boundary_2d;
using socialchoicelab::geometry::Yolk2d;
using socialchoicelab::geometry::yolk_2d;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::vector<Point2d> cycle_voters() {
  return {Point2d(0.3, 1.0), Point2d(1.0, -2.0), Point2d(-1.0, 0.3)};
}

static std::vector<Point2d> cycle_alts() {
  return {Point2d(0.0, 1.0), Point2d(1.0, 0.0), Point2d(-1.0, 0.0)};
}

static std::vector<Point2d> winner_voters() {
  return {Point2d(0.0, 0.0), Point2d(0.1, 0.1), Point2d(0.2, 0.2)};
}

static std::vector<Point2d> winner_alts() {
  return {Point2d(0.1, 0.1), Point2d(1.0, 0.0), Point2d(0.0, 1.0)};
}

// Returns true if the given Point2d is in the vector (up to identity).
static bool contains_point(const std::vector<Point2d>& vec, const Point2d& p) {
  return std::any_of(vec.begin(), vec.end(), [&](const Point2d& q) {
    return q.x() == p.x() && q.y() == p.y();
  });
}

// ===========================================================================
// D1 — covers()
// ===========================================================================

// ---------------------------------------------------------------------------
// Condorcet cycle: no alternative covers any other
// ---------------------------------------------------------------------------

// Cycle: A→B→C→A (A beats B, B beats C, C beats A).
// For x to cover y: (1) x beats y, and (2) every z that beats x also beats y.
// A covers B: A beats B ✓, but C beats A and C does NOT beat B (B beats C).
// So condition (2) fails: C beats A but ¬(C beats B). → A does NOT cover B.
// By symmetry, no covering in the cycle.

TEST(Covers, CondorcetCycle_NoCovering) {
  auto voters = cycle_voters();
  auto alts = cycle_alts();
  Point2d A = alts[0], B = alts[1], C = alts[2];

  EXPECT_FALSE(covers(A, B, alts, voters)) << "A should not cover B in cycle";
  EXPECT_FALSE(covers(B, C, alts, voters)) << "B should not cover C in cycle";
  EXPECT_FALSE(covers(C, A, alts, voters)) << "C should not cover A in cycle";
  EXPECT_FALSE(covers(A, C, alts, voters))
      << "A should not cover C (A doesn't beat C)";
  EXPECT_FALSE(covers(B, A, alts, voters))
      << "B should not cover A (B doesn't beat A)";
  EXPECT_FALSE(covers(C, B, alts, voters))
      << "C should not cover B (C doesn't beat B)";
}

// ---------------------------------------------------------------------------
// Condorcet winner covers all other alternatives
// ---------------------------------------------------------------------------

// A=(0.1,0.1) is the Condorcet winner; all 3 voters are nearest to A.
// A covers B: A beats B (3:0) ✓; nothing beats A → condition (2) vacuously ✓.
// A covers C: same argument.
// B does NOT cover C: B must beat C first.
//   d(V1,B)=1, d(V1,C)=1  (tie). d(V2,B)=0.906, d(V2,C)=0.906 (tie).
//   d(V3,B)=0.825, d(V3,C)=0.825 (tie). 0 voters strictly prefer B → ¬(B beats
//   C).

TEST(Covers, CondorcetWinner_CoversAllOthers) {
  auto voters = winner_voters();
  auto alts = winner_alts();
  Point2d A = alts[0], B = alts[1], C = alts[2];

  EXPECT_TRUE(covers(A, B, alts, voters))
      << "Condorcet winner A should cover B";
  EXPECT_TRUE(covers(A, C, alts, voters))
      << "Condorcet winner A should cover C";
  EXPECT_FALSE(covers(B, A, alts, voters))
      << "B cannot cover A (B doesn't beat A)";
  EXPECT_FALSE(covers(C, A, alts, voters))
      << "C cannot cover A (C doesn't beat A)";
}

// ---------------------------------------------------------------------------
// A point never covers itself
// ---------------------------------------------------------------------------

TEST(Covers, SamePoint_NeverCoversItself) {
  auto voters = cycle_voters();
  auto alts = cycle_alts();
  for (const auto& a : alts) {
    // majority_prefers(a, a) = false (no voter strictly prefers a to itself)
    // so covers(a, a, …) = false by condition (1).
    EXPECT_FALSE(covers(a, a, alts, voters))
        << "An alternative cannot cover itself";
  }
}

// ---------------------------------------------------------------------------
// Supermajority: k=3 removes the covering relation
// ---------------------------------------------------------------------------

// Voters: V1=(0,0), V2=(0.5,0), V3=(2,0).
// Alternatives: A=(0.3,0), B=(1.5,0).
// Under k=2: A beats B (V1,V2 prefer A; only V3 prefers B → 2≥2).
//            B doesn't beat A (only 1 vote).  A covers B.
// Under k=3: A beats B requires all 3; only 2 prefer A → false.
//            B beats A requires all 3; only 1 prefers B → false.
//            Neither covers the other.

TEST(Covers, Supermajority_RemovesCovering) {
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(0.5, 0.0),
                                 Point2d(2.0, 0.0)};
  std::vector<Point2d> alts = {Point2d(0.3, 0.0), Point2d(1.5, 0.0)};
  Point2d A = alts[0], B = alts[1];

  // Simple majority: A covers B.
  EXPECT_TRUE(covers(A, B, alts, voters, DistConfig{}, /*k=*/2))
      << "A should cover B under simple majority (k=2)";

  // Unanimity: neither covers.
  EXPECT_FALSE(covers(A, B, alts, voters, DistConfig{}, /*k=*/3))
      << "A should not cover B under unanimity (k=3): A doesn't beat B";
  EXPECT_FALSE(covers(B, A, alts, voters, DistConfig{}, /*k=*/3))
      << "B should not cover A under unanimity (k=3): B doesn't beat A";
}

// ===========================================================================
// D2 — uncovered_set()
// ===========================================================================

// ---------------------------------------------------------------------------
// Empty alternatives → empty uncovered set
// ---------------------------------------------------------------------------

TEST(UncoveredSet, EmptyAlternatives_ReturnsEmpty) {
  std::vector<Point2d> alts, voters = cycle_voters();
  auto us = uncovered_set(alts, voters);
  EXPECT_TRUE(us.empty());
}

// ---------------------------------------------------------------------------
// Single alternative → always uncovered
// ---------------------------------------------------------------------------

TEST(UncoveredSet, SingleAlternative_AlwaysUncovered) {
  std::vector<Point2d> alts = {Point2d(0.5, 0.5)};
  std::vector<Point2d> voters = cycle_voters();
  auto us = uncovered_set(alts, voters);
  ASSERT_EQ(us.size(), 1u);
  EXPECT_EQ(us[0].x(), 0.5);
  EXPECT_EQ(us[0].y(), 0.5);
}

// ---------------------------------------------------------------------------
// Condorcet cycle → all alternatives uncovered
// ---------------------------------------------------------------------------

TEST(UncoveredSet, CondorcetCycle_AllUncovered) {
  auto voters = cycle_voters();
  auto alts = cycle_alts();
  auto us = uncovered_set(alts, voters);

  EXPECT_EQ(us.size(), 3u) << "All 3 alternatives should be uncovered in a "
                              "Condorcet cycle (no covering relation exists)";
}

// ---------------------------------------------------------------------------
// Condorcet winner → sole uncovered element
// ---------------------------------------------------------------------------

TEST(UncoveredSet, CondorcetWinner_SoleUncoveredElement) {
  auto voters = winner_voters();
  auto alts = winner_alts();
  Point2d A = alts[0];

  auto us = uncovered_set(alts, voters);

  ASSERT_EQ(us.size(), 1u)
      << "Condorcet winner should be the unique uncovered alternative";
  EXPECT_EQ(us[0].x(), A.x());
  EXPECT_EQ(us[0].y(), A.y());
}

// ---------------------------------------------------------------------------
// Supermajority expands the uncovered set
// ---------------------------------------------------------------------------

// Config (see header comment): under k=2, A covers B → uncovered = {A}.
//                               under k=3, no covers  → uncovered = {A, B}.

TEST(UncoveredSet, Supermajority_ExpandsUncoveredSet) {
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(0.5, 0.0),
                                 Point2d(2.0, 0.0)};
  std::vector<Point2d> alts = {Point2d(0.3, 0.0), Point2d(1.5, 0.0)};

  auto us_k2 = uncovered_set(alts, voters, DistConfig{}, /*k=*/2);
  auto us_k3 = uncovered_set(alts, voters, DistConfig{}, /*k=*/3);

  EXPECT_EQ(us_k2.size(), 1u) << "Under k=2, A covers B → only A uncovered";
  EXPECT_EQ(us_k3.size(), 2u)
      << "Under k=3 (unanimity), no covers → both uncovered";
}

// ---------------------------------------------------------------------------
// Uncovered set is always non-empty (invariant)
// ---------------------------------------------------------------------------

TEST(UncoveredSet, AlwaysNonEmpty_ForNonEmptyAlternatives) {
  auto us_cycle = uncovered_set(cycle_alts(), cycle_voters());
  auto us_winner = uncovered_set(winner_alts(), winner_voters());
  EXPECT_FALSE(us_cycle.empty()) << "Uncovered set must be non-empty";
  EXPECT_FALSE(us_winner.empty()) << "Uncovered set must be non-empty";
}

// ===========================================================================
// D3 — uncovered_set_boundary_2d()
// ===========================================================================

// ---------------------------------------------------------------------------
// Error handling
// ---------------------------------------------------------------------------

TEST(UncoveredSetBoundary, ThrowsOnEmptyVoters) {
  std::vector<Point2d> voters;
  EXPECT_THROW(uncovered_set_boundary_2d(voters), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// Non-degenerate voter configuration → non-trivial polygon
//
// Equilateral triangle voters: A=(0,0), B=(1,0), C=(0.5, √3/2).
// No Condorcet winner exists (the winset at the centroid is non-empty).
// The uncovered set is a region, not a single point; the grid approximation
// should produce a convex hull with ≥ 3 vertices.
// ---------------------------------------------------------------------------

TEST(UncoveredSetBoundary, EquilateralTriangle_NonTrivialPolygon) {
  const double sqrt3 = std::sqrt(3.0);
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(1.0, 0.0),
                                 Point2d(0.5, sqrt3 / 2.0)};

  // Use a moderate resolution so the test runs in reasonable time.
  Polygon2E poly = uncovered_set_boundary_2d(voters, DistConfig{},
                                             /*grid_resolution=*/20);
  EXPECT_GE(poly.size(), 3u)
      << "Equilateral triangle (no Condorcet winner) should produce a "
         "non-degenerate uncovered set boundary with ≥ 3 vertices";
}

// ---------------------------------------------------------------------------
// Yolk center ∈ uncovered set (Feld–Grofman–Miller 1988 guarantee)
//
// For the equilateral triangle, the Yolk center = centroid = (0.5, √3/6).
// This point should lie within the bounding box of the uncovered set polygon.
// We check the weaker condition (bbox containment) to avoid needing
// point-in-polygon from CGAL.
// ---------------------------------------------------------------------------

TEST(UncoveredSetBoundary, EquilateralTriangle_BBoxContainsYolkCenter) {
  const double sqrt3 = std::sqrt(3.0);
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(1.0, 0.0),
                                 Point2d(0.5, sqrt3 / 2.0)};

  Yolk2d yolk = yolk_2d(voters);
  Polygon2E poly = uncovered_set_boundary_2d(voters, DistConfig{},
                                             /*grid_resolution=*/25);
  ASSERT_GE(poly.size(), 1u);

  // Bounding box of polygon vertices.
  double px_min = CGAL::to_double(poly.vertex(0).x());
  double px_max = px_min;
  double py_min = CGAL::to_double(poly.vertex(0).y());
  double py_max = py_min;
  for (auto it = poly.vertices_begin(); it != poly.vertices_end(); ++it) {
    double vx = CGAL::to_double(it->x());
    double vy = CGAL::to_double(it->y());
    px_min = std::min(px_min, vx);
    px_max = std::max(px_max, vx);
    py_min = std::min(py_min, vy);
    py_max = std::max(py_max, vy);
  }

  EXPECT_GE(yolk.center.x(), px_min - 0.1)
      << "Yolk centre x should be within the uncovered set bounding box";
  EXPECT_LE(yolk.center.x(), px_max + 0.1)
      << "Yolk centre x should be within the uncovered set bounding box";
  EXPECT_GE(yolk.center.y(), py_min - 0.1)
      << "Yolk centre y should be within the uncovered set bounding box";
  EXPECT_LE(yolk.center.y(), py_max + 0.1)
      << "Yolk centre y should be within the uncovered set bounding box";
}

// ---------------------------------------------------------------------------
// Strong Condorcet winner: uncovered set boundary is near the winner
//
// All 5 voters at (0.5, 0.5). The nearest grid point to (0.5, 0.5) is the
// Condorcet winner among grid points; uncovered set ≈ a single grid point.
// The polygon bounding box should be near (0.5, 0.5).
// ---------------------------------------------------------------------------

TEST(UncoveredSetBoundary, AllVotersCollocated_BoundaryNearVoter) {
  std::vector<Point2d> voters(5, Point2d(0.5, 0.5));
  Polygon2E poly = uncovered_set_boundary_2d(voters, DistConfig{},
                                             /*grid_resolution=*/20);
  ASSERT_GE(poly.size(), 1u);

  // All uncovered grid points should be close to (0.5, 0.5).
  for (auto it = poly.vertices_begin(); it != poly.vertices_end(); ++it) {
    double vx = CGAL::to_double(it->x());
    double vy = CGAL::to_double(it->y());
    EXPECT_NEAR(vx, 0.5, 0.2)
        << "Uncovered set vertex should be near voter colocation point";
    EXPECT_NEAR(vy, 0.5, 0.2)
        << "Uncovered set vertex should be near voter colocation point";
  }
}

// ---------------------------------------------------------------------------
// Resolution parameter: different resolutions both produce non-empty polygons
// ---------------------------------------------------------------------------

TEST(UncoveredSetBoundary, ResolutionParameter_NonEmpty) {
  const double sqrt3 = std::sqrt(3.0);
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(1.0, 0.0),
                                 Point2d(0.5, sqrt3 / 2.0)};

  for (int res : {10, 15, 20}) {
    Polygon2E poly = uncovered_set_boundary_2d(voters, DistConfig{}, res);
    EXPECT_GE(poly.size(), 1u)
        << "Uncovered set boundary should be non-empty at resolution " << res;
  }
}
