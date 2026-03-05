// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <vector>

#include "socialchoicelab/core/linalg.h"
#include "socialchoicelab/core/types.h"
#include "socialchoicelab/preference/distance/distance_functions.h"

using socialchoicelab::core::linalg::Index;
using socialchoicelab::core::types::Point2d;
using socialchoicelab::core::types::Point3d;
using socialchoicelab::core::types::PointNd;
using socialchoicelab::preference::distance::euclidean_distance;
using socialchoicelab::preference::distance::manhattan_distance;

TEST(CoreTypes, Point2dWorksWithDistanceApi) {
  Point2d ideal(0.0, 0.0);
  Point2d alt(3.0, 4.0);
  std::vector<double> w = {1.0, 1.0};
  double d = euclidean_distance(ideal, alt, w);
  EXPECT_DOUBLE_EQ(d, 5.0);
  double m = manhattan_distance(ideal, alt, w);
  EXPECT_DOUBLE_EQ(m, 7.0);
}

TEST(CoreTypes, Point3dWorksWithDistanceApi) {
  Point3d ideal(0.0, 0.0, 0.0);
  Point3d alt(1.0, 0.0, 0.0);
  std::vector<double> w = {1.0, 1.0, 1.0};
  double d = euclidean_distance(ideal, alt, w);
  EXPECT_DOUBLE_EQ(d, 1.0);
}

TEST(CoreTypes, PointNdWorksWithDistanceApi) {
  PointNd ideal(1);
  PointNd alt(1);
  ideal(0) = 0.0;
  alt(0) = 10.0;
  std::vector<double> w = {1.0};
  double d = euclidean_distance(ideal, alt, w);
  EXPECT_DOUBLE_EQ(d, 10.0);
}

TEST(CoreTypes, LinalgIndexIsUsable) {
  Point2d p(1.0, 2.0);
  Index n = p.size();
  EXPECT_EQ(n, 2);
}
