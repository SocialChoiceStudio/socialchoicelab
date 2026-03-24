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

#include <socialchoicelab/aggregation/antiplurality.h>
#include <socialchoicelab/aggregation/approval.h>
#include <socialchoicelab/aggregation/borda.h>
#include <socialchoicelab/aggregation/condorcet_consistency.h>
#include <socialchoicelab/aggregation/pareto.h>
#include <socialchoicelab/aggregation/plurality.h>
#include <socialchoicelab/aggregation/profile.h>
#include <socialchoicelab/aggregation/profile_generators.h>
#include <socialchoicelab/aggregation/scoring_rule.h>
#include <socialchoicelab/aggregation/social_ranking.h>
#include <socialchoicelab/aggregation/tie_break.h>
#include <socialchoicelab/aggregation/voter_sampler.h>
#include <socialchoicelab/competition/experiment_runner.h>
#include <socialchoicelab/geometry/centrality.h>
#include <socialchoicelab/geometry/convex_hull.h>
#include <socialchoicelab/geometry/copeland.h>
#include <socialchoicelab/geometry/core.h>
#include <socialchoicelab/geometry/heart.h>
#include <socialchoicelab/geometry/majority.h>
#include <socialchoicelab/geometry/uncovered_set.h>
#include <socialchoicelab/geometry/voronoi_2d.h>
#include <socialchoicelab/geometry/winset.h>
#include <socialchoicelab/geometry/winset_1d.h>
#include <socialchoicelab/geometry/winset_ops.h>
#include <socialchoicelab/geometry/yolk.h>

#include <Eigen/Dense>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
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

struct SCS_CompetitionTraceImpl {
  socialchoicelab::competition::CompetitionTrace trace;
};

struct SCS_CompetitionExperimentImpl {
  socialchoicelab::competition::CompetitionExperimentResult result;
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

// Validate that all elements in p[0..count-1] are finite (no NaN or Inf).
// Returns false and sets err_buf on first non-finite value; true otherwise.
// If p is null or count <= 0, returns true (nothing to validate).
bool validate_finite_doubles(const double* p, int count, const char* context,
                             char* err_buf, int err_buf_len) {
  if (!p || count <= 0) return true;
  for (int i = 0; i < count; ++i) {
    if (!std::isfinite(p[i])) {
      set_error(
          err_buf, err_buf_len,
          (std::string(context) + ": non-finite value at index " +
           std::to_string(i) +
           " (NaN or Inf). All coordinates and numeric inputs must be finite.")
              .c_str());
      return false;
    }
  }
  return true;
}

int copy_int_vector(const std::vector<int>& values, int* out, int out_len,
                    char* err_buf, int err_buf_len, const char* label) {
  if (!out) {
    set_error(
        err_buf, err_buf_len,
        (std::string(label) + ": output buffer must not be null").c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (out_len < static_cast<int>(values.size())) {
    set_error(err_buf, err_buf_len,
              (std::string(label) + ": output buffer is too small").c_str());
    return SCS_ERROR_BUFFER_TOO_SMALL;
  }
  for (size_t i = 0; i < values.size(); ++i) out[i] = values[i];
  return SCS_OK;
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
  if (!validate_finite_doubles(dist_cfg->salience_weights, dist_cfg->n_weights,
                               "dist_cfg->salience_weights", err_buf,
                               err_buf_len)) {
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

bool to_competition_dist_config(const SCS_DistanceConfig* dist_cfg, int n_dims,
                                socialchoicelab::geometry::DistConfig& out,
                                char* err_buf, int err_buf_len) {
  using DT = socialchoicelab::preference::distance::DistanceType;
  if (!dist_cfg) {
    set_error(err_buf, err_buf_len, "dist_cfg must not be null");
    return false;
  }
  if (dist_cfg->n_weights != n_dims) {
    set_error(err_buf, err_buf_len,
              "dist_cfg->n_weights must match n_dims for competition");
    return false;
  }
  if (!dist_cfg->salience_weights) {
    set_error(err_buf, err_buf_len,
              "dist_cfg->salience_weights must not be null");
    return false;
  }
  if (!validate_finite_doubles(dist_cfg->salience_weights, dist_cfg->n_weights,
                               "dist_cfg->salience_weights", err_buf,
                               err_buf_len)) {
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
  out.salience_weights.assign(dist_cfg->salience_weights,
                              dist_cfg->salience_weights + dist_cfg->n_weights);
  return true;
}

std::vector<socialchoicelab::core::types::PointNd> build_points_nd(
    const double* values, int n_points, int n_dims) {
  std::vector<socialchoicelab::core::types::PointNd> pts;
  pts.reserve(static_cast<std::size_t>(n_points));
  for (int i = 0; i < n_points; ++i) {
    socialchoicelab::core::types::PointNd p(n_dims);
    for (int d = 0; d < n_dims; ++d) p[d] = values[i * n_dims + d];
    pts.push_back(std::move(p));
  }
  return pts;
}

socialchoicelab::competition::CompetitorStrategyKind to_cpp_comp_strategy(
    int kind) {
  using K = socialchoicelab::competition::CompetitorStrategyKind;
  switch (kind) {
    case SCS_COMPETITION_STRATEGY_STICKER:
      return K::kSticker;
    case SCS_COMPETITION_STRATEGY_HUNTER:
      return K::kHunter;
    case SCS_COMPETITION_STRATEGY_AGGREGATOR:
      return K::kAggregator;
    case SCS_COMPETITION_STRATEGY_PREDATOR:
      return K::kPredator;
    case SCS_COMPETITION_STRATEGY_HUNTER_STICKER:
      return K::kHunterSticker;
    default:
      throw std::invalid_argument("unknown SCS_CompetitionStrategyKind value");
  }
}

socialchoicelab::competition::SeatRuleKind to_cpp_seat_rule(
    SCS_CompetitionSeatRule rule) {
  using R = socialchoicelab::competition::SeatRuleKind;
  switch (rule) {
    case SCS_COMPETITION_SEAT_RULE_PLURALITY_TOP_K:
      return R::kPluralityTopK;
    case SCS_COMPETITION_SEAT_RULE_HARE_LARGEST_REMAINDER:
      return R::kHareLargestRemainder;
    default:
      return R::kPluralityTopK;
  }
}

socialchoicelab::competition::StepPolicyConfig to_cpp_step_config(
    const SCS_CompetitionStepConfig& cfg) {
  using K = socialchoicelab::competition::StepPolicyKind;
  socialchoicelab::competition::StepPolicyConfig out;
  switch (cfg.kind) {
    case SCS_COMPETITION_STEP_FIXED:
      out.kind = K::kFixed;
      break;
    case SCS_COMPETITION_STEP_RANDOM_UNIFORM:
      out.kind = K::kRandomUniform;
      break;
    case SCS_COMPETITION_STEP_SHARE_DELTA_PROPORTIONAL:
      out.kind = K::kShareDeltaProportional;
      break;
    default:
      throw std::invalid_argument(
          "unknown SCS_CompetitionStepPolicyKind value");
  }
  out.fixed_step_size = cfg.fixed_step_size;
  out.min_step_size = cfg.min_step_size;
  out.max_step_size = cfg.max_step_size;
  out.proportionality_constant = cfg.proportionality_constant;
  out.jitter = cfg.jitter;
  return out;
}

socialchoicelab::competition::BoundaryPolicyKind to_cpp_boundary_policy(
    SCS_CompetitionBoundaryPolicy policy) {
  using B = socialchoicelab::competition::BoundaryPolicyKind;
  switch (policy) {
    case SCS_COMPETITION_BOUNDARY_PROJECT_TO_BOX:
      return B::kProjectToBox;
    case SCS_COMPETITION_BOUNDARY_STAY_PUT:
      return B::kStayPut;
    case SCS_COMPETITION_BOUNDARY_REFLECT:
      return B::kReflect;
    default:
      return B::kProjectToBox;
  }
}

socialchoicelab::competition::CompetitionObjectiveKind to_cpp_objective_kind(
    SCS_CompetitionObjectiveKind kind) {
  using O = socialchoicelab::competition::CompetitionObjectiveKind;
  switch (kind) {
    case SCS_COMPETITION_OBJECTIVE_VOTE_SHARE:
      return O::kVoteShare;
    case SCS_COMPETITION_OBJECTIVE_SEAT_SHARE:
      return O::kSeatShare;
    default:
      return O::kVoteShare;
  }
}

SCS_CompetitionTerminationReason to_c_termination_reason(
    socialchoicelab::competition::TerminationReason reason) {
  switch (reason) {
    case socialchoicelab::competition::TerminationReason::kMaxRounds:
      return SCS_COMPETITION_TERM_MAX_ROUNDS;
    case socialchoicelab::competition::TerminationReason::kConverged:
      return SCS_COMPETITION_TERM_CONVERGED;
    case socialchoicelab::competition::TerminationReason::kCycleDetected:
      return SCS_COMPETITION_TERM_CYCLE_DETECTED;
    case socialchoicelab::competition::TerminationReason::kNoImprovementWindow:
      return SCS_COMPETITION_TERM_NO_IMPROVEMENT_WINDOW;
    case socialchoicelab::competition::TerminationReason::kCount:
      // kCount is a sizing sentinel; it must never appear in a live trace.
      break;
  }
  return SCS_COMPETITION_TERM_MAX_ROUNDS;
}

socialchoicelab::competition::TerminationConfig to_cpp_termination_config(
    const SCS_CompetitionTerminationConfig& cfg) {
  socialchoicelab::competition::TerminationConfig out;
  out.stop_on_convergence = cfg.stop_on_convergence != 0;
  out.position_epsilon = cfg.position_epsilon;
  out.stop_on_cycle = cfg.stop_on_cycle != 0;
  out.cycle_memory = cfg.cycle_memory;
  out.signature_resolution =
      cfg.signature_resolution > 0.0 ? cfg.signature_resolution : 1e-6;
  out.stop_on_no_improvement = cfg.stop_on_no_improvement != 0;
  out.no_improvement_window = cfg.no_improvement_window;
  out.objective_epsilon = cfg.objective_epsilon;
  return out;
}

bool to_cpp_competition_engine_config(
    const SCS_CompetitionEngineConfig* cfg, const SCS_DistanceConfig* dist_cfg,
    int n_dims, socialchoicelab::competition::CompetitionEngineConfig& out,
    char* err_buf, int err_buf_len) {
  if (!cfg) {
    set_error(err_buf, err_buf_len, "engine_cfg must not be null");
    return false;
  }
  out.election_feedback.seat_count = cfg->seat_count;
  out.election_feedback.seat_rule = to_cpp_seat_rule(cfg->seat_rule);
  if (!to_competition_dist_config(dist_cfg, n_dims,
                                  out.election_feedback.distance_config,
                                  err_buf, err_buf_len)) {
    return false;
  }
  out.step_policy = to_cpp_step_config(cfg->step_config);
  out.boundary_policy = to_cpp_boundary_policy(cfg->boundary_policy);
  out.objective_kind = to_cpp_objective_kind(cfg->objective_kind);
  out.max_rounds = cfg->max_rounds;
  out.termination = to_cpp_termination_config(cfg->termination);
  return true;
}

std::vector<socialchoicelab::competition::CompetitorState> build_competitors(
    const double* positions, const double* headings, const int* strategy_kinds,
    int n_competitors, int n_dims) {
  std::vector<socialchoicelab::competition::CompetitorState> competitors;
  competitors.reserve(static_cast<std::size_t>(n_competitors));
  for (int i = 0; i < n_competitors; ++i) {
    socialchoicelab::competition::CompetitorState competitor;
    competitor.id = i;
    competitor.strategy_kind = to_cpp_comp_strategy(strategy_kinds[i]);
    competitor.position = socialchoicelab::core::types::PointNd(n_dims);
    competitor.heading = socialchoicelab::core::types::PointNd(n_dims);
    for (int d = 0; d < n_dims; ++d) {
      competitor.position[d] = positions[i * n_dims + d];
      competitor.heading[d] = headings ? headings[i * n_dims + d] : 0.0;
    }
    competitors.push_back(std::move(competitor));
  }
  return competitors;
}

int copy_positions(
    const std::vector<socialchoicelab::competition::CompetitorState>&
        competitors,
    double* out_positions, int out_len, char* err_buf, int err_buf_len) {
  const int n_competitors = static_cast<int>(competitors.size());
  const int n_dims =
      n_competitors == 0 ? 0 : static_cast<int>(competitors[0].position.size());
  const int needed = n_competitors * n_dims;
  if (!out_positions) {
    set_error(err_buf, err_buf_len, "positions output buffer must not be null");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (out_len < needed) {
    set_error(err_buf, err_buf_len, "positions output buffer is too small");
    return SCS_ERROR_BUFFER_TOO_SMALL;
  }
  for (int i = 0; i < n_competitors; ++i) {
    for (int d = 0; d < n_dims; ++d) {
      out_positions[i * n_dims + d] = competitors[i].position[d];
    }
  }
  return SCS_OK;
}

int copy_double_vector(const std::vector<double>& values, double* out,
                       int out_len, char* err_buf, int err_buf_len,
                       const char* label) {
  if (!out) {
    set_error(
        err_buf, err_buf_len,
        (std::string(label) + ": output buffer must not be null").c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (out_len < static_cast<int>(values.size())) {
    set_error(err_buf, err_buf_len,
              (std::string(label) + ": output buffer is too small").c_str());
    return SCS_ERROR_BUFFER_TOO_SMALL;
  }
  for (size_t i = 0; i < values.size(); ++i) out[i] = values[i];
  return SCS_OK;
}

std::vector<double> final_metric_shares_from_trace(
    const socialchoicelab::competition::CompetitionTrace& trace,
    bool use_seat_share) {
  std::vector<double> shares;
  shares.reserve(trace.final_competitors.size());
  for (const auto& competitor : trace.final_competitors) {
    if (competitor.current_round_metrics.has_value()) {
      shares.push_back(use_seat_share
                           ? competitor.current_round_metrics->seat_share
                           : competitor.current_round_metrics->vote_share);
    } else {
      shares.push_back(0.0);
    }
  }
  return shares;
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
  if (!validate_finite_doubles(ideal_point, n_dims, "ideal_point", err_buf,
                               err_buf_len) ||
      !validate_finite_doubles(alt_point, n_dims, "alt_point", err_buf,
                               err_buf_len) ||
      !validate_finite_doubles(dist_cfg->salience_weights, n_dims,
                               "dist_cfg->salience_weights", err_buf,
                               err_buf_len)) {
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
  if (!std::isfinite(distance)) {
    set_error(
        err_buf, err_buf_len,
        "scs_distance_to_utility: distance must be finite (no NaN or Inf)");
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
  if (!std::isfinite(utility) || !std::isfinite(max_distance)) {
    set_error(err_buf, err_buf_len,
              "scs_normalize_utility: utility and max_distance must be finite");
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
  if (!std::isfinite(ideal) || !std::isfinite(weight) ||
      !std::isfinite(utility_level)) {
    set_error(
        err_buf, err_buf_len,
        "scs_level_set_1d: ideal, weight, and utility_level must be finite");
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
  if (!std::isfinite(ideal_x) || !std::isfinite(ideal_y) ||
      !std::isfinite(utility_level)) {
    set_error(err_buf, err_buf_len,
              "scs_level_set_2d: ideal_x, ideal_y, and utility_level must be "
              "finite");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (!validate_finite_doubles(dist_cfg->salience_weights, 2,
                               "scs_level_set_2d dist_cfg->salience_weights",
                               err_buf, err_buf_len)) {
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
  {
    const double* d = nullptr;
    int nd = 0;
    switch (level_set->type) {
      case SCS_LEVEL_SET_CIRCLE:
        d = &level_set->center_x;
        nd = 3;  // center_x, center_y, param0
        break;
      case SCS_LEVEL_SET_ELLIPSE:
        d = &level_set->center_x;
        nd = 4;  // center_x, center_y, param0, param1
        break;
      case SCS_LEVEL_SET_SUPERELLIPSE:
        d = &level_set->center_x;
        nd = 5;  // center_x, center_y, param0, param1, exponent_p
        break;
      case SCS_LEVEL_SET_POLYGON:
        d = level_set->vertices;
        nd = level_set->n_vertices * 2;
        if (nd > 8) nd = 8;
        break;
      default:
        break;
    }
    if (d && nd > 0 &&
        !validate_finite_doubles(d, nd, "level_set", err_buf, err_buf_len)) {
      return SCS_ERROR_INVALID_ARGUMENT;
    }
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

extern "C" int scs_ic_polygon_2d(double ideal_x, double ideal_y, double sq_x,
                                 double sq_y, const SCS_LossConfig* loss_cfg,
                                 const SCS_DistanceConfig* dist_cfg,
                                 int num_samples, double* out_xy,
                                 int out_capacity, int* out_n, char* err_buf,
                                 int err_buf_len) {
  if (!loss_cfg || !dist_cfg || !out_n) {
    set_error(err_buf, err_buf_len, "scs_ic_polygon_2d: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (dist_cfg->n_weights != 2 || !dist_cfg->salience_weights) {
    set_error(err_buf, err_buf_len,
              "scs_ic_polygon_2d: dist_cfg must have n_weights == 2");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (!std::isfinite(ideal_x) || !std::isfinite(ideal_y) ||
      !std::isfinite(sq_x) || !std::isfinite(sq_y)) {
    set_error(err_buf, err_buf_len,
              "scs_ic_polygon_2d: coordinates must be finite");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (!validate_finite_doubles(dist_cfg->salience_weights, 2,
                               "scs_ic_polygon_2d dist_cfg->salience_weights",
                               err_buf, err_buf_len)) {
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (num_samples < 3) {
    set_error(err_buf, err_buf_len,
              "scs_ic_polygon_2d: num_samples must be >= 3");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    using socialchoicelab::preference::indifference::DistanceConfig;
    using socialchoicelab::preference::indifference::ic_polygon_2d;
    using socialchoicelab::preference::indifference::LossConfig;
    using socialchoicelab::preference::indifference::Polygon2d;
    LossConfig lcfg = to_cpp_loss_config(*loss_cfg);
    DistanceConfig dcfg = to_cpp_dist_config(*dist_cfg);
    Eigen::Vector2d ideal(ideal_x, ideal_y);
    Eigen::Vector2d sq(sq_x, sq_y);
    Polygon2d poly = ic_polygon_2d(ideal, sq, lcfg, dcfg,
                                   static_cast<std::size_t>(num_samples));
    const int required = static_cast<int>(poly.vertices.size());
    *out_n = required;
    if (out_xy == nullptr) {
      return SCS_OK;
    }
    if (out_capacity < required) {
      set_error(err_buf, err_buf_len,
                "scs_ic_polygon_2d: out_xy buffer too small; check *out_n "
                "for the required number of (x,y) pairs");
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
    if (!validate_finite_doubles(points, n_points * 2, "points", err_buf,
                                 err_buf_len)) {
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
  if (!validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len)) {
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (!std::isfinite(point_a_x) || !std::isfinite(point_a_y) ||
      !std::isfinite(point_b_x) || !std::isfinite(point_b_y)) {
    set_error(err_buf, err_buf_len,
              "scs_majority_prefers_2d: point coordinates must be finite");
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
  if (!validate_finite_doubles(alt_xy, n_alts * 2, "alt_xy", err_buf,
                               err_buf_len) ||
      !validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len)) {
    return SCS_ERROR_INVALID_ARGUMENT;
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
  if (!validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len) ||
      !validate_finite_doubles(weights, n_voters, "weights", err_buf,
                               err_buf_len)) {
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (!std::isfinite(point_a_x) || !std::isfinite(point_a_y) ||
      !std::isfinite(point_b_x) || !std::isfinite(point_b_y) ||
      !std::isfinite(threshold)) {
    set_error(err_buf, err_buf_len,
              "scs_weighted_majority_prefers_2d: point coordinates and "
              "threshold must be finite");
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

// Shared null/size/finite validation used by all three winset factories.
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
  if (!validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len)) {
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
  if (!std::isfinite(status_quo_x) || !std::isfinite(status_quo_y)) {
    set_error(err_buf, err_buf_len,
              "scs_winset_2d: status_quo coordinates must be finite");
    return nullptr;
  }
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
  if (!validate_finite_doubles(weights, n_voters, "weights", err_buf,
                               err_buf_len)) {
    return nullptr;
  }
  if (!std::isfinite(status_quo_x) || !std::isfinite(status_quo_y) ||
      !std::isfinite(threshold)) {
    set_error(
        err_buf, err_buf_len,
        "scs_weighted_winset_2d: status_quo and threshold must be finite");
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
  if (n_veto > 0 &&
      !validate_finite_doubles(veto_ideals_xy, n_veto * 2, "veto_ideals_xy",
                               err_buf, err_buf_len)) {
    return nullptr;
  }
  if (!std::isfinite(status_quo_x) || !std::isfinite(status_quo_y)) {
    set_error(err_buf, err_buf_len,
              "scs_winset_with_veto_2d: status_quo coordinates must be finite");
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
// Euclidean WinSet interval 1D  (C2.9)
// ---------------------------------------------------------------------------

extern "C" int scs_winset_interval_1d(const double* voter_x, int n_voters,
                                      double seat_x, double* out_lo,
                                      double* out_hi, char* err_buf,
                                      int err_buf_len) {
  if (!voter_x || !out_lo || !out_hi) {
    set_error(err_buf, err_buf_len,
              "scs_winset_interval_1d: null required argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (n_voters < 1) {
    set_error(err_buf, err_buf_len,
              "scs_winset_interval_1d: n_voters must be >= 1");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    auto result = socialchoicelab::geometry::winset_interval_1d(
        voter_x, n_voters, seat_x);
    *out_lo = result.lo;
    *out_hi = result.hi;
    return SCS_OK;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

// ---------------------------------------------------------------------------
// Voronoi cells 2D  (C2.8)
// ---------------------------------------------------------------------------

extern "C" int scs_voronoi_cells_2d_size(const double* sites_xy, int n_sites,
                                         double bbox_min_x, double bbox_min_y,
                                         double bbox_max_x, double bbox_max_y,
                                         int* out_total_xy_pairs,
                                         int* out_n_cells, char* err_buf,
                                         int err_buf_len) {
  if (!sites_xy || !out_total_xy_pairs || !out_n_cells) {
    set_error(err_buf, err_buf_len, "scs_voronoi_cells_2d_size: null argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    auto cells = socialchoicelab::geometry::voronoi_cells_2d(
        sites_xy, n_sites, bbox_min_x, bbox_min_y, bbox_max_x, bbox_max_y);
    int total = 0;
    for (const auto& cell : cells) total += static_cast<int>(cell.size());
    *out_total_xy_pairs = total;
    *out_n_cells = static_cast<int>(cells.size());
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_voronoi_cells_2d(const double* sites_xy, int n_sites,
                                    double bbox_min_x, double bbox_min_y,
                                    double bbox_max_x, double bbox_max_y,
                                    double* out_xy, int out_xy_capacity,
                                    int* out_xy_n, int* out_cell_start,
                                    int out_cell_start_capacity, char* err_buf,
                                    int err_buf_len) {
  if (!sites_xy || !out_xy_n) {
    set_error(err_buf, err_buf_len,
              "scs_voronoi_cells_2d: null required argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    auto cells = socialchoicelab::geometry::voronoi_cells_2d(
        sites_xy, n_sites, bbox_min_x, bbox_min_y, bbox_max_x, bbox_max_y);
    const int n_cells = static_cast<int>(cells.size());
    const int required_cell_start = n_sites + 1;
    int total_pairs = 0;
    for (const auto& cell : cells) total_pairs += static_cast<int>(cell.size());

    *out_xy_n = total_pairs;

    const bool size_query_xy = (out_xy == nullptr || out_xy_capacity == 0);
    const bool size_query_cells =
        (out_cell_start == nullptr || out_cell_start_capacity == 0);
    if (size_query_xy && size_query_cells) return SCS_OK;

    if (!size_query_xy && out_xy_capacity < total_pairs) {
      set_error(err_buf, err_buf_len,
                "scs_voronoi_cells_2d: out_xy buffer too small");
      return SCS_ERROR_BUFFER_TOO_SMALL;
    }
    if (!size_query_cells && out_cell_start_capacity < required_cell_start) {
      set_error(err_buf, err_buf_len,
                "scs_voronoi_cells_2d: out_cell_start buffer too small");
      return SCS_ERROR_BUFFER_TOO_SMALL;
    }

    int xy_pos = 0;
    for (int c = 0; c < n_cells; ++c) {
      if (!size_query_cells) out_cell_start[c] = xy_pos;
      const auto& cell = cells[static_cast<size_t>(c)];
      if (!size_query_xy) {
        for (const auto& p : cell) {
          out_xy[2 * xy_pos] = p.x();
          out_xy[2 * xy_pos + 1] = p.y();
          ++xy_pos;
        }
      } else {
        xy_pos += static_cast<int>(cell.size());
      }
    }
    if (!size_query_cells) out_cell_start[n_cells] = xy_pos;

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
  if (!validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len)) {
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
  if (!validate_finite_doubles(alt_xy, n_alts * 2, "alt_xy", err_buf,
                               err_buf_len) ||
      !validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len)) {
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
  if (!validate_finite_doubles(alt_xy, n_alts * 2, "alt_xy", err_buf,
                               err_buf_len) ||
      !validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len)) {
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
  if (!validate_finite_doubles(alt_xy, n_alts * 2, "alt_xy", err_buf,
                               err_buf_len) ||
      !validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len)) {
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
  if (!validate_finite_doubles(alt_xy, n_alts * 2, "alt_xy", err_buf,
                               err_buf_len) ||
      !validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len)) {
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
  if (!validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len)) {
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
  if (!validate_finite_doubles(alt_xy, n_alts * 2, "alt_xy", err_buf,
                               err_buf_len) ||
      !validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len)) {
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
  if (!validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len)) {
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
  if (!validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len)) {
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
  if (!validate_finite_doubles(alt_xy, n_alts * 2, "alt_xy", err_buf,
                               err_buf_len) ||
      !validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len)) {
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
  if (!validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len)) {
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
  if (!validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len)) {
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
  if (!validate_finite_doubles(alt_xy, n_alts * 2, "alt_xy", err_buf,
                               err_buf_len) ||
      !validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len)) {
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
  if (!validate_finite_doubles(utilities, n_voters * n_alts, "utilities",
                               err_buf, err_buf_len)) {
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
  if (!std::isfinite(lo) || !std::isfinite(hi)) {
    set_error(err_buf, err_buf_len,
              "scs_profile_uniform_spatial: lo and hi must be finite");
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
  if (!std::isfinite(mean) || !std::isfinite(stddev)) {
    set_error(err_buf, err_buf_len,
              "scs_profile_gaussian_spatial: mean and stddev must be finite");
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
// Voter sampling  (C5.6)
// ---------------------------------------------------------------------------

extern "C" int scs_draw_voters(int n_voters, int n_dims,
                               const SCS_VoterSamplerConfig* config,
                               SCS_StreamManager* mgr, const char* stream_name,
                               double* out_xy, int out_len, char* err_buf,
                               int err_buf_len) {
  if (!config || !mgr || !stream_name || !out_xy) {
    set_error(err_buf, err_buf_len, "scs_draw_voters: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (n_dims != 2) {
    set_error(err_buf, err_buf_len,
              ("scs_draw_voters: only n_dims == 2 is supported (got " +
               std::to_string(n_dims) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (n_voters < 1) {
    set_error(err_buf, err_buf_len,
              ("scs_draw_voters: n_voters must be >= 1 (got " +
               std::to_string(n_voters) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  const int needed = n_voters * n_dims;
  if (out_len < needed) {
    set_error(
        err_buf, err_buf_len,
        ("scs_draw_voters: out_len too small (need " + std::to_string(needed) +
         ", got " + std::to_string(out_len) + ")")
            .c_str());
    return SCS_ERROR_BUFFER_TOO_SMALL;
  }

  socialchoicelab::aggregation::VoterSamplerKind kind;
  switch (config->kind) {
    case SCS_VOTER_SAMPLER_UNIFORM:
      kind = socialchoicelab::aggregation::VoterSamplerKind::Uniform;
      break;
    case SCS_VOTER_SAMPLER_GAUSSIAN:
      kind = socialchoicelab::aggregation::VoterSamplerKind::Gaussian;
      break;
    default:
      set_error(err_buf, err_buf_len,
                "scs_draw_voters: unknown SCS_VoterSamplerKind");
      return SCS_ERROR_INVALID_ARGUMENT;
  }
  const socialchoicelab::aggregation::VoterSamplerConfig cpp_cfg{
    kind, config->param1, config->param2};
  try {
    auto& prng = mgr->mgr.get_stream(stream_name);
    auto coords = socialchoicelab::aggregation::draw_voters(n_voters, n_dims,
                                                            cpp_cfg, prng);
    for (int i = 0; i < needed; ++i)
      out_xy[i] = coords[static_cast<std::size_t>(i)];
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::bad_alloc&) {
    set_error(err_buf, err_buf_len, "scs_draw_voters: out of memory");
    return SCS_ERROR_OUT_OF_MEMORY;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
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
// Aggregation — C6 internal helpers
// ---------------------------------------------------------------------------

namespace {

using CppTieBreak = socialchoicelab::aggregation::TieBreak;
using CppPRNG = socialchoicelab::core::rng::PRNG;

// Convert SCS_TieBreak to the C++ enum.
CppTieBreak to_cpp_tie_break(SCS_TieBreak tb) {
  return (tb == SCS_TIEBREAK_SMALLEST_INDEX) ? CppTieBreak::kSmallestIndex
                                             : CppTieBreak::kRandom;
}

// Resolve a PRNG* for _one_winner calls.
// Returns false and sets error if kRandom is requested but mgr/stream_name
// are null.
bool resolve_prng(SCS_TieBreak tb, SCS_StreamManager* mgr,
                  const char* stream_name, CppPRNG** out_prng, char* err_buf,
                  int err_buf_len) {
  if (tb == SCS_TIEBREAK_RANDOM) {
    if (!mgr || !stream_name) {
      set_error(err_buf, err_buf_len,
                "SCS_TIEBREAK_RANDOM requires a non-null SCS_StreamManager "
                "and stream_name");
      return false;
    }
    *out_prng = &mgr->mgr.get_stream(stream_name);
  } else {
    *out_prng = nullptr;
  }
  return true;
}

// Copy a vector<int> winners list into a caller buffer with size-query support.
int fill_int_winners(const std::vector<int>& winners, int* out_winners,
                     int out_capacity, int* out_n, char* err_buf,
                     int err_buf_len) {
  const int n = static_cast<int>(winners.size());
  if (out_n) *out_n = n;
  if (!out_winners || out_capacity == 0) return SCS_OK;  // size-query
  if (out_capacity < n) {
    set_error(err_buf, err_buf_len,
              ("output buffer too small: need " + std::to_string(n) +
               " winner indices, capacity " + std::to_string(out_capacity))
                  .c_str());
    return SCS_ERROR_BUFFER_TOO_SMALL;
  }
  for (int i = 0; i < n; ++i) out_winners[i] = winners[i];
  return SCS_OK;
}

}  // anonymous namespace

// ---------------------------------------------------------------------------
// C6.1 — Plurality
// ---------------------------------------------------------------------------

extern "C" int scs_plurality_scores(const SCS_Profile* p, int* out_scores,
                                    int out_len, char* err_buf,
                                    int err_buf_len) {
  if (!p || !out_scores) {
    set_error(err_buf, err_buf_len,
              "scs_plurality_scores: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (out_len < p->profile.n_alts) {
    set_error(err_buf, err_buf_len,
              ("scs_plurality_scores: out_len must be >= n_alts (got " +
               std::to_string(out_len) + " < " +
               std::to_string(p->profile.n_alts) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    auto s = socialchoicelab::aggregation::plurality_scores(p->profile);
    for (int i = 0; i < static_cast<int>(s.size()); ++i) out_scores[i] = s[i];
    return SCS_OK;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_plurality_all_winners(const SCS_Profile* p, int* out_winners,
                                         int out_capacity, int* out_n,
                                         char* err_buf, int err_buf_len) {
  if (!p || !out_n) {
    set_error(err_buf, err_buf_len,
              "scs_plurality_all_winners: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    auto w = socialchoicelab::aggregation::plurality_all_winners(p->profile);
    return fill_int_winners(w, out_winners, out_capacity, out_n, err_buf,
                            err_buf_len);
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_plurality_one_winner(
    const SCS_Profile* p, SCS_TieBreak tie_break, SCS_StreamManager* mgr,
    const char* stream_name, int* out_winner, char* err_buf, int err_buf_len) {
  if (!p || !out_winner) {
    set_error(err_buf, err_buf_len,
              "scs_plurality_one_winner: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  CppPRNG* prng = nullptr;
  if (!resolve_prng(tie_break, mgr, stream_name, &prng, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    *out_winner = socialchoicelab::aggregation::plurality_one_winner(
        p->profile, to_cpp_tie_break(tie_break), prng);
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
// C6.2 — Borda Count
// ---------------------------------------------------------------------------

extern "C" int scs_borda_scores(const SCS_Profile* p, int* out_scores,
                                int out_len, char* err_buf, int err_buf_len) {
  if (!p || !out_scores) {
    set_error(err_buf, err_buf_len, "scs_borda_scores: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (out_len < p->profile.n_alts) {
    set_error(err_buf, err_buf_len,
              ("scs_borda_scores: out_len must be >= n_alts (got " +
               std::to_string(out_len) + " < " +
               std::to_string(p->profile.n_alts) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    auto s = socialchoicelab::aggregation::borda_scores(p->profile);
    for (int i = 0; i < static_cast<int>(s.size()); ++i) out_scores[i] = s[i];
    return SCS_OK;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_borda_all_winners(const SCS_Profile* p, int* out_winners,
                                     int out_capacity, int* out_n,
                                     char* err_buf, int err_buf_len) {
  if (!p || !out_n) {
    set_error(err_buf, err_buf_len,
              "scs_borda_all_winners: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    auto w = socialchoicelab::aggregation::borda_all_winners(p->profile);
    return fill_int_winners(w, out_winners, out_capacity, out_n, err_buf,
                            err_buf_len);
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_borda_one_winner(const SCS_Profile* p,
                                    SCS_TieBreak tie_break,
                                    SCS_StreamManager* mgr,
                                    const char* stream_name, int* out_winner,
                                    char* err_buf, int err_buf_len) {
  if (!p || !out_winner) {
    set_error(err_buf, err_buf_len,
              "scs_borda_one_winner: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  CppPRNG* prng = nullptr;
  if (!resolve_prng(tie_break, mgr, stream_name, &prng, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    *out_winner = socialchoicelab::aggregation::borda_one_winner(
        p->profile, to_cpp_tie_break(tie_break), prng);
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_borda_ranking(const SCS_Profile* p, SCS_TieBreak tie_break,
                                 SCS_StreamManager* mgr,
                                 const char* stream_name, int* out_ranking,
                                 int out_len, char* err_buf, int err_buf_len) {
  if (!p || !out_ranking) {
    set_error(err_buf, err_buf_len, "scs_borda_ranking: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (out_len < p->profile.n_alts) {
    set_error(err_buf, err_buf_len,
              ("scs_borda_ranking: out_len must be >= n_alts (got " +
               std::to_string(out_len) + " < " +
               std::to_string(p->profile.n_alts) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  CppPRNG* prng = nullptr;
  if (!resolve_prng(tie_break, mgr, stream_name, &prng, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    auto r = socialchoicelab::aggregation::borda_ranking(
        p->profile, to_cpp_tie_break(tie_break), prng);
    for (int i = 0; i < static_cast<int>(r.size()); ++i) out_ranking[i] = r[i];
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
// C6.3 — Anti-plurality
// ---------------------------------------------------------------------------

extern "C" int scs_antiplurality_scores(const SCS_Profile* p, int* out_scores,
                                        int out_len, char* err_buf,
                                        int err_buf_len) {
  if (!p || !out_scores) {
    set_error(err_buf, err_buf_len,
              "scs_antiplurality_scores: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (out_len < p->profile.n_alts) {
    set_error(err_buf, err_buf_len,
              ("scs_antiplurality_scores: out_len must be >= n_alts (got " +
               std::to_string(out_len) + " < " +
               std::to_string(p->profile.n_alts) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    auto s = socialchoicelab::aggregation::antiplurality_scores(p->profile);
    for (int i = 0; i < static_cast<int>(s.size()); ++i) out_scores[i] = s[i];
    return SCS_OK;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_antiplurality_all_winners(const SCS_Profile* p,
                                             int* out_winners, int out_capacity,
                                             int* out_n, char* err_buf,
                                             int err_buf_len) {
  if (!p || !out_n) {
    set_error(err_buf, err_buf_len,
              "scs_antiplurality_all_winners: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    auto w =
        socialchoicelab::aggregation::antiplurality_all_winners(p->profile);
    return fill_int_winners(w, out_winners, out_capacity, out_n, err_buf,
                            err_buf_len);
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_antiplurality_one_winner(
    const SCS_Profile* p, SCS_TieBreak tie_break, SCS_StreamManager* mgr,
    const char* stream_name, int* out_winner, char* err_buf, int err_buf_len) {
  if (!p || !out_winner) {
    set_error(err_buf, err_buf_len,
              "scs_antiplurality_one_winner: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  CppPRNG* prng = nullptr;
  if (!resolve_prng(tie_break, mgr, stream_name, &prng, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    *out_winner = socialchoicelab::aggregation::antiplurality_one_winner(
        p->profile, to_cpp_tie_break(tie_break), prng);
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
// C6.4 — Generic positional scoring rule
// ---------------------------------------------------------------------------

extern "C" int scs_scoring_rule_scores(const SCS_Profile* p,
                                       const double* score_weights,
                                       int n_weights, double* out_scores,
                                       int out_len, char* err_buf,
                                       int err_buf_len) {
  if (!p || !score_weights || !out_scores) {
    set_error(err_buf, err_buf_len,
              "scs_scoring_rule_scores: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (!validate_finite_doubles(score_weights, n_weights, "score_weights",
                               err_buf, err_buf_len)) {
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (out_len < p->profile.n_alts) {
    set_error(err_buf, err_buf_len,
              ("scs_scoring_rule_scores: out_len must be >= n_alts (got " +
               std::to_string(out_len) + " < " +
               std::to_string(p->profile.n_alts) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    std::vector<double> sv(score_weights, score_weights + n_weights);
    auto s = socialchoicelab::aggregation::scoring_rule_scores(p->profile, sv);
    for (int i = 0; i < static_cast<int>(s.size()); ++i) out_scores[i] = s[i];
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_scoring_rule_all_winners(const SCS_Profile* p,
                                            const double* score_weights,
                                            int n_weights, int* out_winners,
                                            int out_capacity, int* out_n,
                                            char* err_buf, int err_buf_len) {
  if (!p || !score_weights || !out_n) {
    set_error(err_buf, err_buf_len,
              "scs_scoring_rule_all_winners: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (!validate_finite_doubles(score_weights, n_weights, "score_weights",
                               err_buf, err_buf_len)) {
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    std::vector<double> sv(score_weights, score_weights + n_weights);
    auto w =
        socialchoicelab::aggregation::scoring_rule_all_winners(p->profile, sv);
    return fill_int_winners(w, out_winners, out_capacity, out_n, err_buf,
                            err_buf_len);
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_scoring_rule_one_winner(
    const SCS_Profile* p, const double* score_weights, int n_weights,
    SCS_TieBreak tie_break, SCS_StreamManager* mgr, const char* stream_name,
    int* out_winner, char* err_buf, int err_buf_len) {
  if (!p || !score_weights || !out_winner) {
    set_error(err_buf, err_buf_len,
              "scs_scoring_rule_one_winner: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (!validate_finite_doubles(score_weights, n_weights, "score_weights",
                               err_buf, err_buf_len)) {
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  CppPRNG* prng = nullptr;
  if (!resolve_prng(tie_break, mgr, stream_name, &prng, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    std::vector<double> sv(score_weights, score_weights + n_weights);
    *out_winner = socialchoicelab::aggregation::scoring_rule_one_winner(
        p->profile, sv, to_cpp_tie_break(tie_break), prng);
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
// C6.5 — Approval voting
// ---------------------------------------------------------------------------

extern "C" int scs_approval_scores_spatial(const double* alt_xy, int n_alts,
                                           const double* voter_ideals_xy,
                                           int n_voters, double threshold,
                                           const SCS_DistanceConfig* dist_cfg,
                                           int* out_scores, int out_len,
                                           char* err_buf, int err_buf_len) {
  if (!alt_xy || !voter_ideals_xy || !out_scores) {
    set_error(err_buf, err_buf_len,
              "scs_approval_scores_spatial: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (out_len < n_alts) {
    set_error(err_buf, err_buf_len,
              ("scs_approval_scores_spatial: out_len must be >= n_alts (got " +
               std::to_string(out_len) + " < " + std::to_string(n_alts) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (!validate_finite_doubles(alt_xy, n_alts * 2, "alt_xy", err_buf,
                               err_buf_len) ||
      !validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len)) {
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (!std::isfinite(threshold)) {
    set_error(err_buf, err_buf_len,
              "scs_approval_scores_spatial: threshold must be finite");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    auto alts = build_voters(alt_xy, n_alts);
    auto voters = build_voters(voter_ideals_xy, n_voters);
    auto s = socialchoicelab::aggregation::approval_scores_spatial(
        alts, voters, cfg, threshold);
    for (int i = 0; i < static_cast<int>(s.size()); ++i) out_scores[i] = s[i];
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_approval_all_winners_spatial(
    const double* alt_xy, int n_alts, const double* voter_ideals_xy,
    int n_voters, double threshold, const SCS_DistanceConfig* dist_cfg,
    int* out_winners, int out_capacity, int* out_n, char* err_buf,
    int err_buf_len) {
  if (!alt_xy || !voter_ideals_xy || !out_n) {
    set_error(err_buf, err_buf_len,
              "scs_approval_all_winners_spatial: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (!validate_finite_doubles(alt_xy, n_alts * 2, "alt_xy", err_buf,
                               err_buf_len) ||
      !validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len)) {
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (!std::isfinite(threshold)) {
    set_error(err_buf, err_buf_len,
              "scs_approval_all_winners_spatial: threshold must be finite");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  socialchoicelab::geometry::DistConfig cfg;
  if (!to_dist_config(dist_cfg, cfg, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    auto alts = build_voters(alt_xy, n_alts);
    auto voters = build_voters(voter_ideals_xy, n_voters);
    auto w = socialchoicelab::aggregation::approval_all_winners_spatial(
        alts, voters, cfg, threshold);
    return fill_int_winners(w, out_winners, out_capacity, out_n, err_buf,
                            err_buf_len);
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_approval_scores_topk(const SCS_Profile* p, int k,
                                        int* out_scores, int out_len,
                                        char* err_buf, int err_buf_len) {
  if (!p || !out_scores) {
    set_error(err_buf, err_buf_len,
              "scs_approval_scores_topk: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (out_len < p->profile.n_alts) {
    set_error(err_buf, err_buf_len,
              ("scs_approval_scores_topk: out_len must be >= n_alts (got " +
               std::to_string(out_len) + " < " +
               std::to_string(p->profile.n_alts) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    auto s = socialchoicelab::aggregation::approval_scores_topk(p->profile, k);
    for (int i = 0; i < static_cast<int>(s.size()); ++i) out_scores[i] = s[i];
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_approval_all_winners_topk(const SCS_Profile* p, int k,
                                             int* out_winners, int out_capacity,
                                             int* out_n, char* err_buf,
                                             int err_buf_len) {
  if (!p || !out_n) {
    set_error(err_buf, err_buf_len,
              "scs_approval_all_winners_topk: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    auto w =
        socialchoicelab::aggregation::approval_all_winners_topk(p->profile, k);
    return fill_int_winners(w, out_winners, out_capacity, out_n, err_buf,
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

// ---------------------------------------------------------------------------
// C7.1 — Rank by scores
// ---------------------------------------------------------------------------

extern "C" int scs_rank_by_scores(const double* scores, int n_alts,
                                  SCS_TieBreak tie_break,
                                  SCS_StreamManager* mgr,
                                  const char* stream_name, int* out_ranking,
                                  int out_len, char* err_buf, int err_buf_len) {
  if (!scores || !out_ranking) {
    set_error(err_buf, err_buf_len,
              "scs_rank_by_scores: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (n_alts < 1) {
    set_error(err_buf, err_buf_len,
              ("scs_rank_by_scores: n_alts must be >= 1 (got " +
               std::to_string(n_alts) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (out_len < n_alts) {
    set_error(err_buf, err_buf_len,
              ("scs_rank_by_scores: out_len must be >= n_alts (got " +
               std::to_string(out_len) + " < " + std::to_string(n_alts) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (!validate_finite_doubles(scores, n_alts, "scores", err_buf,
                               err_buf_len)) {
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  CppPRNG* prng = nullptr;
  if (!resolve_prng(tie_break, mgr, stream_name, &prng, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    std::vector<double> sv(scores, scores + n_alts);
    auto r = socialchoicelab::aggregation::rank_by_scores(
        sv, to_cpp_tie_break(tie_break), prng);
    for (int i = 0; i < static_cast<int>(r.size()); ++i) out_ranking[i] = r[i];
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
// C7.2 — Pairwise ranking from matrix
// ---------------------------------------------------------------------------

extern "C" int scs_pairwise_ranking_from_matrix(
    const SCS_PairwiseResult* pairwise_matrix, int n_alts,
    SCS_TieBreak tie_break, SCS_StreamManager* mgr, const char* stream_name,
    int* out_ranking, int out_len, char* err_buf, int err_buf_len) {
  if (!pairwise_matrix || !out_ranking) {
    set_error(err_buf, err_buf_len,
              "scs_pairwise_ranking_from_matrix: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (n_alts < 1) {
    set_error(err_buf, err_buf_len,
              ("scs_pairwise_ranking_from_matrix: n_alts must be >= 1 (got " +
               std::to_string(n_alts) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (out_len < n_alts) {
    set_error(err_buf, err_buf_len,
              ("scs_pairwise_ranking_from_matrix: out_len must be >= n_alts "
               "(got " +
               std::to_string(out_len) + " < " + std::to_string(n_alts) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  CppPRNG* prng = nullptr;
  if (!resolve_prng(tie_break, mgr, stream_name, &prng, err_buf, err_buf_len))
    return SCS_ERROR_INVALID_ARGUMENT;
  try {
    Eigen::MatrixXi mat(n_alts, n_alts);
    for (int i = 0; i < n_alts; ++i)
      for (int j = 0; j < n_alts; ++j)
        mat(i, j) = static_cast<int>(pairwise_matrix[i * n_alts + j]);
    auto r = socialchoicelab::aggregation::pairwise_ranking(
        mat, to_cpp_tie_break(tie_break), prng);
    for (int i = 0; i < static_cast<int>(r.size()); ++i) out_ranking[i] = r[i];
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
// C7.3 — Pareto efficiency
// ---------------------------------------------------------------------------

extern "C" int scs_pareto_set(const SCS_Profile* p, int* out_indices,
                              int out_capacity, int* out_n, char* err_buf,
                              int err_buf_len) {
  if (!p || !out_n) {
    set_error(err_buf, err_buf_len, "scs_pareto_set: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    auto ps = socialchoicelab::aggregation::pareto_set(p->profile);
    return fill_int_winners(ps, out_indices, out_capacity, out_n, err_buf,
                            err_buf_len);
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_is_pareto_efficient(const SCS_Profile* p, int alt_idx,
                                       int* out, char* err_buf,
                                       int err_buf_len) {
  if (!p || !out) {
    set_error(err_buf, err_buf_len,
              "scs_is_pareto_efficient: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    *out =
        socialchoicelab::aggregation::is_pareto_efficient(p->profile, alt_idx)
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
// C7.4 — Condorcet and majority-selection predicates
// ---------------------------------------------------------------------------

extern "C" int scs_has_condorcet_winner_profile(const SCS_Profile* p,
                                                int* out_found, char* err_buf,
                                                int err_buf_len) {
  if (!p || !out_found) {
    set_error(err_buf, err_buf_len,
              "scs_has_condorcet_winner_profile: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    *out_found =
        socialchoicelab::aggregation::has_condorcet_winner_profile(p->profile)
            ? 1
            : 0;
    return SCS_OK;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_condorcet_winner_profile(const SCS_Profile* p,
                                            int* out_found, int* out_winner,
                                            char* err_buf, int err_buf_len) {
  if (!p || !out_found) {
    set_error(err_buf, err_buf_len,
              "scs_condorcet_winner_profile: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    const auto cw =
        socialchoicelab::aggregation::condorcet_winner_profile(p->profile);
    *out_found = cw.has_value() ? 1 : 0;
    if (cw.has_value() && out_winner) *out_winner = *cw;
    return SCS_OK;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_is_selected_by_condorcet_consistent_rules(
    const SCS_Profile* p, int alt_idx, int* out, char* err_buf,
    int err_buf_len) {
  if (!p || !out) {
    set_error(err_buf, err_buf_len,
              "scs_is_selected_by_condorcet_consistent_rules: "
              "null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    *out = socialchoicelab::aggregation::is_condorcet_consistent(p->profile,
                                                                 alt_idx)
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

extern "C" int scs_is_selected_by_majority_consistent_rules(
    const SCS_Profile* p, int alt_idx, int* out, char* err_buf,
    int err_buf_len) {
  if (!p || !out) {
    set_error(err_buf, err_buf_len,
              "scs_is_selected_by_majority_consistent_rules: "
              "null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    *out = socialchoicelab::aggregation::is_majority_consistent(p->profile,
                                                                alt_idx)
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
// Centrality measures (C++ centrality.h)
// ---------------------------------------------------------------------------

extern "C" int scs_marginal_median_2d(const double* voter_ideals_xy,
                                      int n_voters, double* out_x,
                                      double* out_y, char* err_buf,
                                      int err_buf_len) {
  if (!voter_ideals_xy || !out_x || !out_y) {
    set_error(err_buf, err_buf_len,
              "scs_marginal_median_2d: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (n_voters < 1) {
    set_error(err_buf, err_buf_len,
              ("scs_marginal_median_2d: n_voters must be >= 1 (got " +
               std::to_string(n_voters) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (!validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len)) {
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    auto voters = build_voters(voter_ideals_xy, n_voters);
    auto pt = socialchoicelab::geometry::marginal_median_2d(voters);
    *out_x = pt.x();
    *out_y = pt.y();
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_centroid_2d(const double* voter_ideals_xy, int n_voters,
                               double* out_x, double* out_y, char* err_buf,
                               int err_buf_len) {
  if (!voter_ideals_xy || !out_x || !out_y) {
    set_error(err_buf, err_buf_len, "scs_centroid_2d: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (n_voters < 1) {
    set_error(err_buf, err_buf_len,
              ("scs_centroid_2d: n_voters must be >= 1 (got " +
               std::to_string(n_voters) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (!validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len)) {
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    auto voters = build_voters(voter_ideals_xy, n_voters);
    auto pt = socialchoicelab::geometry::centroid_2d(voters);
    *out_x = pt.x();
    *out_y = pt.y();
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" int scs_geometric_mean_2d(const double* voter_ideals_xy,
                                     int n_voters, double* out_x, double* out_y,
                                     char* err_buf, int err_buf_len) {
  if (!voter_ideals_xy || !out_x || !out_y) {
    set_error(err_buf, err_buf_len,
              "scs_geometric_mean_2d: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (n_voters < 1) {
    set_error(err_buf, err_buf_len,
              ("scs_geometric_mean_2d: n_voters must be >= 1 (got " +
               std::to_string(n_voters) + ")")
                  .c_str());
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (!validate_finite_doubles(voter_ideals_xy, n_voters * 2, "voter_ideals_xy",
                               err_buf, err_buf_len)) {
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  try {
    auto voters = build_voters(voter_ideals_xy, n_voters);
    auto pt = socialchoicelab::geometry::geometric_mean_2d(voters);
    *out_x = pt.x();
    *out_y = pt.y();
    return SCS_OK;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INVALID_ARGUMENT;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return SCS_ERROR_INTERNAL;
  }
}

extern "C" SCS_CompetitionTrace* scs_competition_run(
    const double* competitor_positions, const double* competitor_headings,
    const int* strategy_kinds, int n_competitors, const double* voter_ideals,
    int n_voters, int n_dims, const SCS_DistanceConfig* dist_cfg,
    const SCS_CompetitionEngineConfig* engine_cfg, SCS_StreamManager* mgr,
    char* err_buf, int err_buf_len) {
  if (!competitor_positions || !strategy_kinds || !voter_ideals || !dist_cfg ||
      !engine_cfg) {
    set_error(err_buf, err_buf_len,
              "scs_competition_run: null pointer argument");
    return nullptr;
  }
  if (n_competitors < 1 || n_voters < 1 || n_dims < 1) {
    set_error(err_buf, err_buf_len,
              "scs_competition_run: n_competitors, n_voters, and n_dims must "
              "all be >= 1");
    return nullptr;
  }
  if (!validate_finite_doubles(competitor_positions, n_competitors * n_dims,
                               "competitor_positions", err_buf, err_buf_len) ||
      (competitor_headings &&
       !validate_finite_doubles(competitor_headings, n_competitors * n_dims,
                                "competitor_headings", err_buf, err_buf_len)) ||
      !validate_finite_doubles(voter_ideals, n_voters * n_dims, "voter_ideals",
                               err_buf, err_buf_len)) {
    return nullptr;
  }
  try {
    socialchoicelab::competition::CompetitionEngineConfig cpp_cfg;
    if (!to_cpp_competition_engine_config(engine_cfg, dist_cfg, n_dims, cpp_cfg,
                                          err_buf, err_buf_len)) {
      return nullptr;
    }
    cpp_cfg.bounds.lower =
        socialchoicelab::core::types::PointNd::Constant(n_dims, -1.0);
    cpp_cfg.bounds.upper =
        socialchoicelab::core::types::PointNd::Constant(n_dims, 1.0);
    // Infer minimal axis-aligned bounds that contain initial competitors and
    // voters.
    for (int d = 0; d < n_dims; ++d) {
      double min_value = std::numeric_limits<double>::infinity();
      double max_value = -std::numeric_limits<double>::infinity();
      for (int i = 0; i < n_competitors; ++i) {
        const double value = competitor_positions[i * n_dims + d];
        min_value = std::min(min_value, value);
        max_value = std::max(max_value, value);
      }
      for (int i = 0; i < n_voters; ++i) {
        const double value = voter_ideals[i * n_dims + d];
        min_value = std::min(min_value, value);
        max_value = std::max(max_value, value);
      }
      cpp_cfg.bounds.lower[d] = min_value - 1.0;
      cpp_cfg.bounds.upper[d] = max_value + 1.0;
    }

    auto competitors =
        build_competitors(competitor_positions, competitor_headings,
                          strategy_kinds, n_competitors, n_dims);
    auto voters = build_points_nd(voter_ideals, n_voters, n_dims);
    auto trace = socialchoicelab::competition::run_fixed_round_competition(
        cpp_cfg, competitors, voters, mgr ? &mgr->mgr : nullptr);
    return new SCS_CompetitionTraceImpl{std::move(trace)};
  } catch (const std::bad_alloc&) {
    set_error(err_buf, err_buf_len, "scs_competition_run: out of memory");
    return nullptr;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return nullptr;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return nullptr;
  }
}

extern "C" void scs_competition_trace_destroy(SCS_CompetitionTrace* trace) {
  delete trace;
}

extern "C" int scs_competition_trace_dims(const SCS_CompetitionTrace* trace,
                                          int* out_n_rounds,
                                          int* out_n_competitors,
                                          int* out_n_dims, char* err_buf,
                                          int err_buf_len) {
  if (!trace || !out_n_rounds || !out_n_competitors || !out_n_dims) {
    set_error(err_buf, err_buf_len,
              "scs_competition_trace_dims: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  *out_n_rounds = static_cast<int>(trace->trace.rounds.size());
  *out_n_competitors = static_cast<int>(trace->trace.final_competitors.size());
  *out_n_dims =
      trace->trace.final_competitors.empty()
          ? 0
          : static_cast<int>(trace->trace.final_competitors[0].position.size());
  return SCS_OK;
}

extern "C" int scs_competition_trace_termination(
    const SCS_CompetitionTrace* trace, int* out_terminated_early,
    SCS_CompetitionTerminationReason* out_reason, char* err_buf,
    int err_buf_len) {
  if (!trace || !out_terminated_early || !out_reason) {
    set_error(err_buf, err_buf_len,
              "scs_competition_trace_termination: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  *out_terminated_early = trace->trace.terminated_early ? 1 : 0;
  *out_reason = to_c_termination_reason(trace->trace.termination_reason);
  return SCS_OK;
}

extern "C" int scs_competition_trace_round_positions(
    const SCS_CompetitionTrace* trace, int round_index, double* out_positions,
    int out_len, char* err_buf, int err_buf_len) {
  if (!trace) {
    set_error(err_buf, err_buf_len,
              "scs_competition_trace_round_positions: trace is null");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (round_index < 0 ||
      round_index >= static_cast<int>(trace->trace.rounds.size())) {
    set_error(
        err_buf, err_buf_len,
        "scs_competition_trace_round_positions: round_index out of range");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  return copy_positions(trace->trace.rounds[round_index].evaluated_competitors,
                        out_positions, out_len, err_buf, err_buf_len);
}

extern "C" int scs_competition_trace_final_positions(
    const SCS_CompetitionTrace* trace, double* out_positions, int out_len,
    char* err_buf, int err_buf_len) {
  if (!trace) {
    set_error(err_buf, err_buf_len,
              "scs_competition_trace_final_positions: trace is null");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  return copy_positions(trace->trace.final_competitors, out_positions, out_len,
                        err_buf, err_buf_len);
}

extern "C" int scs_competition_trace_round_vote_shares(
    const SCS_CompetitionTrace* trace, int round_index, double* out_shares,
    int out_len, char* err_buf, int err_buf_len) {
  if (!trace) {
    set_error(err_buf, err_buf_len,
              "scs_competition_trace_round_vote_shares: trace is null");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (round_index < 0 ||
      round_index >= static_cast<int>(trace->trace.rounds.size())) {
    set_error(
        err_buf, err_buf_len,
        "scs_competition_trace_round_vote_shares: round_index out of range");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  return copy_double_vector(
      trace->trace.rounds[round_index].feedback.vote_shares, out_shares,
      out_len, err_buf, err_buf_len, "scs_competition_trace_round_vote_shares");
}

extern "C" int scs_competition_trace_round_seat_shares(
    const SCS_CompetitionTrace* trace, int round_index, double* out_shares,
    int out_len, char* err_buf, int err_buf_len) {
  if (!trace) {
    set_error(err_buf, err_buf_len,
              "scs_competition_trace_round_seat_shares: trace is null");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (round_index < 0 ||
      round_index >= static_cast<int>(trace->trace.rounds.size())) {
    set_error(
        err_buf, err_buf_len,
        "scs_competition_trace_round_seat_shares: round_index out of range");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  return copy_double_vector(
      trace->trace.rounds[round_index].feedback.seat_shares, out_shares,
      out_len, err_buf, err_buf_len, "scs_competition_trace_round_seat_shares");
}

extern "C" int scs_competition_trace_round_vote_totals(
    const SCS_CompetitionTrace* trace, int round_index, int* out_totals,
    int out_len, char* err_buf, int err_buf_len) {
  if (!trace) {
    set_error(err_buf, err_buf_len,
              "scs_competition_trace_round_vote_totals: trace is null");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (round_index < 0 ||
      round_index >= static_cast<int>(trace->trace.rounds.size())) {
    set_error(
        err_buf, err_buf_len,
        "scs_competition_trace_round_vote_totals: round_index out of range");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  return copy_int_vector(trace->trace.rounds[round_index].feedback.vote_totals,
                         out_totals, out_len, err_buf, err_buf_len,
                         "scs_competition_trace_round_vote_totals");
}

extern "C" int scs_competition_trace_round_seat_totals(
    const SCS_CompetitionTrace* trace, int round_index, int* out_totals,
    int out_len, char* err_buf, int err_buf_len) {
  if (!trace) {
    set_error(err_buf, err_buf_len,
              "scs_competition_trace_round_seat_totals: trace is null");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  if (round_index < 0 ||
      round_index >= static_cast<int>(trace->trace.rounds.size())) {
    set_error(
        err_buf, err_buf_len,
        "scs_competition_trace_round_seat_totals: round_index out of range");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  return copy_int_vector(trace->trace.rounds[round_index].feedback.seats,
                         out_totals, out_len, err_buf, err_buf_len,
                         "scs_competition_trace_round_seat_totals");
}

extern "C" int scs_competition_trace_final_vote_shares(
    const SCS_CompetitionTrace* trace, double* out_shares, int out_len,
    char* err_buf, int err_buf_len) {
  if (!trace) {
    set_error(err_buf, err_buf_len,
              "scs_competition_trace_final_vote_shares: trace is null");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  return copy_double_vector(final_metric_shares_from_trace(trace->trace, false),
                            out_shares, out_len, err_buf, err_buf_len,
                            "scs_competition_trace_final_vote_shares");
}

extern "C" int scs_competition_trace_final_seat_shares(
    const SCS_CompetitionTrace* trace, double* out_shares, int out_len,
    char* err_buf, int err_buf_len) {
  if (!trace) {
    set_error(err_buf, err_buf_len,
              "scs_competition_trace_final_seat_shares: trace is null");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  return copy_double_vector(final_metric_shares_from_trace(trace->trace, true),
                            out_shares, out_len, err_buf, err_buf_len,
                            "scs_competition_trace_final_seat_shares");
}

extern "C" int scs_competition_trace_strategy_kinds(
    const SCS_CompetitionTrace* trace, int* out_kinds, int out_len,
    char* err_buf, int err_buf_len) {
  if (!trace) {
    set_error(err_buf, err_buf_len,
              "scs_competition_trace_strategy_kinds: trace is null");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  const auto& competitors = trace->trace.final_competitors;
  const int n = static_cast<int>(competitors.size());
  if (out_len < n) {
    set_error(err_buf, err_buf_len,
              "scs_competition_trace_strategy_kinds: out_len too small");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  for (int i = 0; i < n; ++i) {
    out_kinds[i] = static_cast<int>(competitors[i].strategy_kind);
  }
  return SCS_OK;
}

extern "C" SCS_CompetitionExperiment* scs_competition_run_experiment(
    const double* competitor_positions, const double* competitor_headings,
    const int* strategy_kinds, int n_competitors, const double* voter_ideals,
    int n_voters, int n_dims, const SCS_DistanceConfig* dist_cfg,
    const SCS_CompetitionEngineConfig* engine_cfg, uint64_t master_seed,
    int num_runs, int retain_traces, char* err_buf, int err_buf_len) {
  if (!competitor_positions || !strategy_kinds || !voter_ideals || !dist_cfg ||
      !engine_cfg) {
    set_error(err_buf, err_buf_len,
              "scs_competition_run_experiment: null pointer argument");
    return nullptr;
  }
  if (n_competitors < 1 || n_voters < 1 || n_dims < 1 || num_runs < 1) {
    set_error(err_buf, err_buf_len,
              "scs_competition_run_experiment: invalid dimensions or num_runs");
    return nullptr;
  }
  try {
    socialchoicelab::competition::CompetitionEngineConfig cpp_engine_cfg;
    if (!to_cpp_competition_engine_config(engine_cfg, dist_cfg, n_dims,
                                          cpp_engine_cfg, err_buf,
                                          err_buf_len)) {
      return nullptr;
    }
    cpp_engine_cfg.bounds.lower =
        socialchoicelab::core::types::PointNd::Constant(n_dims, -1.0);
    cpp_engine_cfg.bounds.upper =
        socialchoicelab::core::types::PointNd::Constant(n_dims, 1.0);
    for (int d = 0; d < n_dims; ++d) {
      double min_value = std::numeric_limits<double>::infinity();
      double max_value = -std::numeric_limits<double>::infinity();
      for (int i = 0; i < n_competitors; ++i) {
        const double value = competitor_positions[i * n_dims + d];
        min_value = std::min(min_value, value);
        max_value = std::max(max_value, value);
      }
      for (int i = 0; i < n_voters; ++i) {
        const double value = voter_ideals[i * n_dims + d];
        min_value = std::min(min_value, value);
        max_value = std::max(max_value, value);
      }
      cpp_engine_cfg.bounds.lower[d] = min_value - 1.0;
      cpp_engine_cfg.bounds.upper[d] = max_value + 1.0;
    }
    socialchoicelab::competition::CompetitionExperimentConfig cpp_cfg;
    cpp_cfg.engine_config = cpp_engine_cfg;
    cpp_cfg.initial_competitors =
        build_competitors(competitor_positions, competitor_headings,
                          strategy_kinds, n_competitors, n_dims);
    cpp_cfg.voter_ideals = build_points_nd(voter_ideals, n_voters, n_dims);
    cpp_cfg.master_seed = master_seed;
    cpp_cfg.num_runs = num_runs;
    cpp_cfg.retain_traces = retain_traces != 0;
    auto result =
        socialchoicelab::competition::run_competition_experiment(cpp_cfg);
    return new SCS_CompetitionExperimentImpl{std::move(result)};
  } catch (const std::bad_alloc&) {
    set_error(err_buf, err_buf_len,
              "scs_competition_run_experiment: out of memory");
    return nullptr;
  } catch (const std::invalid_argument& e) {
    set_error(err_buf, err_buf_len, e.what());
    return nullptr;
  } catch (const std::exception& e) {
    set_error(err_buf, err_buf_len, e.what());
    return nullptr;
  }
}

extern "C" void scs_competition_experiment_destroy(
    SCS_CompetitionExperiment* experiment) {
  delete experiment;
}

extern "C" int scs_competition_experiment_dims(
    const SCS_CompetitionExperiment* experiment, int* out_num_runs,
    int* out_n_competitors, int* out_n_dims, char* err_buf, int err_buf_len) {
  if (!experiment || !out_num_runs || !out_n_competitors || !out_n_dims) {
    set_error(err_buf, err_buf_len,
              "scs_competition_experiment_dims: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  *out_num_runs = experiment->result.summary.num_runs;
  *out_n_competitors =
      experiment->result.run_summaries.empty()
          ? 0
          : static_cast<int>(
                experiment->result.run_summaries[0].final_vote_shares.size());
  *out_n_dims =
      experiment->result.run_summaries.empty()
          ? 0
          : static_cast<int>(
                experiment->result.run_summaries[0].final_positions[0].size());
  return SCS_OK;
}

extern "C" int scs_competition_experiment_summary(
    const SCS_CompetitionExperiment* experiment, double* out_mean_rounds,
    double* out_early_termination_rate, char* err_buf, int err_buf_len) {
  if (!experiment || !out_mean_rounds || !out_early_termination_rate) {
    set_error(err_buf, err_buf_len,
              "scs_competition_experiment_summary: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  *out_mean_rounds = experiment->result.summary.mean_rounds_completed;
  *out_early_termination_rate =
      experiment->result.summary.early_termination_rate;
  return SCS_OK;
}

extern "C" int scs_competition_experiment_mean_final_vote_shares(
    const SCS_CompetitionExperiment* experiment, double* out_shares,
    int out_len, char* err_buf, int err_buf_len) {
  if (!experiment) {
    set_error(err_buf, err_buf_len,
              "scs_competition_experiment_mean_final_vote_shares: experiment "
              "is null");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  return copy_double_vector(
      experiment->result.summary.mean_final_vote_shares, out_shares, out_len,
      err_buf, err_buf_len,
      "scs_competition_experiment_mean_final_vote_shares");
}

extern "C" int scs_competition_experiment_mean_final_seat_shares(
    const SCS_CompetitionExperiment* experiment, double* out_shares,
    int out_len, char* err_buf, int err_buf_len) {
  if (!experiment) {
    set_error(err_buf, err_buf_len,
              "scs_competition_experiment_mean_final_seat_shares: experiment "
              "is null");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  return copy_double_vector(
      experiment->result.summary.mean_final_seat_shares, out_shares, out_len,
      err_buf, err_buf_len,
      "scs_competition_experiment_mean_final_seat_shares");
}

extern "C" int scs_competition_experiment_run_round_counts(
    const SCS_CompetitionExperiment* experiment, int* out_round_counts,
    int out_len, char* err_buf, int err_buf_len) {
  if (!experiment || !out_round_counts) {
    set_error(
        err_buf, err_buf_len,
        "scs_competition_experiment_run_round_counts: null pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  const int needed = static_cast<int>(experiment->result.run_summaries.size());
  if (out_len < needed) {
    set_error(err_buf, err_buf_len,
              "scs_competition_experiment_run_round_counts: output buffer is "
              "too small");
    return SCS_ERROR_BUFFER_TOO_SMALL;
  }
  for (int i = 0; i < needed; ++i) {
    out_round_counts[i] = experiment->result.run_summaries[i].rounds_completed;
  }
  return SCS_OK;
}

extern "C" int scs_competition_experiment_run_termination_reasons(
    const SCS_CompetitionExperiment* experiment,
    SCS_CompetitionTerminationReason* out_reasons, int out_len, char* err_buf,
    int err_buf_len) {
  if (!experiment || !out_reasons) {
    set_error(err_buf, err_buf_len,
              "scs_competition_experiment_run_termination_reasons: null "
              "pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  const int needed = static_cast<int>(experiment->result.run_summaries.size());
  if (out_len < needed) {
    set_error(err_buf, err_buf_len,
              "scs_competition_experiment_run_termination_reasons: output "
              "buffer is too small");
    return SCS_ERROR_BUFFER_TOO_SMALL;
  }
  for (int i = 0; i < needed; ++i) {
    out_reasons[i] = to_c_termination_reason(
        experiment->result.run_summaries[i].termination_reason);
  }
  return SCS_OK;
}

extern "C" int scs_competition_experiment_run_terminated_early_flags(
    const SCS_CompetitionExperiment* experiment, int* out_flags, int out_len,
    char* err_buf, int err_buf_len) {
  if (!experiment || !out_flags) {
    set_error(err_buf, err_buf_len,
              "scs_competition_experiment_run_terminated_early_flags: null "
              "pointer argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  const int needed = static_cast<int>(experiment->result.run_summaries.size());
  if (out_len < needed) {
    set_error(err_buf, err_buf_len,
              "scs_competition_experiment_run_terminated_early_flags: output "
              "buffer is too small");
    return SCS_ERROR_BUFFER_TOO_SMALL;
  }
  for (int i = 0; i < needed; ++i) {
    out_flags[i] = experiment->result.run_summaries[i].terminated_early ? 1 : 0;
  }
  return SCS_OK;
}

extern "C" int scs_competition_experiment_run_final_vote_shares(
    const SCS_CompetitionExperiment* experiment, double* out_shares,
    int out_len, char* err_buf, int err_buf_len) {
  if (!experiment || !out_shares) {
    set_error(err_buf, err_buf_len,
              "scs_competition_experiment_run_final_vote_shares: null pointer "
              "argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  const int n_runs = static_cast<int>(experiment->result.run_summaries.size());
  const int n_competitors =
      n_runs == 0
          ? 0
          : static_cast<int>(
                experiment->result.run_summaries[0].final_vote_shares.size());
  const int needed = n_runs * n_competitors;
  if (out_len < needed) {
    set_error(err_buf, err_buf_len,
              "scs_competition_experiment_run_final_vote_shares: output buffer "
              "is too small");
    return SCS_ERROR_BUFFER_TOO_SMALL;
  }
  for (int run = 0; run < n_runs; ++run) {
    for (int i = 0; i < n_competitors; ++i) {
      out_shares[run * n_competitors + i] =
          experiment->result.run_summaries[run].final_vote_shares[i];
    }
  }
  return SCS_OK;
}

extern "C" int scs_competition_experiment_run_final_seat_shares(
    const SCS_CompetitionExperiment* experiment, double* out_shares,
    int out_len, char* err_buf, int err_buf_len) {
  if (!experiment || !out_shares) {
    set_error(err_buf, err_buf_len,
              "scs_competition_experiment_run_final_seat_shares: null pointer "
              "argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  const int n_runs = static_cast<int>(experiment->result.run_summaries.size());
  const int n_competitors =
      n_runs == 0
          ? 0
          : static_cast<int>(
                experiment->result.run_summaries[0].final_seat_shares.size());
  const int needed = n_runs * n_competitors;
  if (out_len < needed) {
    set_error(err_buf, err_buf_len,
              "scs_competition_experiment_run_final_seat_shares: output buffer "
              "is too small");
    return SCS_ERROR_BUFFER_TOO_SMALL;
  }
  for (int run = 0; run < n_runs; ++run) {
    for (int i = 0; i < n_competitors; ++i) {
      out_shares[run * n_competitors + i] =
          experiment->result.run_summaries[run].final_seat_shares[i];
    }
  }
  return SCS_OK;
}

extern "C" int scs_competition_experiment_run_final_positions(
    const SCS_CompetitionExperiment* experiment, double* out_positions,
    int out_len, char* err_buf, int err_buf_len) {
  if (!experiment || !out_positions) {
    set_error(err_buf, err_buf_len,
              "scs_competition_experiment_run_final_positions: null pointer "
              "argument");
    return SCS_ERROR_INVALID_ARGUMENT;
  }
  const int n_runs = static_cast<int>(experiment->result.run_summaries.size());
  const int n_competitors =
      n_runs == 0
          ? 0
          : static_cast<int>(
                experiment->result.run_summaries[0].final_positions.size());
  const int n_dims =
      (n_runs == 0 || n_competitors == 0)
          ? 0
          : static_cast<int>(
                experiment->result.run_summaries[0].final_positions[0].size());
  const int needed = n_runs * n_competitors * n_dims;
  if (out_len < needed) {
    set_error(err_buf, err_buf_len,
              "scs_competition_experiment_run_final_positions: output buffer "
              "is too small");
    return SCS_ERROR_BUFFER_TOO_SMALL;
  }
  for (int run = 0; run < n_runs; ++run) {
    for (int c = 0; c < n_competitors; ++c) {
      for (int d = 0; d < n_dims; ++d) {
        out_positions[(run * n_competitors + c) * n_dims + d] =
            experiment->result.run_summaries[run].final_positions[c][d];
      }
    }
  }
  return SCS_OK;
}
