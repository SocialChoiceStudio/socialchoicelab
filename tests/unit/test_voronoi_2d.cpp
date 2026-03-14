// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <socialchoicelab/geometry/voronoi_2d.h>

#include <cmath>
#include <vector>

using socialchoicelab::core::types::Point2d;
using socialchoicelab::geometry::voronoi_cells_2d;

namespace {

TEST(Voronoi2d, TwoSitesBbox) {
  // Two sites at (0,0) and (2,0). Bisector at x=1. Cell for site 0: x <= 1.
  double sites[] = {0.0, 0.0, 2.0, 0.0};
  std::vector<std::vector<Point2d>> cells =
      voronoi_cells_2d(sites, 2, -0.5, -0.5, 2.5, 0.5);
  ASSERT_EQ(cells.size(), 2u);
  // Cell 0: left half of bbox (quadrilateral)
  EXPECT_GE(cells[0].size(), 3u);
  for (const auto& p : cells[0]) {
    EXPECT_LE(p.x(), 1.01);  // allow small numerical tolerance
    EXPECT_GE(p.x(), -0.51);
    EXPECT_GE(p.y(), -0.51);
    EXPECT_LE(p.y(), 0.51);
  }
  // Cell 1: right half
  EXPECT_GE(cells[1].size(), 3u);
  for (const auto& p : cells[1]) {
    EXPECT_GE(p.x(), 0.99);
    EXPECT_LE(p.x(), 2.51);
    EXPECT_GE(p.y(), -0.51);
    EXPECT_LE(p.y(), 0.51);
  }
}

TEST(Voronoi2d, ThreeSites) {
  // Equilateral triangle: (0,0), (1,0), (0.5, sqrt(3)/2). Voronoi cells
  // are bounded; each cell should be a polygon inside the bbox.
  double sites[] = {0.0, 0.0, 1.0, 0.0, 0.5, 0.866025403784439};
  std::vector<std::vector<Point2d>> cells =
      voronoi_cells_2d(sites, 3, -0.2, -0.2, 1.2, 1.1);
  ASSERT_EQ(cells.size(), 3u);
  for (const auto& cell : cells) {
    EXPECT_GE(cell.size(), 3u) << "each cell should be a polygon";
    for (const auto& p : cell) {
      EXPECT_GE(p.x(), -0.21);
      EXPECT_LE(p.x(), 1.21);
      EXPECT_GE(p.y(), -0.21);
      EXPECT_LE(p.y(), 1.11);
    }
  }
}

TEST(Voronoi2d, InvalidNSites) {
  double sites[] = {0.0, 0.0};
  EXPECT_THROW(voronoi_cells_2d(sites, 0, 0, 0, 1, 1), std::invalid_argument);
}

TEST(Voronoi2d, InvalidBbox) {
  double sites[] = {0.0, 0.0};
  EXPECT_THROW(voronoi_cells_2d(sites, 1, 1.0, 0, 0.5, 1),
               std::invalid_argument);
  EXPECT_THROW(voronoi_cells_2d(sites, 1, 0, 1, 1, 0.5), std::invalid_argument);
}

}  // namespace
