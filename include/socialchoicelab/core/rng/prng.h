// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cmath>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace socialchoicelab::core::rng {

/** Default master seed used when none is specified. */
constexpr uint64_t k_default_master_seed = 12345;

/**
 * @brief Seeded pseudo-random number generator
 *
 * Provides reproducible random number generation with multiple
 * independent streams for different components (voters, candidates, etc.).
 * Uses std::mt19937_64 for high-quality random numbers.
 */
class PRNG {
 public:
  using result_type = std::mt19937_64::result_type;
  using engine_type = std::mt19937_64;

  /**
   * @brief Construct with master seed
   * @param master_seed Master seed for reproducible generation
   */
  explicit PRNG(uint64_t master_seed = k_default_master_seed)
      : master_seed_(master_seed), engine_(master_seed), draw_count_(0) {}

  /**
   * @brief Get master seed
   * @return Master seed value
   */
  uint64_t master_seed() const noexcept { return master_seed_; }

  /**
   * @brief Reset with new master seed
   * @param seed New master seed
   */
  void reset(uint64_t seed) {
    master_seed_ = seed;
    engine_.seed(seed);
    draw_count_ = 0;
  }

  /**
   * @brief Generate uniform random integer in [min, max]
   * @tparam T Integer type
   * @param min Minimum value (inclusive)
   * @param max Maximum value (inclusive)
   * @return Random integer
   */
  template <typename T>
  [[nodiscard]] T uniform_int(T min, T max) {
    static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>,
                  "uniform_int requires an integral type (not bool)");
    if (min > max) {
      throw std::invalid_argument("uniform_int: min must be <= max");
    }
    std::uniform_int_distribution<T> dist(min, max);
    ++draw_count_;
    return dist(engine_);
  }

  /**
   * @brief Generate uniform random real in [min, max)
   * @tparam T Real type (float, double)
   * @param min Minimum value (inclusive)
   * @param max Maximum value (exclusive)
   * @return Random real number
   */
  template <typename T>
  [[nodiscard]] T uniform_real(T min, T max) {
    static_assert(std::is_floating_point_v<T>,
                  "uniform_real requires a floating-point type");
    if constexpr (std::is_floating_point_v<T>) {
      if (!std::isfinite(min) || !std::isfinite(max)) {
        throw std::invalid_argument("uniform_real: min and max must be finite");
      }
    }
    if (!(min < max)) {
      throw std::invalid_argument("uniform_real: min must be < max");
    }
    std::uniform_real_distribution<T> dist(min, max);
    ++draw_count_;
    return dist(engine_);
  }

  /**
   * @brief Generate normal (Gaussian) random number
   * @tparam T Real type (float, double)
   * @param mean Mean of distribution
   * @param stddev Standard deviation
   * @return Random number from normal distribution
   */
  template <typename T>
  [[nodiscard]] T normal(T mean, T stddev) {
    static_assert(std::is_floating_point_v<T>,
                  "normal requires a floating-point type");
    if constexpr (std::is_floating_point_v<T>) {
      if (!std::isfinite(mean) || !std::isfinite(stddev)) {
        throw std::invalid_argument("normal: mean and stddev must be finite");
      }
    }
    if (stddev <= T{0}) {
      throw std::invalid_argument("normal: stddev must be positive");
    }
    std::normal_distribution<T> dist(mean, stddev);
    ++draw_count_;
    return dist(engine_);
  }

  /**
   * @brief Generate exponential random number
   * @tparam T Real type (float, double)
   * @param lambda Rate parameter
   * @return Random number from exponential distribution
   */
  template <typename T>
  [[nodiscard]] T exponential(T lambda) {
    static_assert(std::is_floating_point_v<T>,
                  "exponential requires a floating-point type");
    if constexpr (std::is_floating_point_v<T>) {
      if (!std::isfinite(lambda)) {
        throw std::invalid_argument("exponential: lambda must be finite");
      }
    }
    if (lambda <= T{0}) {
      throw std::invalid_argument("exponential: lambda must be positive");
    }
    std::exponential_distribution<T> dist(lambda);
    ++draw_count_;
    return dist(engine_);
  }

  /**
   * @brief Generate gamma random number
   * @tparam T Real type (float, double)
   * @param alpha Shape parameter
   * @param beta Scale parameter (scale = 1/rate)
   * @return Random number from gamma distribution
   */
  template <typename T>
  [[nodiscard]] T gamma(T alpha, T beta) {
    static_assert(std::is_floating_point_v<T>,
                  "gamma requires a floating-point type");
    if constexpr (std::is_floating_point_v<T>) {
      if (!std::isfinite(alpha) || !std::isfinite(beta)) {
        throw std::invalid_argument("gamma: alpha and beta must be finite");
      }
    }
    if (alpha <= T{0} || beta <= T{0}) {
      throw std::invalid_argument("gamma: alpha and beta must be positive");
    }
    std::gamma_distribution<T> dist(alpha, beta);
    ++draw_count_;
    return dist(engine_);
  }

  /**
   * @brief Generate beta random number
   * @tparam T Real type (float, double)
   * @param alpha First shape parameter (must be > 0)
   * @param beta_param Second shape parameter (must be > 0)
   * @return Random number from beta distribution
   */
  template <typename T>
  [[nodiscard]] T beta(T alpha, T beta_param) {
    static_assert(std::is_floating_point_v<T>,
                  "beta requires a floating-point type");
    if constexpr (std::is_floating_point_v<T>) {
      if (!std::isfinite(alpha) || !std::isfinite(beta_param)) {
        throw std::invalid_argument(
            "beta: alpha and beta_param must be finite");
      }
    }
    if (alpha <= T{0} || beta_param <= T{0}) {
      throw std::invalid_argument(
          "beta: alpha and beta_param must be positive");
    }
    std::gamma_distribution<T> gamma_alpha(alpha, 1.0);
    std::gamma_distribution<T> gamma_beta(beta_param, 1.0);

    T x = gamma_alpha(engine_);
    T y = gamma_beta(engine_);

    T sum = x + y;
    if (sum <= T{0}) {
      throw std::domain_error(
          "beta: both gamma draws were zero or negative (division by zero)");
    }
    ++draw_count_;
    return x / sum;
  }

  /**
   * @brief Generate random boolean
   * @param probability Probability of true (default 0.5)
   * @return Random boolean
   */
  [[nodiscard]] bool bernoulli(double probability = 0.5) {
    if (!std::isfinite(probability)) {
      throw std::invalid_argument("bernoulli: probability must be finite");
    }
    if (probability < 0.0 || probability > 1.0) {
      throw std::invalid_argument("bernoulli: probability must be in [0, 1]");
    }
    std::bernoulli_distribution dist(probability);
    ++draw_count_;
    return dist(engine_);
  }

  /**
   * @brief Generate random choice from discrete distribution
   *
   * @tparam T Weight type (numeric)
   * @param weights Non-empty weight vector. All weights must be ≥ 0; at least
   * one must be > 0. For floating-point T, all weights must be finite.
   * @return Index of chosen element
   * @throws std::invalid_argument if weights is empty, any weight is negative
   * or non-finite, or all weights are zero.
   */
  template <typename T>
  [[nodiscard]] size_t discrete_choice(const std::vector<T>& weights) {
    if (weights.empty()) {
      throw std::invalid_argument("discrete_choice: weights must not be empty");
    }
    for (size_t i = 0; i < weights.size(); ++i) {
      if constexpr (std::is_floating_point_v<T>) {
        if (!std::isfinite(static_cast<double>(weights[i]))) {
          throw std::invalid_argument("discrete_choice: weight[" +
                                      std::to_string(i) + "] is not finite");
        }
      }
      if (weights[i] < T{0}) {
        throw std::invalid_argument("discrete_choice: weight[" +
                                    std::to_string(i) + "] must be >= 0");
      }
    }
    bool any_positive = false;
    for (const auto& w : weights) {
      if (w > T{0}) {
        any_positive = true;
        break;
      }
    }
    if (!any_positive) {
      throw std::invalid_argument(
          "discrete_choice: at least one weight must be > 0");
    }
    std::discrete_distribution<size_t> dist(weights.begin(), weights.end());
    ++draw_count_;
    return dist(engine_);
  }

  /**
   * @brief Generate random choice from uniform distribution
   * @param size Number of choices (must be at least 1)
   * @return Random index in [0, size)
   */
  [[nodiscard]] size_t uniform_choice(size_t size) {
    if (size == 0) {
      throw std::invalid_argument("uniform_choice: size must be at least 1");
    }
    return uniform_int<size_t>(0,
                               size - 1);  // uniform_int increments draw_count_
  }

  /**
   * @brief Skip ahead in the sequence
   * @param steps Number of steps to skip
   */
  void skip(uint64_t steps) { engine_.discard(steps); }

  /**
   * @brief Get current state for debugging
   * @return String representation of current state
   * @throws std::bad_alloc if string allocation fails
   */
  std::string state_string() const {
    return "PRNG(master_seed=" + std::to_string(master_seed_) +
           ", draws=" + std::to_string(draw_count_) + ")";
  }

  /**
   * @brief Get the underlying engine (for advanced use)
   *
   * Internal/test use only. Do not expose through the c_api — bypasses
   * reproducibility guarantees. See consensus plan Item 31 and
   * docs/architecture/design_document.md § c_api design inputs.
   *
   * @return Reference to the random engine
   */
  engine_type& engine() { return engine_; }
  const engine_type& engine() const { return engine_; }

 private:
  uint64_t master_seed_;
  engine_type engine_;
  uint64_t draw_count_;  // Number of draws since construction or last reset
                         // (Item 15)
};

}  // namespace socialchoicelab::core::rng
