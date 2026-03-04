// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <Eigen/Dense>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

#include "socialchoicelab/preference/distance/distance_functions.h"

using namespace socialchoicelab::preference::distance;

// Use the original distance functions namespace to avoid conflicts
namespace original_distance = socialchoicelab::preference::distance;

class PerformanceComparisonTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Fixed seed for reproducible tests (no std::random_device)
    std::mt19937 gen(12345);
    std::uniform_real_distribution<double> dis(-10.0, 10.0);

    // Generate 1000 random 2D points
    for (int i = 0; i < 1000; ++i) {
      double x = dis(gen);
      double y = dis(gen);

      eigen_points_1.push_back(Eigen::Vector2d(x, y));
      eigen_points_2.push_back(Eigen::Vector2d(x + 0.1, y + 0.1));
    }

    // Generate weights
    for (int i = 0; i < 1000; ++i) {
      weights.push_back({1.0, 1.0});
    }
  }

  std::vector<Eigen::Vector2d> eigen_points_1, eigen_points_2;
  std::vector<std::vector<double>> weights;
};

TEST_F(PerformanceComparisonTest, DistanceCalculationSpeed) {
  const int iterations = 1000;

  // Test Eigen performance
  auto start = std::chrono::high_resolution_clock::now();
  double eigen_sum = 0.0;
  for (int i = 0; i < iterations; ++i) {
    for (size_t j = 0; j < eigen_points_1.size(); ++j) {
      eigen_sum +=
          euclidean_distance(eigen_points_1[j], eigen_points_2[j], weights[j]);
    }
  }
  auto eigen_end = std::chrono::high_resolution_clock::now();
  auto eigen_duration =
      std::chrono::duration_cast<std::chrono::microseconds>(eigen_end - start);

  // Print results
  std::cout << "\n=== Eigen Performance Test ===" << std::endl;
  std::cout << "Eigen time:   " << eigen_duration.count() << " microseconds"
            << std::endl;
  std::cout << "Eigen sum:    " << eigen_sum << std::endl;

  // Correctness only; timing is machine-dependent (report only)
  EXPECT_GT(eigen_sum, 0.0);
}

TEST_F(PerformanceComparisonTest, EigenVectorOperations) {
  // Test that Eigen vector operations produce correct results at scale
  const int size = 10000;

  // Eigen memory usage
  std::vector<Eigen::Vector2d> eigen_vec;
  eigen_vec.reserve(size);
  for (int i = 0; i < size; ++i) {
    eigen_vec.emplace_back(1.0, 2.0);
  }

  // Should work without issues
  EXPECT_EQ(eigen_vec.size(), size);

  // Test that we can perform operations
  double eigen_sum = 0.0;
  for (const auto& p : eigen_vec) {
    eigen_sum += p.norm();
  }

  EXPECT_NEAR(eigen_sum, size * std::sqrt(5.0),
              1e-8);  // sqrt(1^2 + 2^2) = sqrt(5)
}

