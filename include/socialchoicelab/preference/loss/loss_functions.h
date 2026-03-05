// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <Eigen/Dense>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "socialchoicelab/core/numeric_constants.h"

namespace socialchoicelab::preference::loss {

/**
 * @brief Loss function types based on empirical research
 */
enum class LossFunctionType {
  LINEAR,     // Linear loss: u = -α|d| (Singh 2013: empirically superior)
  QUADRATIC,  // Quadratic loss: u = -αd² (traditional spatial voting)
  GAUSSIAN,  // Gaussian (reverse S-shaped): u = α(1 - e^(-βd²)) (Conevska-Mutlu
             // 2025)
  THRESHOLD  // Threshold-based with indifference regions
};

/**
 * @brief Linear loss function
 *
 * Based on Singh (2013) empirical findings that linear loss outperforms
 * quadratic across 20 countries. Linear loss imposes constant penalty on
 * deviations. Returns positive loss magnitude; utility is u = -L(d) (see
 * distance_to_utility).
 *
 * Formula: L(d) = α|d|
 * where d is distance and α is sensitivity parameter
 *
 * @tparam T Numeric type
 * @param distance Distance from ideal point
 * @param sensitivity Sensitivity parameter (α > 0)
 * @return Positive loss magnitude
 */
template <typename T>
T linear_loss(T distance, T sensitivity = T{1.0}) {
  static_assert(std::is_arithmetic_v<T>,
                "Linear loss requires arithmetic type");
  if constexpr (std::is_floating_point_v<T>) {
    if (!std::isfinite(distance))
      throw std::invalid_argument("linear_loss: distance must be finite");
    if (!std::isfinite(sensitivity))
      throw std::invalid_argument("linear_loss: sensitivity must be finite");
  }
  if (sensitivity <= T{0})
    throw std::invalid_argument("linear_loss: sensitivity must be positive");
  return sensitivity * std::abs(distance);
}

/**
 * @brief Quadratic loss function
 *
 * Traditional spatial voting assumption. Imposes increasing penalty on distant
 * alternatives. Returns positive loss magnitude; utility is u = -L(d) (see
 * distance_to_utility).
 *
 * Formula: L(d) = αd²
 * where d is distance and α is sensitivity parameter
 *
 * @tparam T Numeric type
 * @param distance Distance from ideal point
 * @param sensitivity Sensitivity parameter (α > 0)
 * @return Positive loss magnitude
 */
template <typename T>
T quadratic_loss(T distance, T sensitivity = T{1.0}) {
  static_assert(std::is_arithmetic_v<T>,
                "Quadratic loss requires arithmetic type");
  if constexpr (std::is_floating_point_v<T>) {
    if (!std::isfinite(distance))
      throw std::invalid_argument("quadratic_loss: distance must be finite");
    if (!std::isfinite(sensitivity))
      throw std::invalid_argument("quadratic_loss: sensitivity must be finite");
  }
  if (sensitivity <= T{0})
    throw std::invalid_argument("quadratic_loss: sensitivity must be positive");
  return sensitivity * distance * distance;
}

/**
 * @brief Gaussian (reverse S-shaped) loss function
 *
 * Based on Conevska-Mutlu (2025) findings that reverse S-shaped functions
 * better predict real voter behavior than concave functions.
 * Returns positive loss magnitude; utility is u = -L(d) (see
 * distance_to_utility).
 *
 * Formula: L(d) = α(1 - e^(-βd²))
 * where d is distance, α is maximum loss, β controls steepness
 *
 * @tparam T Numeric type
 * @param distance Distance from ideal point
 * @param max_loss Maximum loss value (α > 0)
 * @param steepness Steepness parameter (β > 0)
 * @return Positive loss magnitude
 */
template <typename T>
T gaussian_loss(T distance, T max_loss = T{1.0}, T steepness = T{1.0}) {
  static_assert(std::is_arithmetic_v<T>,
                "Gaussian loss requires arithmetic type");
  if constexpr (std::is_floating_point_v<T>) {
    if (!std::isfinite(distance))
      throw std::invalid_argument("gaussian_loss: distance must be finite");
    if (!std::isfinite(max_loss))
      throw std::invalid_argument("gaussian_loss: max_loss must be finite");
    if (!std::isfinite(steepness))
      throw std::invalid_argument("gaussian_loss: steepness must be finite");
  }
  if (max_loss <= T{0})
    throw std::invalid_argument("gaussian_loss: max_loss must be positive");
  if (steepness <= T{0})
    throw std::invalid_argument("gaussian_loss: steepness must be positive");
  return max_loss * (T{1.0} - std::exp(-steepness * distance * distance));
}

/**
 * @brief Threshold loss function with indifference region
 *
 * Voters are indifferent within threshold distance, then linear loss applies.
 * Returns positive loss magnitude; utility is u = -L(d) (see
 * distance_to_utility).
 *
 * Formula: L(d) = α * max(0, |d| - threshold)
 *
 * @tparam T Numeric type
 * @param distance Distance from ideal point
 * @param threshold Indifference threshold
 * @param sensitivity Sensitivity parameter (α > 0)
 * @return Positive loss magnitude
 */
template <typename T>
T threshold_loss(T distance, T threshold, T sensitivity = T{1.0}) {
  static_assert(std::is_arithmetic_v<T>,
                "Threshold loss requires arithmetic type");
  if constexpr (std::is_floating_point_v<T>) {
    if (!std::isfinite(distance))
      throw std::invalid_argument("threshold_loss: distance must be finite");
    if (!std::isfinite(threshold))
      throw std::invalid_argument("threshold_loss: threshold must be finite");
    if (!std::isfinite(sensitivity))
      throw std::invalid_argument("threshold_loss: sensitivity must be finite");
  }
  if (threshold < T{0})
    throw std::invalid_argument("threshold_loss: threshold must be >= 0");
  if (sensitivity <= T{0})
    throw std::invalid_argument("threshold_loss: sensitivity must be positive");
  return sensitivity * std::max(T{0}, std::abs(distance) - threshold);
}

/**
 * @brief Convert distance to utility using specified loss function
 *
 * @tparam T Numeric type
 * @param distance Distance from ideal point
 * @param loss_type Type of loss function to apply
 * @param sensitivity Sensitivity parameter (α)
 * @param max_loss Maximum loss for Gaussian (α)
 * @param steepness Steepness for Gaussian (β)
 * @param threshold Threshold for threshold loss
 * @return Utility value (negative of loss)
 */
template <typename T>
T distance_to_utility(T distance, LossFunctionType loss_type,
                      T sensitivity = T{1.0}, T max_loss = T{1.0},
                      T steepness = T{1.0}, T threshold = T{0.5}) {
  if constexpr (std::is_floating_point_v<T>) {
    if (!std::isfinite(distance))
      throw std::invalid_argument("distance_to_utility: distance must be finite");
  }

  T loss;

  switch (loss_type) {
    case LossFunctionType::LINEAR:
      loss = linear_loss(distance, sensitivity);
      break;
    case LossFunctionType::QUADRATIC:
      loss = quadratic_loss(distance, sensitivity);
      break;
    case LossFunctionType::GAUSSIAN:
      loss = gaussian_loss(distance, max_loss, steepness);
      break;
    case LossFunctionType::THRESHOLD:
      loss = threshold_loss(distance, threshold, sensitivity);
      break;
    default:
      throw std::invalid_argument("Unknown LossFunctionType");
  }

  return -loss;  // Convert loss to utility
}

/**
 * @brief Normalize utility to [0, 1] range
 *
 * Uses the same loss parameters as were used to produce the raw utility,
 * so normalization is correct for GAUSSIAN and THRESHOLD types.
 *
 * Degenerate case: when the utility range (max_utility - min_utility) is
 * zero or negligible (within relative/absolute tolerance; see
 * socialchoicelab::core::near_zero), the function returns 1.0 so that "all
 * utilities are the same" is interpreted as "all tied for best."
 *
 * @tparam T Numeric type
 * @param utility Raw utility value
 * @param max_distance Maximum possible distance for normalization
 * @param loss_type Loss function type used
 * @param sensitivity Sensitivity parameter (for LINEAR, QUADRATIC, THRESHOLD)
 * @param max_loss Maximum loss for GAUSSIAN
 * @param steepness Steepness for GAUSSIAN
 * @param threshold Threshold for THRESHOLD loss
 * @return Normalized utility in [0, 1]
 */
template <typename T>
T normalize_utility(T utility, T max_distance, LossFunctionType loss_type,
                    T sensitivity = T{1.0}, T max_loss = T{1.0},
                    T steepness = T{1.0}, T threshold = T{0.5}) {
  if constexpr (std::is_floating_point_v<T>) {
    if (!std::isfinite(utility))
      throw std::invalid_argument("normalize_utility: utility must be finite");
    if (!std::isfinite(max_distance))
      throw std::invalid_argument(
          "normalize_utility: max_distance must be finite");
  }
  if (max_distance < T{0})
    throw std::invalid_argument(
        "normalize_utility: max_distance must be >= 0");

  // Calculate maximum possible utility (at distance 0)
  T max_utility = distance_to_utility(T{0}, loss_type, sensitivity, max_loss,
                                      steepness, threshold);

  // Calculate minimum possible utility (at max_distance)
  T min_utility = distance_to_utility(max_distance, loss_type, sensitivity,
                                      max_loss, steepness, threshold);

  // Normalize to [0, 1]; use tolerance so degenerate range is robust (Item 28)
  if constexpr (std::is_floating_point_v<T>) {
    T range = max_utility - min_utility;
    T scale = std::max({std::abs(max_utility), std::abs(min_utility), T{1}});
    T tol = std::max(static_cast<T>(socialchoicelab::core::near_zero::k_near_zero_rel) * scale,
                     static_cast<T>(socialchoicelab::core::near_zero::k_near_zero_abs));
    if (std::abs(range) <= tol) {
      return T{1.0};  // All utilities the same → treat as best
    }
  } else {
    if (max_utility == min_utility) {
      return T{1.0};
    }
  }

  return (utility - min_utility) / (max_utility - min_utility);
}

}  // namespace socialchoicelab::preference::loss
