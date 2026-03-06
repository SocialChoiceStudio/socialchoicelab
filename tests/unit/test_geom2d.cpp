// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <socialchoicelab/geometry/geom2d.h>

#include <cmath>

namespace scg = socialchoicelab::geometry;
namespace scc = socialchoicelab::core;

// ---------------------------------------------------------------------------
// Conversion: to_exact / to_numeric
// ---------------------------------------------------------------------------

TEST(ToExact, PositiveCoordinates) {
  scc::types::Point2d p(3.5, 7.25);
  scg::Point2E e = scg::to_exact(p);
  EXPECT_DOUBLE_EQ(CGAL::to_double(e.x()), 3.5);
  EXPECT_DOUBLE_EQ(CGAL::to_double(e.y()), 7.25);
}

TEST(ToExact, ZeroPoint) {
  scc::types::Point2d p(0.0, 0.0);
  scg::Point2E e = scg::to_exact(p);
  EXPECT_DOUBLE_EQ(CGAL::to_double(e.x()), 0.0);
  EXPECT_DOUBLE_EQ(CGAL::to_double(e.y()), 0.0);
}

TEST(ToExact, NegativeCoordinates) {
  scc::types::Point2d p(-1.0, -2.5);
  scg::Point2E e = scg::to_exact(p);
  EXPECT_DOUBLE_EQ(CGAL::to_double(e.x()), -1.0);
  EXPECT_DOUBLE_EQ(CGAL::to_double(e.y()), -2.5);
}

TEST(ToNumeric, RoundTrip) {
  scc::types::Point2d original(1.5, -3.25);
  scg::Point2E exact = scg::to_exact(original);
  scc::types::Point2d back = scg::to_numeric(exact);
  EXPECT_DOUBLE_EQ(back.x(), original.x());
  EXPECT_DOUBLE_EQ(back.y(), original.y());
}

TEST(ToNumeric, UnitPoints) {
  for (double x : {0.0, 1.0, -1.0}) {
    for (double y : {0.0, 1.0, -1.0}) {
      scc::types::Point2d p(x, y);
      scc::types::Point2d back = scg::to_numeric(scg::to_exact(p));
      EXPECT_DOUBLE_EQ(back.x(), x);
      EXPECT_DOUBLE_EQ(back.y(), y);
    }
  }
}

// ---------------------------------------------------------------------------
// Predicate: orientation
// ---------------------------------------------------------------------------

TEST(Orientation, LeftTurn) {
  // (0,0) → (1,0) → (1,1): point is to the left of the ray.
  scg::Point2E a(0, 0), b(1, 0), c(1, 1);
  EXPECT_EQ(scg::orientation(a, b, c), scg::Orientation::kLeftTurn);
}

TEST(Orientation, RightTurn) {
  // (0,0) → (1,0) → (1,-1): point is to the right of the ray.
  scg::Point2E a(0, 0), b(1, 0), c(1, -1);
  EXPECT_EQ(scg::orientation(a, b, c), scg::Orientation::kRightTurn);
}

TEST(Orientation, Collinear) {
  // Three points on the x-axis.
  scg::Point2E a(0, 0), b(1, 0), c(2, 0);
  EXPECT_EQ(scg::orientation(a, b, c), scg::Orientation::kCollinear);
}

TEST(Orientation, CollinearNegative) {
  scg::Point2E a(0, 0), b(-1, -1), c(-2, -2);
  EXPECT_EQ(scg::orientation(a, b, c), scg::Orientation::kCollinear);
}

TEST(Orientation, SymmetryLeftRight) {
  // Swapping b and c reverses the orientation.
  scg::Point2E a(0, 0), b(1, 0), c(0, 1);
  EXPECT_EQ(scg::orientation(a, b, c), scg::Orientation::kLeftTurn);
  EXPECT_EQ(scg::orientation(a, c, b), scg::Orientation::kRightTurn);
}

// ---------------------------------------------------------------------------
// Predicate: bounded_side
// ---------------------------------------------------------------------------

// Helper: unit square CCW (0,0),(1,0),(1,1),(0,1)
static scg::Polygon2E unit_square() {
  scg::Polygon2E poly;
  poly.push_back(scg::Point2E(0, 0));
  poly.push_back(scg::Point2E(1, 0));
  poly.push_back(scg::Point2E(1, 1));
  poly.push_back(scg::Point2E(0, 1));
  return poly;
}

TEST(BoundedSide, InsideSquare) {
  scg::Polygon2E sq = unit_square();
  EXPECT_EQ(scg::bounded_side(sq, scg::Point2E(0.5, 0.5)),
            scg::BoundedSide::kOnBoundedSide);
}

TEST(BoundedSide, InsideSquareNearCorner) {
  scg::Polygon2E sq = unit_square();
  EXPECT_EQ(scg::bounded_side(sq, scg::Point2E(0.01, 0.01)),
            scg::BoundedSide::kOnBoundedSide);
}

TEST(BoundedSide, OnBottomEdge) {
  scg::Polygon2E sq = unit_square();
  EXPECT_EQ(scg::bounded_side(sq, scg::Point2E(0.5, 0)),
            scg::BoundedSide::kOnBoundary);
}

TEST(BoundedSide, OnCorner) {
  scg::Polygon2E sq = unit_square();
  EXPECT_EQ(scg::bounded_side(sq, scg::Point2E(0, 0)),
            scg::BoundedSide::kOnBoundary);
}

TEST(BoundedSide, OutsideSquare) {
  scg::Polygon2E sq = unit_square();
  EXPECT_EQ(scg::bounded_side(sq, scg::Point2E(2, 2)),
            scg::BoundedSide::kOnUnboundedSide);
}

TEST(BoundedSide, OutsideSquareNearEdge) {
  scg::Polygon2E sq = unit_square();
  EXPECT_EQ(scg::bounded_side(sq, scg::Point2E(1.001, 0.5)),
            scg::BoundedSide::kOnUnboundedSide);
}

TEST(BoundedSide, InsideTriangle) {
  scg::Polygon2E tri;
  tri.push_back(scg::Point2E(0, 0));
  tri.push_back(scg::Point2E(4, 0));
  tri.push_back(scg::Point2E(0, 4));
  EXPECT_EQ(scg::bounded_side(tri, scg::Point2E(1, 1)),
            scg::BoundedSide::kOnBoundedSide);
  EXPECT_EQ(scg::bounded_side(tri, scg::Point2E(3, 3)),
            scg::BoundedSide::kOnUnboundedSide);
}
