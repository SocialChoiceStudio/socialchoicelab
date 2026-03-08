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
// API version  (C0.1)
// ---------------------------------------------------------------------------

#define SCS_API_VERSION_MAJOR 0
#define SCS_API_VERSION_MINOR 2
#define SCS_API_VERSION_PATCH 0

// ---------------------------------------------------------------------------
// Symbol visibility  (C0.1)
// ---------------------------------------------------------------------------

#if defined(_WIN32)
#if defined(SCS_API_BUILD)
#define SCS_API __declspec(dllexport)
#else
#define SCS_API __declspec(dllimport)
#endif
#else
#define SCS_API __attribute__((visibility("default")))
#endif

// ---------------------------------------------------------------------------
// Majority / supermajority sentinels  (C0.1)
// ---------------------------------------------------------------------------

/** Pass as the `k` parameter to request simple majority (⌊n/2⌋ + 1 voters). */
#define SCS_MAJORITY_SIMPLE (-1)

// ---------------------------------------------------------------------------
// Default sampling constants  (C0.1)
// ---------------------------------------------------------------------------

/** Default number of samples for winset boundary export. */
#define SCS_DEFAULT_WINSET_SAMPLES 64
/** Default number of samples for yolk/boundary grid resolution. */
#define SCS_DEFAULT_BOUNDARY_GRID_RESOLUTION 15

// ---------------------------------------------------------------------------
// Error codes
// ---------------------------------------------------------------------------

/** Success. */
#define SCS_OK 0
/** Invalid argument (e.g. wrong dimension, out-of-range parameter,
 *  null pointer where one is required). */
#define SCS_ERROR_INVALID_ARGUMENT 1
/** Internal / unexpected error. */
#define SCS_ERROR_INTERNAL 2
/** Caller-provided output buffer is valid but too small.
 *  When returned from a variable-size output function, *out_n (if present)
 *  is set to the number of elements required. (C0.2) */
#define SCS_ERROR_BUFFER_TOO_SMALL 3
/** A handle construction or internal heap allocation failed. (C0.2) */
#define SCS_ERROR_OUT_OF_MEMORY 4

// ---------------------------------------------------------------------------
// Pairwise comparison result domain  (C0.3)
// ---------------------------------------------------------------------------

/** ABI-stable scalar type for pairwise majority comparison results.
 *  Use the named constants below; do not compare against raw integers. */
typedef int32_t SCS_PairwiseResult;

/** Candidate A loses to candidate B under the given majority rule. */
#define SCS_PAIRWISE_LOSS ((SCS_PairwiseResult) - 1)
/** Candidates A and B tie (neither achieves the required majority). */
#define SCS_PAIRWISE_TIE ((SCS_PairwiseResult)0)
/** Candidate A beats candidate B under the given majority rule. */
#define SCS_PAIRWISE_WIN ((SCS_PairwiseResult)1)

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
 *
 * salience_weights is borrowed input storage — it must remain valid for the
 * duration of the call. Do not cache this pointer in long-lived handles.
 * n_weights must equal the dimension of the points passed to the distance /
 * level-set functions.
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
 *   CIRCLE:       center_x/y, param0 = radius.
 *   ELLIPSE:      center_x/y, param0 = semi_axis_0, param1 = semi_axis_1.
 *   SUPERELLIPSE: center_x/y, param0 = semi_axis_0, param1 = semi_axis_1,
 *                 exponent_p.
 *   POLYGON:      center_x/y, n_vertices (always 4),
 *                 vertices[0..2*n_vertices-1] as [x0,y0, x1,y1, x2,y2, x3,y3].
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
// API version query  (C0.1)
// ---------------------------------------------------------------------------

/**
 * @brief Query the compiled API version.
 *
 * @param[out] out_major  Major version (breaking changes).
 * @param[out] out_minor  Minor version (additive changes).
 * @param[out] out_patch  Patch version (behavioural / doc fixes).
 * @param err_buf         Optional error message buffer.
 * @param err_buf_len     Length of err_buf.
 * @return SCS_OK or error code.
 */
SCS_API int scs_api_version(int* out_major, int* out_minor, int* out_patch,
                            char* err_buf, int err_buf_len);

// ---------------------------------------------------------------------------
// Distance functions
// ---------------------------------------------------------------------------

/**
 * @brief Compute distance between two N-dimensional points.
 *
 * @param ideal_point     Array of n_dims doubles.
 * @param alt_point       Array of n_dims doubles.
 * @param n_dims          Dimension (must match dist_cfg->n_weights).
 * @param dist_cfg        Distance configuration.
 * @param[out] out        Computed distance.
 * @param err_buf         Optional buffer for error message (may be NULL).
 * @param err_buf_len     Length of err_buf (may be 0).
 * @return SCS_OK or error code.
 */
SCS_API int scs_calculate_distance(const double* ideal_point,
                                   const double* alt_point, int n_dims,
                                   const SCS_DistanceConfig* dist_cfg,
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
SCS_API int scs_distance_to_utility(double distance,
                                    const SCS_LossConfig* loss_cfg, double* out,
                                    char* err_buf, int err_buf_len);

/**
 * @brief Normalize utility to [0, 1] given the maximum possible distance.
 *
 * @param utility         Raw utility (from scs_distance_to_utility).
 * @param max_distance    Maximum distance used to define the worst utility.
 * @param loss_cfg        Loss configuration (must match how utility was
 *                        computed).
 * @param[out] out        Normalized utility in [0, 1].
 * @param err_buf         Optional error message buffer.
 * @param err_buf_len     Length of err_buf.
 * @return SCS_OK or error code.
 */
SCS_API int scs_normalize_utility(double utility, double max_distance,
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
SCS_API int scs_level_set_1d(double ideal, double weight, double utility_level,
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
SCS_API int scs_level_set_2d(double ideal_x, double ideal_y,
                             double utility_level,
                             const SCS_LossConfig* loss_cfg,
                             const SCS_DistanceConfig* dist_cfg,
                             SCS_LevelSet2d* out, char* err_buf,
                             int err_buf_len);

/**
 * @brief Sample a SCS_LevelSet2d as a polygon (interleaved x,y vertices).
 *
 * For CIRCLE/ELLIPSE/SUPERELLIPSE: samples num_samples points around the
 * curve. For POLYGON: copies the 4 exact vertices (num_samples is ignored).
 *
 * **Size-query mode:** pass out_xy = NULL (and any out_capacity). The function
 * writes the required number of (x,y) pairs into *out_n and returns SCS_OK.
 * Use this to allocate the right buffer before the fill call.
 *
 * **Fill mode:** pass a non-null out_xy with out_capacity set to the number
 * of (x,y) pairs the buffer can hold. If out_capacity is too small,
 * SCS_ERROR_BUFFER_TOO_SMALL is returned and *out_n is set to the required
 * size. Otherwise the vertices are written and *out_n is set to the count
 * written.
 *
 * @param level_set       Input level-set (from scs_level_set_2d).
 * @param num_samples     Samples for smooth shapes (>= 3); ignored for POLYGON.
 * @param[out] out_xy     Caller buffer for interleaved [x0,y0,x1,y1,...],
 *                        or NULL for size-query.
 * @param out_capacity    Capacity of out_xy in (x,y) pairs; ignored if NULL.
 * @param[out] out_n      Number of (x,y) pairs required / written.
 * @param err_buf         Optional error message buffer.
 * @param err_buf_len     Length of err_buf.
 * @return SCS_OK, SCS_ERROR_BUFFER_TOO_SMALL, or other error code.
 */
SCS_API int scs_level_set_to_polygon(const SCS_LevelSet2d* level_set,
                                     int num_samples, double* out_xy,
                                     int out_capacity, int* out_n,
                                     char* err_buf, int err_buf_len);

// ---------------------------------------------------------------------------
// Geometry functions
// ---------------------------------------------------------------------------

/**
 * @brief Compute the 2D convex hull of a set of ideal points.
 *
 * Under Euclidean preferences the convex hull of the voter ideal points equals
 * the Pareto set. Output vertices are in counter-clockwise order.
 *
 * @param points      Flat array of 2*n_points doubles [x0,y0,x1,y1,...].
 * @param n_points    Number of input points (must be >= 1; all coordinates
 *                    must be finite).
 * @param[out] out_xy Output buffer for hull vertices (interleaved [x0,y0,...]).
 *                    Must hold at least 2*n_points doubles.
 * @param[out] out_n  Number of hull vertices written.
 * @param err_buf     Optional error message buffer.
 * @param err_buf_len Length of err_buf.
 * @return SCS_OK or error code.
 */
SCS_API int scs_convex_hull_2d(const double* points, int n_points,
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
SCS_API SCS_StreamManager* scs_stream_manager_create(uint64_t master_seed,
                                                     char* err_buf,
                                                     int err_buf_len);

/**
 * @brief Destroy a StreamManager and release its resources.
 * @param mgr Handle from scs_stream_manager_create (may be NULL — no-op).
 */
SCS_API void scs_stream_manager_destroy(SCS_StreamManager* mgr);

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
SCS_API int scs_register_streams(SCS_StreamManager* mgr, const char** names,
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
SCS_API int scs_reset_all(SCS_StreamManager* mgr, uint64_t master_seed,
                          char* err_buf, int err_buf_len);

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
SCS_API int scs_reset_stream(SCS_StreamManager* mgr, const char* stream_name,
                             uint64_t seed, char* err_buf, int err_buf_len);

/**
 * @brief Skip (discard) n values in a named stream.
 * @return SCS_OK or error code.
 */
SCS_API int scs_skip(SCS_StreamManager* mgr, const char* stream_name,
                     uint64_t n, char* err_buf, int err_buf_len);

// ---------------------------------------------------------------------------
// PRNG draw functions  (each takes mgr + stream_name → out)
// ---------------------------------------------------------------------------

/**
 * @brief Draw a uniform real in [min, max) from the named stream.
 * @return SCS_OK or error code.
 */
SCS_API int scs_uniform_real(SCS_StreamManager* mgr, const char* stream_name,
                             double min, double max, double* out, char* err_buf,
                             int err_buf_len);

/**
 * @brief Draw a normal variate with given mean and stddev from the named
 *        stream.
 * @return SCS_OK or error code.
 */
SCS_API int scs_normal(SCS_StreamManager* mgr, const char* stream_name,
                       double mean, double stddev, double* out, char* err_buf,
                       int err_buf_len);

/**
 * @brief Draw a Bernoulli variate (0 or 1) with given probability.
 * @return SCS_OK or error code.
 */
SCS_API int scs_bernoulli(SCS_StreamManager* mgr, const char* stream_name,
                          double probability, int* out, char* err_buf,
                          int err_buf_len);

/**
 * @brief Draw a uniform integer in [min, max] from the named stream.
 * @return SCS_OK or error code.
 */
SCS_API int scs_uniform_int(SCS_StreamManager* mgr, const char* stream_name,
                            int64_t min, int64_t max, int64_t* out,
                            char* err_buf, int err_buf_len);

/**
 * @brief Draw a uniform choice index in [0, n) from the named stream.
 * @return SCS_OK or error code.
 */
SCS_API int scs_uniform_choice(SCS_StreamManager* mgr, const char* stream_name,
                               int64_t n, int64_t* out, char* err_buf,
                               int err_buf_len);

// ---------------------------------------------------------------------------
// Geometry — winset opaque handle  (C2)
// ---------------------------------------------------------------------------

/**
 * @brief Opaque handle for a 2D winset region.
 *
 * Internally wraps a CGAL General_polygon_set_2. All factory functions return
 * a heap-allocated handle; the caller owns it and must call
 * scs_winset_destroy when done. NULL handles are not valid inputs to any
 * function except scs_winset_destroy (which is a no-op on NULL).
 *
 * Boolean operations (union, intersection, etc.) always return a NEW owned
 * handle; the inputs are not modified.
 */
typedef struct SCS_WinsetImpl SCS_Winset;

// ---------------------------------------------------------------------------
// Geometry — Yolk POD struct  (C3)
// ---------------------------------------------------------------------------

/**
 * @brief 2D Yolk result: center and radius.
 *
 * ⚠ APPROXIMATION WARNING: scs_yolk_2d computes the LP yolk (smallest circle
 * intersecting the limiting median lines), NOT the true yolk (smallest circle
 * intersecting ALL median lines). These can differ — see the project notes in
 * where_we_are.md and milestone_gates.md for references and the planned
 * replacement algorithms (Gudmundsson & Wong 2019, Liu & Tovey 2023).
 * Do not treat results as exact.
 */
typedef struct {
  double center_x;
  double center_y;
  double radius;
} SCS_Yolk2d;

// ---------------------------------------------------------------------------
// Geometry — majority preference  (C1)
// ---------------------------------------------------------------------------

/**
 * @brief Test whether a k-majority of voters prefer point A to point B.
 *
 * Voter i strictly prefers A iff d(pᵢ, A) < d(pᵢ, B). Ties contribute 0.
 * Returns 1 in *out if at least k voters prefer A, else 0.
 *
 * @param point_a_x, point_a_y   First alternative.
 * @param point_b_x, point_b_y   Second alternative.
 * @param voter_ideals_xy         Flat array [x0,y0,x1,y1,...] length
 * 2*n_voters.
 * @param n_voters                Number of voters (>= 1).
 * @param dist_cfg                Distance configuration (n_weights must be 2).
 * @param k                       Majority threshold: SCS_MAJORITY_SIMPLE or
 *                                1..n_voters.
 * @param[out] out                1 if A defeats B under k-majority, else 0.
 * @param err_buf                 Optional error message buffer.
 * @param err_buf_len             Length of err_buf.
 * @return SCS_OK or error code.
 */
SCS_API int scs_majority_prefers_2d(double point_a_x, double point_a_y,
                                    double point_b_x, double point_b_y,
                                    const double* voter_ideals_xy, int n_voters,
                                    const SCS_DistanceConfig* dist_cfg, int k,
                                    int* out, char* err_buf, int err_buf_len);

/**
 * @brief Compute the n_alts × n_alts pairwise preference matrix.
 *
 * Entry [i*n_alts + j] is:
 *   SCS_PAIRWISE_WIN  if alternatives[i] is preferred by more voters than [j]
 *   SCS_PAIRWISE_TIE  if equal numbers prefer each
 *   SCS_PAIRWISE_LOSS if alternatives[j] is preferred by more voters than [i]
 *
 * The matrix is anti-symmetric: M[i,j] = -M[j,i], M[i,i] = SCS_PAIRWISE_TIE.
 *
 * @param alt_xy            Flat array [x0,y0,x1,y1,...] length 2*n_alts.
 * @param n_alts            Number of alternatives (>= 1).
 * @param voter_ideals_xy   Flat array [x0,y0,...] length 2*n_voters.
 * @param n_voters          Number of voters (>= 1).
 * @param dist_cfg          Distance configuration (n_weights must be 2).
 * @param k                 Majority threshold (passed through for validation;
 *                          matrix entries reflect raw vote margins, not k).
 * @param[out] out_matrix   Row-major SCS_PairwiseResult buffer, length
 *                          n_alts*n_alts. Must be non-null.
 * @param out_len           Capacity of out_matrix (must be >= n_alts*n_alts).
 * @param err_buf           Optional error message buffer.
 * @param err_buf_len       Length of err_buf.
 * @return SCS_OK or error code.
 */
SCS_API int scs_pairwise_matrix_2d(const double* alt_xy, int n_alts,
                                   const double* voter_ideals_xy, int n_voters,
                                   const SCS_DistanceConfig* dist_cfg, int k,
                                   SCS_PairwiseResult* out_matrix, int out_len,
                                   char* err_buf, int err_buf_len);

/**
 * @brief Test whether the weighted majority of voters prefer point A to point
 * B.
 *
 * Returns 1 in *out if the total weight of voters preferring A meets or
 * exceeds threshold * (sum of all weights).
 *
 * @param point_a_x, point_a_y   First alternative.
 * @param point_b_x, point_b_y   Second alternative.
 * @param voter_ideals_xy         Flat array [x0,y0,...] length 2*n_voters.
 * @param n_voters                Number of voters (>= 1).
 * @param weights                 Voter weights, length n_voters. All must be
 *                                finite and strictly positive.
 * @param dist_cfg                Distance configuration (n_weights must be 2).
 * @param threshold               Weight fraction in (0, 1]. Use 0.5 for simple
 *                                weighted majority.
 * @param[out] out                1 if weighted majority prefers A, else 0.
 * @param err_buf                 Optional error message buffer.
 * @param err_buf_len             Length of err_buf.
 * @return SCS_OK or error code.
 */
SCS_API int scs_weighted_majority_prefers_2d(
    double point_a_x, double point_a_y, double point_b_x, double point_b_y,
    const double* voter_ideals_xy, int n_voters, const double* weights,
    const SCS_DistanceConfig* dist_cfg, double threshold, int* out,
    char* err_buf, int err_buf_len);

// ---------------------------------------------------------------------------
// Geometry — winset factory functions  (C2.2–C2.4)
// ---------------------------------------------------------------------------

/**
 * @brief Compute the 2D k-majority winset of a status quo.
 *
 * @param status_quo_x, status_quo_y  Status quo point.
 * @param voter_ideals_xy             Flat [x0,y0,...] array, length 2*n_voters.
 * @param n_voters                    Number of voters (>= 1).
 * @param dist_cfg                    Distance configuration (n_weights == 2).
 * @param k                           SCS_MAJORITY_SIMPLE or 1..n_voters.
 * @param num_samples                 Boundary approximation quality (>= 4);
 *                                    use SCS_DEFAULT_WINSET_SAMPLES if unsure.
 * @param err_buf                     Optional error message buffer.
 * @param err_buf_len                 Length of err_buf.
 * @return New SCS_Winset* (caller owns; call scs_winset_destroy), or NULL on
 *         error.
 */
SCS_API SCS_Winset* scs_winset_2d(double status_quo_x, double status_quo_y,
                                  const double* voter_ideals_xy, int n_voters,
                                  const SCS_DistanceConfig* dist_cfg, int k,
                                  int num_samples, char* err_buf,
                                  int err_buf_len);

/**
 * @brief Compute the weighted-majority 2D winset of a status quo.
 *
 * @param weights      Voter weights (length n_voters; all must be > 0).
 * @param threshold    Weight fraction in (0, 1] required for majority.
 * @return New SCS_Winset* (caller owns), or NULL on error.
 */
SCS_API SCS_Winset* scs_weighted_winset_2d(
    double status_quo_x, double status_quo_y, const double* voter_ideals_xy,
    int n_voters, const double* weights, const SCS_DistanceConfig* dist_cfg,
    double threshold, int num_samples, char* err_buf, int err_buf_len);

/**
 * @brief Compute the k-majority winset constrained by veto players.
 *
 * A veto player at ideal point v blocks any policy change to a point x unless
 * x is strictly inside v's preferred-to set at the status quo.
 *
 * @param veto_ideals_xy  Flat [x0,y0,...] array of veto player ideals (length
 *                        2*n_veto). May be NULL when n_veto == 0.
 * @param n_veto          Number of veto players (0 = no veto constraint).
 * @return New SCS_Winset* (caller owns), or NULL on error.
 */
SCS_API SCS_Winset* scs_winset_with_veto_2d(
    double status_quo_x, double status_quo_y, const double* voter_ideals_xy,
    int n_voters, const SCS_DistanceConfig* dist_cfg,
    const double* veto_ideals_xy, int n_veto, int k, int num_samples,
    char* err_buf, int err_buf_len);

// ---------------------------------------------------------------------------
// Geometry — winset lifecycle and query helpers  (C2.5)
// ---------------------------------------------------------------------------

/**
 * @brief Destroy a winset handle and release all resources.
 * @param ws Handle from a scs_winset_* factory. NULL is a no-op.
 */
SCS_API void scs_winset_destroy(SCS_Winset* ws);

/**
 * @brief Test whether the winset is empty (i.e. no policy beats the SQ).
 * @param[out] out_is_empty  1 if empty, 0 if non-empty.
 * @return SCS_OK or error code.
 */
SCS_API int scs_winset_is_empty(const SCS_Winset* ws, int* out_is_empty,
                                char* err_buf, int err_buf_len);

/**
 * @brief Test whether a 2D point lies strictly inside the winset.
 * @param[out] out_contains  1 if inside, 0 otherwise.
 * @return SCS_OK or error code.
 */
SCS_API int scs_winset_contains_point_2d(const SCS_Winset* ws, double x,
                                         double y, int* out_contains,
                                         char* err_buf, int err_buf_len);

/**
 * @brief Compute the axis-aligned bounding box of the winset.
 *
 * @param[out] out_found  1 if the winset is non-empty and bbox was written,
 *                        0 if the winset is empty (no bbox).
 * @param[out] out_min_x, out_min_y, out_max_x, out_max_y  Bbox corners.
 * @return SCS_OK or error code.
 */
SCS_API int scs_winset_bbox_2d(const SCS_Winset* ws, int* out_found,
                               double* out_min_x, double* out_min_y,
                               double* out_max_x, double* out_max_y,
                               char* err_buf, int err_buf_len);

// ---------------------------------------------------------------------------
// Geometry — winset approximate boundary export  (C2.6)
// ---------------------------------------------------------------------------

/**
 * @brief Query the buffer sizes required for scs_winset_sample_boundary_2d.
 *
 * @param[out] out_xy_pairs  Total number of (x,y) vertex pairs across all
 *                           boundary paths.
 * @param[out] out_n_paths   Number of boundary paths (outer rings + holes).
 * @return SCS_OK or error code.
 */
SCS_API int scs_winset_boundary_size_2d(const SCS_Winset* ws, int* out_xy_pairs,
                                        int* out_n_paths, char* err_buf,
                                        int err_buf_len);

/**
 * @brief Export the winset boundary as sampled closed paths.
 *
 * Boundary paths are the pre-sampled polygon vertices stored inside the
 * winset; no additional resampling is applied. Each path is a closed ring
 * (do not repeat the first vertex at the end).
 *
 * **Size-query mode:** pass out_xy = NULL or out_xy_capacity = 0 to retrieve
 * *out_xy_n (total pairs needed) without writing coordinates. Similarly pass
 * out_path_starts = NULL or out_path_capacity = 0 to retrieve *out_n_paths.
 *
 * **Fill mode:** supply buffers with capacities >= the queried sizes.
 * SCS_ERROR_BUFFER_TOO_SMALL is returned if either buffer is too small; both
 * *out_xy_n and *out_n_paths are set to the required sizes.
 *
 * @param out_xy              Interleaved [x0,y0,x1,y1,...] vertex buffer,
 *                            or NULL for size query.
 * @param out_xy_capacity     Capacity of out_xy in (x,y) pairs.
 * @param[out] out_xy_n       Pairs required / written.
 * @param out_path_starts     Buffer of pair-index offsets, length out_n_paths,
 *                            or NULL for size query.
 * @param out_path_capacity   Capacity of out_path_starts (in paths).
 * @param out_path_is_hole    Optional: 0 = outer ring, 1 = hole.
 *                            Must be length out_path_capacity if non-null.
 * @param[out] out_n_paths    Paths required / written.
 * @return SCS_OK, SCS_ERROR_BUFFER_TOO_SMALL, or other error code.
 */
SCS_API int scs_winset_sample_boundary_2d(
    const SCS_Winset* ws, double* out_xy, int out_xy_capacity, int* out_xy_n,
    int* out_path_starts, int out_path_capacity, int* out_path_is_hole,
    int* out_n_paths, char* err_buf, int err_buf_len);

// ---------------------------------------------------------------------------
// Geometry — winset boolean set operations  (C2.7)
// ---------------------------------------------------------------------------

/**
 * @brief Return the union of two winset regions (a ∪ b).
 * @return New SCS_Winset* (caller owns), or NULL on error.
 */
SCS_API SCS_Winset* scs_winset_union(const SCS_Winset* a, const SCS_Winset* b,
                                     char* err_buf, int err_buf_len);

/**
 * @brief Return the intersection of two winset regions (a ∩ b).
 * @return New SCS_Winset* (caller owns), or NULL on error.
 */
SCS_API SCS_Winset* scs_winset_intersection(const SCS_Winset* a,
                                            const SCS_Winset* b, char* err_buf,
                                            int err_buf_len);

/**
 * @brief Return the set difference of two winset regions (a \ b).
 * @return New SCS_Winset* (caller owns), or NULL on error.
 */
SCS_API SCS_Winset* scs_winset_difference(const SCS_Winset* a,
                                          const SCS_Winset* b, char* err_buf,
                                          int err_buf_len);

/**
 * @brief Return the symmetric difference of two winset regions (a △ b).
 * @return New SCS_Winset* (caller owns), or NULL on error.
 */
SCS_API SCS_Winset* scs_winset_symmetric_difference(const SCS_Winset* a,
                                                    const SCS_Winset* b,
                                                    char* err_buf,
                                                    int err_buf_len);

// ---------------------------------------------------------------------------
// Geometry — winset clone  (C2.8)
// ---------------------------------------------------------------------------

/**
 * @brief Deep-copy a winset handle.
 * @return New SCS_Winset* (caller owns), or NULL on error.
 */
SCS_API SCS_Winset* scs_winset_clone(const SCS_Winset* ws, char* err_buf,
                                     int err_buf_len);

// ---------------------------------------------------------------------------
// Geometry — Copeland scores and winner  (C4.1, C4.2)
// ---------------------------------------------------------------------------

/**
 * @brief Compute Copeland scores for each alternative.
 *
 * The Copeland score of alternative i is the number of alternatives it beats
 * in pairwise majority comparisons minus the number it loses to.
 *
 * @param alt_xy            Flat [x0,y0,...] array, length 2*n_alts.
 * @param n_alts            Number of alternatives (>= 1).
 * @param voter_ideals_xy   Flat [x0,y0,...] array, length 2*n_voters.
 * @param n_voters          Number of voters (>= 1).
 * @param dist_cfg          Distance configuration.
 * @param k                 SCS_MAJORITY_SIMPLE or 1..n_voters.
 * @param out_scores        Output buffer of length >= n_alts.
 * @param out_len           Length of out_scores (must be >= n_alts).
 * @return SCS_OK or error code.
 */
SCS_API int scs_copeland_scores_2d(const double* alt_xy, int n_alts,
                                   const double* voter_ideals_xy, int n_voters,
                                   const SCS_DistanceConfig* dist_cfg, int k,
                                   int* out_scores, int out_len, char* err_buf,
                                   int err_buf_len);

/**
 * @brief Find the 0-based index of the Copeland winner.
 *
 * Ties are broken by smallest alternative index.
 *
 * @param[out] out_winner_idx  0-based index of the winning alternative.
 * @return SCS_OK or error code.
 */
SCS_API int scs_copeland_winner_2d(const double* alt_xy, int n_alts,
                                   const double* voter_ideals_xy, int n_voters,
                                   const SCS_DistanceConfig* dist_cfg, int k,
                                   int* out_winner_idx, char* err_buf,
                                   int err_buf_len);

// ---------------------------------------------------------------------------
// Geometry — Condorcet winner and core  (C4.3, C4.4)
// ---------------------------------------------------------------------------

/**
 * @brief Check whether a Condorcet winner exists in a finite alternative set.
 *
 * @param[out] out_found   Set to 1 if a Condorcet winner exists, 0 otherwise.
 * @return SCS_OK or error code.
 */
SCS_API int scs_has_condorcet_winner_2d(const double* alt_xy, int n_alts,
                                        const double* voter_ideals_xy,
                                        int n_voters,
                                        const SCS_DistanceConfig* dist_cfg,
                                        int k, int* out_found, char* err_buf,
                                        int err_buf_len);

/**
 * @brief Find the 0-based index of the Condorcet winner, if one exists.
 *
 * @param[out] out_found       Set to 1 if a Condorcet winner exists.
 * @param[out] out_winner_idx  0-based index; valid only when out_found == 1.
 * @return SCS_OK or error code.
 */
SCS_API int scs_condorcet_winner_2d(const double* alt_xy, int n_alts,
                                    const double* voter_ideals_xy, int n_voters,
                                    const SCS_DistanceConfig* dist_cfg, int k,
                                    int* out_found, int* out_winner_idx,
                                    char* err_buf, int err_buf_len);

/**
 * @brief Compute the core in continuous 2D space.
 *
 * The core is non-empty only when a Condorcet point exists. For most voter
 * configurations the core is empty.
 *
 * @param[out] out_found  Set to 1 if the core is non-empty, 0 otherwise.
 * @param[out] out_x      x-coordinate of the core point (valid when found).
 * @param[out] out_y      y-coordinate of the core point (valid when found).
 * @return SCS_OK or error code.
 */
SCS_API int scs_core_2d(const double* voter_ideals_xy, int n_voters,
                        const SCS_DistanceConfig* dist_cfg, int k,
                        int* out_found, double* out_x, double* out_y,
                        char* err_buf, int err_buf_len);

// ---------------------------------------------------------------------------
// Geometry — Uncovered set (discrete and continuous)  (C4.5, C4.6)
// ---------------------------------------------------------------------------

/**
 * @brief Compute the uncovered set over a finite alternative set.
 *
 * An alternative is uncovered if no other alternative beats it both directly
 * and transitively (Miller 1980 covering relation).
 *
 * **Size-query mode:** pass out_indices = NULL or out_capacity = 0 to retrieve
 * *out_n (number of uncovered alternatives) without writing indices.
 *
 * @param[out] out_indices  0-based indices of uncovered alternatives.
 * @param out_capacity      Length of out_indices.
 * @param[out] out_n        Number of uncovered alternatives (required/written).
 * @return SCS_OK, SCS_ERROR_BUFFER_TOO_SMALL, or other error code.
 */
SCS_API int scs_uncovered_set_2d(const double* alt_xy, int n_alts,
                                 const double* voter_ideals_xy, int n_voters,
                                 const SCS_DistanceConfig* dist_cfg, int k,
                                 int* out_indices, int out_capacity, int* out_n,
                                 char* err_buf, int err_buf_len);

/**
 * @brief Count the approximate boundary vertices of the continuous uncovered
 * set polygon.
 *
 * Returns the number of (x,y) pairs that scs_uncovered_set_boundary_2d would
 * write.
 *
 * @param[out] out_xy_pairs  Number of (x,y) pairs in the boundary.
 * @return SCS_OK or error code.
 */
SCS_API int scs_uncovered_set_boundary_size_2d(
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg, int grid_resolution, int k,
    int* out_xy_pairs, char* err_buf, int err_buf_len);

/**
 * @brief Export the approximate boundary of the continuous uncovered set.
 *
 * Uses a grid-based approximation. Pass grid_resolution =
 * SCS_DEFAULT_BOUNDARY_GRID_RESOLUTION for a reasonable default.
 *
 * @param out_xy          Flat [x0,y0,...] buffer, or NULL for size query.
 * @param out_capacity    Capacity of out_xy in (x,y) pairs.
 * @param[out] out_n      Pairs required / written.
 * @return SCS_OK, SCS_ERROR_BUFFER_TOO_SMALL, or other error code.
 */
SCS_API int scs_uncovered_set_boundary_2d(const double* voter_ideals_xy,
                                          int n_voters,
                                          const SCS_DistanceConfig* dist_cfg,
                                          int grid_resolution, int k,
                                          double* out_xy, int out_capacity,
                                          int* out_n, char* err_buf,
                                          int err_buf_len);

// ---------------------------------------------------------------------------
// Geometry — Heart (discrete and continuous)  (C4.7, C4.8)
// ---------------------------------------------------------------------------

/**
 * @brief Compute the Heart over a finite alternative set (discrete variant).
 *
 * ⚠ APPROXIMATION: the continuous Heart boundary is an open research problem.
 * This discrete variant returns the Heart alternatives from a finite set.
 * See where_we_are.md for caveats.
 *
 * **Size-query mode:** pass out_indices = NULL or out_capacity = 0.
 *
 * @param[out] out_indices  0-based indices of Heart alternatives.
 * @param out_capacity      Length of out_indices.
 * @param[out] out_n        Number of Heart alternatives (required/written).
 * @return SCS_OK, SCS_ERROR_BUFFER_TOO_SMALL, or other error code.
 */
SCS_API int scs_heart_2d(const double* alt_xy, int n_alts,
                         const double* voter_ideals_xy, int n_voters,
                         const SCS_DistanceConfig* dist_cfg, int k,
                         int* out_indices, int out_capacity, int* out_n,
                         char* err_buf, int err_buf_len);

/**
 * @brief Count the approximate boundary vertices of the continuous Heart
 * polygon.
 *
 * @param[out] out_xy_pairs  Number of (x,y) pairs in the boundary.
 * @return SCS_OK or error code.
 */
SCS_API int scs_heart_boundary_size_2d(const double* voter_ideals_xy,
                                       int n_voters,
                                       const SCS_DistanceConfig* dist_cfg,
                                       int grid_resolution, int k,
                                       int* out_xy_pairs, char* err_buf,
                                       int err_buf_len);

/**
 * @brief Export the approximate boundary of the continuous Heart polygon.
 *
 * ⚠ APPROXIMATION: see scs_heart_2d for caveats.
 *
 * @param out_xy          Flat [x0,y0,...] buffer, or NULL for size query.
 * @param out_capacity    Capacity of out_xy in (x,y) pairs.
 * @param[out] out_n      Pairs required / written.
 * @return SCS_OK, SCS_ERROR_BUFFER_TOO_SMALL, or other error code.
 */
SCS_API int scs_heart_boundary_2d(const double* voter_ideals_xy, int n_voters,
                                  const SCS_DistanceConfig* dist_cfg,
                                  int grid_resolution, int k, double* out_xy,
                                  int out_capacity, int* out_n, char* err_buf,
                                  int err_buf_len);

// ---------------------------------------------------------------------------
// Geometry — Yolk  (C3)
// ---------------------------------------------------------------------------

/**
 * @brief Compute an approximation of the 2D k-majority Yolk.
 *
 * ⚠ APPROXIMATION: this function computes the LP yolk (smallest circle
 * intersecting the limiting median lines), not the true Yolk. The two can
 * differ. See the SCS_Yolk2d struct comment and where_we_are.md for details.
 *
 * @param voter_ideals_xy   Flat [x0,y0,...] array, length 2*n_voters.
 * @param n_voters          Number of voters (>= 3 for a non-trivial Yolk).
 * @param dist_cfg          Distance configuration (n_weights == 2; Euclidean
 *                          recommended — the theoretical Yolk is Euclidean).
 * @param k                 SCS_MAJORITY_SIMPLE or 1..n_voters.
 * @param num_samples       Directional samples for the solver. Higher values
 *                          improve accuracy at the cost of runtime. A value
 *                          of 720 (SCS_DEFAULT_BOUNDARY_GRID_RESOLUTION * 48)
 *                          is reasonable for exploratory use.
 * @param[out] out          SCS_Yolk2d result (center and radius).
 * @param err_buf           Optional error message buffer.
 * @param err_buf_len       Length of err_buf.
 * @return SCS_OK or error code.
 */
SCS_API int scs_yolk_2d(const double* voter_ideals_xy, int n_voters,
                        const SCS_DistanceConfig* dist_cfg, int k,
                        int num_samples, SCS_Yolk2d* out, char* err_buf,
                        int err_buf_len);

// ---------------------------------------------------------------------------
// Aggregation — Profile opaque handle and tie-break  (C5.1)
// ---------------------------------------------------------------------------

/** @brief Opaque handle to an ordinal preference profile. */
typedef struct SCS_ProfileImpl SCS_Profile;

/**
 * @brief ABI-stable tie-break selector.
 *
 * SCS_TIEBREAK_RANDOM  requires a non-null SCS_StreamManager* and valid
 *                       stream name at call sites that perform tie-breaking.
 * SCS_TIEBREAK_SMALLEST_INDEX  is deterministic; accepts a null manager.
 */
typedef int32_t SCS_TieBreak;

#define SCS_TIEBREAK_RANDOM ((int32_t)0)
#define SCS_TIEBREAK_SMALLEST_INDEX ((int32_t)1)

// ---------------------------------------------------------------------------
// Aggregation — Profile constructors  (C5.2, C5.3, C5.4)
// ---------------------------------------------------------------------------

/**
 * @brief Build an ordinal preference profile from a 2D spatial model.
 *
 * Each voter ranks alternatives by distance (closest = most preferred).
 * Ties in distance are broken by smallest alternative index.
 *
 * @param alt_xy            Flat [x0,y0,...] array, length 2*n_alts.
 * @param n_alts            Number of alternatives (>= 1).
 * @param voter_ideals_xy   Flat [x0,y0,...] array, length 2*n_voters.
 * @param n_voters          Number of voters (>= 1).
 * @param dist_cfg          Distance configuration (Euclidean recommended).
 * @return New SCS_Profile* (caller owns), or NULL on error.
 */
SCS_API SCS_Profile* scs_profile_build_spatial(
    const double* alt_xy, int n_alts, const double* voter_ideals_xy,
    int n_voters, const SCS_DistanceConfig* dist_cfg, char* err_buf,
    int err_buf_len);

/**
 * @brief Build a profile from a row-major utility matrix.
 *
 * Layout: utilities[voter * n_alts + alt] = utility of voter for alternative.
 * Higher utility means the alternative is preferred. Ties broken by smallest
 * alternative index.
 *
 * @param utilities  Flat row-major array of size n_voters * n_alts.
 * @return New SCS_Profile* (caller owns), or NULL on error.
 */
SCS_API SCS_Profile* scs_profile_from_utility_matrix(const double* utilities,
                                                     int n_voters, int n_alts,
                                                     char* err_buf,
                                                     int err_buf_len);

/**
 * @brief Generate a profile under the impartial culture model.
 *
 * Each voter's ranking is drawn independently and uniformly from all m!
 * orderings (Fisher-Yates shuffle). Purely ordinal; no spatial model.
 *
 * @param mgr          Stream manager (must be non-null).
 * @param stream_name  Named stream to use for randomness.
 * @return New SCS_Profile* (caller owns), or NULL on error.
 */
SCS_API SCS_Profile* scs_profile_impartial_culture(int n_voters, int n_alts,
                                                   SCS_StreamManager* mgr,
                                                   const char* stream_name,
                                                   char* err_buf,
                                                   int err_buf_len);

/**
 * @brief Generate a spatial profile with voters drawn from U([lo,hi]²).
 *
 * Both alternatives and voter ideals are drawn from the uniform distribution
 * on [lo, hi]² using the named stream. Only n_dims == 2 is supported.
 *
 * @param mgr          Stream manager (must be non-null).
 * @param stream_name  Named stream to use for randomness.
 * @return New SCS_Profile* (caller owns), or NULL on error.
 */
SCS_API SCS_Profile* scs_profile_uniform_spatial(
    int n_voters, int n_alts, int n_dims, double lo, double hi,
    const SCS_DistanceConfig* dist_cfg, SCS_StreamManager* mgr,
    const char* stream_name, char* err_buf, int err_buf_len);

/**
 * @brief Generate a spatial profile with voter ideals drawn from
 * N(mean, stddev²) per dimension.
 *
 * Both alternatives and voter ideals are drawn from the same normal
 * distribution. Only n_dims == 2 is supported.
 *
 * @param mgr          Stream manager (must be non-null).
 * @param stream_name  Named stream to use for randomness.
 * @return New SCS_Profile* (caller owns), or NULL on error.
 */
SCS_API SCS_Profile* scs_profile_gaussian_spatial(
    int n_voters, int n_alts, int n_dims, double mean, double stddev,
    const SCS_DistanceConfig* dist_cfg, SCS_StreamManager* mgr,
    const char* stream_name, char* err_buf, int err_buf_len);

// ---------------------------------------------------------------------------
// Aggregation — Profile lifecycle and inspection  (C5.5)
// ---------------------------------------------------------------------------

/**
 * @brief Destroy a profile handle. NULL is a no-op.
 */
SCS_API void scs_profile_destroy(SCS_Profile* p);

/**
 * @brief Query the dimensions of a profile.
 *
 * @param[out] out_n_voters  Number of voters.
 * @param[out] out_n_alts    Number of alternatives.
 * @return SCS_OK or error code.
 */
SCS_API int scs_profile_dims(const SCS_Profile* p, int* out_n_voters,
                             int* out_n_alts, char* err_buf, int err_buf_len);

/**
 * @brief Copy one voter's ranking into a caller-supplied buffer.
 *
 * out_ranking[r] is the index of the alternative at rank r (0 = most
 * preferred). The buffer must have length >= n_alts.
 *
 * @param voter    0-based voter index.
 * @param out_len  Length of out_ranking (must be >= n_alts).
 * @return SCS_OK or error code.
 */
SCS_API int scs_profile_get_ranking(const SCS_Profile* p, int voter,
                                    int* out_ranking, int out_len,
                                    char* err_buf, int err_buf_len);

// ---------------------------------------------------------------------------
// Aggregation — Profile bulk export  (C5.6)
// ---------------------------------------------------------------------------

/**
 * @brief Export all rankings into a flat row-major buffer.
 *
 * Layout: out_rankings[voter * n_alts + rank] = alternative index.
 * The buffer must have length >= n_voters * n_alts.
 *
 * @param out_len  Length of out_rankings (must be >= n_voters * n_alts).
 * @return SCS_OK or error code.
 */
SCS_API int scs_profile_export_rankings(const SCS_Profile* p, int* out_rankings,
                                        int out_len, char* err_buf,
                                        int err_buf_len);

// ---------------------------------------------------------------------------
// Aggregation — Profile clone  (C5.7)
// ---------------------------------------------------------------------------

/**
 * @brief Deep-copy a profile handle.
 * @return New SCS_Profile* (caller owns), or NULL on error.
 */
SCS_API SCS_Profile* scs_profile_clone(const SCS_Profile* p, char* err_buf,
                                       int err_buf_len);

// ---------------------------------------------------------------------------
// Aggregation — Voting rules  (C6)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// C6.1 — Plurality
// ---------------------------------------------------------------------------

/**
 * @brief Compute plurality scores (first-place vote counts per alternative).
 * @param out_len Must be >= p->n_alts.
 * @return SCS_OK or error code.
 */
SCS_API int scs_plurality_scores(const SCS_Profile* p, int* out_scores,
                                 int out_len, char* err_buf, int err_buf_len);

/**
 * @brief Return all plurality winners (alternatives tied for the most
 * first-place votes).
 *
 * **Size-query mode:** pass out_winners = NULL or out_capacity = 0.
 *
 * @param[out] out_n  Number of winners required / written.
 * @return SCS_OK, SCS_ERROR_BUFFER_TOO_SMALL, or other error code.
 */
SCS_API int scs_plurality_all_winners(const SCS_Profile* p, int* out_winners,
                                      int out_capacity, int* out_n,
                                      char* err_buf, int err_buf_len);

/**
 * @brief Return a single plurality winner, applying tie_break if needed.
 *
 * @param tie_break   SCS_TIEBREAK_RANDOM or SCS_TIEBREAK_SMALLEST_INDEX.
 * @param mgr         Required when tie_break == SCS_TIEBREAK_RANDOM; may be
 *                    NULL when SCS_TIEBREAK_SMALLEST_INDEX.
 * @param stream_name Named stream for randomness; required with kRandom.
 * @param[out] out_winner  0-based index of the winner.
 * @return SCS_OK or error code.
 */
SCS_API int scs_plurality_one_winner(const SCS_Profile* p,
                                     SCS_TieBreak tie_break,
                                     SCS_StreamManager* mgr,
                                     const char* stream_name, int* out_winner,
                                     char* err_buf, int err_buf_len);

// ---------------------------------------------------------------------------
// C6.2 — Borda Count
// ---------------------------------------------------------------------------

/**
 * @brief Compute Borda scores (sum of positional weights n_alts-1, n_alts-2,
 * ..., 0 per voter).
 * @param out_len Must be >= p->n_alts.
 */
SCS_API int scs_borda_scores(const SCS_Profile* p, int* out_scores, int out_len,
                             char* err_buf, int err_buf_len);

/** @brief All Borda winners (tied for highest Borda score). Size-query mode
 * supported. */
SCS_API int scs_borda_all_winners(const SCS_Profile* p, int* out_winners,
                                  int out_capacity, int* out_n, char* err_buf,
                                  int err_buf_len);

/** @brief Single Borda winner with tie-breaking. */
SCS_API int scs_borda_one_winner(const SCS_Profile* p, SCS_TieBreak tie_break,
                                 SCS_StreamManager* mgr,
                                 const char* stream_name, int* out_winner,
                                 char* err_buf, int err_buf_len);

/**
 * @brief Full social ordering by descending Borda score.
 *
 * out_ranking[0] is the most-preferred alternative; ties within score groups
 * are broken by tie_break.
 *
 * @param out_len Must be >= p->n_alts.
 */
SCS_API int scs_borda_ranking(const SCS_Profile* p, SCS_TieBreak tie_break,
                              SCS_StreamManager* mgr, const char* stream_name,
                              int* out_ranking, int out_len, char* err_buf,
                              int err_buf_len);

// ---------------------------------------------------------------------------
// C6.3 — Anti-plurality
// ---------------------------------------------------------------------------

/**
 * @brief Compute anti-plurality scores.
 *
 * scores[j] = number of voters for whom alternative j is NOT ranked last.
 * Higher is better; the winner maximises this count (equivalently, minimises
 * last-place appearances).
 *
 * @param out_len Must be >= p->n_alts.
 */
SCS_API int scs_antiplurality_scores(const SCS_Profile* p, int* out_scores,
                                     int out_len, char* err_buf,
                                     int err_buf_len);

/** @brief All anti-plurality winners. Size-query mode supported. */
SCS_API int scs_antiplurality_all_winners(const SCS_Profile* p,
                                          int* out_winners, int out_capacity,
                                          int* out_n, char* err_buf,
                                          int err_buf_len);

/** @brief Single anti-plurality winner with tie-breaking. */
SCS_API int scs_antiplurality_one_winner(
    const SCS_Profile* p, SCS_TieBreak tie_break, SCS_StreamManager* mgr,
    const char* stream_name, int* out_winner, char* err_buf, int err_buf_len);

// ---------------------------------------------------------------------------
// C6.4 — Generic positional scoring rule
// ---------------------------------------------------------------------------

/**
 * @brief Compute scores under a generic positional scoring rule.
 *
 * @param score_weights  Non-increasing array of length n_weights == p->n_alts.
 *                       score_weights[r] is the score awarded to the
 *                       alternative ranked r-th (0 = best).
 * @param out_scores     Output as doubles to avoid truncation. Length >=
 *                       p->n_alts.
 */
SCS_API int scs_scoring_rule_scores(const SCS_Profile* p,
                                    const double* score_weights, int n_weights,
                                    double* out_scores, int out_len,
                                    char* err_buf, int err_buf_len);

/** @brief All scoring-rule winners. Size-query mode supported. */
SCS_API int scs_scoring_rule_all_winners(const SCS_Profile* p,
                                         const double* score_weights,
                                         int n_weights, int* out_winners,
                                         int out_capacity, int* out_n,
                                         char* err_buf, int err_buf_len);

/** @brief Single scoring-rule winner with tie-breaking. */
SCS_API int scs_scoring_rule_one_winner(
    const SCS_Profile* p, const double* score_weights, int n_weights,
    SCS_TieBreak tie_break, SCS_StreamManager* mgr, const char* stream_name,
    int* out_winner, char* err_buf, int err_buf_len);

// ---------------------------------------------------------------------------
// C6.5 — Approval voting (spatial and ordinal top-k variants)
// ---------------------------------------------------------------------------

/**
 * @brief Compute approval scores under the spatial threshold model.
 *
 * A voter approves an alternative if its distance from the voter's ideal point
 * is at most @p threshold.
 *
 * @param threshold   Non-negative approval radius.
 * @param out_len     Must be >= n_alts.
 */
SCS_API int scs_approval_scores_spatial(const double* alt_xy, int n_alts,
                                        const double* voter_ideals_xy,
                                        int n_voters, double threshold,
                                        const SCS_DistanceConfig* dist_cfg,
                                        int* out_scores, int out_len,
                                        char* err_buf, int err_buf_len);

/**
 * @brief All spatial-approval winners. Size-query mode supported.
 *
 * Returns alternatives with the highest approval count (at least one approver).
 * If no alternative receives any approval, the output set is empty.
 */
SCS_API int scs_approval_all_winners_spatial(const double* alt_xy, int n_alts,
                                             const double* voter_ideals_xy,
                                             int n_voters, double threshold,
                                             const SCS_DistanceConfig* dist_cfg,
                                             int* out_winners, int out_capacity,
                                             int* out_n, char* err_buf,
                                             int err_buf_len);

/**
 * @brief Compute ordinal top-k approval scores.
 *
 * Each voter approves their top @p k ranked alternatives. Score = approval
 * count.
 *
 * @param k  Must be in [1, n_alts].
 */
SCS_API int scs_approval_scores_topk(const SCS_Profile* p, int k,
                                     int* out_scores, int out_len,
                                     char* err_buf, int err_buf_len);

/** @brief All top-k approval winners. Size-query mode supported. */
SCS_API int scs_approval_all_winners_topk(const SCS_Profile* p, int k,
                                          int* out_winners, int out_capacity,
                                          int* out_n, char* err_buf,
                                          int err_buf_len);

// ---------------------------------------------------------------------------
// Aggregation — Social rankings and properties  (C7)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// C7.1 — Rank by scores
// ---------------------------------------------------------------------------

/**
 * @brief Sort alternatives by descending score, with tie-breaking.
 *
 * Produces a full ordering of n_alts alternatives from a flat score array.
 * Ties within a score group are broken by tie_break.
 *
 * @param scores      Double score vector of length n_alts.
 * @param n_alts      Number of alternatives (>= 1).
 * @param out_ranking Output: alternative indices 0…n_alts-1, best first.
 *                    out_len must be >= n_alts.
 * @return SCS_OK or error code.
 */
SCS_API int scs_rank_by_scores(const double* scores, int n_alts,
                               SCS_TieBreak tie_break, SCS_StreamManager* mgr,
                               const char* stream_name, int* out_ranking,
                               int out_len, char* err_buf, int err_buf_len);

// ---------------------------------------------------------------------------
// C7.2 — Pairwise ranking from matrix
// ---------------------------------------------------------------------------

/**
 * @brief Rank alternatives by Copeland score derived from a pairwise matrix.
 *
 * The Copeland score of alternative i is the number of alternatives j for
 * which @p pairwise_matrix[i * n_alts + j] == SCS_PAIRWISE_WIN.
 *
 * The typical input is the matrix produced by @c scs_pairwise_matrix_2d,
 * but any conforming SCS_PairwiseResult matrix is accepted.
 *
 * @param pairwise_matrix  Row-major n_alts × n_alts SCS_PairwiseResult array.
 * @param out_ranking      Alternative indices, best (highest Copeland) first.
 *                         out_len must be >= n_alts.
 * @return SCS_OK or error code.
 */
SCS_API int scs_pairwise_ranking_from_matrix(
    const SCS_PairwiseResult* pairwise_matrix, int n_alts,
    SCS_TieBreak tie_break, SCS_StreamManager* mgr, const char* stream_name,
    int* out_ranking, int out_len, char* err_buf, int err_buf_len);

// ---------------------------------------------------------------------------
// C7.3 — Pareto efficiency
// ---------------------------------------------------------------------------

/**
 * @brief Return the Pareto set: all alternatives not Pareto-dominated.
 *
 * Alternative a Pareto-dominates b if every voter weakly prefers a and at
 * least one voter strictly prefers a.  The Pareto set is always non-empty.
 *
 * **Size-query mode:** pass out_indices = NULL or out_capacity = 0.
 *
 * @param[out] out_n  Number of Pareto-efficient alternatives.
 * @return SCS_OK, SCS_ERROR_BUFFER_TOO_SMALL, or other error code.
 */
SCS_API int scs_pareto_set(const SCS_Profile* p, int* out_indices,
                           int out_capacity, int* out_n, char* err_buf,
                           int err_buf_len);

/**
 * @brief Check whether a single alternative is Pareto-efficient.
 *
 * @param alt_idx    0-based index of the alternative to test.
 * @param[out] out   Set to 1 if Pareto-efficient, 0 otherwise.
 * @return SCS_OK or error code.
 */
SCS_API int scs_is_pareto_efficient(const SCS_Profile* p, int alt_idx, int* out,
                                    char* err_buf, int err_buf_len);

// ---------------------------------------------------------------------------
// C7.4 — Condorcet and majority-selection predicates
// ---------------------------------------------------------------------------

/**
 * @brief Check whether the majority relation from the profile has a Condorcet
 * winner.
 *
 * A Condorcet winner beats every other alternative by strict majority (more
 * than half of voters).
 *
 * @param[out] out_found  Set to 1 if a Condorcet winner exists, 0 otherwise.
 * @return SCS_OK or error code.
 */
SCS_API int scs_has_condorcet_winner_profile(const SCS_Profile* p,
                                             int* out_found, char* err_buf,
                                             int err_buf_len);

/**
 * @brief Return the Condorcet winner from the profile, if one exists.
 *
 * When no Condorcet winner exists, @p out_winner is left untouched.
 *
 * @param[out] out_found   Set to 1 if a winner was found, 0 otherwise.
 * @param[out] out_winner  0-based index of the winner; untouched if not found.
 *                         May be NULL (caller only needs the found flag).
 * @return SCS_OK or error code.
 */
SCS_API int scs_condorcet_winner_profile(const SCS_Profile* p, int* out_found,
                                         int* out_winner, char* err_buf,
                                         int err_buf_len);

/**
 * @brief Check whether a given alternative is Condorcet-consistent.
 *
 * Returns 1 in two cases:
 *   (a) No Condorcet winner exists (vacuously consistent).
 *   (b) A Condorcet winner exists and alt_idx is that winner.
 *
 * A correct Condorcet-consistent voting rule must always select an alternative
 * that passes this predicate.
 *
 * @param alt_idx   0-based index of the alternative to test.
 * @param[out] out  Set to 1 if Condorcet-consistent, 0 otherwise.
 * @return SCS_OK or error code.
 */
SCS_API int scs_is_selected_by_condorcet_consistent_rules(const SCS_Profile* p,
                                                          int alt_idx, int* out,
                                                          char* err_buf,
                                                          int err_buf_len);

/**
 * @brief Check whether a given alternative is majority-consistent.
 *
 * An alternative is majority-consistent iff it beats every other alternative
 * by strict majority (i.e. it IS the Condorcet winner).  This is a stronger
 * condition than Condorcet consistency: if no Condorcet winner exists, this
 * returns 0 for every candidate.
 *
 * @param alt_idx   0-based index of the alternative to test.
 * @param[out] out  Set to 1 if majority-consistent, 0 otherwise.
 * @return SCS_OK or error code.
 */
SCS_API int scs_is_selected_by_majority_consistent_rules(const SCS_Profile* p,
                                                         int alt_idx, int* out,
                                                         char* err_buf,
                                                         int err_buf_len);

// ---------------------------------------------------------------------------
// Centrality measures
// ---------------------------------------------------------------------------

/**
 * @brief Coordinate-wise median of voter ideal points (marginal median).
 *
 * Each output coordinate is the median of the corresponding input coordinates
 * computed independently (issue-by-issue median voter; Black 1948).  Not
 * generally a majority-rule equilibrium in 2D.
 *
 * @param voter_ideals_xy  Flat [x0,y0,...] array, length 2*n_voters.
 * @param n_voters         Number of voters (>= 1).
 * @param[out] out_x       x-coordinate of the marginal median.
 * @param[out] out_y       y-coordinate of the marginal median.
 * @return SCS_OK or error code.
 */
SCS_API int scs_marginal_median_2d(const double* voter_ideals_xy, int n_voters,
                                   double* out_x, double* out_y, char* err_buf,
                                   int err_buf_len);

/**
 * @brief Coordinate-wise arithmetic mean (centroid) of voter ideal points.
 *
 * Each output coordinate is the arithmetic mean of the corresponding input
 * coordinates (centre of mass / mean voter position).  Not generally a
 * majority-rule equilibrium in 2D.
 *
 * @param voter_ideals_xy  Flat [x0,y0,...] array, length 2*n_voters.
 * @param n_voters         Number of voters (>= 1).
 * @param[out] out_x       x-coordinate of the centroid.
 * @param[out] out_y       y-coordinate of the centroid.
 * @return SCS_OK or error code.
 */
SCS_API int scs_centroid_2d(const double* voter_ideals_xy, int n_voters,
                            double* out_x, double* out_y, char* err_buf,
                            int err_buf_len);

/**
 * @brief Coordinate-wise geometric mean of voter ideal points.
 *
 * Each output coordinate is exp(mean(log(xᵢ))).  Requires all coordinates to
 * be strictly positive; returns SCS_ERROR_INVALID_ARGUMENT with a descriptive
 * message if any coordinate is <= 0 (e.g. NOMINATE-scale [-1,1] data).
 *
 * @param voter_ideals_xy  Flat [x0,y0,...] array, length 2*n_voters.
 * @param n_voters         Number of voters (>= 1).
 * @param[out] out_x       x-coordinate of the geometric mean.
 * @param[out] out_y       y-coordinate of the geometric mean.
 * @return SCS_OK or error code.
 */
SCS_API int scs_geometric_mean_2d(const double* voter_ideals_xy, int n_voters,
                                  double* out_x, double* out_y, char* err_buf,
                                  int err_buf_len);

#ifdef __cplusplus
}  // extern "C"
#endif
