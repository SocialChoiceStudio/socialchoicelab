// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <Eigen/Dense>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace socialchoicelab::preference::distance {

/**
 * @brief Distance function types
 */
enum class DistanceType {
  EUCLIDEAN,  // p = 2 (L2 norm)
  MANHATTAN,  // p = 1 (L1 norm)
  CHEBYSHEV,  // p = ∞ (L∞ norm)
  MINKOWSKI   // General p (Lp norm)
};

/** Above this order_p, Minkowski is approximated by Chebyshev (L∞) to avoid
 * overflow in (Σ term^p)^(1/p). Heuristic; revisit if you need a formal bound.
 */
constexpr double k_minkowski_chebyshev_cutoff = 100.0;

// Forward declaration so minkowski_distance can call chebyshev_distance.
template <typename Derived1, typename Derived2, typename T>
T chebyshev_distance(const Eigen::MatrixBase<Derived1>& ideal_point,
                     const Eigen::MatrixBase<Derived2>& alternative_point,
                     const std::vector<T>& salience_weights);

/**
 * @brief Minkowski distance with salience weights (Convention B)
 *
 * Implements weighted Minkowski distance using **Convention B** (dimension
 * pre-scaling): each dimension's difference is scaled by its weight *before*
 * exponentiation.
 *
 * @code
 * d = ( Σ (w_i * |x_i - y_i|)^p )^(1/p)
 * @endcode
 *
 * Where:
 * - w_i is the salience weight for dimension i
 * - p is the Minkowski order parameter
 * - x_i, y_i are the coordinates in dimension i
 *
 * **Convention B vs Convention A:** Convention A would weight outside the
 * exponent: `d = (Σ w_i * |x_i - y_i|^p)^(1/p)`. This function uses
 * Convention B throughout. Doubling w_i has the same effect as halving the
 * unit length in dimension i. See docs/architecture/design_document.md for
 * rationale.
 *
 * @tparam Derived1 Eigen expression type for ideal point
 * @tparam Derived2 Eigen expression type for alternative point
 * @tparam T Numeric type (float, double, etc.)
 * @param ideal_point Voter's ideal point
 * @param alternative_point Alternative/candidate position
 * @param order_p Minkowski order parameter (p ≥ 1, must be finite)
 * @param salience_weights Salience weights for each dimension (mandatory, one
 * per dimension). Must be finite and ≥ 0; zero means that dimension is ignored
 * (dimension masking).
 * @return Weighted Minkowski distance
 */
template <typename Derived1, typename Derived2, typename T>
T minkowski_distance(const Eigen::MatrixBase<Derived1>& ideal_point,
                     const Eigen::MatrixBase<Derived2>& alternative_point,
                     T order_p, const std::vector<T>& salience_weights) {
  static_assert(std::is_floating_point_v<T>,
                "minkowski_distance requires a floating-point type");

  const Eigen::Index N = ideal_point.size();
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

  if constexpr (std::is_floating_point_v<T>) {
    if (!std::isfinite(order_p)) {
      throw std::invalid_argument("order_p must be finite");
    }
  }

  for (Eigen::Index i = 0; i < N; ++i) {
    if (!std::isfinite(static_cast<double>(ideal_point[i]))) {
      throw std::invalid_argument("ideal_point[" + std::to_string(i) +
                                  "] is not finite");
    }
    if (!std::isfinite(static_cast<double>(alternative_point[i]))) {
      throw std::invalid_argument("alternative_point[" + std::to_string(i) +
                                  "] is not finite");
    }
    if constexpr (std::is_floating_point_v<T>) {
      if (!std::isfinite(salience_weights[i])) {
        throw std::invalid_argument("salience_weights[" + std::to_string(i) +
                                    "] is not finite");
      }
    }
    if (salience_weights[i] < T{0}) {
      throw std::invalid_argument("salience_weights[" + std::to_string(i) +
                                  "] must be >= 0");
    }
  }

  const T eps = static_cast<T>(1e-9);

  // p=1: Manhattan (sum of absolute weighted differences), avoid std::pow
  if (std::abs(order_p - T{1}) < eps) {
    T sum = T{0};
    for (Eigen::Index i = 0; i < N; ++i) {
      sum +=
          salience_weights[i] * std::abs(ideal_point[i] - alternative_point[i]);
    }
    return sum;
  }

  // p=2: Euclidean (sqrt of sum of squared weighted differences), avoid
  // std::pow
  if (std::abs(order_p - T{2}) < eps) {
    T sum_sq = T{0};
    for (Eigen::Index i = 0; i < N; ++i) {
      T wd =
          salience_weights[i] * std::abs(ideal_point[i] - alternative_point[i]);
      sum_sq += wd * wd;
    }
    return std::sqrt(sum_sq);
  }

  // For very large p, approximate by Chebyshev (L∞) to avoid overflow
  if (order_p > static_cast<T>(k_minkowski_chebyshev_cutoff)) {
    return chebyshev_distance(ideal_point, alternative_point, salience_weights);
  }

  // Calculate weighted Minkowski distance using raw C++ math
  T sum = T{0};
  for (Eigen::Index i = 0; i < N; ++i) {
    T weighted_diff =
        salience_weights[i] * std::abs(ideal_point[i] - alternative_point[i]);
    sum += std::pow(weighted_diff, order_p);
  }
  return std::pow(sum, T{1.0} / order_p);
}

/**
 * @brief Euclidean distance (Minkowski with p=2)
 *
 * @tparam Derived1 Eigen expression type for ideal point
 * @tparam Derived2 Eigen expression type for alternative point
 * @tparam T Numeric type
 * @param ideal_point Voter's ideal point
 * @param alternative_point Alternative position
 * @param salience_weights Salience weights for each dimension (mandatory, one
 * per dimension). Must be finite and ≥ 0; zero means that dimension is ignored
 * (dimension masking).
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
 * @tparam Derived1 Eigen expression type for ideal point
 * @tparam Derived2 Eigen expression type for alternative point
 * @tparam T Numeric type
 * @param ideal_point Voter's ideal point
 * @param alternative_point Alternative position
 * @param salience_weights Salience weights for each dimension (mandatory, one
 * per dimension). Must be finite and ≥ 0; zero means that dimension is ignored
 * (dimension masking).
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
 * @tparam Derived1 Eigen expression type for ideal point
 * @tparam Derived2 Eigen expression type for alternative point
 * @tparam T Numeric type
 * @param ideal_point Voter's ideal point
 * @param alternative_point Alternative position
 * @param salience_weights Salience weights for each dimension (mandatory, one
 * per dimension). Must be finite and ≥ 0; zero means that dimension is ignored
 * (dimension masking).
 * @return Chebyshev distance
 */
template <typename Derived1, typename Derived2, typename T>
T chebyshev_distance(const Eigen::MatrixBase<Derived1>& ideal_point,
                     const Eigen::MatrixBase<Derived2>& alternative_point,
                     const std::vector<T>& salience_weights) {
  const Eigen::Index N = ideal_point.size();
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

  for (Eigen::Index i = 0; i < N; ++i) {
    if (!std::isfinite(static_cast<double>(ideal_point[i]))) {
      throw std::invalid_argument("ideal_point[" + std::to_string(i) +
                                  "] is not finite");
    }
    if (!std::isfinite(static_cast<double>(alternative_point[i]))) {
      throw std::invalid_argument("alternative_point[" + std::to_string(i) +
                                  "] is not finite");
    }
    if constexpr (std::is_floating_point_v<T>) {
      if (!std::isfinite(salience_weights[i])) {
        throw std::invalid_argument("salience_weights[" + std::to_string(i) +
                                    "] is not finite");
      }
    }
    if (salience_weights[i] < T{0}) {
      throw std::invalid_argument("salience_weights[" + std::to_string(i) +
                                  "] must be >= 0");
    }
  }

  T max_weighted_diff = T{0};
  for (Eigen::Index i = 0; i < N; ++i) {
    T weighted_diff =
        salience_weights[i] * std::abs(ideal_point[i] - alternative_point[i]);
    max_weighted_diff = std::max(max_weighted_diff, weighted_diff);
  }

  return max_weighted_diff;
}

/**
 * @brief Distance function factory
 *
 * @tparam Derived1 Eigen expression type for ideal point
 * @tparam Derived2 Eigen expression type for alternative point
 * @tparam T Numeric type
 * @param ideal_point Voter's ideal point
 * @param alternative_point Alternative position
 * @param distance_type Type of distance to calculate
 * @param order_p Minkowski order (only used for MINKOWSKI type; must be finite
 * and ≥ 1)
 * @param salience_weights Salience weights for each dimension (mandatory). Must
 * be finite and ≥ 0; zero means that dimension is ignored (dimension masking).
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

}  // namespace socialchoicelab::preference::distance
