// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// scs_api.cpp — extern "C" implementation of the SocialChoiceLab C API.
//
// Design rules:
//  - No C++ exception escapes this translation unit.
//    Every public function wraps its body in try/catch; on exception it fills
//    err_buf (if provided) and returns a non-zero error code.
//  - No STL type is exposed in the returned values; all output is via
//    caller-provided C buffers or the POD structs defined in scs_api.h.
//  - scs_stream_manager_create returns a heap-allocated SCS_StreamManagerImpl;
//    the caller must call scs_stream_manager_destroy.

#include "scs_api.h"  // NOLINT(build/include_subdir)

#include <cstdio>
#include <cstring>
#include <numbers>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "socialchoicelab/core/rng/stream_manager.h"
#include "socialchoicelab/preference/distance/distance_functions.h"
#include "socialchoicelab/preference/indifference/level_set.h"
#include "socialchoicelab/preference/loss/loss_functions.h"

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

void set_error(char* err_buf, int err_buf_len, const char* msg) {
  if (err_buf && err_buf_len > 0) {
    std::strncpy(err_buf, msg, static_cast<size_t>(err_buf_len) - 1u);
    err_buf[err_buf_len - 1] = '\0';
  }
}

// Map C loss type to C++ enum.
socialchoicelab::preference::loss::LossFunctionType to_cpp_loss(
    SCS_LossType t) {
  using LT = socialchoicelab::preference::loss::LossFunctionType;
  switch (t) {
    case SCS_LOSS_LINEAR:
      return LT::LINEAR;
    case SCS_LOSS_QUADRATIC:
      return LT::QUADRATIC;
    case SCS_LOSS_GAUSSIAN:
      return LT::GAUSSIAN;
    case SCS_LOSS_THRESHOLD:
      return LT::THRESHOLD;
    default:
      return LT::QUADRATIC;
  }
}

// Build C++ LossConfig from C POD struct.
socialchoicelab::preference::indifference::LossConfig to_cpp_loss_config(
    const SCS_LossConfig& cfg) {
  return {to_cpp_loss(cfg.loss_type), cfg.sensitivity, cfg.max_loss,
          cfg.steepness, cfg.threshold};
}

// Map C distance type to C++ enum.
socialchoicelab::preference::distance::DistanceType to_cpp_dist(
    SCS_DistanceType t) {
  using DT = socialchoicelab::preference::distance::DistanceType;
  switch (t) {
    case SCS_DIST_EUCLIDEAN:
      return DT::EUCLIDEAN;
    case SCS_DIST_MANHATTAN:
      return DT::MANHATTAN;
    case SCS_DIST_CHEBYSHEV:
      return DT::CHEBYSHEV;
    case SCS_DIST_MINKOWSKI:
      return DT::MINKOWSKI;
    default:
      return DT::EUCLIDEAN;
  }
}

// Build C++ DistanceConfig from C POD struct + array pointer.
socialchoicelab::preference::indifference::DistanceConfig to_cpp_dist_config(
    const SCS_DistanceConfig& cfg) {
  std::vector<double> weights(cfg.salience_weights,
                              cfg.salience_weights + cfg.n_weights);
  return {to_cpp_dist(cfg.distance_type), cfg.order_p, std::move(weights)};
}

}  // namespace

// ---------------------------------------------------------------------------
// Opaque handle definition
// ---------------------------------------------------------------------------

struct SCS_StreamManagerImpl {
  socialchoicelab::core::rng::StreamManager mgr;
  explicit SCS_StreamManagerImpl(uint64_t seed) : mgr(seed) {}
};

// ---------------------------------------------------------------------------
// Distance
// ---------------------------------------------------------------------------

extern "C" int scs_calculate_distance(const double* ideal_point,
                                      const double* alt_point, int n_dims,
                                      const SCS_DistanceConfig* dist_cfg,
                                      double* out, char* err_buf,
                                      int err_buf_len) {
  if (!ideal_point || !alt_point || !dist_cfg || !out || n_dims <= 0 ||
      !dist_cfg->salience_weights) {
    set_error(err_buf, err_buf_len,
              "scs_calculate_distance: null pointer or non-positive n_dims");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (dist_cfg->n_weights != n_dims) {
    set_error(err_buf, err_buf_len,
              "scs_calculate_distance: n_weights must equal n_dims");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    using socialchoicelab::preference::distance::chebyshev_distance;
    using socialchoicelab::preference::distance::euclidean_distance;
    using socialchoicelab::preference::distance::manhattan_distance;
    using socialchoicelab::preference::distance::minkowski_distance;
    Eigen::Map<const Eigen::VectorXd> ip(ideal_point, n_dims);
    Eigen::Map<const Eigen::VectorXd> ap(alt_point, n_dims);
    std::vector<double> weights(dist_cfg->salience_weights,
                                dist_cfg->salience_weights + n_dims);
    switch (dist_cfg->distance_type) {
      case SCS_DIST_EUCLIDEAN:
        *out = euclidean_distance(ip, ap, weights);
        break;
      case SCS_DIST_MANHATTAN:
        *out = manhattan_distance(ip, ap, weights);
        break;
      case SCS_DIST_CHEBYSHEV:
        *out = chebyshev_distance(ip, ap, weights);
        break;
      case SCS_DIST_MINKOWSKI:
        *out = minkowski_distance(ip, ap, dist_cfg->order_p, weights);
        break;
      default:
        set_error(err_buf, err_buf_len,
                  "scs_calculate_distance: unknown distance type");
        return SCS_ERROR_INVALID_ARGUMENT;
    }
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

// ---------------------------------------------------------------------------
// Loss / utility
// ---------------------------------------------------------------------------

extern "C" int scs_distance_to_utility(double distance,
                                       const SCS_LossConfig* loss_cfg,
                                       double* out, char* err_buf,
                                       int err_buf_len) {
  if (!loss_cfg || !out) {
    set_error(err_buf, err_buf_len,
              "scs_distance_to_utility: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    using socialchoicelab::preference::loss::distance_to_utility;
    *out = distance_to_utility(distance, to_cpp_loss(loss_cfg->loss_type),
                               loss_cfg->sensitivity, loss_cfg->max_loss,
                               loss_cfg->steepness, loss_cfg->threshold);
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_normalize_utility(double utility, double max_distance,
                                     const SCS_LossConfig* loss_cfg,
                                     double* out, char* err_buf,
                                     int err_buf_len) {
  if (!loss_cfg || !out) {
    set_error(err_buf, err_buf_len,
              "scs_normalize_utility: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    using socialchoicelab::preference::loss::normalize_utility;
    *out = normalize_utility(utility, max_distance,
                             to_cpp_loss(loss_cfg->loss_type),
                             loss_cfg->sensitivity, loss_cfg->max_loss,
                             loss_cfg->steepness, loss_cfg->threshold);
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

// ---------------------------------------------------------------------------
// Indifference / level sets
// ---------------------------------------------------------------------------

extern "C" int scs_level_set_1d(double ideal, double weight,
                                double utility_level,
                                const SCS_LossConfig* loss_cfg,
                                double* out_points, int* out_n, char* err_buf,
                                int err_buf_len) {
  if (!loss_cfg || !out_points || !out_n) {
    set_error(err_buf, err_buf_len, "scs_level_set_1d: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    using socialchoicelab::preference::indifference::DistanceConfig;
    using socialchoicelab::preference::indifference::level_set_1d;
    using socialchoicelab::preference::indifference::LossConfig;
    LossConfig lcfg = to_cpp_loss_config(*loss_cfg);
    DistanceConfig dcfg;
    dcfg.distance_type =
        socialchoicelab::preference::distance::DistanceType::EUCLIDEAN;
    dcfg.order_p = 2.0;
    dcfg.salience_weights = {weight};

    Eigen::VectorXd ip(1);
    ip(0) = ideal;
    auto pts = level_set_1d(ip, utility_level, lcfg, dcfg);
    *out_n = static_cast<int>(pts.size());
    for (int i = 0; i < *out_n; ++i) {
      out_points[i] = pts[static_cast<size_t>(i)](0);
    }
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_level_set_2d(double ideal_x, double ideal_y,
                                double utility_level,
                                const SCS_LossConfig* loss_cfg,
                                const SCS_DistanceConfig* dist_cfg,
                                SCS_LevelSet2d* out, char* err_buf,
                                int err_buf_len) {
  if (!loss_cfg || !dist_cfg || !out) {
    set_error(err_buf, err_buf_len, "scs_level_set_2d: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (dist_cfg->n_weights != 2 || !dist_cfg->salience_weights) {
    set_error(err_buf, err_buf_len,
              "scs_level_set_2d: dist_cfg must have n_weights == 2");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    using socialchoicelab::preference::indifference::Circle2d;
    using socialchoicelab::preference::indifference::DistanceConfig;
    using socialchoicelab::preference::indifference::Ellipse2d;
    using socialchoicelab::preference::indifference::level_set_2d;
    using socialchoicelab::preference::indifference::LevelSet2d;
    using socialchoicelab::preference::indifference::LossConfig;
    using socialchoicelab::preference::indifference::Polygon2d;
    using socialchoicelab::preference::indifference::Superellipse2d;
    LossConfig lcfg = to_cpp_loss_config(*loss_cfg);
    DistanceConfig dcfg = to_cpp_dist_config(*dist_cfg);

    Eigen::Vector2d ideal(ideal_x, ideal_y);
    LevelSet2d ls = level_set_2d(ideal, utility_level, lcfg, dcfg);

    std::memset(out, 0, sizeof(SCS_LevelSet2d));

    std::visit(
        [out](const auto& shape) {
          using T = std::decay_t<decltype(shape)>;
          if constexpr (std::is_same_v<T, Circle2d>) {
            out->type = SCS_LEVEL_SET_CIRCLE;
            out->center_x = shape.center(0);
            out->center_y = shape.center(1);
            out->param0 = shape.radius;
          } else if constexpr (std::is_same_v<T, Ellipse2d>) {
            out->type = SCS_LEVEL_SET_ELLIPSE;
            out->center_x = shape.center(0);
            out->center_y = shape.center(1);
            out->param0 = shape.semi_axis_0;
            out->param1 = shape.semi_axis_1;
          } else if constexpr (std::is_same_v<T, Superellipse2d>) {
            out->type = SCS_LEVEL_SET_SUPERELLIPSE;
            out->center_x = shape.center(0);
            out->center_y = shape.center(1);
            out->param0 = shape.semi_axis_0;
            out->param1 = shape.semi_axis_1;
            out->exponent_p = shape.exponent_p;
          } else if constexpr (std::is_same_v<T, Polygon2d>) {
            out->type = SCS_LEVEL_SET_POLYGON;
            out->center_x = 0.0;
            out->center_y = 0.0;
            out->n_vertices = static_cast<int>(shape.vertices.size());
            for (int i = 0; i < out->n_vertices && i < 4; ++i) {
              out->vertices[2 * i] = shape.vertices[static_cast<size_t>(i)](0);
              out->vertices[2 * i + 1] =
                  shape.vertices[static_cast<size_t>(i)](1);
            }
          }
        },
        ls);

    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_level_set_to_polygon(const SCS_LevelSet2d* level_set,
                                        int num_samples, double* out_xy,
                                        int* out_n, char* err_buf,
                                        int err_buf_len) {
  if (!level_set || !out_xy || !out_n) {
    set_error(err_buf, err_buf_len,
              "scs_level_set_to_polygon: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (level_set->type != SCS_LEVEL_SET_POLYGON && num_samples < 3) {
    set_error(err_buf, err_buf_len,
              "scs_level_set_to_polygon: num_samples must be >= 3 for smooth "
              "shapes");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    using socialchoicelab::preference::indifference::Circle2d;
    using socialchoicelab::preference::indifference::Ellipse2d;
    using socialchoicelab::preference::indifference::LevelSet2d;
    using socialchoicelab::preference::indifference::Polygon2d;
    using socialchoicelab::preference::indifference::Superellipse2d;
    using socialchoicelab::preference::indifference::to_polygon;

    // Rebuild the C++ LevelSet2d from the C POD struct.
    LevelSet2d ls;
    switch (level_set->type) {
      case SCS_LEVEL_SET_CIRCLE: {
        Circle2d c;
        c.center = Eigen::Vector2d(level_set->center_x, level_set->center_y);
        c.radius = level_set->param0;
        ls = c;
        break;
      }
      case SCS_LEVEL_SET_ELLIPSE: {
        Ellipse2d e;
        e.center = Eigen::Vector2d(level_set->center_x, level_set->center_y);
        e.semi_axis_0 = level_set->param0;
        e.semi_axis_1 = level_set->param1;
        ls = e;
        break;
      }
      case SCS_LEVEL_SET_SUPERELLIPSE: {
        Superellipse2d s;
        s.center = Eigen::Vector2d(level_set->center_x, level_set->center_y);
        s.semi_axis_0 = level_set->param0;
        s.semi_axis_1 = level_set->param1;
        s.exponent_p = level_set->exponent_p;
        ls = s;
        break;
      }
      case SCS_LEVEL_SET_POLYGON: {
        Polygon2d poly;
        poly.vertices.resize(static_cast<size_t>(level_set->n_vertices));
        for (int i = 0; i < level_set->n_vertices && i < 4; ++i) {
          poly.vertices[static_cast<size_t>(i)] = Eigen::Vector2d(
              level_set->vertices[2 * i], level_set->vertices[2 * i + 1]);
        }
        ls = poly;
        break;
      }
      default:
        set_error(err_buf, err_buf_len,
                  "scs_level_set_to_polygon: unknown level set type");
        return SCS_ERROR_INVALID_ARGUMENT;
    }

    Polygon2d poly = to_polygon(
        ls, static_cast<std::size_t>(level_set->type == SCS_LEVEL_SET_POLYGON
                                         ? level_set->n_vertices
                                         : num_samples));
    *out_n = static_cast<int>(poly.vertices.size());
    for (int i = 0; i < *out_n; ++i) {
      out_xy[2 * i] = poly.vertices[static_cast<size_t>(i)](0);
      out_xy[2 * i + 1] = poly.vertices[static_cast<size_t>(i)](1);
    }
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

// ---------------------------------------------------------------------------
// StreamManager lifecycle
// ---------------------------------------------------------------------------

extern "C" SCS_StreamManager* scs_stream_manager_create(uint64_t master_seed,
                                                        char* err_buf,
                                                        int err_buf_len) {
  try {
    return new SCS_StreamManagerImpl(master_seed);
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return nullptr;
  }
}

extern "C" void scs_stream_manager_destroy(SCS_StreamManager* mgr) {
  delete mgr;
}

extern "C" int scs_register_streams(SCS_StreamManager* mgr, const char** names,
                                    int n_names, char* err_buf,
                                    int err_buf_len) {
  if (!mgr) {
    set_error(err_buf, err_buf_len, "scs_register_streams: mgr is null");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (n_names > 0 && !names) {
    set_error(err_buf, err_buf_len,
              "scs_register_streams: names is null but n_names > 0");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    std::vector<std::string> name_vec;
    name_vec.reserve(static_cast<size_t>(n_names));
    for (int i = 0; i < n_names; ++i) {
      if (!names[i]) {
        set_error(err_buf, err_buf_len,
                  "scs_register_streams: names[i] is null");
        return SCS_ERROR_INVALID_ARGUMENT;
      }
      name_vec.emplace_back(names[i]);
    }
    mgr->mgr.register_streams(name_vec);
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_reset_all(SCS_StreamManager* mgr, uint64_t master_seed,
                             char* err_buf, int err_buf_len) {
  if (!mgr) {
    set_error(err_buf, err_buf_len, "scs_reset_all: mgr is null");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    mgr->mgr.reset_all(master_seed);
    return SCS_OK;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_reset_stream(SCS_StreamManager* mgr, const char* stream_name,
                                uint64_t seed, char* err_buf, int err_buf_len) {
  if (!mgr || !stream_name) {
    set_error(err_buf, err_buf_len,
              "scs_reset_stream: mgr or stream_name is null");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    mgr->mgr.reset_stream(stream_name, seed);
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_skip(SCS_StreamManager* mgr, const char* stream_name,
                        uint64_t n, char* err_buf, int err_buf_len) {
  if (!mgr || !stream_name) {
    set_error(err_buf, err_buf_len, "scs_skip: mgr or stream_name is null");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    mgr->mgr.get_stream(stream_name).skip(n);
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

// ---------------------------------------------------------------------------
// PRNG draw functions
// ---------------------------------------------------------------------------

extern "C" int scs_uniform_real(SCS_StreamManager* mgr, const char* stream_name,
                                double min, double max, double* out,
                                char* err_buf, int err_buf_len) {
  if (!mgr || !stream_name || !out) {
    set_error(err_buf, err_buf_len, "scs_uniform_real: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    *out = mgr->mgr.get_stream(stream_name).uniform_real(min, max);
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_normal(SCS_StreamManager* mgr, const char* stream_name,
                          double mean, double stddev, double* out,
                          char* err_buf, int err_buf_len) {
  if (!mgr || !stream_name || !out) {
    set_error(err_buf, err_buf_len, "scs_normal: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    *out = mgr->mgr.get_stream(stream_name).normal(mean, stddev);
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_bernoulli(SCS_StreamManager* mgr, const char* stream_name,
                             double probability, int* out, char* err_buf,
                             int err_buf_len) {
  if (!mgr || !stream_name || !out) {
    set_error(err_buf, err_buf_len, "scs_bernoulli: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    *out = mgr->mgr.get_stream(stream_name).bernoulli(probability) ? 1 : 0;
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_uniform_int(SCS_StreamManager* mgr, const char* stream_name,
                               int64_t min, int64_t max, int64_t* out,
                               char* err_buf, int err_buf_len) {
  if (!mgr || !stream_name || !out) {
    set_error(err_buf, err_buf_len, "scs_uniform_int: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    *out = static_cast<int64_t>(
        mgr->mgr.get_stream(stream_name).uniform_int(min, max));
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_uniform_choice(SCS_StreamManager* mgr,
                                  const char* stream_name, int64_t n,
                                  int64_t* out, char* err_buf,
                                  int err_buf_len) {
  if (!mgr || !stream_name || !out) {
    set_error(err_buf, err_buf_len,
              "scs_uniform_choice: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (n <= 0) {
    set_error(err_buf, err_buf_len, "scs_uniform_choice: n must be positive");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    *out = static_cast<int64_t>(mgr->mgr.get_stream(stream_name)
                                    .uniform_choice(static_cast<size_t>(n)));
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}
