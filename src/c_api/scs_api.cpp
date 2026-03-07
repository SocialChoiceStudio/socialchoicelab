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

#include <socialchoicelab/aggregation/profile.h>
#include <socialchoicelab/aggregation/profile_generators.h>
#include <socialchoicelab/geometry/convex_hull.h>
#include <socialchoicelab/geometry/copeland.h>
#include <socialchoicelab/geometry/core.h>
#include <socialchoicelab/geometry/heart.h>
#include <socialchoicelab/geometry/majority.h>
#include <socialchoicelab/geometry/uncovered_set.h>
#include <socialchoicelab/geometry/winset.h>
#include <socialchoicelab/geometry/winset_ops.h>
#include <socialchoicelab/geometry/yolk.h>

#include <Eigen/Dense>
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
// SCS_WinsetImpl — opaque handle wrapping WinsetRegion  (C2.1)
// ---------------------------------------------------------------------------

struct SCS_WinsetImpl {
  socialchoicelab::geometry::WinsetRegion region;
};

// ---------------------------------------------------------------------------
// SCS_ProfileImpl — opaque handle wrapping Profile  (C5.1)
// ---------------------------------------------------------------------------

struct SCS_ProfileImpl {
  socialchoicelab::aggregation::Profile profile;
};

// ---------------------------------------------------------------------------
// API version  (C0.1)
// ---------------------------------------------------------------------------

extern "C" int scs_api_version(int* out_major, int* out_minor, int* out_patch,
                               char* err_buf, int err_buf_len) {
  if (!out_major || !out_minor || !out_patch) {
    if (err_buf && err_buf_len > 0) {
      std::strncpy(err_buf, "scs_api_version: null output pointer",
                   static_cast<size_t>(err_buf_len) - 1u);
      err_buf[err_buf_len - 1] = '\0';
    }
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  *out_major = SCS_API_VERSION_MAJOR;
  *out_minor = SCS_API_VERSION_MINOR;
  *out_patch = SCS_API_VERSION_PATCH;
  return SCS_OK;
}

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

// Convert a C SCS_DistanceConfig to the C++ geometry::DistConfig.
// Validates that dist_cfg is non-null and has exactly 2 weights.
// Returns false and sets err_buf on failure.
bool to_dist_config(const SCS_DistanceConfig* dist_cfg,
                    socialchoicelab::geometry::DistConfig& out, char* err_buf,
                    int err_buf_len) {
  using DT = socialchoicelab::preference::distance::DistanceType;
  if (!dist_cfg) {
    set_error(err_buf, err_buf_len, "dist_cfg must not be null");
    return false;
  }
  if (dist_cfg->n_weights != 2) {
    set_error(err_buf, err_buf_len,
              "dist_cfg->n_weights must be 2 for 2D functions");
    return false;
  }
  if (!dist_cfg->salience_weights) {
    set_error(err_buf, err_buf_len,
              "dist_cfg->salience_weights must not be null");
    return false;
  }
  switch (dist_cfg->distance_type) {
    case SCS_DIST_EUCLIDEAN:
      out.type = DT::EUCLIDEAN;
      break;
    case SCS_DIST_MANHATTAN:
      out.type = DT::MANHATTAN;
      break;
    case SCS_DIST_CHEBYSHEV:
      out.type = DT::CHEBYSHEV;
      break;
    case SCS_DIST_MINKOWSKI:
      out.type = DT::MINKOWSKI;
      break;
    default:
      set_error(err_buf, err_buf_len, "unknown SCS_DistanceType value");
      return false;
  }
  out.order_p = dist_cfg->order_p;
  out.salience_weights = {dist_cfg->salience_weights[0],
                          dist_cfg->salience_weights[1]};
  return true;
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

// Build a vector<Point2d> from a flat C array.  Assumes n >= 1 and
// voter_ideals_xy is non-null (caller already validated).
std::vector<socialchoicelab::core::types::Point2d> build_voters(
    const double* xy, int n) {
  std::vector<socialchoicelab::core::types::Point2d> v;
  v.reserve(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) v.emplace_back(xy[2 * i], xy[2 * i + 1]);
  return v;
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
                                        int out_capacity, int* out_n,
                                        char* err_buf, int err_buf_len) {
  if (!level_set || !out_n) {
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
    const int required = static_cast<int>(poly.vertices.size());
    *out_n = required;

    // Size-query mode: caller passes null buffer to discover required capacity.
    if (out_xy == nullptr) {
      return SCS_OK;
    }

    if (out_capacity < required) {
      set_error(err_buf, err_buf_len,
                "scs_level_set_to_polygon: out_xy buffer too small; check "
                "*out_n for the required number of (x,y) pairs");
      return SCS_ERROR_BUFFER_TOO_SMALL;
    }

    for (int i = 0; i < required; ++i) {
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
// Geometry
// ---------------------------------------------------------------------------

extern "C" int scs_convex_hull_2d(const double* points, int n_points,
                                  double* out_xy, int* out_n, char* err_buf,
                                  int err_buf_len) {
  try {
    if (points == nullptr || out_xy == nullptr || out_n == nullptr) {
      set_error(err_buf, err_buf_len,
                "scs_convex_hull_2d: null pointer argument.");
      return SCS_ERROR_INVALID_ARGUMENT;
    }
    if (n_points < 1) {
      set_error(err_buf, err_buf_len,
                ("scs_convex_hull_2d: n_points must be >= 1 (got " +
                 std::to_string(n_points) + ").")
                    .c_str());
      return SCS_ERROR_INVALID_ARGUMENT;
    }
    std::vector<socialchoicelab::core::types::Point2d> pts;
    pts.reserve(static_cast<std::size_t>(n_points));
    for (int i = 0; i < n_points; ++i) {
      pts.emplace_back(points[2 * i], points[2 * i + 1]);
    }
    socialchoicelab::geometry::Polygon2E hull =
        socialchoicelab::geometry::convex_hull_2d(pts);
    *out_n = static_cast<int>(hull.size());
    int k = 0;
    for (auto it = hull.vertices_begin(); it != hull.vertices_end(); ++it) {
      out_xy[k++] = CGAL::to_double(it->x());
      out_xy[k++] = CGAL::to_double(it->y());
    }
    return SCS_OK;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  } catch (...) {
    set_error(err_buf, err_buf_len, "scs_convex_hull_2d: unknown exception.");
    return SCS_ERROR_INTERNAL;
  }
}

// ---------------------------------------------------------------------------
// Geometry — majority preference  (C1)
// ---------------------------------------------------------------------------

extern "C" int scs_majority_prefers_2d(double point_a_x, double point_a_y,
                                       double point_b_x, double point_b_y,
                                       const double* voter_ideals_xy,
                                       int n_voters,
                                       const SCS_DistanceConfig* dist_cfg,
                                       int k, int* out, char* err_buf,
                                       int err_buf_len) {
  if (!voter_ideals_xy || !out) {
    set_error(err_buf, err_buf_len,
              "scs_majority_prefers_2d: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (n_voters < 1) {
    set_error(err_buf, err_buf_len,
              ("scs_majority_prefers_2d: n_voters must be >= 1 (got " +
               std::to_string(n_voters) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    using socialchoicelab::core::types::Point2d;
    std::vector<Point2d> voters;
    voters.reserve(static_cast<std::size_t>(n_voters));
    for (int i = 0; i < n_voters; ++i)
      voters.emplace_back(voter_ideals_xy[2 * i], voter_ideals_xy[2 * i + 1]);
    const Point2d a(point_a_x, point_a_y);
    const Point2d b(point_b_x, point_b_y);
    *out = socialchoicelab::geometry::majority_prefers(a, b, voters, cfg, k)
               ? 1
               : 0;
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_pairwise_matrix_2d(const double* alt_xy, int n_alts,
                                      const double* voter_ideals_xy,
                                      int n_voters,
                                      const SCS_DistanceConfig* dist_cfg, int k,
                                      SCS_PairwiseResult* out_matrix,
                                      int out_len, char* err_buf,
                                      int err_buf_len) {
  if (!alt_xy || !voter_ideals_xy || !out_matrix) {
    set_error(err_buf, err_buf_len,
              "scs_pairwise_matrix_2d: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (n_alts < 1) {
    set_error(err_buf, err_buf_len,
              ("scs_pairwise_matrix_2d: n_alts must be >= 1 (got " +
               std::to_string(n_alts) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (n_voters < 1) {
    set_error(err_buf, err_buf_len,
              ("scs_pairwise_matrix_2d: n_voters must be >= 1 (got " +
               std::to_string(n_voters) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  const int required = n_alts * n_alts;
  if (out_len < required) {
    set_error(
        err_buf, err_buf_len,
        ("scs_pairwise_matrix_2d: out_len must be >= n_alts*n_alts=" +
         std::to_string(required) + " (got " + std::to_string(out_len) + ")")
            .c_str());
    return SCS_ERROR_BUFFER_TOO_SMALL;
  }
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    using socialchoicelab::core::types::Point2d;
    std::vector<Point2d> alts, voters;
    alts.reserve(static_cast<std::size_t>(n_alts));
    voters.reserve(static_cast<std::size_t>(n_voters));
    for (int i = 0; i < n_alts; ++i)
      alts.emplace_back(alt_xy[2 * i], alt_xy[2 * i + 1]);
    for (int i = 0; i < n_voters; ++i)
      voters.emplace_back(voter_ideals_xy[2 * i], voter_ideals_xy[2 * i + 1]);

    Eigen::MatrixXi M =
        socialchoicelab::geometry::pairwise_matrix(alts, voters, cfg, k);

    for (int i = 0; i < n_alts; ++i) {
      for (int j = 0; j < n_alts; ++j) {
        const int margin = M(i, j);
        SCS_PairwiseResult cell;
        if (margin > 0)
          cell = SCS_PAIRWISE_WIN;
        else if (margin < 0)
          cell = SCS_PAIRWISE_LOSS;
        else
          cell = SCS_PAIRWISE_TIE;
        out_matrix[i * n_alts + j] = cell;
      }
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

extern "C" int scs_weighted_majority_prefers_2d(
    double point_a_x, double point_a_y, double point_b_x, double point_b_y,
    const double* voter_ideals_xy, int n_voters, const double* weights,
    const SCS_DistanceConfig* dist_cfg, double threshold, int* out,
    char* err_buf, int err_buf_len) {
  if (!voter_ideals_xy || !weights || !out) {
    set_error(err_buf, err_buf_len,
              "scs_weighted_majority_prefers_2d: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (n_voters < 1) {
    set_error(err_buf, err_buf_len,
              ("scs_weighted_majority_prefers_2d: n_voters must be >= 1 (got " +
               std::to_string(n_voters) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    using socialchoicelab::core::types::Point2d;
    std::vector<Point2d> voters;
    std::vector<double> w(weights, weights + n_voters);
    voters.reserve(static_cast<std::size_t>(n_voters));
    for (int i = 0; i < n_voters; ++i)
      voters.emplace_back(voter_ideals_xy[2 * i], voter_ideals_xy[2 * i + 1]);
    const Point2d a(point_a_x, point_a_y);
    const Point2d b(point_b_x, point_b_y);
    *out = socialchoicelab::geometry::weighted_majority_prefers(a, b, voters, w,
                                                                cfg, threshold)
               ? 1
               : 0;
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
// Geometry — winset factory  (C2.2–C2.4)
// ---------------------------------------------------------------------------

// Shared null/size validation used by all three winset factories.
static bool validate_winset_inputs(const double* voter_ideals_xy, int n_voters,
                                   int num_samples, char* err_buf,
                                   int err_buf_len) {
  if (!voter_ideals_xy) {
    set_error(err_buf, err_buf_len, "voter_ideals_xy must not be null");
    return false;
  }
  if (n_voters < 1) {
    set_error(err_buf, err_buf_len,
              ("n_voters must be >= 1 (got " + std::to_string(n_voters) + ")")
                  .c_str());
    return false;
  }
  if (num_samples < 4) {
    set_error(
        err_buf, err_buf_len,
        ("num_samples must be >= 4 (got " + std::to_string(num_samples) + ")")
            .c_str());
    return false;
  }
  return true;
}

extern "C" SCS_Winset* scs_winset_2d(double status_quo_x, double status_quo_y,
                                     const double* voter_ideals_xy,
                                     int n_voters,
                                     const SCS_DistanceConfig* dist_cfg, int k,
                                     int num_samples, char* err_buf,
                                     int err_buf_len) {
  if (!validate_winset_inputs(voter_ideals_xy, n_voters, num_samples, err_buf,
                              err_buf_len))
    return nullptr;
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len)) return nullptr;
  try {
    using socialchoicelab::core::types::Point2d;
    auto ws = new SCS_WinsetImpl{};
    ws->region = socialchoicelab::geometry::winset_2d(
        Point2d(status_quo_x, status_quo_y),
        build_voters(voter_ideals_xy, n_voters), cfg, k, num_samples);
    return ws;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return nullptr;
  }
}

extern "C" SCS_Winset* scs_weighted_winset_2d(
    double status_quo_x, double status_quo_y, const double* voter_ideals_xy,
    int n_voters, const double* weights, const SCS_DistanceConfig* dist_cfg,
    double threshold, int num_samples, char* err_buf, int err_buf_len) {
  if (!validate_winset_inputs(voter_ideals_xy, n_voters, num_samples, err_buf,
                              err_buf_len))
    return nullptr;
  if (!weights) {
    set_error(err_buf, err_buf_len, "weights must not be null");
    return nullptr;
  }
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len)) return nullptr;
  try {
    using socialchoicelab::core::types::Point2d;
    std::vector<double> w(weights, weights + n_voters);
    auto ws = new SCS_WinsetImpl{};
    ws->region = socialchoicelab::geometry::weighted_winset_2d(
        Point2d(status_quo_x, status_quo_y),
        build_voters(voter_ideals_xy, n_voters), w, cfg, threshold,
        num_samples);
    return ws;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return nullptr;
  }
}

extern "C" SCS_Winset* scs_winset_with_veto_2d(
    double status_quo_x, double status_quo_y, const double* voter_ideals_xy,
    int n_voters, const SCS_DistanceConfig* dist_cfg,
    const double* veto_ideals_xy, int n_veto, int k, int num_samples,
    char* err_buf, int err_buf_len) {
  if (!validate_winset_inputs(voter_ideals_xy, n_voters, num_samples, err_buf,
                              err_buf_len))
    return nullptr;
  if (n_veto > 0 && !veto_ideals_xy) {
    set_error(err_buf, err_buf_len,
              "veto_ideals_xy must not be null when n_veto > 0");
    return nullptr;
  }
  if (n_veto < 0) {
    set_error(err_buf, err_buf_len, "n_veto must be >= 0");
    return nullptr;
  }
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len)) return nullptr;
  try {
    using socialchoicelab::core::types::Point2d;
    auto vetos = (n_veto > 0) ? build_voters(veto_ideals_xy, n_veto)
                              : std::vector<Point2d>{};
    auto ws = new SCS_WinsetImpl{};
    ws->region = socialchoicelab::geometry::winset_with_veto_2d(
        Point2d(status_quo_x, status_quo_y),
        build_voters(voter_ideals_xy, n_voters), cfg, vetos, k, num_samples);
    return ws;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return nullptr;
  }
}

// ---------------------------------------------------------------------------
// Geometry — winset lifecycle and query  (C2.5)
// ---------------------------------------------------------------------------

extern "C" void scs_winset_destroy(SCS_Winset* ws) { delete ws; }

extern "C" int scs_winset_is_empty(const SCS_Winset* ws, int* out_is_empty,
                                   char* err_buf, int err_buf_len) {
  if (!ws || !out_is_empty) {
    set_error(err_buf, err_buf_len, "scs_winset_is_empty: null argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    *out_is_empty = ws->region.is_empty() ? 1 : 0;
    return SCS_OK;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_winset_contains_point_2d(const SCS_Winset* ws, double x,
                                            double y, int* out_contains,
                                            char* err_buf, int err_buf_len) {
  if (!ws || !out_contains) {
    set_error(err_buf, err_buf_len,
              "scs_winset_contains_point_2d: null argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    using socialchoicelab::core::types::Point2d;
    *out_contains =
        socialchoicelab::geometry::winset_contains(ws->region, Point2d(x, y))
            ? 1
            : 0;
    return SCS_OK;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_winset_bbox_2d(const SCS_Winset* ws, int* out_found,
                                  double* out_min_x, double* out_min_y,
                                  double* out_max_x, double* out_max_y,
                                  char* err_buf, int err_buf_len) {
  if (!ws || !out_found || !out_min_x || !out_min_y || !out_max_x ||
      !out_max_y) {
    set_error(err_buf, err_buf_len, "scs_winset_bbox_2d: null argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    if (ws->region.is_empty()) {
      *out_found = 0;
      return SCS_OK;
    }
    std::vector<socialchoicelab::geometry::PolygonWithHoles2E> pwhs;
    ws->region.polygons_with_holes(std::back_inserter(pwhs));
    double xmin = std::numeric_limits<double>::max();
    double ymin = std::numeric_limits<double>::max();
    double xmax = std::numeric_limits<double>::lowest();
    double ymax = std::numeric_limits<double>::lowest();
    for (const auto& pwh : pwhs) {
      for (auto it = pwh.outer_boundary().vertices_begin();
           it != pwh.outer_boundary().vertices_end(); ++it) {
        const double vx = CGAL::to_double(it->x());
        const double vy = CGAL::to_double(it->y());
        xmin = std::min(xmin, vx);
        ymin = std::min(ymin, vy);
        xmax = std::max(xmax, vx);
        ymax = std::max(ymax, vy);
      }
    }
    *out_found = 1;
    *out_min_x = xmin;
    *out_min_y = ymin;
    *out_max_x = xmax;
    *out_max_y = ymax;
    return SCS_OK;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

// ---------------------------------------------------------------------------
// Geometry — winset boundary export  (C2.6)
// ---------------------------------------------------------------------------

// Collect all boundary rings (outer + holes) from a WinsetRegion.
// Returns pairs of (vertex_count, is_hole) per ring.
static std::vector<std::pair<int, int>> collect_rings(
    const socialchoicelab::geometry::WinsetRegion& region) {
  std::vector<socialchoicelab::geometry::PolygonWithHoles2E> pwhs;
  region.polygons_with_holes(std::back_inserter(pwhs));
  std::vector<std::pair<int, int>> rings;
  for (const auto& pwh : pwhs) {
    rings.push_back(
        {static_cast<int>(pwh.outer_boundary().size()), 0 /* outer */});
    for (auto h = pwh.holes_begin(); h != pwh.holes_end(); ++h)
      rings.push_back({static_cast<int>(h->size()), 1 /* hole */});
  }
  return rings;
}

extern "C" int scs_winset_boundary_size_2d(const SCS_Winset* ws,
                                           int* out_xy_pairs, int* out_n_paths,
                                           char* err_buf, int err_buf_len) {
  if (!ws || !out_xy_pairs || !out_n_paths) {
    set_error(err_buf, err_buf_len,
              "scs_winset_boundary_size_2d: null argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    auto rings = collect_rings(ws->region);
    int total_pairs = 0;
    for (const auto& r : rings) total_pairs += r.first;
    *out_xy_pairs = total_pairs;
    *out_n_paths = static_cast<int>(rings.size());
    return SCS_OK;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_winset_sample_boundary_2d(
    const SCS_Winset* ws, double* out_xy, int out_xy_capacity, int* out_xy_n,
    int* out_path_starts, int out_path_capacity, int* out_path_is_hole,
    int* out_n_paths, char* err_buf, int err_buf_len) {
  if (!ws || !out_xy_n || !out_n_paths) {
    set_error(err_buf, err_buf_len,
              "scs_winset_sample_boundary_2d: null required argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    auto rings = collect_rings(ws->region);
    int total_pairs = 0;
    for (const auto& r : rings) total_pairs += r.first;
    const int n_paths = static_cast<int>(rings.size());

    *out_xy_n = total_pairs;
    *out_n_paths = n_paths;

    // Size-query mode: caller may omit one or both output buffers.
    const bool size_query_xy = (out_xy == nullptr || out_xy_capacity == 0);
    const bool size_query_paths =
        (out_path_starts == nullptr || out_path_capacity == 0);
    if (size_query_xy && size_query_paths) return SCS_OK;

    // Buffer-size check.
    if (!size_query_xy && out_xy_capacity < total_pairs) {
      set_error(err_buf, err_buf_len,
                "scs_winset_sample_boundary_2d: out_xy buffer too small");
      return SCS_ERROR_BUFFER_TOO_SMALL;
    }
    if (!size_query_paths && out_path_capacity < n_paths) {
      set_error(err_buf, err_buf_len,
                "scs_winset_sample_boundary_2d: out_path_starts buffer too "
                "small");
      return SCS_ERROR_BUFFER_TOO_SMALL;
    }

    // Fill: iterate polygons-with-holes again to write vertices.
    std::vector<socialchoicelab::geometry::PolygonWithHoles2E> pwhs;
    ws->region.polygons_with_holes(std::back_inserter(pwhs));
    int xy_pos = 0;
    int path_idx = 0;
    auto write_ring = [&](const auto& ring, int is_hole) {
      if (!size_query_paths) {
        out_path_starts[path_idx] = xy_pos;
        if (out_path_is_hole) out_path_is_hole[path_idx] = is_hole;
      }
      if (!size_query_xy) {
        for (auto it = ring.vertices_begin(); it != ring.vertices_end(); ++it) {
          out_xy[2 * xy_pos] = CGAL::to_double(it->x());
          out_xy[2 * xy_pos + 1] = CGAL::to_double(it->y());
          ++xy_pos;
        }
      } else {
        xy_pos += static_cast<int>(ring.size());
      }
      ++path_idx;
    };
    for (const auto& pwh : pwhs) {
      write_ring(pwh.outer_boundary(), 0);
      for (auto h = pwh.holes_begin(); h != pwh.holes_end(); ++h)
        write_ring(*h, 1);
    }
    return SCS_OK;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

// ---------------------------------------------------------------------------
// Geometry — winset boolean operations  (C2.7)
// ---------------------------------------------------------------------------

#define WINSET_BINOP(name, cpp_fn)                                      \
  extern "C" SCS_Winset* name(const SCS_Winset* a, const SCS_Winset* b, \
                              char* err_buf, int err_buf_len) {         \
    if (!a || !b) {                                                     \
      set_error(err_buf, err_buf_len, #name ": null argument");         \
      return nullptr;                                                   \
    }                                                                   \
    try {                                                               \
      auto* result = new SCS_WinsetImpl{};                              \
      result->region =                                                  \
          socialchoicelab::geometry::cpp_fn(a->region, b->region);      \
      return result;                                                    \
    } catch (const std::exception& e) {                                 \
      set_error(err_buf, err_buf_len, e.what());                        \
      return nullptr;                                                   \
    }                                                                   \
  }

WINSET_BINOP(scs_winset_union, winset_union)
WINSET_BINOP(scs_winset_intersection, winset_intersection)
WINSET_BINOP(scs_winset_difference, winset_difference)
WINSET_BINOP(scs_winset_symmetric_difference, winset_symmetric_difference)

#undef WINSET_BINOP

// ---------------------------------------------------------------------------
// Geometry — winset clone  (C2.8)
// ---------------------------------------------------------------------------

extern "C" SCS_Winset* scs_winset_clone(const SCS_Winset* ws, char* err_buf,
                                        int err_buf_len) {
  if (!ws) {
    set_error(err_buf, err_buf_len, "scs_winset_clone: null argument");
    return nullptr;
  }
  try {
    return new SCS_WinsetImpl{ws->region};
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return nullptr;
  }
}

// ---------------------------------------------------------------------------
// Geometry — Yolk  (C3)
// ---------------------------------------------------------------------------

extern "C" int scs_yolk_2d(const double* voter_ideals_xy, int n_voters,
                           const SCS_DistanceConfig* dist_cfg, int k,
                           int num_samples, SCS_Yolk2d* out, char* err_buf,
                           int err_buf_len) {
  if (!voter_ideals_xy || !out) {
    set_error(err_buf, err_buf_len, "scs_yolk_2d: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (n_voters < 2) {
    set_error(err_buf, err_buf_len,
              ("scs_yolk_2d: n_voters must be >= 2 (got " +
               std::to_string(n_voters) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  // dist_cfg is validated by to_dist_config but yolk_2d only uses the k
  // parameter and voter positions — the distance metric is not used.
  // We validate the pointer to catch accidental null, but do not apply it.
  if (!dist_cfg) {
    set_error(err_buf, err_buf_len, "scs_yolk_2d: dist_cfg must not be null");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    auto voters = build_voters(voter_ideals_xy, n_voters);
    auto yolk = socialchoicelab::geometry::yolk_2d(voters, k, num_samples);
    out->center_x = yolk.center.x();
    out->center_y = yolk.center.y();
    out->radius = yolk.radius;
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
// Geometry — internal helpers for C4  (anonymous namespace)
// ---------------------------------------------------------------------------

namespace {

using C4Point2d = socialchoicelab::core::types::Point2d;
using C4Polygon2E = socialchoicelab::geometry::Polygon2E;

// Find the 0-based index of p in alts using exact equality.
// Safe because C4 functions return subsets of the input alternatives — no
// arithmetic is applied to the coordinates before the comparison.
int find_alt_index(const std::vector<C4Point2d>& alts, const C4Point2d& p) {
  for (int i = 0; i < static_cast<int>(alts.size()); ++i) {
    if (alts[i].x() == p.x() && alts[i].y() == p.y()) return i;
  }
  return -1;
}

// Export a Polygon2E as flat interleaved (x,y) pairs.
// If out_xy is null or out_capacity == 0 (size-query mode), only sets *out_n.
// Returns SCS_OK or SCS_ERROR_BUFFER_TOO_SMALL.
int export_polygon2e(const C4Polygon2E& poly, double* out_xy, int out_capacity,
                     int* out_n, char* err_buf, int err_buf_len) {
  const int n = static_cast<int>(poly.size());
  if (out_n) *out_n = n;
  if (!out_xy || out_capacity == 0) return SCS_OK;  // size-query
  if (out_capacity < n) {
    set_error(err_buf, err_buf_len,
              ("output buffer too small: need " + std::to_string(n) +
               " (x,y) pairs, got capacity " + std::to_string(out_capacity))
                  .c_str());
    return SCS_ERROR_BUFFER_TOO_SMALL;
  }
  int i = 0;
  for (auto v = poly.vertices_begin(); v != poly.vertices_end(); ++v) {
    out_xy[2 * i] = CGAL::to_double(v->x());
    out_xy[2 * i + 1] = CGAL::to_double(v->y());
    ++i;
  }
  return SCS_OK;
}

}  // anonymous namespace

// ---------------------------------------------------------------------------
// Geometry — Copeland scores and winner  (C4.1, C4.2)
// ---------------------------------------------------------------------------

extern "C" int scs_copeland_scores_2d(const double* alt_xy, int n_alts,
                                      const double* voter_ideals_xy,
                                      int n_voters,
                                      const SCS_DistanceConfig* dist_cfg, int k,
                                      int* out_scores, int out_len,
                                      char* err_buf, int err_buf_len) {
  if (!alt_xy || !voter_ideals_xy || !out_scores) {
    set_error(err_buf, err_buf_len,
              "scs_copeland_scores_2d: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (n_alts < 1) {
    set_error(err_buf, err_buf_len,
              ("scs_copeland_scores_2d: n_alts must be >= 1 (got " +
               std::to_string(n_alts) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (out_len < n_alts) {
    set_error(err_buf, err_buf_len,
              ("scs_copeland_scores_2d: out_len must be >= n_alts (got " +
               std::to_string(out_len) + " < " + std::to_string(n_alts) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    auto alts = build_voters(alt_xy, n_alts);
    auto voters = build_voters(voter_ideals_xy, n_voters);
    auto scores =
        socialchoicelab::geometry::copeland_scores(alts, voters, cfg, k);
    for (int i = 0; i < static_cast<int>(scores.size()); ++i) {
      out_scores[i] = scores[i];
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

extern "C" int scs_copeland_winner_2d(const double* alt_xy, int n_alts,
                                      const double* voter_ideals_xy,
                                      int n_voters,
                                      const SCS_DistanceConfig* dist_cfg, int k,
                                      int* out_winner_idx, char* err_buf,
                                      int err_buf_len) {
  if (!alt_xy || !voter_ideals_xy || !out_winner_idx) {
    set_error(err_buf, err_buf_len,
              "scs_copeland_winner_2d: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (n_alts < 1) {
    set_error(err_buf, err_buf_len,
              ("scs_copeland_winner_2d: n_alts must be >= 1 (got " +
               std::to_string(n_alts) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    auto alts = build_voters(alt_xy, n_alts);
    auto voters = build_voters(voter_ideals_xy, n_voters);
    auto scores =
        socialchoicelab::geometry::copeland_scores(alts, voters, cfg, k);
    int best = 0;
    for (int i = 1; i < static_cast<int>(scores.size()); ++i) {
      if (scores[i] > scores[best]) best = i;
    }
    *out_winner_idx = best;
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
// Geometry — Condorcet winner and core  (C4.3, C4.4)
// ---------------------------------------------------------------------------

extern "C" int scs_has_condorcet_winner_2d(const double* alt_xy, int n_alts,
                                           const double* voter_ideals_xy,
                                           int n_voters,
                                           const SCS_DistanceConfig* dist_cfg,
                                           int k, int* out_found, char* err_buf,
                                           int err_buf_len) {
  if (!alt_xy || !voter_ideals_xy || !out_found) {
    set_error(err_buf, err_buf_len,
              "scs_has_condorcet_winner_2d: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    auto alts = build_voters(alt_xy, n_alts);
    auto voters = build_voters(voter_ideals_xy, n_voters);
    *out_found =
        socialchoicelab::geometry::has_condorcet_winner(alts, voters, cfg, k)
            ? 1
            : 0;
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_condorcet_winner_2d(
    const double* alt_xy, int n_alts, const double* voter_ideals_xy,
    int n_voters, const SCS_DistanceConfig* dist_cfg, int k, int* out_found,
    int* out_winner_idx, char* err_buf, int err_buf_len) {
  if (!alt_xy || !voter_ideals_xy || !out_found || !out_winner_idx) {
    set_error(err_buf, err_buf_len,
              "scs_condorcet_winner_2d: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    auto alts = build_voters(alt_xy, n_alts);
    auto voters = build_voters(voter_ideals_xy, n_voters);
    auto opt =
        socialchoicelab::geometry::condorcet_winner(alts, voters, cfg, k);
    if (opt.has_value()) {
      *out_found = 1;
      int idx = find_alt_index(alts, *opt);
      *out_winner_idx = (idx >= 0) ? idx : 0;
    } else {
      *out_found = 0;
      *out_winner_idx = -1;
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

extern "C" int scs_core_2d(const double* voter_ideals_xy, int n_voters,
                           const SCS_DistanceConfig* dist_cfg, int k,
                           int* out_found, double* out_x, double* out_y,
                           char* err_buf, int err_buf_len) {
  if (!voter_ideals_xy || !out_found || !out_x || !out_y) {
    set_error(err_buf, err_buf_len, "scs_core_2d: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    auto voters = build_voters(voter_ideals_xy, n_voters);
    auto opt = socialchoicelab::geometry::core_2d(voters, cfg, k);
    if (opt.has_value()) {
      *out_found = 1;
      *out_x = opt->x();
      *out_y = opt->y();
    } else {
      *out_found = 0;
      *out_x = 0.0;
      *out_y = 0.0;
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
// Geometry — Uncovered set (discrete and continuous)  (C4.5, C4.6)
// ---------------------------------------------------------------------------

extern "C" int scs_uncovered_set_2d(const double* alt_xy, int n_alts,
                                    const double* voter_ideals_xy, int n_voters,
                                    const SCS_DistanceConfig* dist_cfg, int k,
                                    int* out_indices, int out_capacity,
                                    int* out_n, char* err_buf,
                                    int err_buf_len) {
  if (!alt_xy || !voter_ideals_xy || !out_n) {
    set_error(err_buf, err_buf_len,
              "scs_uncovered_set_2d: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    auto alts = build_voters(alt_xy, n_alts);
    auto voters = build_voters(voter_ideals_xy, n_voters);
    auto unc = socialchoicelab::geometry::uncovered_set(alts, voters, cfg, k);
    const int n = static_cast<int>(unc.size());
    *out_n = n;
    if (!out_indices || out_capacity == 0) return SCS_OK;  // size-query
    if (out_capacity < n) {
      set_error(
          err_buf, err_buf_len,
          ("scs_uncovered_set_2d: buffer too small: need " + std::to_string(n) +
           " indices, capacity " + std::to_string(out_capacity))
              .c_str());
      return SCS_ERROR_BUFFER_TOO_SMALL;
    }
    for (int i = 0; i < n; ++i) {
      int idx = find_alt_index(alts, unc[static_cast<size_t>(i)]);
      out_indices[i] = (idx >= 0) ? idx : -1;
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

extern "C" int scs_uncovered_set_boundary_size_2d(
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg, int grid_resolution, int k,
    int* out_xy_pairs, char* err_buf, int err_buf_len) {
  if (!voter_ideals_xy || !out_xy_pairs) {
    set_error(err_buf, err_buf_len,
              "scs_uncovered_set_boundary_size_2d: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    auto voters = build_voters(voter_ideals_xy, n_voters);
    auto poly = socialchoicelab::geometry::uncovered_set_boundary_2d(
        voters, cfg, grid_resolution, k);
    *out_xy_pairs = static_cast<int>(poly.size());
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_uncovered_set_boundary_2d(const double* voter_ideals_xy,
                                             int n_voters,
                                             const SCS_DistanceConfig* dist_cfg,
                                             int grid_resolution, int k,
                                             double* out_xy, int out_capacity,
                                             int* out_n, char* err_buf,
                                             int err_buf_len) {
  if (!voter_ideals_xy || !out_n) {
    set_error(err_buf, err_buf_len,
              "scs_uncovered_set_boundary_2d: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    auto voters = build_voters(voter_ideals_xy, n_voters);
    auto poly = socialchoicelab::geometry::uncovered_set_boundary_2d(
        voters, cfg, grid_resolution, k);
    return export_polygon2e(poly, out_xy, out_capacity, out_n, err_buf,
                            err_buf_len);
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

// ---------------------------------------------------------------------------
// Geometry — Heart (discrete and continuous)  (C4.7, C4.8)
// ---------------------------------------------------------------------------

extern "C" int scs_heart_2d(const double* alt_xy, int n_alts,
                            const double* voter_ideals_xy, int n_voters,
                            const SCS_DistanceConfig* dist_cfg, int k,
                            int* out_indices, int out_capacity, int* out_n,
                            char* err_buf, int err_buf_len) {
  if (!alt_xy || !voter_ideals_xy || !out_n) {
    set_error(err_buf, err_buf_len, "scs_heart_2d: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    auto alts = build_voters(alt_xy, n_alts);
    auto voters = build_voters(voter_ideals_xy, n_voters);
    auto h = socialchoicelab::geometry::heart(alts, voters, cfg, k);
    const int n = static_cast<int>(h.size());
    *out_n = n;
    if (!out_indices || out_capacity == 0) return SCS_OK;  // size-query
    if (out_capacity < n) {
      set_error(err_buf, err_buf_len,
                ("scs_heart_2d: buffer too small: need " + std::to_string(n) +
                 " indices, capacity " + std::to_string(out_capacity))
                    .c_str());
      return SCS_ERROR_BUFFER_TOO_SMALL;
    }
    for (int i = 0; i < n; ++i) {
      int idx = find_alt_index(alts, h[static_cast<size_t>(i)]);
      out_indices[i] = (idx >= 0) ? idx : -1;
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

extern "C" int scs_heart_boundary_size_2d(const double* voter_ideals_xy,
                                          int n_voters,
                                          const SCS_DistanceConfig* dist_cfg,
                                          int grid_resolution, int k,
                                          int* out_xy_pairs, char* err_buf,
                                          int err_buf_len) {
  if (!voter_ideals_xy || !out_xy_pairs) {
    set_error(err_buf, err_buf_len,
              "scs_heart_boundary_size_2d: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    auto voters = build_voters(voter_ideals_xy, n_voters);
    auto poly = socialchoicelab::geometry::heart_boundary_2d(
        voters, cfg, grid_resolution, k);
    *out_xy_pairs = static_cast<int>(poly.size());
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_heart_boundary_2d(const double* voter_ideals_xy,
                                     int n_voters,
                                     const SCS_DistanceConfig* dist_cfg,
                                     int grid_resolution, int k, double* out_xy,
                                     int out_capacity, int* out_n,
                                     char* err_buf, int err_buf_len) {
  if (!voter_ideals_xy || !out_n) {
    set_error(err_buf, err_buf_len,
              "scs_heart_boundary_2d: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    auto voters = build_voters(voter_ideals_xy, n_voters);
    auto poly = socialchoicelab::geometry::heart_boundary_2d(
        voters, cfg, grid_resolution, k);
    return export_polygon2e(poly, out_xy, out_capacity, out_n, err_buf,
                            err_buf_len);
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

// ---------------------------------------------------------------------------
// Aggregation — Profile constructors  (C5.2, C5.3, C5.4)
// ---------------------------------------------------------------------------

extern "C" SCS_Profile* scs_profile_build_spatial(
    const double* alt_xy, int n_alts, const double* voter_ideals_xy,
    int n_voters, const SCS_DistanceConfig* dist_cfg, char* err_buf,
    int err_buf_len) {
  if (!alt_xy || !voter_ideals_xy) {
    set_error(err_buf, err_buf_len,
              "scs_profile_build_spatial: null pointer argument");
    return nullptr;
  }
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len)) return nullptr;
  try {
    auto alts = build_voters(alt_xy, n_alts);
    auto voters = build_voters(voter_ideals_xy, n_voters);
    auto* p = new SCS_ProfileImpl{};
    p->profile =
        socialchoicelab::aggregation::build_spatial_profile(alts, voters, cfg);
    return p;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return nullptr;
  } catch (const std::bad_alloc&) {
    set_error(err_buf, err_buf_len, "scs_profile_build_spatial: out of memory");
    return nullptr;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return nullptr;
  }
}

extern "C" SCS_Profile* scs_profile_from_utility_matrix(const double* utilities,
                                                        int n_voters,
                                                        int n_alts,
                                                        char* err_buf,
                                                        int err_buf_len) {
  if (!utilities) {
    set_error(err_buf, err_buf_len,
              "scs_profile_from_utility_matrix: null pointer argument");
    return nullptr;
  }
  if (n_voters < 1 || n_alts < 1) {
    set_error(err_buf, err_buf_len,
              "scs_profile_from_utility_matrix: n_voters and n_alts must be "
              ">= 1");
    return nullptr;
  }
  try {
    // Map the flat C row-major array into an Eigen MatrixXd (column-major).
    Eigen::Map<const Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic,
                                   Eigen::RowMajor>>
        mat(utilities, n_voters, n_alts);
    Eigen::MatrixXd mat_col = mat;
    auto* p = new SCS_ProfileImpl{};
    p->profile =
        socialchoicelab::aggregation::profile_from_utility_matrix(mat_col);
    return p;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return nullptr;
  } catch (const std::bad_alloc&) {
    set_error(err_buf, err_buf_len,
              "scs_profile_from_utility_matrix: out of memory");
    return nullptr;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return nullptr;
  }
}

extern "C" SCS_Profile* scs_profile_impartial_culture(int n_voters, int n_alts,
                                                      SCS_StreamManager* mgr,
                                                      const char* stream_name,
                                                      char* err_buf,
                                                      int err_buf_len) {
  if (!mgr || !stream_name) {
    set_error(err_buf, err_buf_len,
              "scs_profile_impartial_culture: null pointer argument");
    return nullptr;
  }
  try {
    auto& prng = mgr->mgr.get_stream(stream_name);
    auto* p = new SCS_ProfileImpl{};
    p->profile =
        socialchoicelab::aggregation::impartial_culture(n_voters, n_alts, prng);
    return p;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return nullptr;
  } catch (const std::bad_alloc&) {
    set_error(err_buf, err_buf_len,
              "scs_profile_impartial_culture: out of memory");
    return nullptr;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return nullptr;
  }
}

extern "C" SCS_Profile* scs_profile_uniform_spatial(
    int n_voters, int n_alts, int n_dims, double lo, double hi,
    const SCS_DistanceConfig* dist_cfg, SCS_StreamManager* mgr,
    const char* stream_name, char* err_buf, int err_buf_len) {
  if (!mgr || !stream_name) {
    set_error(err_buf, err_buf_len,
              "scs_profile_uniform_spatial: null pointer argument");
    return nullptr;
  }
  if (n_dims != 2) {
    set_error(err_buf, err_buf_len,
              ("scs_profile_uniform_spatial: only n_dims == 2 is supported "
               "(got " +
               std::to_string(n_dims) + ")")
                  .c_str());
    return nullptr;
  }
  if (lo >= hi) {
    set_error(err_buf, err_buf_len,
              ("scs_profile_uniform_spatial: lo must be < hi (got lo=" +
               std::to_string(lo) + ", hi=" + std::to_string(hi) + ")")
                  .c_str());
    return nullptr;
  }
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len)) return nullptr;
  try {
    using socialchoicelab::core::types::Point2d;
    auto& prng = mgr->mgr.get_stream(stream_name);
    // Generate n_alts alternatives from U([lo,hi]²) using the same stream.
    std::vector<Point2d> alts;
    alts.reserve(static_cast<size_t>(n_alts));
    for (int i = 0; i < n_alts; ++i) {
      alts.emplace_back(prng.uniform_real(lo, hi), prng.uniform_real(lo, hi));
    }
    auto* p = new SCS_ProfileImpl{};
    p->profile = socialchoicelab::aggregation::uniform_spatial_profile(
        n_voters, alts, lo, hi, lo, hi, cfg, prng);
    return p;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return nullptr;
  } catch (const std::bad_alloc&) {
    set_error(err_buf, err_buf_len,
              "scs_profile_uniform_spatial: out of memory");
    return nullptr;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return nullptr;
  }
}

extern "C" SCS_Profile* scs_profile_gaussian_spatial(
    int n_voters, int n_alts, int n_dims, double mean, double stddev,
    const SCS_DistanceConfig* dist_cfg, SCS_StreamManager* mgr,
    const char* stream_name, char* err_buf, int err_buf_len) {
  if (!mgr || !stream_name) {
    set_error(err_buf, err_buf_len,
              "scs_profile_gaussian_spatial: null pointer argument");
    return nullptr;
  }
  if (n_dims != 2) {
    set_error(err_buf, err_buf_len,
              ("scs_profile_gaussian_spatial: only n_dims == 2 is supported "
               "(got " +
               std::to_string(n_dims) + ")")
                  .c_str());
    return nullptr;
  }
  if (stddev <= 0.0) {
    set_error(err_buf, err_buf_len,
              ("scs_profile_gaussian_spatial: stddev must be positive (got " +
               std::to_string(stddev) + ")")
                  .c_str());
    return nullptr;
  }
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len)) return nullptr;
  try {
    using socialchoicelab::core::types::Point2d;
    auto& prng = mgr->mgr.get_stream(stream_name);
    // Generate n_alts alternatives from N(mean, stddev²) using the same stream.
    std::vector<Point2d> alts;
    alts.reserve(static_cast<size_t>(n_alts));
    for (int i = 0; i < n_alts; ++i) {
      alts.emplace_back(prng.normal(mean, stddev), prng.normal(mean, stddev));
    }
    const Point2d center(mean, mean);
    auto* p = new SCS_ProfileImpl{};
    p->profile = socialchoicelab::aggregation::gaussian_spatial_profile(
        n_voters, alts, center, stddev, cfg, prng);
    return p;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return nullptr;
  } catch (const std::bad_alloc&) {
    set_error(err_buf, err_buf_len,
              "scs_profile_gaussian_spatial: out of memory");
    return nullptr;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return nullptr;
  }
}

// ---------------------------------------------------------------------------
// Aggregation — Profile lifecycle and inspection  (C5.5)
// ---------------------------------------------------------------------------

extern "C" void scs_profile_destroy(SCS_Profile* p) { delete p; }

extern "C" int scs_profile_dims(const SCS_Profile* p, int* out_n_voters,
                                int* out_n_alts, char* err_buf,
                                int err_buf_len) {
  if (!p || !out_n_voters || !out_n_alts) {
    set_error(err_buf, err_buf_len, "scs_profile_dims: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  *out_n_voters = p->profile.n_voters;
  *out_n_alts = p->profile.n_alts;
  return SCS_OK;
}

extern "C" int scs_profile_get_ranking(const SCS_Profile* p, int voter,
                                       int* out_ranking, int out_len,
                                       char* err_buf, int err_buf_len) {
  if (!p || !out_ranking) {
    set_error(err_buf, err_buf_len,
              "scs_profile_get_ranking: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (voter < 0 || voter >= p->profile.n_voters) {
    set_error(err_buf, err_buf_len,
              ("scs_profile_get_ranking: voter index out of range (got " +
               std::to_string(voter) +
               ", n_voters=" + std::to_string(p->profile.n_voters) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (out_len < p->profile.n_alts) {
    set_error(err_buf, err_buf_len,
              ("scs_profile_get_ranking: out_len must be >= n_alts (got " +
               std::to_string(out_len) + " < " +
               std::to_string(p->profile.n_alts) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  const auto& r = p->profile.rankings[static_cast<size_t>(voter)];
  for (int i = 0; i < p->profile.n_alts; ++i) {
    out_ranking[i] = r[static_cast<size_t>(i)];
  }
  return SCS_OK;
}

// ---------------------------------------------------------------------------
// Aggregation — Profile bulk export  (C5.6)
// ---------------------------------------------------------------------------

extern "C" int scs_profile_export_rankings(const SCS_Profile* p,
                                           int* out_rankings, int out_len,
                                           char* err_buf, int err_buf_len) {
  if (!p || !out_rankings) {
    set_error(err_buf, err_buf_len,
              "scs_profile_export_rankings: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  const int required = p->profile.n_voters * p->profile.n_alts;
  if (out_len < required) {
    set_error(err_buf, err_buf_len,
              ("scs_profile_export_rankings: out_len must be >= n_voters * "
               "n_alts (got " +
               std::to_string(out_len) + " < " + std::to_string(required) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  for (int v = 0; v < p->profile.n_voters; ++v) {
    const auto& r = p->profile.rankings[static_cast<size_t>(v)];
    for (int rank = 0; rank < p->profile.n_alts; ++rank) {
      out_rankings[v * p->profile.n_alts + rank] = r[static_cast<size_t>(rank)];
    }
  }
  return SCS_OK;
}

// ---------------------------------------------------------------------------
// Aggregation — Profile clone  (C5.7)
// ---------------------------------------------------------------------------

extern "C" SCS_Profile* scs_profile_clone(const SCS_Profile* p, char* err_buf,
                                          int err_buf_len) {
  if (!p) {
    set_error(err_buf, err_buf_len, "scs_profile_clone: null pointer argument");
    return nullptr;
  }
  try {
    return new SCS_ProfileImpl{p->profile};
  } catch (const std::bad_alloc&) {
    set_error(err_buf, err_buf_len, "scs_profile_clone: out of memory");
    return nullptr;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return nullptr;
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
