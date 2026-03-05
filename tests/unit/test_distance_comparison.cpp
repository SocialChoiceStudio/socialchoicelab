// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <Eigen/Dense>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <random>
#include <vector>

#include "socialchoicelab/preference/distance/distance_functions.h"

using socialchoicelab::preference::distance::minkowski_distance;

class DistanceComparisonTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Fixed seed for reproducible tests (no std::random_device)
    gen.seed(12345);
  }

  std::mt19937 gen;
  std::uniform_real_distribution<double> point_dist{-100.0, 100.0};
  std::uniform_real_distribution<double> order_dist{1.0, 10.0};

  // Generate random point pairs
  std::vector<std::pair<Eigen::Vector2d, Eigen::Vector2d>> generatePointPairs(
      int nPairs) {
    std::vector<std::pair<Eigen::Vector2d, Eigen::Vector2d>> pairs;
    for (int i = 0; i < nPairs; ++i) {
      Eigen::Vector2d point_a(point_dist(gen), point_dist(gen));
      Eigen::Vector2d point_b(point_dist(gen), point_dist(gen));
      pairs.push_back({point_a, point_b});
    }
    return pairs;
  }

  // Generate Minkowski orders (random + fixed values)
  std::vector<double> generateOrders(int nOrders) {
    std::vector<double> orders;
    orders.push_back(1.0);     // Fixed
    orders.push_back(2.0);     // Fixed
    orders.push_back(1000.0);  // Fixed (now safe with socialchoicelab fix)
    for (int i = 0; i < nOrders; ++i) {
      orders.push_back(order_dist(gen));
    }
    std::sort(orders.begin(), orders.end());  // Sort for monotonicity check
    return orders;
  }

  // Calculate Raw C++ Minkowski distance
  double calculateRawMinkowski(const Eigen::Vector2d& a,
                               const Eigen::Vector2d& b, double order) {
    if (order > 100.0) {
      // For large orders, use Chebyshev distance to avoid overflow
      return std::max(std::abs(b[0] - a[0]), std::abs(b[1] - a[1]));
    } else {
      return std::pow(std::pow(std::abs(b[0] - a[0]), order) +
                          std::pow(std::abs(b[1] - a[1]), order),
                      1.0 / order);
    }
  }
};

// Test with a small number of pairs and orders
TEST_F(DistanceComparisonTest, SmallTest) {
  int nPairs = 3;
  int nOrders = 2;

  auto pairs = generatePointPairs(nPairs);
  auto orders = generateOrders(nOrders);

  for (const auto& pair : pairs) {
    const Eigen::Vector2d& point_a = pair.first;
    const Eigen::Vector2d& point_b = pair.second;

    for (double order : orders) {
      // Calculate using socialchoicelab function
      std::vector<double> equal_weights = {1.0, 1.0};
      double minkowski_socialchoicelab =
          minkowski_distance(point_a, point_b, order, equal_weights);

      // Calculate using Raw C++ math
      double minkowski_rawcpp = calculateRawMinkowski(point_a, point_b, order);

      // Compare results
      EXPECT_NEAR(minkowski_socialchoicelab, minkowski_rawcpp, 1e-15)
          << "Order " << order << " failed for points " << point_a << " and "
          << point_b;
    }
  }
}

// Test with more pairs and orders
TEST_F(DistanceComparisonTest, MediumTest) {
  int nPairs = 10;
  int nOrders = 5;

  auto pairs = generatePointPairs(nPairs);
  auto orders = generateOrders(nOrders);

  for (const auto& pair : pairs) {
    const Eigen::Vector2d& point_a = pair.first;
    const Eigen::Vector2d& point_b = pair.second;

    for (double order : orders) {
      // Calculate using socialchoicelab function
      std::vector<double> equal_weights = {1.0, 1.0};
      double minkowski_socialchoicelab =
          minkowski_distance(point_a, point_b, order, equal_weights);

      // Calculate using Raw C++ math
      double minkowski_rawcpp = calculateRawMinkowski(point_a, point_b, order);

      // Compare results
      EXPECT_NEAR(minkowski_socialchoicelab, minkowski_rawcpp, 1e-15)
          << "Order " << order << " failed for points " << point_a << " and "
          << point_b;
    }
  }
}

// Test monotonicity property
TEST_F(DistanceComparisonTest, MonotonicityTest) {
  int nPairs = 5;
  int nOrders = 8;

  auto pairs = generatePointPairs(nPairs);
  auto orders = generateOrders(nOrders);

  for (const auto& pair : pairs) {
    const Eigen::Vector2d& point_a = pair.first;
    const Eigen::Vector2d& point_b = pair.second;

    std::vector<double> distances;
    for (double order : orders) {
      std::vector<double> equal_weights = {1.0, 1.0};
      double dist = minkowski_distance(point_a, point_b, order, equal_weights);
      distances.push_back(dist);
    }

    // Check monotonicity: distances should be non-increasing as order increases
    // Use tolerance for floating-point precision
    for (size_t i = 1; i < distances.size(); ++i) {
      EXPECT_LE(distances[i], distances[i - 1] + 1e-12)
          << "Monotonicity failed: order " << orders[i - 1] << " -> "
          << orders[i] << " gave " << distances[i - 1] << " -> "
          << distances[i];
    }
  }
}

// Test edge cases
TEST_F(DistanceComparisonTest, EdgeCases) {
  // Test with identical points
  Eigen::Vector2d identical(0.0, 0.0);
  std::vector<double> equal_weights = {1.0, 1.0};

  for (double order : {1.0, 2.0, 10.0, 1000.0}) {
    double dist =
        minkowski_distance(identical, identical, order, equal_weights);
    EXPECT_DOUBLE_EQ(dist, 0.0)
        << "Identical points should have distance 0 for order " << order;
  }

  // Test with points on axes
  Eigen::Vector2d origin(0.0, 0.0);
  Eigen::Vector2d x_axis(5.0, 0.0);
  Eigen::Vector2d y_axis(0.0, 3.0);

  double dist_x = minkowski_distance(origin, x_axis, 1.0, equal_weights);
  double dist_y = minkowski_distance(origin, y_axis, 1.0, equal_weights);

  EXPECT_DOUBLE_EQ(dist_x, 5.0) << "Distance along x-axis should be 5.0";
  EXPECT_DOUBLE_EQ(dist_y, 3.0) << "Distance along y-axis should be 3.0";
}
