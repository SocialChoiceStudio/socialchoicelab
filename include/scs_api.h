// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// scs_api.h — Public C API for SocialChoiceLab core.
//
// All functions return SCS_OK (0) on success. On failure they return a non-zero
// error code and, if err_buf and err_buf_len are provided (non-null, len > 0),
// write a null-terminated message into err_buf.
//
// No C++ types, STL, or exceptions cross this boundary. Inputs and outputs are
// plain C types (double*, int, const char*, POD structs, opaque handles).
//
// Design constraints: see design_document.md § FFI & Interfaces and
// § c_api design inputs (Items 29–31).

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Error codes
// ---------------------------------------------------------------------------

/** Success. */
#define SCS_OK 0
/** Invalid argument (e.g. wrong dimension, out-of-range parameter). */
#define SCS_ERROR_INVALID_ARGUMENT 1
/** Internal / unexpected error. */
#define SCS_ERROR_INTERNAL 2

// ---------------------------------------------------------------------------
// Loss configuration  (Item 29: single struct instead of six parameters)
// ---------------------------------------------------------------------------

/** Loss function type discriminant. */
typedef enum {
  SCS_LOSS_LINEAR = 0,    /**< u = -α|d| */
  SCS_LOSS_QUADRATIC = 1, /**< u = -αd²  */
  SCS_LOSS_GAUSSIAN = 2,  /**< u = -α(1 - e^(-βd²)) */
  SCS_LOSS_THRESHOLD = 3  /**< u = -α max(0, |d| - τ) */
} SCS_LossType;

/**
 * @brief Loss function configuration.
 * Parameters used per loss type (others are ignored):
 *   LINEAR    → sensitivity
 *   QUADRATIC → sensitivity
 *   GAUSSIAN  → max_loss, steepness
 *   THRESHOLD → threshold, sensitivity
 */
typedef struct {
  SCS_LossType loss_type;
  double sensitivity; /**< α: scale for LINEAR / QUADRATIC / THRESHOLD. */
  double max_loss;    /**< α: asymptote for GAUSSIAN. */
  double steepness;   /**< β: rate for GAUSSIAN. */
  double threshold;   /**< τ: indifference radius for THRESHOLD. */
} SCS_LossConfig;

// ---------------------------------------------------------------------------
// Distance configuration
// ---------------------------------------------------------------------------

/** Distance metric type. */
typedef enum {
  SCS_DIST_EUCLIDEAN = 0, /**< L2 norm (p=2) */
  SCS_DIST_MANHATTAN = 1, /**< L1 norm (p=1) */
  SCS_DIST_CHEBYSHEV = 2, /**< L∞ norm (p=∞) */
  SCS_DIST_MINKOWSKI = 3  /**< General Lp norm */
} SCS_DistanceType;

/**
 * @brief Distance configuration.
 * salience_weights must be a pointer to n_weights doubles (length must equal
 * the dimension of the points passed to the distance/level-set functions).
 */
typedef struct {
  SCS_DistanceType distance_type;
  double order_p; /**< Only used for SCS_DIST_MINKOWSKI (≥ 1). */
  const double* salience_weights;
  int n_weights;
} SCS_DistanceConfig;

// ---------------------------------------------------------------------------
// Level set (indifference curve) output types
// ---------------------------------------------------------------------------

/** Level set shape type for 2D results. */
typedef enum {
  SCS_LEVEL_SET_CIRCLE = 0,       /**< Euclidean, equal weights. */
  SCS_LEVEL_SET_ELLIPSE = 1,      /**< Euclidean, unequal weights. */
  SCS_LEVEL_SET_SUPERELLIPSE = 2, /**< Minkowski 1 < p < ∞. */
  SCS_LEVEL_SET_POLYGON = 3       /**< Manhattan or Chebyshev (4 vertices). */
} SCS_LevelSetType;

/**
 * @brief 2D level-set result.
 *
 * Fields by type:
 *   CIRCLE:      center_x/y, param0 = radius.
 *   ELLIPSE:     center_x/y, param0 = semi_axis_0, param1 = semi_axis_1.
 *   SUPERELLIPSE: center_x/y, param0 = semi_axis_0, param1 = semi_axis_1,
 * exponent_p. POLYGON:     center_x/y, n_vertices (always 4),
 * vertices[0..2*n_vertices-1] as [x0,y0, x1,y1, x2,y2, x3,y3].
 */
typedef struct {
  SCS_LevelSetType type;
  double center_x;
  double center_y;
  double param0;      /**< radius / semi_axis_0 */
  double param1;      /**< semi_axis_1 (ellipse/superellipse) */
  double exponent_p;  /**< exponent (superellipse) */
  int n_vertices;     /**< polygon only; 0 for other types */
  double vertices[8]; /**< polygon: [x0,y0,x1,y1,x2,y2,x3,y3]; others: zero */
} SCS_LevelSet2d;

// ---------------------------------------------------------------------------
// StreamManager opaque handle  (Item 30: registration enforced at boundary)
// ---------------------------------------------------------------------------

/** Opaque handle for a StreamManager instance. */
typedef struct SCS_StreamManagerImpl SCS_StreamManager;

// ---------------------------------------------------------------------------
// Distance functions
// ---------------------------------------------------------------------------

/**
 * @brief Compute distance between two N-dimensional points.
 *
 * @param ideal_point     Array of n_dims doubles.
 * @param alt_point       Array of n_dims doubles.
 * @param n_dims          Dimension (must match dist_cfg->n_weights).
 * @param dist_cfg        Distance configuration (salience_weights.n_weights ==
 * n_dims).
 * @param[out] out        Computed distance.
 * @param err_buf         Optional buffer for error message (may be NULL).
 * @param err_buf_len     Length of err_buf (may be 0).
 * @return SCS_OK or error code.
 */
int scs_calculate_distance(const double* ideal_point, const double* alt_point,
                           int n_dims, const SCS_DistanceConfig* dist_cfg,
                           double* out, char* err_buf, int err_buf_len);

// ---------------------------------------------------------------------------
// Loss / utility functions
// ---------------------------------------------------------------------------

/**
 * @brief Convert distance to utility using the given loss function.
 *
 * @param distance        Non-negative distance value.
 * @param loss_cfg        Loss configuration.
 * @param[out] out        Utility value (≤ 0 for these loss functions).
 * @param err_buf         Optional error message buffer.
 * @param err_buf_len     Length of err_buf.
 * @return SCS_OK or error code.
 */
int scs_distance_to_utility(double distance, const SCS_LossConfig* loss_cfg,
                            double* out, char* err_buf, int err_buf_len);

/**
 * @brief Normalize utility to [0, 1] given the maximum possible distance.
 *
 * @param utility         Raw utility (from scs_distance_to_utility).
 * @param max_distance    Maximum distance used to define the worst utility.
 * @param loss_cfg        Loss configuration (must match how utility was
 * computed).
 * @param[out] out        Normalized utility in [0, 1].
 * @param err_buf         Optional error message buffer.
 * @param err_buf_len     Length of err_buf.
 * @return SCS_OK or error code.
 */
int scs_normalize_utility(double utility, double max_distance,
                          const SCS_LossConfig* loss_cfg, double* out,
                          char* err_buf, int err_buf_len);

// ---------------------------------------------------------------------------
// Indifference / level-set functions
// ---------------------------------------------------------------------------

/**
 * @brief 1D level set: up to 2 points where u(x) = utility_level.
 *
 * @param ideal           Ideal point coordinate (1D).
 * @param weight          Salience weight for the single dimension (> 0).
 * @param utility_level   Target utility level.
 * @param loss_cfg        Loss configuration.
 * @param[out] out_points Buffer for up to 2 doubles (caller must provide >= 2).
 * @param[out] out_n      Number of points written: 0, 1, or 2.
 * @param err_buf         Optional error message buffer.
 * @param err_buf_len     Length of err_buf.
 * @return SCS_OK or error code.
 */
int scs_level_set_1d(double ideal, double weight, double utility_level,
                     const SCS_LossConfig* loss_cfg, double* out_points,
                     int* out_n, char* err_buf, int err_buf_len);

/**
 * @brief 2D level set: exact shape (circle, ellipse, superellipse, or polygon).
 *
 * @param ideal_x         Ideal point x coordinate.
 * @param ideal_y         Ideal point y coordinate.
 * @param utility_level   Target utility level.
 * @param loss_cfg        Loss configuration.
 * @param dist_cfg        Distance configuration (n_weights must be 2).
 * @param[out] out        Level-set result (type + shape parameters/vertices).
 * @param err_buf         Optional error message buffer.
 * @param err_buf_len     Length of err_buf.
 * @return SCS_OK or error code.
 */
int scs_level_set_2d(double ideal_x, double ideal_y, double utility_level,
                     const SCS_LossConfig* loss_cfg,
                     const SCS_DistanceConfig* dist_cfg, SCS_LevelSet2d* out,
                     char* err_buf, int err_buf_len);

/**
 * @brief Sample a SCS_LevelSet2d as a polygon (interleaved x,y vertices).
 *
 * For CIRCLE/ELLIPSE/SUPERELLIPSE: samples num_samples points around the curve.
 * For POLYGON: copies the 4 exact vertices (num_samples is ignored).
 *
 * @param level_set       Input level-set (from scs_level_set_2d).
 * @param num_samples     Samples for smooth shapes (e.g. 64 or 128; >= 3).
 * @param[out] out_xy     Caller buffer for interleaved [x0,y0,x1,y1,...].
 *                        Must be at least 2*num_samples doubles for smooth
 *                        shapes, or 2*4 = 8 for polygon type.
 * @param[out] out_n      Number of (x,y) pairs written.
 * @param err_buf         Optional error message buffer.
 * @param err_buf_len     Length of err_buf.
 * @return SCS_OK or error code.
 */
int scs_level_set_to_polygon(const SCS_LevelSet2d* level_set, int num_samples,
                             double* out_xy, int* out_n, char* err_buf,
                             int err_buf_len);

// ---------------------------------------------------------------------------
// StreamManager lifecycle  (Item 31: PRNG engine not exposed)
// ---------------------------------------------------------------------------

/**
 * @brief Create a new StreamManager with the given master seed.
 *
 * @param master_seed     Master seed for all streams.
 * @param err_buf         Optional error message buffer.
 * @param err_buf_len     Length of err_buf.
 * @return Opaque handle (caller owns; must call scs_stream_manager_destroy),
 *         or NULL on error.
 */
SCS_StreamManager* scs_stream_manager_create(uint64_t master_seed,
                                             char* err_buf, int err_buf_len);

/**
 * @brief Destroy a StreamManager and release its resources.
 * @param mgr Handle from scs_stream_manager_create (may be NULL, which is a
 * no-op).
 */
void scs_stream_manager_destroy(SCS_StreamManager* mgr);

/**
 * @brief Register allowed stream names.
 *
 * After this call, only these names may be used with scs_*_from_stream
 * functions (Item 30). Pass n_names=0 to clear the allowlist.
 *
 * @param mgr         StreamManager handle.
 * @param names       Array of null-terminated stream name strings.
 * @param n_names     Number of entries in names (0 to clear allowlist).
 * @param err_buf     Optional error message buffer.
 * @param err_buf_len Length of err_buf.
 * @return SCS_OK or error code.
 */
int scs_register_streams(SCS_StreamManager* mgr, const char** names,
                         int n_names, char* err_buf, int err_buf_len);

/**
 * @brief Reset all streams with a new master seed.
 *
 * @param mgr         StreamManager handle.
 * @param master_seed New master seed.
 * @param err_buf     Optional error message buffer.
 * @param err_buf_len Length of err_buf.
 * @return SCS_OK or error code.
 */
int scs_reset_all(SCS_StreamManager* mgr, uint64_t master_seed, char* err_buf,
                  int err_buf_len);

/**
 * @brief Reset a specific named stream to a given seed.
 *
 * @param mgr         StreamManager handle.
 * @param stream_name Null-terminated stream name.
 * @param seed        New seed for this stream.
 * @param err_buf     Optional error message buffer.
 * @param err_buf_len Length of err_buf.
 * @return SCS_OK or error code.
 */
int scs_reset_stream(SCS_StreamManager* mgr, const char* stream_name,
                     uint64_t seed, char* err_buf, int err_buf_len);

/**
 * @brief Skip (discard) n values in a named stream.
 * @return SCS_OK or error code.
 */
int scs_skip(SCS_StreamManager* mgr, const char* stream_name, uint64_t n,
             char* err_buf, int err_buf_len);

// ---------------------------------------------------------------------------
// PRNG draw functions  (each takes mgr + stream_name → out)
// ---------------------------------------------------------------------------

/**
 * @brief Draw a uniform real in [min, max) from the named stream.
 * @return SCS_OK or error code.
 */
int scs_uniform_real(SCS_StreamManager* mgr, const char* stream_name,
                     double min, double max, double* out, char* err_buf,
                     int err_buf_len);

/**
 * @brief Draw a normal variate with given mean and stddev from the named
 * stream.
 * @return SCS_OK or error code.
 */
int scs_normal(SCS_StreamManager* mgr, const char* stream_name, double mean,
               double stddev, double* out, char* err_buf, int err_buf_len);

/**
 * @brief Draw a Bernoulli variate (0 or 1) with given probability.
 * @return SCS_OK or error code.
 */
int scs_bernoulli(SCS_StreamManager* mgr, const char* stream_name,
                  double probability, int* out, char* err_buf, int err_buf_len);

/**
 * @brief Draw a uniform integer in [min, max] from the named stream.
 * @return SCS_OK or error code.
 */
int scs_uniform_int(SCS_StreamManager* mgr, const char* stream_name,
                    int64_t min, int64_t max, int64_t* out, char* err_buf,
                    int err_buf_len);

/**
 * @brief Draw a uniform choice index in [0, n) from the named stream.
 * @return SCS_OK or error code.
 */
int scs_uniform_choice(SCS_StreamManager* mgr, const char* stream_name,
                       int64_t n, int64_t* out, char* err_buf, int err_buf_len);

#ifdef __cplusplus
}  // extern "C"
#endif
