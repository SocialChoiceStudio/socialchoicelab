// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <Eigen/Dense>
#include <algorithm>
#include <cmath>
#include <vector>

#include "socialchoicelab/preference/loss/loss_functions.h"

using socialchoicelab::preference::loss::distance_to_utility;
using socialchoicelab::preference::loss::gaussian_loss;
using socialchoicelab::preference::loss::linear_loss;
using socialchoicelab::preference::loss::LossFunctionType;
using socialchoicelab::preference::loss::normalize_utility;
using socialchoicelab::preference::loss::quadratic_loss;
using socialchoicelab::preference::loss::threshold_loss;

class LossFunctionsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Test distances for various scenarios
    distances = {0.0, 0.5, 1.0, 2.0, 5.0, 10.0};
    sensitivity = 2.0;
    max_loss = 5.0;
    steepness = 1.0;
    threshold = 1.5;
  }

  std::vector<double> distances;
  double sensitivity;
  double max_loss;
  double steepness;
  double threshold;
};

// Test linear loss function
TEST_F(LossFunctionsTest, LinearLoss) {
  // Test basic linear loss: L(d) = α|d| (positive loss magnitude)
  for (double d : distances) {
    double loss = linear_loss(d, sensitivity);
    double expected = sensitivity * std::abs(d);
    EXPECT_DOUBLE_EQ(loss, expected) << "Linear loss failed for distance " << d;
  }

  // Test at distance 0 (should be 0)
  EXPECT_DOUBLE_EQ(linear_loss(0.0, sensitivity), 0.0);

  // Test negative distance (should be same as positive)
  EXPECT_DOUBLE_EQ(linear_loss(-2.0, sensitivity),
                   linear_loss(2.0, sensitivity));

  // Test sensitivity parameter
  double loss_default = linear_loss(3.0);
  double loss_custom = linear_loss(3.0, sensitivity);
  EXPECT_DOUBLE_EQ(loss_default, 3.0);
  EXPECT_DOUBLE_EQ(loss_custom, 6.0);
}

// Test quadratic loss function
TEST_F(LossFunctionsTest, QuadraticLoss) {
  // Test basic quadratic loss: L(d) = αd² (positive loss magnitude)
  for (double d : distances) {
    double loss = quadratic_loss(d, sensitivity);
    double expected = sensitivity * d * d;
    EXPECT_DOUBLE_EQ(loss, expected)
        << "Quadratic loss failed for distance " << d;
  }

  // Test at distance 0 (should be 0)
  EXPECT_DOUBLE_EQ(quadratic_loss(0.0, sensitivity), 0.0);

  // Test negative distance (should be same as positive)
  EXPECT_DOUBLE_EQ(quadratic_loss(-2.0, sensitivity),
                   quadratic_loss(2.0, sensitivity));

  // Test sensitivity parameter
  double loss_default = quadratic_loss(3.0);
  double loss_custom = quadratic_loss(3.0, sensitivity);
  EXPECT_DOUBLE_EQ(loss_default, 9.0);
  EXPECT_DOUBLE_EQ(loss_custom, 18.0);
}

// Test gaussian loss function
TEST_F(LossFunctionsTest, GaussianLoss) {
  // Test basic gaussian loss: L(d) = α(1 - e^(-βd²))
  for (double d : distances) {
    double loss = gaussian_loss(d, max_loss, steepness);
    double expected = max_loss * (1.0 - std::exp(-steepness * d * d));
    EXPECT_NEAR(loss, expected, 1e-10)
        << "Gaussian loss failed for distance " << d;
  }

  // Test at distance 0 (should be 0)
  EXPECT_DOUBLE_EQ(gaussian_loss(0.0, max_loss, steepness), 0.0);

  // Test negative distance (should be same as positive)
  EXPECT_DOUBLE_EQ(gaussian_loss(-2.0, max_loss, steepness),
                   gaussian_loss(2.0, max_loss, steepness));

  // Test max_loss parameter
  double loss_default = gaussian_loss(2.0);
  double loss_custom = gaussian_loss(2.0, max_loss, steepness);
  double expected_default = 1.0 * (1.0 - std::exp(-1.0 * 4.0));
  double expected_custom = max_loss * (1.0 - std::exp(-steepness * 4.0));
  EXPECT_NEAR(loss_default, expected_default, 1e-10);
  EXPECT_NEAR(loss_custom, expected_custom, 1e-10);

  // Test steepness parameter
  double loss_steep = gaussian_loss(2.0, max_loss, 2.0 * steepness);
  double expected_steep = max_loss * (1.0 - std::exp(-2.0 * steepness * 4.0));
  EXPECT_NEAR(loss_steep, expected_steep, 1e-10);
}

// Test threshold loss function
TEST_F(LossFunctionsTest, ThresholdLoss) {
  // Test basic threshold loss: L(d) = α * max(0, |d| - threshold) (positive
  // loss magnitude)
  for (double d : distances) {
    double loss = threshold_loss(d, threshold, sensitivity);
    double expected = sensitivity * std::max(0.0, std::abs(d) - threshold);
    EXPECT_DOUBLE_EQ(loss, expected)
        << "Threshold loss failed for distance " << d;
  }

  // Test distances below threshold (should be 0)
  EXPECT_DOUBLE_EQ(threshold_loss(0.5, threshold, sensitivity), 0.0);
  EXPECT_DOUBLE_EQ(threshold_loss(1.0, threshold, sensitivity), 0.0);
  EXPECT_DOUBLE_EQ(threshold_loss(1.5, threshold, sensitivity), 0.0);

  // Test distances above threshold
  EXPECT_DOUBLE_EQ(threshold_loss(2.0, threshold, sensitivity),
                   1.0);  // 2.0 * (2.0 - 1.5)
  EXPECT_DOUBLE_EQ(threshold_loss(3.0, threshold, sensitivity),
                   3.0);  // 2.0 * (3.0 - 1.5)

  // Test negative distance (should be same as positive)
  EXPECT_DOUBLE_EQ(threshold_loss(-2.0, threshold, sensitivity),
                   threshold_loss(2.0, threshold, sensitivity));
}

// Test monotonicity properties
TEST_F(LossFunctionsTest, MonotonicityProperties) {
  std::vector<double> increasing_distances = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0};

  // Linear loss (positive magnitude) should increase with distance
  for (size_t i = 1; i < increasing_distances.size(); ++i) {
    double loss_prev = linear_loss(increasing_distances[i - 1], sensitivity);
    double loss_curr = linear_loss(increasing_distances[i], sensitivity);
    EXPECT_GT(loss_curr, loss_prev)
        << "Linear loss should increase with distance";
  }

  // Quadratic loss (positive magnitude) should increase with distance
  for (size_t i = 1; i < increasing_distances.size(); ++i) {
    double loss_prev = quadratic_loss(increasing_distances[i - 1], sensitivity);
    double loss_curr = quadratic_loss(increasing_distances[i], sensitivity);
    EXPECT_GT(loss_curr, loss_prev)
        << "Quadratic loss should increase with distance";
  }

  // Gaussian loss should be monotonically increasing (approaching max_loss)
  for (size_t i = 1; i < increasing_distances.size(); ++i) {
    double loss_prev =
        gaussian_loss(increasing_distances[i - 1], max_loss, steepness);
    double loss_curr =
        gaussian_loss(increasing_distances[i], max_loss, steepness);
    EXPECT_GT(loss_curr, loss_prev)
        << "Gaussian loss should be monotonically increasing";
  }

  // Threshold loss (positive magnitude) should increase with distance (above
  // threshold)
  for (size_t i = 1; i < increasing_distances.size(); ++i) {
    double loss_prev =
        threshold_loss(increasing_distances[i - 1], threshold, sensitivity);
    double loss_curr =
        threshold_loss(increasing_distances[i], threshold, sensitivity);
    EXPECT_GE(loss_curr, loss_prev)
        << "Threshold loss should be non-decreasing with distance";
  }
}

// Test boundary conditions
TEST_F(LossFunctionsTest, BoundaryConditions) {
  // All loss functions should return 0 at distance 0
  EXPECT_DOUBLE_EQ(linear_loss(0.0, sensitivity), 0.0);
  EXPECT_DOUBLE_EQ(quadratic_loss(0.0, sensitivity), 0.0);
  EXPECT_DOUBLE_EQ(gaussian_loss(0.0, max_loss, steepness), 0.0);
  EXPECT_DOUBLE_EQ(threshold_loss(0.0, threshold, sensitivity), 0.0);

  // Test very large distances (all losses positive except at 0)
  double large_distance = 1000.0;
  EXPECT_GT(linear_loss(large_distance, sensitivity), 0.0);
  EXPECT_GT(quadratic_loss(large_distance, sensitivity), 0.0);
  EXPECT_GT(gaussian_loss(large_distance, max_loss, steepness), 0.0);
  EXPECT_GT(threshold_loss(large_distance, threshold, sensitivity), 0.0);

  // Gaussian loss should approach max_loss as distance approaches infinity
  double very_large_distance = 1000000.0;
  double gaussian_large =
      gaussian_loss(very_large_distance, max_loss, steepness);
  EXPECT_NEAR(gaussian_large, max_loss, 1e-6);
}

// Test distance_to_utility conversion
TEST_F(LossFunctionsTest, DistanceToUtilityConversion) {
  // sensitivity=2.0, max_loss=5.0, steepness=1.0, threshold=1.5
  // test_distance=3.0

  // LINEAR: loss = sensitivity * d = 2.0 * 3.0 = 6.0 → utility = -6.0
  double utility_linear =
      distance_to_utility(3.0, LossFunctionType::LINEAR, sensitivity);
  EXPECT_DOUBLE_EQ(utility_linear, -6.0);

  // QUADRATIC: loss = sensitivity * d^2 = 2.0 * 9.0 = 18.0 → utility = -18.0
  double utility_quadratic =
      distance_to_utility(3.0, LossFunctionType::QUADRATIC, sensitivity);
  EXPECT_DOUBLE_EQ(utility_quadratic, -18.0);

  // GAUSSIAN: loss = max_loss * (1 - e^(-steepness * d^2))
  //         = 5.0 * (1 - e^(-1.0 * 9.0)) → utility = -5.0*(1 - e^-9)
  double utility_gaussian = distance_to_utility(
      3.0, LossFunctionType::GAUSSIAN, sensitivity, max_loss, steepness);
  double expected_gaussian = -max_loss * (1.0 - std::exp(-steepness * 9.0));
  EXPECT_NEAR(utility_gaussian, expected_gaussian, 1e-10);
  EXPECT_LT(utility_gaussian, 0.0);

  // THRESHOLD: loss = sensitivity * max(0, d - threshold)
  //          = 2.0 * (3.0 - 1.5) = 3.0 → utility = -3.0
  double utility_threshold =
      distance_to_utility(3.0, LossFunctionType::THRESHOLD, sensitivity,
                          max_loss, steepness, threshold);
  EXPECT_DOUBLE_EQ(utility_threshold, -3.0);

  // At distance 0, all utilities should be 0 (no loss when at the ideal point)
  EXPECT_DOUBLE_EQ(
      distance_to_utility(0.0, LossFunctionType::LINEAR, sensitivity), 0.0);
  EXPECT_DOUBLE_EQ(
      distance_to_utility(0.0, LossFunctionType::QUADRATIC, sensitivity), 0.0);
  EXPECT_NEAR(distance_to_utility(0.0, LossFunctionType::GAUSSIAN, sensitivity,
                                  max_loss, steepness),
              0.0, 1e-15);
  EXPECT_DOUBLE_EQ(
      distance_to_utility(0.0, LossFunctionType::THRESHOLD, sensitivity,
                          max_loss, steepness, threshold),
      0.0);

  // Utility must be negative for positive distances (farther = worse)
  EXPECT_LT(utility_linear, 0.0);
  EXPECT_LT(utility_quadratic, 0.0);
  EXPECT_LT(utility_threshold, 0.0);

  // Unknown enum value must throw, not silently fall back
  EXPECT_THROW(distance_to_utility(3.0, static_cast<LossFunctionType>(999)),
               std::invalid_argument);
}

// Test utility normalization
TEST_F(LossFunctionsTest, UtilityNormalization) {
  double max_distance = 10.0;
  std::vector<double> test_distances = {0.0, 2.0, 5.0, 10.0};

  for (LossFunctionType loss_type :
       {LossFunctionType::LINEAR, LossFunctionType::QUADRATIC,
        LossFunctionType::GAUSSIAN, LossFunctionType::THRESHOLD}) {
    for (double d : test_distances) {
      double raw_utility = distance_to_utility(d, loss_type, sensitivity,
                                               max_loss, steepness, threshold);
      double normalized =
          normalize_utility(raw_utility, max_distance, loss_type, sensitivity,
                            max_loss, steepness, threshold);

      // Normalized utility should be in [0, 1] range
      EXPECT_GE(normalized, 0.0)
          << "Normalized utility should be >= 0 for loss_type "
          << static_cast<int>(loss_type) << " at distance " << d;
      EXPECT_LE(normalized, 1.0)
          << "Normalized utility should be <= 1 for loss_type "
          << static_cast<int>(loss_type) << " at distance " << d;
    }

    // Utility at distance 0 should normalize to 1 (highest utility)
    double utility_at_zero = distance_to_utility(
        0.0, loss_type, sensitivity, max_loss, steepness, threshold);
    double normalized_at_zero =
        normalize_utility(utility_at_zero, max_distance, loss_type, sensitivity,
                          max_loss, steepness, threshold);
    EXPECT_DOUBLE_EQ(normalized_at_zero, 1.0)
        << "Normalized utility at distance 0 should be 1.0 for loss_type "
        << static_cast<int>(loss_type);
  }
}

// Test template instantiation with different numeric types
TEST_F(LossFunctionsTest, TemplateInstantiation) {
  // Test with float (positive loss magnitude)
  float f_distance = 2.5f;
  float f_linear = linear_loss(f_distance, 1.5f);
  float f_quadratic = quadratic_loss(f_distance, 1.5f);
  float f_gaussian = gaussian_loss(f_distance, 3.0f, 0.5f);
  float f_threshold = threshold_loss(f_distance, 1.0f, 1.5f);

  EXPECT_FLOAT_EQ(f_linear, 1.5f * 2.5f);
  EXPECT_FLOAT_EQ(f_quadratic, 1.5f * 2.5f * 2.5f);

  // Test with int (should compile and work)
  int i_distance = 3;
  int i_linear = linear_loss(i_distance, 2);
  EXPECT_EQ(i_linear, 6);  // 2 * 3 (positive loss)
}
