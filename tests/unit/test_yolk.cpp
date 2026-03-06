// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// test_yolk.cpp — GTest suite for yolk.h (C1: quantile_lines_2d;
//                                          C2: yolk_2d; C3: yolk_radius).
//
// Tests cover:
//   - Correct line count (n*(n-1)/2 per voter pair).
//   - Known k-quantile lines for specific configurations.
//   - Simple majority and supermajority thresholds.
//   - Error handling (n < 2, k out of range).
//   - Distance helper (distance_to_line).
//   - Yolk2d analytical result for equilateral triangle.
//   - Yolk radius = 0 when Condorcet winner exists (collinear median).
//   - Supermajority increases Yolk radius.
//
// Theoretical grounding:
//   Feld, Grofman & Miller (1988) "Centripetal Forces in Spatial Voting".
//   Under simple majority for n=3 voters, each voter is the "median" for a
//   range of directions, and exactly 3 distinct critical-direction k-quantile
//   lines exist (one per voter pair).

#include <gtest/gtest.h>
#include <socialchoicelab/geometry/yolk.h>

#include <cmath>
#include <vector>

using socialchoicelab::core::types::Point2d;
using socialchoicelab::geometry::distance_to_line;
using socialchoicelab::geometry::limiting_quantile_lines_2d;
using socialchoicelab::geometry::Line2E;
using socialchoicelab::geometry::quantile_lines_2d;
using socialchoicelab::geometry::quantile_lines_2d_annotated;
using socialchoicelab::geometry::QuantileLine2d;
using socialchoicelab::geometry::Yolk2d;
using socialchoicelab::geometry::yolk_2d;
using socialchoicelab::geometry::yolk_radius;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Signed distance from numeric point to CGAL line, via double conversion.
static double dist(const Point2d& p, const Line2E& line) {
  return socialchoicelab::geometry::distance_to_line(p, line);
}

// ---------------------------------------------------------------------------
// Line count
// ---------------------------------------------------------------------------

TEST(QuantileLines, LineCount_TwoVoters) {
  // n=2: 2*(2-1)/2 = 1 line.
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(1.0, 0.0)};
  auto lines = quantile_lines_2d(voters);
  EXPECT_EQ(static_cast<int>(lines.size()), 1);
}

TEST(QuantileLines, LineCount_ThreeVoters) {
  // n=3: 3*(3-1)/2 = 3 lines (one per voter pair).
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(1.0, 0.0),
                                 Point2d(0.5, 1.0)};
  auto lines = quantile_lines_2d(voters);
  EXPECT_EQ(static_cast<int>(lines.size()), 3);
}

TEST(QuantileLines, LineCount_FiveVoters) {
  // n=5: 5*(5-1)/2 = 10 lines.
  std::vector<Point2d> voters = {
    Point2d(0.0, 0.0),  Point2d(1.0, 0.0),  Point2d(0.5, 1.0),
    Point2d(-0.5, 0.5), Point2d(0.3, -0.4),
  };
  auto lines = quantile_lines_2d(voters);
  EXPECT_EQ(static_cast<int>(lines.size()), 10);
}

// ---------------------------------------------------------------------------
// Known median lines — two voters
// ---------------------------------------------------------------------------

// With 2 voters and k=1 (any preference), the k-quantile line at the critical
// direction e ⊥ (p2 − p1) passes through the 1st projection value.
// Voters: A=(0,0), B=(1,0). Direction e ⊥ (1,0) = (0,1).
// Projections onto (0,1): A→0, B→0. k=1: proj = 0. Line: y = 0.
TEST(QuantileLines, TwoVoters_Horizontal_K1) {
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(1.0, 0.0)};
  auto lines = quantile_lines_2d(voters, /*k=*/1);
  ASSERT_EQ(lines.size(), 1u);
  // Line is y = 0: distance from (0.5, 0) = 0; from (0.5, 1) = 1.
  EXPECT_NEAR(dist(Point2d(0.5, 0.0), lines[0]), 0.0, 1e-10);
  EXPECT_NEAR(dist(Point2d(0.5, 1.0), lines[0]), 1.0, 1e-10);
}

// With 2 voters and k=2, the k-quantile line passes through the 2nd (higher)
// projection value — same result as k=1 since both voters project to 0.
TEST(QuantileLines, TwoVoters_Horizontal_K2) {
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(1.0, 0.0)};
  auto lines = quantile_lines_2d(voters, /*k=*/2);
  ASSERT_EQ(lines.size(), 1u);
  EXPECT_NEAR(dist(Point2d(0.5, 0.0), lines[0]), 0.0, 1e-10);
}

// ---------------------------------------------------------------------------
// Right-triangle voters — known median lines
// ---------------------------------------------------------------------------
// Voters: A=(0,0), B=(1,0), C=(0,1). n=3, k=2 (simple majority).
// Pair (A,B): dv=(1,0), normal=(0,1). Projections onto (0,1): 0,0,1.
//   k=2: proj=0. Line: y=0.
// Pair (A,C): dv=(0,1), normal=(-1,0). Projections onto (-1,0): 0,-1,0.
//   sorted: -1,0,0. k=2: proj=0. Line: -x=0, i.e. x=0.
// Pair (B,C): dv=(-1,1), normal=(-1,-1). Projections onto (-1,-1): 0,-1,-1.
//   sorted: -1,-1,0. k=2: proj=-1. Line: -x-y=-1, i.e. x+y=1.

TEST(QuantileLines, RightTriangle_MedianLines) {
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(1.0, 0.0),
                                 Point2d(0.0, 1.0)};
  auto lines = quantile_lines_2d(voters);  // k=-1 → simple majority k=2
  ASSERT_EQ(lines.size(), 3u);

  // Each of the 3 known median lines must be present (as one of the 3 lines).
  // Test by checking that known "on-line" points are at distance ≈ 0 from
  // exactly one line, and known "off-line" points are at the expected distance.

  // Known lines (up to sign/order):
  //   L1: y = 0        — passes through A=(0,0) and B=(1,0).
  //   L2: x = 0        — passes through A=(0,0) and C=(0,1).
  //   L3: x + y = 1    — passes through B=(1,0) and C=(0,1).

  auto near_zero = [](double v) { return std::abs(v) < 1e-9; };

  bool found_L1 = false, found_L2 = false, found_L3 = false;
  for (const auto& line : lines) {
    bool on_A_and_B = near_zero(dist(Point2d(0.0, 0.0), line)) &&
                      near_zero(dist(Point2d(1.0, 0.0), line));
    bool on_A_and_C = near_zero(dist(Point2d(0.0, 0.0), line)) &&
                      near_zero(dist(Point2d(0.0, 1.0), line));
    bool on_B_and_C = near_zero(dist(Point2d(1.0, 0.0), line)) &&
                      near_zero(dist(Point2d(0.0, 1.0), line));
    if (on_A_and_B) found_L1 = true;
    if (on_A_and_C) found_L2 = true;
    if (on_B_and_C) found_L3 = true;
  }
  EXPECT_TRUE(found_L1) << "Missing L1: y=0 (through A and B)";
  EXPECT_TRUE(found_L2) << "Missing L2: x=0 (through A and C)";
  EXPECT_TRUE(found_L3) << "Missing L3: x+y=1 (through B and C)";
}

// ---------------------------------------------------------------------------
// Supermajority changes which voter is at the k-th quantile
// ---------------------------------------------------------------------------

// Voters: A=(0,0), B=(1,0), C=(3,0) — collinear on x-axis.
// Pair (A,C): dv=(3,0), normal=(0,3) ∝ (0,1).
// Projections onto (0,1): all 0 (everyone on x-axis).
// k=2 (simple majority of 3): proj=0 → line y=0.
// k=3 (unanimity): proj=0 → still line y=0 (all projections are 0).
// So the line y=0 appears for all k on this collinear case.
TEST(QuantileLines, CollinearVoters_AlwaysYAxis) {
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(1.0, 0.0),
                                 Point2d(3.0, 0.0)};
  for (int k : {1, 2, 3}) {
    auto lines = quantile_lines_2d(voters, k);
    ASSERT_EQ(lines.size(), 3u);
    // Every line must pass through some point on the x-axis (y=0).
    for (const auto& line : lines) {
      // All voters on x-axis project to 0 in any vertical direction,
      // so all critical lines are horizontal (y = constant).
      // The point (0.5, 0) should be on or near every such line.
      EXPECT_NEAR(dist(Point2d(0.5, 0.0), line), 0.0, 1e-9)
          << "Line is not y=0 for collinear voters (k=" << k << ")";
    }
  }
}

// Voters: A=(0,0), B=(1,0), C=(3,0), D=(5,0) — 4 voters on x-axis.
// Simple majority k=3: 3rd of 4 projections.
// Pair (A,D): dv=(5,0), normal=(0,5) ∝ (0,1).
//   Projections onto (0,1): 0,0,0,0 → k=3: 0. Line y=0.
// All pairs give the same: all voters project to 0 in (0,1) direction → y=0.
TEST(QuantileLines, FourCollinearVoters_AllLinesHorizontal) {
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(1.0, 0.0),
                                 Point2d(3.0, 0.0), Point2d(5.0, 0.0)};
  auto lines = quantile_lines_2d(voters);  // k=3 (simple majority of 4)
  ASSERT_EQ(lines.size(), 6u);             // C(4,2)=6
  for (const auto& line : lines) {
    EXPECT_NEAR(dist(Point2d(2.0, 0.0), line), 0.0, 1e-9)
        << "Line is not y=0 for collinear voters";
  }
}

// ---------------------------------------------------------------------------
// Supermajority: k=n (unanimity) pushes k-quantile line to outermost voter
// ---------------------------------------------------------------------------

// Voters: A=(0,0), B=(2,0), C=(4,0). In vertical critical direction (0,1),
// all project to 0. But for pair (A,B): dv=(2,0), normal=(0,2) ∝ (0,1) →
// same as above. For pair (A,C): same. For pair (B,C): same.
// This collinear case is degenerate. Use a 2D layout instead.

// Voters: A=(0,0), B=(2,0), C=(0,2). n=3.
// Pair (A,B): dv=(2,0), normal=(0,2). Projections onto (0,2): 0,0,4.
//   k=1: proj=0 → y=0 (multiplied by 2, so line 0x + 2y = 0 → y=0).
//   k=2: proj=0 → y=0.
//   k=3: proj=4 → 2y=4 → y=2 (line through C=(0,2)).
// So for pair (A,B), unanimity (k=3) shifts the line to pass through C.
TEST(QuantileLines, Unanimity_ShiftsLineToOutermostVoter) {
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(2.0, 0.0),
                                 Point2d(0.0, 2.0)};
  auto lines_k2 = quantile_lines_2d(voters, /*k=*/2);
  auto lines_k3 = quantile_lines_2d(voters, /*k=*/3);

  // For pair (A=(0,0), B=(2,0)): with k=2 the line passes through A and B
  // (distance from A=0, B=0); with k=3 it passes through C=(0,2).
  // Identify the line for pair (A,B) in both sets: it's the one where
  // dist(A)=0 and dist(B)=0 under k=2.
  auto near_zero = [](double v) { return std::abs(v) < 1e-9; };

  bool found_AB_k2 = false, found_AB_k3 = false;
  for (const auto& line : lines_k2) {
    if (near_zero(dist(Point2d(0.0, 0.0), line)) &&
        near_zero(dist(Point2d(2.0, 0.0), line))) {
      found_AB_k2 = true;
    }
  }
  for (const auto& line : lines_k3) {
    // Under k=3 the pair (A,B) line should pass through C=(0,2).
    if (near_zero(dist(Point2d(0.0, 2.0), line))) {
      found_AB_k3 = true;
    }
  }
  EXPECT_TRUE(found_AB_k2)
      << "k=2 line for pair (A,B) should pass through both A and B";
  EXPECT_TRUE(found_AB_k3)
      << "k=3 (unanimity) line for pair (A,B) should pass through C";
}

// ---------------------------------------------------------------------------
// Distance helper
// ---------------------------------------------------------------------------

TEST(DistanceToLine, Horizontal_Line_Y0) {
  // Line: y = 0 (represented as 0x + 1y + 0 = 0).
  Line2E line(0, 1, 0);
  EXPECT_NEAR(dist(Point2d(3.0, 5.0), line), 5.0, 1e-10);
  EXPECT_NEAR(dist(Point2d(-1.0, 0.0), line), 0.0, 1e-10);
  EXPECT_NEAR(dist(Point2d(0.0, -3.0), line), 3.0, 1e-10);
}

TEST(DistanceToLine, Diagonal_Line_XPlusY_Equals_1) {
  // Line: x + y = 1, i.e. x + y − 1 = 0.
  Line2E line(1, 1, -1);
  // Distance from origin = 1/sqrt(2).
  EXPECT_NEAR(dist(Point2d(0.0, 0.0), line), 1.0 / std::sqrt(2.0), 1e-10);
  // Points on the line: (0.5, 0.5), (1, 0), (0, 1).
  EXPECT_NEAR(dist(Point2d(0.5, 0.5), line), 0.0, 1e-10);
  EXPECT_NEAR(dist(Point2d(1.0, 0.0), line), 0.0, 1e-10);
}

// ---------------------------------------------------------------------------
// Error handling
// ---------------------------------------------------------------------------

TEST(QuantileLines, ThrowsOnSingleVoter) {
  std::vector<Point2d> voters = {Point2d(0.0, 0.0)};
  EXPECT_THROW(quantile_lines_2d(voters), std::invalid_argument);
}

TEST(QuantileLines, ThrowsOnEmptyVoters) {
  std::vector<Point2d> voters;
  EXPECT_THROW(quantile_lines_2d(voters), std::invalid_argument);
}

TEST(QuantileLines, ThrowsOnKOutOfRange) {
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(1.0, 0.0)};
  EXPECT_THROW(quantile_lines_2d(voters, 3), std::invalid_argument);
  EXPECT_THROW(quantile_lines_2d(voters, 0), std::invalid_argument);
}

// ===========================================================================
// C2: k-Yolk computation  (yolk_2d)
// C3: Yolk radius          (yolk_radius)
// ===========================================================================

// ---------------------------------------------------------------------------
// Error handling
// ---------------------------------------------------------------------------

TEST(Yolk2d, ThrowsOnSingleVoter) {
  std::vector<Point2d> voters = {Point2d(0.5, 0.5)};
  EXPECT_THROW(yolk_2d(voters), std::invalid_argument);
}

TEST(Yolk2d, ThrowsOnEmptyVoters) {
  std::vector<Point2d> voters;
  EXPECT_THROW(yolk_2d(voters), std::invalid_argument);
}

TEST(YolkRadius, ThrowsOnSingleVoter) {
  std::vector<Point2d> voters = {Point2d(0.5, 0.5)};
  EXPECT_THROW(yolk_radius(voters), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// Collinear voters with Condorcet winner at median → Yolk radius ≈ 0
//
// Voters: A=(0,0), B=(0.5,0), C=(1,0). n=3, k=2 (simple majority).
// In every direction the k-quantile line passes through the median voter B.
// All k-quantile lines therefore share the common point (0.5, 0), so the
// Yolk degenerates: centre = (0.5, 0), radius = 0.
// ---------------------------------------------------------------------------

TEST(Yolk2d, CollinearMedian_RadiusNearZero) {
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(0.5, 0.0),
                                 Point2d(1.0, 0.0)};
  Yolk2d y = yolk_2d(voters);
  EXPECT_LE(y.radius, 0.01) << "Yolk radius should be near 0 (Condorcet "
                               "winner exists at median voter)";
  EXPECT_NEAR(y.center.x(), 0.5, 0.05)
      << "Yolk centre x should be near the median voter (0.5, 0)";
  EXPECT_NEAR(y.center.y(), 0.0, 0.05)
      << "Yolk centre y should be near the median voter (0.5, 0)";
}

// ---------------------------------------------------------------------------
// Equilateral triangle → analytical Yolk
//
// Voters: A=(0,0), B=(1,0), C=(0.5, √3/2). n=3, k=2 (simple majority).
// By 3-fold symmetry the Yolk centre is the centroid G=(0.5, √3/6).
//
// The three critical k-quantile lines (at θ = π/6, π/2, 5π/6, where two
// voters tie) each lie at distance √3/6 from G.  All other k-quantile lines
// pass *through* G (distance 0).  Hence:
//
//   Yolk centre  = (0.5, √3/6)  ≈  (0.5, 0.2887)
//   Yolk radius  = √3/6         ≈  0.2887
// ---------------------------------------------------------------------------

TEST(Yolk2d, EquilateralTriangle_AnalyticalResult) {
  const double sqrt3 = std::sqrt(3.0);
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(1.0, 0.0),
                                 Point2d(0.5, sqrt3 / 2.0)};
  Yolk2d y = yolk_2d(voters);

  const double expected_r = sqrt3 / 6.0;  // ≈ 0.2887
  EXPECT_NEAR(y.radius, expected_r, 0.01)
      << "Yolk radius should equal √3/6 for equilateral triangle";
  EXPECT_NEAR(y.center.x(), 0.5, 0.05)
      << "Yolk centre x should be at centroid (0.5, √3/6)";
  EXPECT_NEAR(y.center.y(), sqrt3 / 6.0, 0.05)
      << "Yolk centre y should be at centroid (0.5, √3/6)";
}

// ---------------------------------------------------------------------------
// Non-negative radius for an asymmetric configuration
// ---------------------------------------------------------------------------

TEST(Yolk2d, Asymmetric4Voters_NonNegativeRadius) {
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(1.0, 0.0),
                                 Point2d(0.0, 1.0), Point2d(0.7, 0.8)};
  Yolk2d y = yolk_2d(voters);
  EXPECT_GE(y.radius, 0.0) << "Yolk radius is always non-negative";
}

// ---------------------------------------------------------------------------
// Supermajority shifts the Yolk: k=n (unanimity) should give a larger Yolk
// radius than simple majority for the equilateral triangle, because unanimity
// k-quantile lines follow the outermost voter in each direction.
// ---------------------------------------------------------------------------

TEST(Yolk2d, EquilateralTriangle_UnanimityLargerRadius) {
  const double sqrt3 = std::sqrt(3.0);
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(1.0, 0.0),
                                 Point2d(0.5, sqrt3 / 2.0)};
  double r_majority = yolk_2d(voters, /*k=*/-1).radius;  // k=2
  double r_unanimity = yolk_2d(voters, /*k=*/3).radius;  // k=3

  EXPECT_GT(r_unanimity, r_majority)
      << "Unanimity Yolk radius should exceed simple majority Yolk radius "
         "for the equilateral triangle";
}

// ---------------------------------------------------------------------------
// C3: yolk_radius convenience wrapper produces the same value as yolk_2d
// ---------------------------------------------------------------------------

TEST(YolkRadius, MatchesYolk2d) {
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(1.0, 0.0),
                                 Point2d(0.5, std::sqrt(3.0) / 2.0)};
  Yolk2d y = yolk_2d(voters);
  double r = yolk_radius(voters);
  EXPECT_DOUBLE_EQ(r, y.radius)
      << "yolk_radius convenience function must return the same value as "
         "yolk_2d.radius";
}

// ===========================================================================
// C1b: Annotated and limiting k-quantile lines
// ===========================================================================

// ---------------------------------------------------------------------------
// Right-triangle: n=3, k=2 → all 3 critical-direction lines are limiting
// (each pair's common projection IS the median).
// ---------------------------------------------------------------------------

TEST(AnnotatedLines, RightTriangle_AllLimiting) {
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(1.0, 0.0),
                                 Point2d(0.0, 1.0)};
  auto annotated = quantile_lines_2d_annotated(voters);
  ASSERT_EQ(annotated.size(), 3u);
  for (const auto& ql : annotated) {
    EXPECT_TRUE(ql.is_limiting)
        << "For n=3 right-triangle with k=2, every critical-direction line "
           "should be limiting (pair "
        << ql.voter_i << "," << ql.voter_j << ")";
  }
}

TEST(AnnotatedLines, RightTriangle_VoterPairIndices) {
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(1.0, 0.0),
                                 Point2d(0.0, 1.0)};
  auto annotated = quantile_lines_2d_annotated(voters);
  ASSERT_EQ(annotated.size(), 3u);
  EXPECT_EQ(annotated[0].voter_i, 0);
  EXPECT_EQ(annotated[0].voter_j, 1);
  EXPECT_EQ(annotated[1].voter_i, 0);
  EXPECT_EQ(annotated[1].voter_j, 2);
  EXPECT_EQ(annotated[2].voter_i, 1);
  EXPECT_EQ(annotated[2].voter_j, 2);
}

// ---------------------------------------------------------------------------
// Laing-Olmsted-Bear 5-voter: 10 lines total, 6 limiting, 4 non-limiting
// ---------------------------------------------------------------------------

TEST(AnnotatedLines, LaingOlmstedBear_LimitingCount) {
  std::vector<Point2d> voters = {
    Point2d(20.8, 79.6),    // P1
    Point2d(100.4, 69.2),   // P2
    Point2d(121.2, 115.2),  // P3
    Point2d(100.4, 6.4),    // P4
    Point2d(7.2, 20.0),     // P5
  };
  auto annotated = quantile_lines_2d_annotated(voters);
  ASSERT_EQ(annotated.size(), 10u);

  int n_limiting = 0;
  for (const auto& ql : annotated) {
    if (ql.is_limiting) ++n_limiting;
  }
  EXPECT_EQ(n_limiting, 6)
      << "Laing-Olmsted-Bear should have exactly 6 limiting median lines";
}

TEST(AnnotatedLines, LaingOlmstedBear_LimitingPairs) {
  std::vector<Point2d> voters = {
    Point2d(20.8, 79.6),    // P1=0
    Point2d(100.4, 69.2),   // P2=1
    Point2d(121.2, 115.2),  // P3=2
    Point2d(100.4, 6.4),    // P4=3
    Point2d(7.2, 20.0),     // P5=4
  };
  auto annotated = quantile_lines_2d_annotated(voters);

  // Expected limiting pairs: (0,1),(0,3),(1,2),(1,3),(1,4),(2,4)
  auto is_pair = [](const QuantileLine2d& ql, int a, int b) {
    return ql.voter_i == a && ql.voter_j == b;
  };

  for (const auto& ql : annotated) {
    bool expected_limiting = is_pair(ql, 0, 1) || is_pair(ql, 0, 3) ||
                             is_pair(ql, 1, 2) || is_pair(ql, 1, 3) ||
                             is_pair(ql, 1, 4) || is_pair(ql, 2, 4);
    EXPECT_EQ(ql.is_limiting, expected_limiting)
        << "Pair (" << ql.voter_i << "," << ql.voter_j << ") limiting flag "
        << "should be " << expected_limiting;
  }
}

TEST(AnnotatedLines, LimitingLines_PassThroughBothVoters) {
  std::vector<Point2d> voters = {
    Point2d(20.8, 79.6), Point2d(100.4, 69.2), Point2d(121.2, 115.2),
    Point2d(100.4, 6.4), Point2d(7.2, 20.0),
  };
  auto annotated = quantile_lines_2d_annotated(voters);

  for (const auto& ql : annotated) {
    double d_i = dist(voters[static_cast<std::size_t>(ql.voter_i)], ql.line);
    double d_j = dist(voters[static_cast<std::size_t>(ql.voter_j)], ql.line);
    if (ql.is_limiting) {
      EXPECT_NEAR(d_i, 0.0, 1e-8)
          << "Limiting line for pair (" << ql.voter_i << "," << ql.voter_j
          << ") should pass through voter " << ql.voter_i;
      EXPECT_NEAR(d_j, 0.0, 1e-8)
          << "Limiting line for pair (" << ql.voter_i << "," << ql.voter_j
          << ") should pass through voter " << ql.voter_j;
    }
  }
}

TEST(AnnotatedLines, NonLimitingLines_DoNotPassThroughBoth) {
  std::vector<Point2d> voters = {
    Point2d(20.8, 79.6), Point2d(100.4, 69.2), Point2d(121.2, 115.2),
    Point2d(100.4, 6.4), Point2d(7.2, 20.0),
  };
  auto annotated = quantile_lines_2d_annotated(voters);

  for (const auto& ql : annotated) {
    if (ql.is_limiting) continue;
    double d_i = dist(voters[static_cast<std::size_t>(ql.voter_i)], ql.line);
    double d_j = dist(voters[static_cast<std::size_t>(ql.voter_j)], ql.line);
    bool both_on = (d_i < 1e-8) && (d_j < 1e-8);
    EXPECT_FALSE(both_on)
        << "Non-limiting line for pair (" << ql.voter_i << "," << ql.voter_j
        << ") should NOT pass through both voters of the pair";
  }
}

// ---------------------------------------------------------------------------
// limiting_quantile_lines_2d convenience wrapper
// ---------------------------------------------------------------------------

TEST(LimitingLines, ConvenienceWrapper_ReturnsOnlyLimiting) {
  std::vector<Point2d> voters = {
    Point2d(20.8, 79.6), Point2d(100.4, 69.2), Point2d(121.2, 115.2),
    Point2d(100.4, 6.4), Point2d(7.2, 20.0),
  };
  auto limiting = limiting_quantile_lines_2d(voters);
  EXPECT_EQ(static_cast<int>(limiting.size()), 6);
}

TEST(LimitingLines, RightTriangle_AllThreeReturned) {
  std::vector<Point2d> voters = {Point2d(0.0, 0.0), Point2d(1.0, 0.0),
                                 Point2d(0.0, 1.0)};
  auto limiting = limiting_quantile_lines_2d(voters);
  EXPECT_EQ(static_cast<int>(limiting.size()), 3);
}

// ---------------------------------------------------------------------------
// Error handling for annotated/limiting functions
// ---------------------------------------------------------------------------

TEST(AnnotatedLines, ThrowsOnSingleVoter) {
  std::vector<Point2d> voters = {Point2d(0.0, 0.0)};
  EXPECT_THROW(quantile_lines_2d_annotated(voters), std::invalid_argument);
}

TEST(LimitingLines, ThrowsOnSingleVoter) {
  std::vector<Point2d> voters = {Point2d(0.0, 0.0)};
  EXPECT_THROW(limiting_quantile_lines_2d(voters), std::invalid_argument);
}
