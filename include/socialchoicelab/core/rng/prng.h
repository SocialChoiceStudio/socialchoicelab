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
      : master_seed_(master_seed), engine_(master_seed) {}

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
  }

  /**
   * @brief Generate uniform random integer in [min, max]
   * @tparam T Integer type
   * @param min Minimum value (inclusive)
   * @param max Maximum value (inclusive)
   * @return Random integer
   */
  template <typename T>
  T uniform_int(T min, T max) {
    if (min > max) {
      throw std::invalid_argument("uniform_int: min must be <= max");
    }
    std::uniform_int_distribution<T> dist(min, max);
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
  T uniform_real(T min, T max) {
    if constexpr (std::is_floating_point_v<T>) {
      if (!std::isfinite(min) || !std::isfinite(max)) {
        throw std::invalid_argument("uniform_real: min and max must be finite");
      }
    }
    if (!(min < max)) {
      throw std::invalid_argument("uniform_real: min must be < max");
    }
    std::uniform_real_distribution<T> dist(min, max);
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
  T normal(T mean, T stddev) {
    if constexpr (std::is_floating_point_v<T>) {
      if (!std::isfinite(mean) || !std::isfinite(stddev)) {
        throw std::invalid_argument("normal: mean and stddev must be finite");
      }
    }
    if (stddev <= T{0}) {
      throw std::invalid_argument("normal: stddev must be positive");
    }
    std::normal_distribution<T> dist(mean, stddev);
    return dist(engine_);
  }

  /**
   * @brief Generate exponential random number
   * @tparam T Real type (float, double)
   * @param lambda Rate parameter
   * @return Random number from exponential distribution
   */
  template <typename T>
  T exponential(T lambda) {
    if constexpr (std::is_floating_point_v<T>) {
      if (!std::isfinite(lambda)) {
        throw std::invalid_argument("exponential: lambda must be finite");
      }
    }
    if (lambda <= T{0}) {
      throw std::invalid_argument("exponential: lambda must be positive");
    }
    std::exponential_distribution<T> dist(lambda);
    return dist(engine_);
  }

  /**
   * @brief Generate gamma random number
   * @tparam T Real type (float, double)
   * @param alpha Shape parameter
   * @param beta Rate parameter
   * @return Random number from gamma distribution
   */
  template <typename T>
  T gamma(T alpha, T beta) {
    if constexpr (std::is_floating_point_v<T>) {
      if (!std::isfinite(alpha) || !std::isfinite(beta)) {
        throw std::invalid_argument("gamma: alpha and beta must be finite");
      }
    }
    if (alpha <= T{0} || beta <= T{0}) {
      throw std::invalid_argument("gamma: alpha and beta must be positive");
    }
    std::gamma_distribution<T> dist(alpha, beta);
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
  T beta(T alpha, T beta_param) {
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
    return x / sum;
  }

  /**
   * @brief Generate random boolean
   * @param probability Probability of true (default 0.5)
   * @return Random boolean
   */
  bool bernoulli(double probability = 0.5) {
    if (!std::isfinite(probability)) {
      throw std::invalid_argument("bernoulli: probability must be finite");
    }
    if (probability < 0.0 || probability > 1.0) {
      throw std::invalid_argument("bernoulli: probability must be in [0, 1]");
    }
    std::bernoulli_distribution dist(probability);
    return dist(engine_);
  }

  /**
   * @brief Generate random choice from discrete distribution
   * @tparam T Value type
   * @param weights Weights for each choice (must not be empty)
   * @return Index of chosen element
   */
  template <typename T>
  size_t discrete_choice(const std::vector<T>& weights) {
    if (weights.empty()) {
      throw std::invalid_argument("discrete_choice: weights must not be empty");
    }
    std::discrete_distribution<size_t> dist(weights.begin(), weights.end());
    return dist(engine_);
  }

  /**
   * @brief Generate random choice from uniform distribution
   * @param size Number of choices (must be at least 1)
   * @return Random index in [0, size)
   */
  size_t uniform_choice(size_t size) {
    if (size == 0) {
      throw std::invalid_argument("uniform_choice: size must be at least 1");
    }
    return uniform_int<size_t>(0, size - 1);
  }

  /**
   * @brief Skip ahead in the sequence
   * @param steps Number of steps to skip
   */
  void skip(uint64_t steps) { engine_.discard(steps); }

  /**
   * @brief Get current state for debugging
   * @return String representation of current state
   */
  std::string state_string() const {
    return "PRNG(master_seed=" + std::to_string(master_seed_) + ")";
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
};

}  // namespace socialchoicelab::core::rng
