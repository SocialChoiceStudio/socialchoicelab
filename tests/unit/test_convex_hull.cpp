// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <socialchoicelab/geometry/convex_hull.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace scg = socialchoicelab::geometry;
namespace scc = socialchoicelab::core;

using Pt = scc::types::Point2d;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Return the set of hull vertex coordinates as doubles for easy comparison.
static std::vector<std::pair<double, double>> hull_vertices(
    const scg::Polygon2E& hull) {
  std::vector<std::pair<double, double>> out;
  for (auto it = hull.vertices_begin(); it != hull.vertices_end(); ++it) {
    out.emplace_back(CGAL::to_double(it->x()), CGAL::to_double(it->y()));
  }
  return out;
}

// ---------------------------------------------------------------------------
// Correctness: basic shapes
// ---------------------------------------------------------------------------

TEST(ConvexHull2d, SinglePoint) {
  std::vector<Pt> pts = {Pt(3.0, 5.0)};
  scg::Polygon2E hull = scg::convex_hull_2d(pts);
  EXPECT_EQ(hull.size(), 1u);
  auto verts = hull_vertices(hull);
  EXPECT_DOUBLE_EQ(verts[0].first, 3.0);
  EXPECT_DOUBLE_EQ(verts[0].second, 5.0);
}

TEST(ConvexHull2d, TwoPoints) {
  std::vector<Pt> pts = {Pt(0.0, 0.0), Pt(2.0, 0.0)};
  scg::Polygon2E hull = scg::convex_hull_2d(pts);
  EXPECT_EQ(hull.size(), 2u);
}

TEST(ConvexHull2d, Triangle) {
  // Right triangle with vertices at (0,0), (1,0), (0,1).
  std::vector<Pt> pts = {Pt(0.0, 0.0), Pt(1.0, 0.0), Pt(0.0, 1.0)};
  scg::Polygon2E hull = scg::convex_hull_2d(pts);
  EXPECT_EQ(hull.size(), 3u);
}

TEST(ConvexHull2d, Square) {
  std::vector<Pt> pts = {Pt(0.0, 0.0), Pt(1.0, 0.0), Pt(1.0, 1.0),
                         Pt(0.0, 1.0)};
  scg::Polygon2E hull = scg::convex_hull_2d(pts);
  EXPECT_EQ(hull.size(), 4u);
}

TEST(ConvexHull2d, InteriorPointExcluded) {
  // Add an interior point to the square — hull should still have 4 vertices.
  std::vector<Pt> pts = {Pt(0.0, 0.0), Pt(1.0, 0.0), Pt(1.0, 1.0), Pt(0.0, 1.0),
                         Pt(0.5, 0.5)};
  scg::Polygon2E hull = scg::convex_hull_2d(pts);
  EXPECT_EQ(hull.size(), 4u);
}

TEST(ConvexHull2d, CollinearPoints_ReturnsTwoExtremes) {
  // Four collinear points on the x-axis: hull has 2 vertices.
  std::vector<Pt> pts = {Pt(0.0, 0.0), Pt(1.0, 0.0), Pt(2.0, 0.0),
                         Pt(3.0, 0.0)};
  scg::Polygon2E hull = scg::convex_hull_2d(pts);
  EXPECT_EQ(hull.size(), 2u);
  // Extreme points should be (0,0) and (3,0).
  auto verts = hull_vertices(hull);
  double min_x = std::min(verts[0].first, verts[1].first);
  double max_x = std::max(verts[0].first, verts[1].first);
  EXPECT_DOUBLE_EQ(min_x, 0.0);
  EXPECT_DOUBLE_EQ(max_x, 3.0);
}

TEST(ConvexHull2d, LargerPointCloud) {
  // Pentagon: 5 vertices, 1 interior point — hull has 5 vertices.
  std::vector<Pt> pts = {
    Pt(2.0, 0.0),       Pt(0.618, 1.902),  Pt(-1.618, 1.176),
    Pt(-1.618, -1.176), Pt(0.618, -1.902), Pt(0.0, 0.0)  // interior
  };
  scg::Polygon2E hull = scg::convex_hull_2d(pts);
  EXPECT_EQ(hull.size(), 5u);
}

// ---------------------------------------------------------------------------
// Orientation: hull is CCW
// ---------------------------------------------------------------------------

TEST(ConvexHull2d, HullIsCounterClockwise) {
  // For a well-formed polygon CGAL::convex_hull_2 returns CCW order.
  // CCW <=> signed area > 0 for Polygon_2.
  std::vector<Pt> pts = {Pt(0.0, 0.0), Pt(3.0, 0.0), Pt(3.0, 3.0),
                         Pt(0.0, 3.0)};
  scg::Polygon2E hull = scg::convex_hull_2d(pts);
  ASSERT_GE(hull.size(), 3u);
  EXPECT_GT(CGAL::to_double(hull.area()), 0.0);
}

// ---------------------------------------------------------------------------
// Pareto set property (Euclidean): all input points are inside or on the hull
// ---------------------------------------------------------------------------

TEST(ConvexHull2d, AllInputPointsInsideOrOnHull) {
  std::vector<Pt> pts = {Pt(0.0, 0.0), Pt(4.0, 0.0), Pt(4.0, 4.0),
                         Pt(0.0, 4.0), Pt(1.0, 1.0), Pt(2.0, 3.0)};
  scg::Polygon2E hull = scg::convex_hull_2d(pts);
  for (const auto& p : pts) {
    auto side = scg::bounded_side(hull, scg::to_exact(p));
    EXPECT_NE(side, scg::BoundedSide::kOnUnboundedSide)
        << "Input point (" << p.x() << "," << p.y()
        << ") is outside its own convex hull.";
  }
}

// ---------------------------------------------------------------------------
// Error handling
// ---------------------------------------------------------------------------

TEST(ConvexHull2d, Error_EmptyInput) {
  EXPECT_THROW(scg::convex_hull_2d({}), std::invalid_argument);
}

TEST(ConvexHull2d, Error_NaNCoordinate) {
  std::vector<Pt> pts = {Pt(0.0, 0.0),
                         Pt(std::numeric_limits<double>::quiet_NaN(), 1.0)};
  EXPECT_THROW(scg::convex_hull_2d(pts), std::invalid_argument);
}

TEST(ConvexHull2d, Error_InfCoordinate) {
  std::vector<Pt> pts = {Pt(0.0, 0.0),
                         Pt(std::numeric_limits<double>::infinity(), 1.0)};
  EXPECT_THROW(scg::convex_hull_2d(pts), std::invalid_argument);
}

TEST(ConvexHull2d, Error_NegInfCoordinate) {
  std::vector<Pt> pts = {Pt(0.0, -std::numeric_limits<double>::infinity()),
                         Pt(1.0, 1.0)};
  EXPECT_THROW(scg::convex_hull_2d(pts), std::invalid_argument);
}
