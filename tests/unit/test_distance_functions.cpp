// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <Eigen/Dense>
#include <cmath>
#include <vector>

#include "socialchoicelab/preference/distance/distance_functions.h"

using socialchoicelab::preference::distance::calculate_distance;
using socialchoicelab::preference::distance::chebyshev_distance;
using socialchoicelab::preference::distance::DistanceType;
using socialchoicelab::preference::distance::euclidean_distance;
using socialchoicelab::preference::distance::manhattan_distance;
using socialchoicelab::preference::distance::minkowski_distance;

class DistanceFunctionsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Test points: A = (0, 0), B = (1000, 1)
    point_a = Eigen::Vector2d(0.0, 0.0);
    point_b = Eigen::Vector2d(1000.0, 1.0);
  }

  Eigen::Vector2d point_a;
  Eigen::Vector2d point_b;
};

// Test specific distance values for the edge case
TEST_F(DistanceFunctionsTest, EdgeCaseExactValues) {
  std::vector<double> equal_weights = {1.0, 1.0};

  // δ = 1 (Manhattan distance)
  double manhattan = manhattan_distance(point_a, point_b, equal_weights);
  EXPECT_DOUBLE_EQ(manhattan, 1001.0);

  // δ = 2 (Euclidean distance)
  double euclidean = euclidean_distance(point_a, point_b, equal_weights);
  EXPECT_NEAR(euclidean, 1000.000499999875, 1e-10);

  // δ = ∞ (Chebyshev distance)
  double chebyshev = chebyshev_distance(point_a, point_b, equal_weights);
  EXPECT_DOUBLE_EQ(chebyshev, 1000.0);
}

// Test monotonicity property
TEST_F(DistanceFunctionsTest, MonotonicityProperty) {
  std::vector<double> delta_values = {1.0,  1.5,   2.0, 3.0,
                                      10.0, 1000.0};  // Use 1000 instead of inf
  std::vector<double> distances;
  std::vector<double> equal_weights = {1.0, 1.0};

  // Calculate distances for each δ value
  for (double delta : delta_values) {
    double dist = minkowski_distance(point_a, point_b, delta, equal_weights);
    distances.push_back(dist);
  }

  // Verify monotonicity: distances should be non-increasing as δ increases
  for (size_t i = 1; i < distances.size(); ++i) {
    EXPECT_LE(distances[i], distances[i - 1])
        << "Distance should be non-increasing: δ=" << delta_values[i - 1]
        << " gives " << distances[i - 1] << ", δ=" << delta_values[i]
        << " gives " << distances[i];
  }

  // Verify specific values
  EXPECT_DOUBLE_EQ(distances[0], 1001.0);               // δ = 1
  EXPECT_NEAR(distances[2], 1000.000499999875, 1e-10);  // δ = 2
  EXPECT_DOUBLE_EQ(distances[5], 1000.0);  // δ = 1000 (Chebyshev approximation)
}

// Test that distance is always positive (except for identical points)
TEST_F(DistanceFunctionsTest, PositiveDistance) {
  // Test with different δ values
  std::vector<double> delta_values = {1.0, 1.5, 2.0, 3.0, 10.0};
  std::vector<double> equal_weights = {1.0, 1.0};

  for (double delta : delta_values) {
    double dist = minkowski_distance(point_a, point_b, delta, equal_weights);
    EXPECT_GT(dist, 0.0) << "Distance should be positive for δ=" << delta;
  }
}

// Test symmetry property
TEST_F(DistanceFunctionsTest, SymmetryProperty) {
  std::vector<double> delta_values = {1.0, 1.5, 2.0, 3.0, 10.0};
  std::vector<double> equal_weights = {1.0, 1.0};

  for (double delta : delta_values) {
    double dist_a_to_b =
        minkowski_distance(point_a, point_b, delta, equal_weights);
    double dist_b_to_a =
        minkowski_distance(point_b, point_a, delta, equal_weights);
    EXPECT_DOUBLE_EQ(dist_a_to_b, dist_b_to_a)
        << "Distance should be symmetric for δ=" << delta;
  }
}

// Test that invalid order_p throws
TEST_F(DistanceFunctionsTest, MinkowskiOrderPMustBeAtLeastOne) {
  std::vector<double> equal_weights = {1.0, 1.0};
  EXPECT_THROW(minkowski_distance(point_a, point_b, 0.0, equal_weights),
               std::invalid_argument);
  EXPECT_THROW(minkowski_distance(point_a, point_b, 0.5, equal_weights),
               std::invalid_argument);
  EXPECT_THROW(minkowski_distance(point_a, point_b, -1.0, equal_weights),
               std::invalid_argument);
}

// Test that dimension mismatch throws
TEST_F(DistanceFunctionsTest, DimensionMismatchThrows) {
  std::vector<double> two_weights = {1.0, 1.0};
  std::vector<double> three_weights = {1.0, 1.0, 1.0};
  Eigen::Vector3d point_3d(1.0, 2.0, 3.0);
  EXPECT_THROW(euclidean_distance(point_a, point_3d, two_weights),
               std::invalid_argument);
  EXPECT_THROW(manhattan_distance(point_3d, point_a, three_weights),
               std::invalid_argument);
  EXPECT_THROW(chebyshev_distance(point_a, point_3d, two_weights),
               std::invalid_argument);
}

// Test that empty salience weights throws
TEST_F(DistanceFunctionsTest, EmptySalienceWeightsThrows) {
  std::vector<double> empty_weights;
  EXPECT_THROW(euclidean_distance(point_a, point_b, empty_weights),
               std::invalid_argument);
  EXPECT_THROW(manhattan_distance(point_a, point_b, empty_weights),
               std::invalid_argument);
  EXPECT_THROW(chebyshev_distance(point_a, point_b, empty_weights),
               std::invalid_argument);
}

// Test that wrong-size salience weights throws
TEST_F(DistanceFunctionsTest, WrongSizeSalienceWeightsThrows) {
  std::vector<double> one_weight = {1.0};
  std::vector<double> three_weights = {1.0, 1.0, 1.0};
  EXPECT_THROW(euclidean_distance(point_a, point_b, one_weight),
               std::invalid_argument);
  EXPECT_THROW(manhattan_distance(point_a, point_b, three_weights),
               std::invalid_argument);
  EXPECT_THROW(chebyshev_distance(point_a, point_b, one_weight),
               std::invalid_argument);
}

// Test that unknown DistanceType throws
TEST_F(DistanceFunctionsTest, UnknownDistanceTypeThrows) {
  std::vector<double> equal_weights = {1.0, 1.0};
  EXPECT_THROW(
      calculate_distance(point_a, point_b, static_cast<DistanceType>(99), 2.0,
                         equal_weights),
      std::invalid_argument);
}

// Test salience weights
TEST_F(DistanceFunctionsTest, SalienceWeights) {
  std::vector<double> weights = {2.0,
                                 0.5};  // Double weight on x, half weight on y

  // With weights [2.0, 0.5], the weighted Manhattan distance should be:
  // 2.0 * |1000| + 0.5 * |1| = 2000 + 0.5 = 2000.5
  double weighted_manhattan = manhattan_distance(point_a, point_b, weights);
  EXPECT_DOUBLE_EQ(weighted_manhattan, 2000.5);

  // Test that equal weights give same result as unweighted
  std::vector<double> equal_weights = {1.0, 1.0};
  double unweighted_manhattan =
      manhattan_distance(point_a, point_b, equal_weights);
  double raw_manhattan =
      std::abs(point_b[0] - point_a[0]) + std::abs(point_b[1] - point_a[1]);
  EXPECT_DOUBLE_EQ(unweighted_manhattan, raw_manhattan);
}

// Weighted Euclidean and Chebyshev, and 3D points (item 44)
TEST_F(DistanceFunctionsTest, WeightedEuclideanAndChebyshev) {
  std::vector<double> weights = {2.0, 0.5};
  // Weighted Euclidean: sqrt((2*1000)^2 + (0.5*1)^2) = sqrt(4e6 + 0.25)
  double we = euclidean_distance(point_a, point_b, weights);
  EXPECT_NEAR(we, std::sqrt(4000000.25), 1e-10);
  // Weighted Chebyshev: max(2000, 0.5) = 2000
  double wc = chebyshev_distance(point_a, point_b, weights);
  EXPECT_DOUBLE_EQ(wc, 2000.0);
}

TEST_F(DistanceFunctionsTest, ThreeDimensionalPoints) {
  Eigen::Vector3d a(0.0, 0.0, 0.0);
  Eigen::Vector3d b(3.0, 4.0, 0.0);  // 5 in xy-plane
  std::vector<double> equal_weights_3d = {1.0, 1.0, 1.0};
  EXPECT_DOUBLE_EQ(manhattan_distance(a, b, equal_weights_3d), 7.0);
  EXPECT_DOUBLE_EQ(euclidean_distance(a, b, equal_weights_3d), 5.0);
  EXPECT_DOUBLE_EQ(chebyshev_distance(a, b, equal_weights_3d), 4.0);

  std::vector<double> weights_3d = {1.0, 1.0, 2.0};  // double weight on z
  Eigen::Vector3d c(0.0, 0.0, 5.0);
  // Manhattan: 1*0 + 1*0 + 2*5 = 10
  EXPECT_DOUBLE_EQ(manhattan_distance(a, c, weights_3d), 10.0);
}

// Triangle inequality: d(a,c) <= d(a,b) + d(b,c) for metrics (item 45)
TEST_F(DistanceFunctionsTest, TriangleInequalityEqualWeights) {
  std::vector<double> two = {1.0, 1.0};
  Eigen::Vector2d a(0.0, 0.0);
  Eigen::Vector2d b(1.0, 2.0);
  Eigen::Vector2d c(4.0, 1.0);

  double d_ab_e = euclidean_distance(a, b, two);
  double d_bc_e = euclidean_distance(b, c, two);
  double d_ac_e = euclidean_distance(a, c, two);
  EXPECT_LE(d_ac_e, d_ab_e + d_bc_e + 1e-10) << "Euclidean triangle inequality";

  double d_ab_m = manhattan_distance(a, b, two);
  double d_bc_m = manhattan_distance(b, c, two);
  double d_ac_m = manhattan_distance(a, c, two);
  EXPECT_LE(d_ac_m, d_ab_m + d_bc_m + 1e-10) << "Manhattan triangle inequality";

  double d_ab_c = chebyshev_distance(a, b, two);
  double d_bc_c = chebyshev_distance(b, c, two);
  double d_ac_c = chebyshev_distance(a, c, two);
  EXPECT_LE(d_ac_c, d_ab_c + d_bc_c + 1e-10) << "Chebyshev triangle inequality";

  double d_ab_p = minkowski_distance(a, b, 1.5, two);
  double d_bc_p = minkowski_distance(b, c, 1.5, two);
  double d_ac_p = minkowski_distance(a, c, 1.5, two);
  EXPECT_LE(d_ac_p, d_ab_p + d_bc_p + 1e-10)
      << "Minkowski p=1.5 triangle inequality";
}
