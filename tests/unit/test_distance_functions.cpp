// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <Eigen/Dense>
#include <cmath>
#include <vector>

#include "socialchoicelab/preference/distance/distance_functions.h"

using namespace socialchoicelab::preference::distance;

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
