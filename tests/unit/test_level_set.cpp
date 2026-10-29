// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include "socialchoicelab/core/types.h"
#include "socialchoicelab/preference/distance/distance_functions.h"
#include "socialchoicelab/preference/indifference/level_set.h"
#include "socialchoicelab/preference/loss/loss_functions.h"

using socialchoicelab::core::types::Point2d;
using socialchoicelab::core::types::PointNd;
using socialchoicelab::preference::distance::calculate_distance;
using socialchoicelab::preference::distance::DistanceType;
using socialchoicelab::preference::indifference::Circle2d;
using socialchoicelab::preference::indifference::DistanceConfig;
using socialchoicelab::preference::indifference::Ellipse2d;
using socialchoicelab::preference::indifference::ic_polygon_2d;
using socialchoicelab::preference::indifference::level_set_1d;
using socialchoicelab::preference::indifference::level_set_2d;
using socialchoicelab::preference::indifference::LevelSet2d;
using socialchoicelab::preference::indifference::LossConfig;
using socialchoicelab::preference::indifference::Polygon2d;
using socialchoicelab::preference::indifference::Superellipse2d;
using socialchoicelab::preference::indifference::to_polygon;
using socialchoicelab::preference::loss::distance_to_utility;
using socialchoicelab::preference::loss::LossFunctionType;

TEST(LevelSet1d, TwoPointsSymmetric) {
  PointNd ideal(1);
  ideal(0) = 5.0;
  LossConfig loss;
  loss.loss_type = LossFunctionType::QUADRATIC;
  loss.sensitivity = 1.0;
  DistanceConfig dist;
  dist.distance_type = DistanceType::EUCLIDEAN;
  dist.salience_weights = {1.0};
  double level = -4.0;  // u = -d^2 => d = 2
  auto pts = level_set_1d(ideal, level, loss, dist);
  ASSERT_EQ(pts.size(), 2u);
  EXPECT_DOUBLE_EQ(pts[0](0), 3.0);
  EXPECT_DOUBLE_EQ(pts[1](0), 7.0);
}

TEST(LevelSet1d, OnePointAtIdeal) {
  PointNd ideal(1);
  ideal(0) = 0.0;
  LossConfig loss;
  loss.loss_type = LossFunctionType::LINEAR;
  DistanceConfig dist;
  dist.salience_weights = {1.0};
  double level = 0.0;  // max utility at d=0
  auto pts = level_set_1d(ideal, level, loss, dist);
  ASSERT_EQ(pts.size(), 1u);
  EXPECT_DOUBLE_EQ(pts[0](0), 0.0);
}

TEST(LevelSet1d, EmptyWhenLevelUnreachable) {
  PointNd ideal(1);
  ideal(0) = 0.0;
  LossConfig loss;
  loss.loss_type = LossFunctionType::QUADRATIC;
  DistanceConfig dist;
  dist.salience_weights = {1.0};
  double level = 1.0;  // above max utility 0
  auto pts = level_set_1d(ideal, level, loss, dist);
  EXPECT_TRUE(pts.empty());
}

TEST(LevelSet2d, EuclideanEqualWeightsReturnsCircle) {
  Point2d ideal(0.0, 0.0);
  LossConfig loss;
  loss.loss_type = LossFunctionType::QUADRATIC;
  loss.sensitivity = 1.0;
  DistanceConfig dist;
  dist.distance_type = DistanceType::EUCLIDEAN;
  dist.salience_weights = {1.0, 1.0};
  double level = -1.0;  // d = 1
  LevelSet2d ls = level_set_2d(ideal, level, loss, dist);
  ASSERT_TRUE(std::holds_alternative<Circle2d>(ls));
  const auto& c = std::get<Circle2d>(ls);
  EXPECT_DOUBLE_EQ(c.center(0), 0.0);
  EXPECT_DOUBLE_EQ(c.center(1), 0.0);
  EXPECT_DOUBLE_EQ(c.radius, 1.0);
}

TEST(LevelSet2d, EuclideanUnequalWeightsReturnsEllipse) {
  Point2d ideal(0.0, 0.0);
  LossConfig loss;
  loss.loss_type = LossFunctionType::QUADRATIC;
  DistanceConfig dist;
  dist.distance_type = DistanceType::EUCLIDEAN;
  dist.salience_weights = {2.0, 1.0};  // r/w0 = r/2, r/w1 = r
  double level = -4.0;  // d = 2 => semi_axis_0 = 1, semi_axis_1 = 2
  LevelSet2d ls = level_set_2d(ideal, level, loss, dist);
  ASSERT_TRUE(std::holds_alternative<Ellipse2d>(ls));
  const auto& e = std::get<Ellipse2d>(ls);
  EXPECT_DOUBLE_EQ(e.semi_axis_0, 1.0);
  EXPECT_DOUBLE_EQ(e.semi_axis_1, 2.0);
}

TEST(LevelSet2d, ManhattanReturnsPolygonFourVertices) {
  Point2d ideal(0.0, 0.0);
  LossConfig loss;
  loss.loss_type = LossFunctionType::LINEAR;
  loss.sensitivity = 1.0;
  DistanceConfig dist;
  dist.distance_type = DistanceType::MANHATTAN;
  dist.salience_weights = {1.0, 1.0};
  double level = -2.0;  // d = 2
  LevelSet2d ls = level_set_2d(ideal, level, loss, dist);
  ASSERT_TRUE(std::holds_alternative<Polygon2d>(ls));
  const auto& p = std::get<Polygon2d>(ls);
  ASSERT_EQ(p.vertices.size(), 4u);
  EXPECT_DOUBLE_EQ(p.vertices[0](0), 2.0);
  EXPECT_DOUBLE_EQ(p.vertices[0](1), 0.0);
  EXPECT_DOUBLE_EQ(p.vertices[1](0), 0.0);
  EXPECT_DOUBLE_EQ(p.vertices[1](1), 2.0);
}

TEST(LevelSet2d, ChebyshevReturnsPolygonFourVertices) {
  Point2d ideal(0.0, 0.0);
  LossConfig loss;
  loss.loss_type = LossFunctionType::LINEAR;
  DistanceConfig dist;
  dist.distance_type = DistanceType::CHEBYSHEV;
  dist.salience_weights = {1.0, 1.0};
  double level = -1.0;  // d = 1 => square ±1
  LevelSet2d ls = level_set_2d(ideal, level, loss, dist);
  ASSERT_TRUE(std::holds_alternative<Polygon2d>(ls));
  const auto& poly = std::get<Polygon2d>(ls);
  ASSERT_EQ(poly.vertices.size(), 4u);
  EXPECT_DOUBLE_EQ(poly.vertices[0](0), 1.0);
  EXPECT_DOUBLE_EQ(poly.vertices[0](1), 1.0);
}

TEST(LevelSet2d, MinkowskiMiddlePReturnsSuperellipse) {
  Point2d ideal(0.0, 0.0);
  LossConfig loss;
  loss.loss_type = LossFunctionType::QUADRATIC;
  DistanceConfig dist;
  dist.distance_type = DistanceType::MINKOWSKI;
  dist.order_p = 3.0;
  dist.salience_weights = {1.0, 1.0};
  double level = -1.0;
  LevelSet2d ls = level_set_2d(ideal, level, loss, dist);
  ASSERT_TRUE(std::holds_alternative<Superellipse2d>(ls));
  const auto& s = std::get<Superellipse2d>(ls);
  EXPECT_DOUBLE_EQ(s.exponent_p, 3.0);
  EXPECT_DOUBLE_EQ(s.semi_axis_0, 1.0);
  EXPECT_DOUBLE_EQ(s.semi_axis_1, 1.0);
}

TEST(LevelSet2d, CirclePointsOnCurve) {
  Point2d ideal(1.0, 2.0);
  LossConfig loss;
  loss.loss_type = LossFunctionType::QUADRATIC;
  DistanceConfig dist;
  dist.distance_type = DistanceType::EUCLIDEAN;
  dist.salience_weights = {1.0, 1.0};
  double level = -9.0;  // d = 3
  LevelSet2d ls = level_set_2d(ideal, level, loss, dist);
  Polygon2d poly = to_polygon(ls, 64);
  ASSERT_EQ(poly.vertices.size(), 64u);
  std::vector<double> w = {1.0, 1.0};
  for (const auto& pt : poly.vertices) {
    double d = calculate_distance(ideal, pt, DistanceType::EUCLIDEAN, 2.0, w);
    EXPECT_NEAR(d, 3.0, 1e-10);
    double u = distance_to_utility(d, LossFunctionType::QUADRATIC, 1.0);
    EXPECT_NEAR(u, level, 1e-10);
  }
}

TEST(LevelSet2d, InvalidDimensionThrows) {
  PointNd ideal_2d_wrong(2);
  ideal_2d_wrong(0) = 0.0;
  ideal_2d_wrong(1) = 0.0;
  LossConfig loss;
  DistanceConfig dist;
  dist.salience_weights = {1.0};
  EXPECT_THROW((void)level_set_1d(ideal_2d_wrong, 0.0, loss, dist),
               std::invalid_argument);
  Point2d ideal_2d(0.0, 0.0);
  dist.salience_weights = {1.0};  // size 1, but level_set_2d requires size 2
  EXPECT_THROW((void)level_set_2d(ideal_2d, 0.0, loss, dist),
               std::invalid_argument);
}

// --- Equal non-unit salience weights (review finding S02a-001) ---
//
// r is the weighted distance, so the level set's semi-axes are r / w. Before
// the fix the equal-weights Euclidean branch returned a circle of radius r,
// so weights (2, 2) gave a curve twice the true size and (0.5, 0.5) half.

TEST(LevelSet2d, EqualNonUnitWeightsCircleRadiusIsROverW) {
  LossConfig loss;
  loss.loss_type = LossFunctionType::QUADRATIC;
  DistanceConfig dist;
  dist.distance_type = DistanceType::EUCLIDEAN;
  for (double w : {2.0, 0.5}) {
    dist.salience_weights = {w, w};
    // u = -d^2 = -4 -> weighted distance d = 2 -> radius 2 / w.
    LevelSet2d ls = level_set_2d(Point2d(0.0, 0.0), -4.0, loss, dist);
    ASSERT_TRUE(std::holds_alternative<Circle2d>(ls)) << "w = " << w;
    EXPECT_DOUBLE_EQ(std::get<Circle2d>(ls).radius, 2.0 / w) << "w = " << w;
  }
}

// For every distance type and for equal non-unit and unequal weights, the IC
// through sq has every vertex at weighted distance d(ideal, sq), and the
// vertex at parameter angle 0 is sq itself (sq lies on the +x axis).
TEST(LevelSet2d, IcPolygonVerticesAtWeightedDistanceForAllMetrics) {
  struct Metric {
    DistanceType type;
    double p;
    bool vertex0_is_sq;  // Chebyshev's first vertex is a corner, not sq.
  };
  const Metric metrics[] = {{DistanceType::EUCLIDEAN, 2.0, true},
                            {DistanceType::MANHATTAN, 1.0, true},
                            {DistanceType::CHEBYSHEV, 1.0, false},
                            {DistanceType::MINKOWSKI, 3.0, true}};
  const std::vector<std::vector<double>> weight_sets = {
    {2.0, 2.0}, {0.5, 0.5}, {2.0, 0.5}};
  const Point2d ideal(0.0, 0.0);
  const Point2d sq(1.0, 0.0);
  LossConfig loss;
  loss.loss_type = LossFunctionType::QUADRATIC;
  for (const auto& m : metrics) {
    for (const auto& w : weight_sets) {
      DistanceConfig dist;
      dist.distance_type = m.type;
      dist.order_p = m.p;
      dist.salience_weights = w;
      const double r = calculate_distance(ideal, sq, m.type, m.p, w);
      Polygon2d poly = ic_polygon_2d(ideal, sq, loss, dist, 64);
      ASSERT_FALSE(poly.vertices.empty());
      const std::string ctx =
          "type " + std::to_string(static_cast<int>(m.type)) + ", w = (" +
          std::to_string(w[0]) + ", " + std::to_string(w[1]) + ")";
      for (const auto& v : poly.vertices) {
        EXPECT_NEAR(calculate_distance(ideal, v, m.type, m.p, w), r, 1e-9)
            << ctx;
      }
      if (m.vertex0_is_sq) {
        EXPECT_NEAR(poly.vertices[0](0), sq(0), 1e-9) << ctx;
        EXPECT_NEAR(poly.vertices[0](1), sq(1), 1e-9) << ctx;
      }
    }
  }
}

// --- Zero / negative / non-finite weights (review finding S02b-003) ---

TEST(LevelSet2d, NonPositiveOrNonFiniteWeightThrowsWithMessage) {
  LossConfig loss;
  loss.loss_type = LossFunctionType::QUADRATIC;
  DistanceConfig dist;
  dist.distance_type = DistanceType::EUCLIDEAN;
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();
  for (const auto& w : std::vector<std::vector<double>>{
         {0.0, 1.0}, {1.0, 0.0}, {-1.0, 1.0}, {nan, 1.0}, {1.0, inf}}) {
    dist.salience_weights = w;
    try {
      (void)level_set_2d(Point2d(0.0, 0.0), -1.0, loss, dist);
      ADD_FAILURE() << "level_set_2d accepted weights (" << w[0] << ", " << w[1]
                    << ")";
    } catch (const std::invalid_argument& e) {
      const std::string msg = e.what();
      EXPECT_NE(msg.find("level_set_2d: salience_weights["), std::string::npos)
          << msg;
      EXPECT_NE(msg.find("finite and > 0"), std::string::npos) << msg;
    }
    EXPECT_THROW(
        (void)ic_polygon_2d(Point2d(0.0, 0.0), Point2d(1.0, 0.0), loss, dist),
        std::invalid_argument);
  }
}

TEST(LevelSet2d, SemiAxisOverflowThrows) {
  // A tiny positive weight passes the weight check, but r / w overflows.
  LossConfig loss;
  loss.loss_type = LossFunctionType::QUADRATIC;
  DistanceConfig dist;
  dist.distance_type = DistanceType::EUCLIDEAN;
  dist.salience_weights = {1e-310, 1.0};
  try {
    (void)level_set_2d(Point2d(0.0, 0.0), -1.0, loss, dist);
    ADD_FAILURE() << "level_set_2d accepted a semi-axis that overflows";
  } catch (const std::invalid_argument& e) {
    EXPECT_NE(std::string(e.what()).find("too large to represent"),
              std::string::npos)
        << e.what();
  }
}
