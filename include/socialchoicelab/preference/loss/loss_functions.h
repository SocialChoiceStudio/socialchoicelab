// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <type_traits>

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
[[nodiscard]] T linear_loss(T distance, T sensitivity = T{1.0}) {
  static_assert(std::is_floating_point_v<T>,
                "Linear loss requires a floating-point type");
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
[[nodiscard]] T quadratic_loss(T distance, T sensitivity = T{1.0}) {
  static_assert(std::is_floating_point_v<T>,
                "Quadratic loss requires a floating-point type");
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
[[nodiscard]] T gaussian_loss(T distance, T max_loss = T{1.0},
                              T steepness = T{1.0}) {
  static_assert(std::is_floating_point_v<T>,
                "gaussian_loss requires a floating-point type");
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
[[nodiscard]] T threshold_loss(T distance, T threshold,
                               T sensitivity = T{1.0}) {
  static_assert(std::is_floating_point_v<T>,
                "Threshold loss requires a floating-point type");
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
 * @note For THRESHOLD loss, always pass @p threshold explicitly; the default
 * 0.5 may not match your model.
 *
 * Parameter usage by loss type (unlisted parameters are ignored):
 * | Loss type  | Used parameters   | Ignored parameters              |
 * |------------|-------------------|---------------------------------|
 * | LINEAR     | sensitivity       | max_loss, steepness, threshold  |
 * | QUADRATIC  | sensitivity       | max_loss, steepness, threshold  |
 * | GAUSSIAN   | max_loss, steepness | sensitivity, threshold        |
 * | THRESHOLD  | threshold, sensitivity | max_loss, steepness         |
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
[[nodiscard]] T distance_to_utility(T distance, LossFunctionType loss_type,
                                    T sensitivity = T{1.0}, T max_loss = T{1.0},
                                    T steepness = T{1.0},
                                    T threshold = T{0.5}) {
  if constexpr (std::is_floating_point_v<T>) {
    if (!std::isfinite(distance))
      throw std::invalid_argument(
          "distance_to_utility: distance must be finite");
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
 * Parameter usage by loss type (same as distance_to_utility; unlisted are
 * ignored): LINEAR/QUADRATIC → sensitivity; GAUSSIAN → max_loss, steepness;
 * THRESHOLD → threshold, sensitivity.
 *
 * For integer @p T, the result is 0 or 1 (binary clipping); continuous
 * [0,1] normalization is meaningful only for floating-point types.
 *
 * Callers must provide consistent inputs: @p utility should be within the
 * range implied by the loss (e.g. utility ≤ max possible utility for the
 * given @p max_distance). Behavior is algebraically correct but output may
 * fall outside [0,1] if inputs are inconsistent.
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
 * @return Normalized utility in [0, 1] (or 0/1 for integer T)
 */
template <typename T>
[[nodiscard]] T normalize_utility(T utility, T max_distance,
                                  LossFunctionType loss_type,
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
    throw std::invalid_argument("normalize_utility: max_distance must be >= 0");

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
    T tol = std::max(
        static_cast<T>(socialchoicelab::core::near_zero::k_near_zero_rel) *
            scale,
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

/**
 * @brief Precomputes utility bounds once for batch normalization (Item 12)
 *
 * Use when normalizing many utilities with the same loss and max_distance:
 * construct once with the loss parameters, then call normalize(utility) for
 * each value. Avoids recomputing max_utility and min_utility on every call.
 * Aligns with the planned c_api SCS_LossConfig (Item 29, third review).
 *
 * Parameter usage by loss type: same as distance_to_utility (LINEAR/QUADRATIC
 * → sensitivity; GAUSSIAN → max_loss, steepness; THRESHOLD → threshold,
 * sensitivity).
 */
template <typename T>
class UtilityNormalizer {
 public:
  /**
   * @brief Construct and precompute bounds for the given loss configuration
   * @param max_distance Maximum possible distance (same meaning as
   * normalize_utility)
   * @param loss_type Loss function type
   * @param sensitivity Sensitivity (α) for LINEAR, QUADRATIC, THRESHOLD
   * @param max_loss Maximum loss for GAUSSIAN
   * @param steepness Steepness for GAUSSIAN
   * @param threshold Threshold for THRESHOLD
   */
  UtilityNormalizer(T max_distance, LossFunctionType loss_type,
                    T sensitivity = T{1.0}, T max_loss = T{1.0},
                    T steepness = T{1.0}, T threshold = T{0.5}) {
    if (max_distance < T{0})
      throw std::invalid_argument(
          "UtilityNormalizer: max_distance must be >= 0");
    max_utility_ = distance_to_utility(T{0}, loss_type, sensitivity, max_loss,
                                       steepness, threshold);
    min_utility_ = distance_to_utility(max_distance, loss_type, sensitivity,
                                       max_loss, steepness, threshold);
  }

  /**
   * @brief Normalize a single utility to [0, 1] using precomputed bounds
   * @param utility Raw utility value (must be finite for floating-point T)
   * @return Normalized utility in [0, 1] (or 0/1 for integer T)
   */
  [[nodiscard]] T normalize(T utility) const {
    if constexpr (std::is_floating_point_v<T>) {
      if (!std::isfinite(utility))
        throw std::invalid_argument(
            "UtilityNormalizer::normalize: utility must be finite");
    }
    if constexpr (std::is_floating_point_v<T>) {
      T range = max_utility_ - min_utility_;
      T scale =
          std::max({std::abs(max_utility_), std::abs(min_utility_), T{1}});
      T tol = std::max(
          static_cast<T>(socialchoicelab::core::near_zero::k_near_zero_rel) *
              scale,
          static_cast<T>(socialchoicelab::core::near_zero::k_near_zero_abs));
      if (std::abs(range) <= tol) return T{1.0};
    } else {
      if (max_utility_ == min_utility_) return T{1.0};
    }
    return (utility - min_utility_) / (max_utility_ - min_utility_);
  }

 private:
  T max_utility_;
  T min_utility_;
};

}  // namespace socialchoicelab::preference::loss
