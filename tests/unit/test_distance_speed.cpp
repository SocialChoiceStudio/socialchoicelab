// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <Eigen/Dense>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

#include "socialchoicelab/preference/distance/distance_functions.h"

using socialchoicelab::preference::distance::minkowski_distance;

// Benchmark function to time execution
template <typename Func>
double benchmark(Func func, int iterations = 100000) {
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < iterations; ++i) {
    func();
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
  return duration.count() /
         static_cast<double>(iterations);  // nanoseconds per call
}

class DistanceSpeedTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Fixed seed for reproducible tests (no std::random_device)
    gen.seed(12345);

    // Generate test pairs
    for (int i = 0; i < 10; ++i) {
      Eigen::Vector2d point_a(point_dist(gen), point_dist(gen));
      Eigen::Vector2d point_b(point_dist(gen), point_dist(gen));
      pairs.push_back({point_a, point_b});
    }

    orders = {1.0, 2.0, 3.0, 10.0, 100.0, 1000.0};
    equal_weights = {1.0, 1.0};
    different_weights = {2.0, 0.5};
  }

  std::mt19937 gen;
  std::uniform_real_distribution<double> point_dist{-100.0, 100.0};
  std::vector<std::pair<Eigen::Vector2d, Eigen::Vector2d>> pairs;
  std::vector<double> orders;
  std::vector<double> equal_weights;
  std::vector<double> different_weights;

  // Calculate raw C++ Minkowski distance (equal weights)
  double calculateRawCppMinkowskiEqual(const Eigen::Vector2d& a,
                                       const Eigen::Vector2d& b, double order) {
    if (order > 100.0) {
      return std::max(std::abs(b[0] - a[0]), std::abs(b[1] - a[1]));
    } else {
      double sum = std::pow(std::abs(b[0] - a[0]), order) +
                   std::pow(std::abs(b[1] - a[1]), order);
      return std::pow(sum, 1.0 / order);
    }
  }

  // Calculate raw C++ Minkowski distance (weighted)
  double calculateRawCppMinkowskiWeighted(const Eigen::Vector2d& a,
                                          const Eigen::Vector2d& b,
                                          double order,
                                          const std::vector<double>& weights) {
    if (order > 100.0) {
      return std::max(weights[0] * std::abs(b[0] - a[0]),
                      weights[1] * std::abs(b[1] - a[1]));
    } else {
      double sum = std::pow(weights[0] * std::abs(b[0] - a[0]), order) +
                   std::pow(weights[1] * std::abs(b[1] - a[1]), order);
      return std::pow(sum, 1.0 / order);
    }
  }
};

// Test performance with equal weights
TEST_F(DistanceSpeedTest, EqualWeightsPerformance) {
  const int iterations = 10000;  // Reduced for test speed

  for (double order : orders) {
    // Benchmark our function
    double socialchoicelab_time = benchmark(
        [&]() {
          for (const auto& pair : pairs) {
            volatile double result = minkowski_distance(pair.first, pair.second,
                                                        order, equal_weights);
            (void)result;
          }
        },
        iterations);

    // Benchmark raw C++ math
    double rawcpp_time = benchmark(
        [&]() {
          for (const auto& pair : pairs) {
            volatile double result =
                calculateRawCppMinkowskiEqual(pair.first, pair.second, order);
            (void)result;
          }
        },
        iterations);

    double ratio = socialchoicelab_time / rawcpp_time;

    // Report only; no timing assertion (machine-dependent). Use for monitoring.
    std::cout << "Order " << order
              << ": socialchoicelab=" << socialchoicelab_time
              << "ns, Raw C++=" << rawcpp_time << "ns, Ratio=" << ratio
              << std::endl;
  }
}

// Test performance with different weights
TEST_F(DistanceSpeedTest, WeightedPerformance) {
  const int iterations = 10000;  // Reduced for test speed

  for (double order : orders) {
    // Benchmark our function
    double socialchoicelab_time = benchmark(
        [&]() {
          for (const auto& pair : pairs) {
            volatile double result = minkowski_distance(
                pair.first, pair.second, order, different_weights);
            (void)result;
          }
        },
        iterations);

    // Benchmark raw C++ math
    double rawcpp_time = benchmark(
        [&]() {
          for (const auto& pair : pairs) {
            volatile double result = calculateRawCppMinkowskiWeighted(
                pair.first, pair.second, order, different_weights);
            (void)result;
          }
        },
        iterations);

    double ratio = socialchoicelab_time / rawcpp_time;

    // Report only; no timing assertion (machine-dependent). Use for monitoring.
    std::cout << "Order " << order
              << " (weighted): socialchoicelab=" << socialchoicelab_time
              << "ns, Raw C++=" << rawcpp_time << "ns, Ratio=" << ratio
              << std::endl;
  }
}

// Test that our function produces correct results (accuracy check)
TEST_F(DistanceSpeedTest, AccuracyCheck) {
  for (const auto& pair : pairs) {
    for (double order : orders) {
      // Test equal weights
      double our_equal =
          minkowski_distance(pair.first, pair.second, order, equal_weights);
      double raw_equal =
          calculateRawCppMinkowskiEqual(pair.first, pair.second, order);
      EXPECT_NEAR(our_equal, raw_equal, 1e-15)
          << "Equal weights accuracy failed for order " << order;

      // Test different weights
      double our_weighted =
          minkowski_distance(pair.first, pair.second, order, different_weights);
      double raw_weighted = calculateRawCppMinkowskiWeighted(
          pair.first, pair.second, order, different_weights);
      EXPECT_NEAR(our_weighted, raw_weighted, 1e-15)
          << "Weighted accuracy failed for order " << order;
    }
  }
}

// Report timing variance (informational only; no assertion)
TEST_F(DistanceSpeedTest, PerformanceConsistency) {
  const int iterations = 5000;
  const double order = 2.0;  // Euclidean distance

  std::vector<double> times;
  for (int run = 0; run < 5; ++run) {
    double time = benchmark(
        [&]() {
          for (const auto& pair : pairs) {
            volatile double result = minkowski_distance(pair.first, pair.second,
                                                        order, equal_weights);
            (void)result;
          }
        },
        iterations);
    times.push_back(time);
  }

  double mean_time =
      std::accumulate(times.begin(), times.end(), 0.0) / times.size();
  std::cout << "PerformanceConsistency: mean=" << mean_time
            << " ns/call over 5 runs (Euclidean)" << std::endl;
}
