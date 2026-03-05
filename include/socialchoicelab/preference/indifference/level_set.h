// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cmath>
#include <numbers>
#include <stdexcept>
#include <variant>
#include <vector>

#include "socialchoicelab/core/numeric_constants.h"
#include "socialchoicelab/core/types.h"
#include "socialchoicelab/preference/distance/distance_functions.h"
#include "socialchoicelab/preference/loss/loss_functions.h"

namespace socialchoicelab::preference::indifference {

using socialchoicelab::core::types::Point2d;
using socialchoicelab::core::types::PointNd;

// --- Config structs (match distance_to_utility and calculate_distance) ---

/**
 * @brief Loss function configuration for level-set computation.
 * Defaults match preference::loss::distance_to_utility.
 */
struct LossConfig {
  loss::LossFunctionType loss_type = loss::LossFunctionType::QUADRATIC;
  double sensitivity = 1.0;
  double max_loss = 1.0;
  double steepness = 1.0;
  double threshold = 0.5;
};

/**
 * @brief Distance configuration for level-set computation.
 * salience_weights.size() must equal ideal point dimension.
 */
struct DistanceConfig {
  distance::DistanceType distance_type = distance::DistanceType::EUCLIDEAN;
  double order_p = 2.0;
  std::vector<double> salience_weights;
};

// --- 2D shape types (exact representations when possible) ---

/** @brief Circle: center and radius. Euclidean, equal weights. */
struct Circle2d {
  Point2d center;
  double radius;
};

/** @brief Axis-aligned ellipse: center and semi-axes. Euclidean, unequal
 * weights. */
struct Ellipse2d {
  Point2d center;
  double semi_axis_0;  // along first coordinate
  double semi_axis_1;  // along second coordinate
};

/** @brief Axis-aligned superellipse (Lamé): center, semi-axes, exponent p.
 * Minkowski 1<p<∞. */
struct Superellipse2d {
  Point2d center;
  double semi_axis_0;
  double semi_axis_1;
  double exponent_p;
};

/** @brief Closed polygon: ordered vertices (first not repeated at end). */
struct Polygon2d {
  std::vector<Point2d> vertices;
};

/** @brief Level set in 2D: one of circle, ellipse, superellipse, or polygon. */
using LevelSet2d = std::variant<Circle2d, Ellipse2d, Superellipse2d, Polygon2d>;

namespace detail {

/** @brief Returns distance r such that distance_to_utility(r, ...) == level.
 * Throws if level unreachable. */
[[nodiscard]] inline double distance_from_utility_level(double level,
                                                        const LossConfig& cfg) {
  using namespace socialchoicelab::core::near_zero;
  if (!std::isfinite(level)) {
    throw std::invalid_argument("level_set: utility level must be finite");
  }
  switch (cfg.loss_type) {
    case loss::LossFunctionType::LINEAR: {
      if (level > 0) {
        throw std::invalid_argument(
            "level_set: utility level exceeds maximum (0); no finite distance");
      }
      if (cfg.sensitivity <= 0 || !std::isfinite(cfg.sensitivity)) {
        throw std::invalid_argument(
            "level_set: LossConfig sensitivity must be positive and finite");
      }
      double loss = -level;
      return loss / cfg.sensitivity;
    }
    case loss::LossFunctionType::QUADRATIC: {
      if (level > 0) {
        throw std::invalid_argument(
            "level_set: utility level exceeds maximum (0); no finite distance");
      }
      if (cfg.sensitivity <= 0 || !std::isfinite(cfg.sensitivity)) {
        throw std::invalid_argument(
            "level_set: LossConfig sensitivity must be positive and finite");
      }
      double loss = -level;
      if (loss < 0) return 0;
      return std::sqrt(loss / cfg.sensitivity);
    }
    case loss::LossFunctionType::GAUSSIAN: {
      if (level > 0) {
        throw std::invalid_argument(
            "level_set: utility level exceeds maximum (0); no finite distance");
      }
      if (level <= -cfg.max_loss) {
        throw std::invalid_argument(
            "level_set: utility level at or below minimum for Gaussian; no "
            "finite distance");
      }
      if (cfg.max_loss <= 0 || cfg.steepness <= 0 ||
          !std::isfinite(cfg.max_loss) || !std::isfinite(cfg.steepness)) {
        throw std::invalid_argument(
            "level_set: LossConfig max_loss and steepness must be positive and "
            "finite");
      }
      double t = 1.0 + level / cfg.max_loss;
      if (t <= 0) return 0;
      double d_sq = -std::log(t) / cfg.steepness;
      if (d_sq <= 0) return 0;
      return std::sqrt(d_sq);
    }
    case loss::LossFunctionType::THRESHOLD: {
      if (level > 0) {
        throw std::invalid_argument(
            "level_set: utility level exceeds maximum (0); no finite distance");
      }
      if (cfg.sensitivity <= 0 || !std::isfinite(cfg.sensitivity) ||
          cfg.threshold < 0 || !std::isfinite(cfg.threshold)) {
        throw std::invalid_argument(
            "level_set: LossConfig threshold >= 0 and sensitivity positive and "
            "finite");
      }
      if (level >= 0) {
        return cfg.threshold;
      }
      double loss = -level;
      return cfg.threshold + loss / cfg.sensitivity;
    }
    default:
      throw std::invalid_argument("level_set: unknown LossFunctionType");
  }
}

inline bool equal_weights_2d(const std::vector<double>& w) {
  if (w.size() != 2u) return false;
  const double scale = std::max(std::abs(w[0]), std::abs(w[1]));
  const double tol =
      std::max(socialchoicelab::core::near_zero::k_near_zero_abs,
               socialchoicelab::core::near_zero::k_near_zero_rel * scale);
  return std::abs(w[0] - w[1]) <= tol;
}

}  // namespace detail

// --- Entry points ---

/**
 * @brief Level set in 1D: points where u(x) = level.
 * @return 0, 1, or 2 points; empty if level unreachable.
 */
[[nodiscard]] inline std::vector<PointNd> level_set_1d(
    const PointNd& ideal_point, double utility_level,
    const LossConfig& loss_config, const DistanceConfig& distance_config) {
  if (ideal_point.size() != 1) {
    throw std::invalid_argument(
        "level_set_1d: ideal_point must have dimension 1");
  }
  if (distance_config.salience_weights.size() != 1u) {
    throw std::invalid_argument(
        "level_set_1d: salience_weights must have size 1");
  }
  double r;
  try {
    r = detail::distance_from_utility_level(utility_level, loss_config);
  } catch (const std::invalid_argument&) {
    return {};
  }
  const double w = distance_config.salience_weights[0];
  if (w <= 0 || !std::isfinite(w)) {
    throw std::invalid_argument(
        "level_set_1d: salience weight must be positive and finite");
  }
  if (r <= 0) {
    PointNd one(1);
    one(0) = ideal_point(0);
    return {one};
  }
  const double half = r / w;
  PointNd a(1), b(1);
  a(0) = ideal_point(0) - half;
  b(0) = ideal_point(0) + half;
  return {a, b};
}

/**
 * @brief Level set in 2D as an exact shape (circle, ellipse, superellipse, or
 * polygon).
 */
[[nodiscard]] inline LevelSet2d level_set_2d(
    const Point2d& ideal_point, double utility_level,
    const LossConfig& loss_config, const DistanceConfig& distance_config) {
  if (distance_config.salience_weights.size() != 2u) {
    throw std::invalid_argument(
        "level_set_2d: salience_weights must have size 2");
  }
  const double r =
      detail::distance_from_utility_level(utility_level, loss_config);
  const double w0 = distance_config.salience_weights[0];
  const double w1 = distance_config.salience_weights[1];
  if (w0 <= 0 || w1 <= 0 || !std::isfinite(w0) || !std::isfinite(w1)) {
    throw std::invalid_argument(
        "level_set_2d: salience weights must be positive and finite");
  }
  const double a0 = r / w0;
  const double a1 = r / w1;

  switch (distance_config.distance_type) {
    case distance::DistanceType::EUCLIDEAN:
      if (detail::equal_weights_2d(distance_config.salience_weights)) {
        return Circle2d{ideal_point, r};
      }
      return Ellipse2d{ideal_point, a0, a1};

    case distance::DistanceType::MANHATTAN: {
      Polygon2d poly;
      poly.vertices.resize(4);
      poly.vertices[0] = ideal_point + Point2d(a0, 0);
      poly.vertices[1] = ideal_point + Point2d(0, a1);
      poly.vertices[2] = ideal_point + Point2d(-a0, 0);
      poly.vertices[3] = ideal_point + Point2d(0, -a1);
      return poly;
    }

    case distance::DistanceType::CHEBYSHEV: {
      Polygon2d poly;
      poly.vertices.resize(4);
      poly.vertices[0] = ideal_point + Point2d(a0, a1);
      poly.vertices[1] = ideal_point + Point2d(-a0, a1);
      poly.vertices[2] = ideal_point + Point2d(-a0, -a1);
      poly.vertices[3] = ideal_point + Point2d(a0, -a1);
      return poly;
    }

    case distance::DistanceType::MINKOWSKI: {
      double p = distance_config.order_p;
      if (!std::isfinite(p) || p < 1.0) {
        throw std::invalid_argument(
            "level_set_2d: Minkowski order_p must be finite and >= 1");
      }
      if (p <= 1.0 + 1e-9) {
        Polygon2d poly;
        poly.vertices.resize(4);
        poly.vertices[0] = ideal_point + Point2d(a0, 0);
        poly.vertices[1] = ideal_point + Point2d(0, a1);
        poly.vertices[2] = ideal_point + Point2d(-a0, 0);
        poly.vertices[3] = ideal_point + Point2d(0, -a1);
        return poly;
      }
      if (p >= 100.0) {
        Polygon2d poly;
        poly.vertices.resize(4);
        poly.vertices[0] = ideal_point + Point2d(a0, a1);
        poly.vertices[1] = ideal_point + Point2d(-a0, a1);
        poly.vertices[2] = ideal_point + Point2d(-a0, -a1);
        poly.vertices[3] = ideal_point + Point2d(a0, -a1);
        return poly;
      }
      return Superellipse2d{ideal_point, a0, a1, p};
    }

    default:
      throw std::invalid_argument("level_set_2d: unknown DistanceType");
  }
}

/**
 * @brief Sample a LevelSet2d into a closed polygon with num_samples vertices.
 */
[[nodiscard]] inline Polygon2d to_polygon(const LevelSet2d& level_set,
                                          std::size_t num_samples = 128) {
  return std::visit(
      [num_samples](const auto& shape) -> Polygon2d {
        using T = std::decay_t<decltype(shape)>;
        if constexpr (std::is_same_v<T, Polygon2d>) {
          return shape;
        }
        if constexpr (std::is_same_v<T, Circle2d>) {
          Polygon2d out;
          out.vertices.reserve(num_samples);
          for (std::size_t i = 0; i < num_samples; ++i) {
            const double t = 2.0 * std::numbers::pi * static_cast<double>(i) /
                             static_cast<double>(num_samples);
            out.vertices.push_back(shape.center +
                                   shape.radius *
                                       Point2d(std::cos(t), std::sin(t)));
          }
          return out;
        }
        if constexpr (std::is_same_v<T, Ellipse2d>) {
          Polygon2d out;
          out.vertices.reserve(num_samples);
          for (std::size_t i = 0; i < num_samples; ++i) {
            const double t = 2.0 * std::numbers::pi * static_cast<double>(i) /
                             static_cast<double>(num_samples);
            out.vertices.push_back(shape.center +
                                   Point2d(shape.semi_axis_0 * std::cos(t),
                                           shape.semi_axis_1 * std::sin(t)));
          }
          return out;
        }
        if constexpr (std::is_same_v<T, Superellipse2d>) {
          Polygon2d out;
          out.vertices.reserve(num_samples);
          const double p = shape.exponent_p;
          for (std::size_t i = 0; i < num_samples; ++i) {
            const double t = 2.0 * std::numbers::pi * static_cast<double>(i) /
                             static_cast<double>(num_samples);
            const double c = std::cos(t);
            const double s = std::sin(t);
            const double r = std::pow(
                std::pow(std::abs(c), p) + std::pow(std::abs(s), p), -1.0 / p);
            out.vertices.push_back(
                shape.center +
                Point2d(shape.semi_axis_0 * r * c, shape.semi_axis_1 * r * s));
          }
          return out;
        }
        return Polygon2d{};
      },
      level_set);
}

}  // namespace socialchoicelab::preference::indifference
