// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <Eigen/Dense>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace socialchoicelab {
namespace preference {
namespace distance {

/**
 * @brief Distance function types
 */
enum class DistanceType {
  EUCLIDEAN,  // p = 2 (L2 norm)
  MANHATTAN,  // p = 1 (L1 norm)
  CHEBYSHEV,  // p = ∞ (L∞ norm)
  MINKOWSKI   // General p (Lp norm)
};

/**
 * @brief Minkowski distance with salience weights
 *
 * Implements weighted Minkowski distance:
 * d = (Σ w_i * |x_i - y_i|^p)^(1/p)
 *
 * Where:
 * - w_i is the salience weight for dimension i
 * - p is the Minkowski order parameter
 * - x_i, y_i are the coordinates in dimension i
 *
 * @tparam T Numeric type (float, double, etc.)
 * @tparam N Dimension of the points
 * @param ideal_point Voter's ideal point
 * @param alternative_point Alternative/candidate position
 * @param order_p Minkowski order parameter (p ≥ 1)
 * @param salience_weights Importance weights for each dimension (default: equal
 * weights)
 * @return Weighted Minkowski distance
 */
template <typename Derived1, typename Derived2, typename T>
T minkowski_distance(const Eigen::MatrixBase<Derived1>& ideal_point,
                     const Eigen::MatrixBase<Derived2>& alternative_point,
                     T order_p, const std::vector<T>& salience_weights) {
  static_assert(std::is_arithmetic_v<T>,
                "Minkowski distance requires arithmetic type");

  const int N = ideal_point.size();
  if (N != alternative_point.size()) {
    throw std::invalid_argument(
        "Ideal point and alternative point must have same dimension");
  }

  // Weighted Minkowski distance requires a salience vector equal to the
  // dimensionality of the space
  if (salience_weights.empty()) {
    throw std::invalid_argument(
        "Weighted Minkowski distance requires a salience vector equal to the "
        "dimensionality of the space");
  }

  if (salience_weights.size() != N) {
    throw std::invalid_argument(
        "Weighted Minkowski distance requires a salience vector equal to the "
        "dimensionality of the space");
  }

  if (order_p < T{1}) {
    throw std::invalid_argument("Minkowski order_p must be >= 1");
  }

  // Safety check: for very large orders, use Chebyshev distance to avoid
  // overflow
  if (order_p > T{100.0}) {
    // For large p, Minkowski distance approaches Chebyshev distance (L∞ norm)
    T max_weighted_diff = T{0};
    for (int i = 0; i < N; ++i) {
      T weighted_diff =
          salience_weights[i] * std::abs(ideal_point[i] - alternative_point[i]);
      max_weighted_diff = std::max(max_weighted_diff, weighted_diff);
    }
    return max_weighted_diff;
  }

  // Calculate weighted Minkowski distance using raw C++ math
  T sum = T{0};
  for (int i = 0; i < N; ++i) {
    T weighted_diff =
        salience_weights[i] * std::abs(ideal_point[i] - alternative_point[i]);
    sum += std::pow(weighted_diff, order_p);
  }
  return std::pow(sum, T{1.0} / order_p);
}

/**
 * @brief Euclidean distance (Minkowski with p=2)
 *
 * @tparam T Numeric type
 * @tparam N Dimension
 * @param ideal_point Voter's ideal point
 * @param alternative_point Alternative position
 * @param salience_weights Optional salience weights (default: equal)
 * @return Euclidean distance
 */
template <typename Derived1, typename Derived2, typename T>
T euclidean_distance(const Eigen::MatrixBase<Derived1>& ideal_point,
                     const Eigen::MatrixBase<Derived2>& alternative_point,
                     const std::vector<T>& salience_weights) {
  return minkowski_distance(ideal_point, alternative_point, T{2.0},
                            salience_weights);
}

/**
 * @brief Manhattan distance (Minkowski with p=1)
 *
 * @tparam T Numeric type
 * @tparam N Dimension
 * @param ideal_point Voter's ideal point
 * @param alternative_point Alternative position
 * @param salience_weights Optional salience weights (default: equal)
 * @return Manhattan distance
 */
template <typename Derived1, typename Derived2, typename T>
T manhattan_distance(const Eigen::MatrixBase<Derived1>& ideal_point,
                     const Eigen::MatrixBase<Derived2>& alternative_point,
                     const std::vector<T>& salience_weights) {
  return minkowski_distance(ideal_point, alternative_point, T{1.0},
                            salience_weights);
}

/**
 * @brief Chebyshev distance (Minkowski with p=∞)
 *
 * @tparam T Numeric type
 * @tparam N Dimension
 * @param ideal_point Voter's ideal point
 * @param alternative_point Alternative position
 * @param salience_weights Optional salience weights (default: equal)
 * @return Chebyshev distance
 */
template <typename Derived1, typename Derived2, typename T>
T chebyshev_distance(const Eigen::MatrixBase<Derived1>& ideal_point,
                     const Eigen::MatrixBase<Derived2>& alternative_point,
                     const std::vector<T>& salience_weights) {
  const int N = ideal_point.size();
  if (N != alternative_point.size()) {
    throw std::invalid_argument(
        "Ideal point and alternative point must have same dimension");
  }

  // Weighted Chebyshev distance requires a salience vector equal to the
  // dimensionality of the space
  if (salience_weights.empty()) {
    throw std::invalid_argument(
        "Weighted Chebyshev distance requires a salience vector equal to the "
        "dimensionality of the space");
  }

  if (salience_weights.size() != N) {
    throw std::invalid_argument(
        "Weighted Chebyshev distance requires a salience vector equal to the "
        "dimensionality of the space");
  }

  // For Chebyshev distance, we need to handle p=∞ specially

  T max_weighted_diff = T{0};
  for (int i = 0; i < N; ++i) {
    T weighted_diff =
        salience_weights[i] * std::abs(ideal_point[i] - alternative_point[i]);
    max_weighted_diff = std::max(max_weighted_diff, weighted_diff);
  }

  return max_weighted_diff;
}

/**
 * @brief Distance function factory
 *
 * @tparam T Numeric type
 * @tparam N Dimension
 * @param ideal_point Voter's ideal point
 * @param alternative_point Alternative position
 * @param distance_type Type of distance to calculate
 * @param order_p Minkowski order (only used for MINKOWSKI type)
 * @param salience_weights Optional salience weights
 * @return Calculated distance
 */
template <typename Derived1, typename Derived2, typename T>
T calculate_distance(const Eigen::MatrixBase<Derived1>& ideal_point,
                     const Eigen::MatrixBase<Derived2>& alternative_point,
                     DistanceType distance_type, T order_p,
                     const std::vector<T>& salience_weights) {
  switch (distance_type) {
    case DistanceType::EUCLIDEAN:
      return euclidean_distance(ideal_point, alternative_point,
                                salience_weights);
    case DistanceType::MANHATTAN:
      return manhattan_distance(ideal_point, alternative_point,
                                salience_weights);
    case DistanceType::CHEBYSHEV:
      return chebyshev_distance(ideal_point, alternative_point,
                                salience_weights);
    case DistanceType::MINKOWSKI:
      return minkowski_distance(ideal_point, alternative_point, order_p,
                                salience_weights);
    default:
      throw std::invalid_argument("Unknown DistanceType");
  }
}

// Type aliases for common cases
// Note: Template functions don't work well with decltype, so we'll use function
// pointers instead using Distance2D = decltype(minkowski_distance<double>);
// using Distance3D = decltype(minkowski_distance<double>);

}  // namespace distance
}  // namespace preference
}  // namespace socialchoicelab
