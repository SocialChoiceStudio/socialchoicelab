// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// test_c_api.cpp — GTest suite for the scs_api C boundary.
//
// All calls use only the types and functions declared in scs_api.h
// (C types: plain structs, raw pointers, int, double, etc.).
// No C++ core headers are included here.

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

#include "scs_api.h"  // NOLINT(build/include_subdir)

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static SCS_LossConfig make_quadratic(double sensitivity = 1.0) {
  SCS_LossConfig cfg{};
  cfg.loss_type = SCS_LOSS_QUADRATIC;
  cfg.sensitivity = sensitivity;
  cfg.max_loss = 1.0;
  cfg.steepness = 1.0;
  cfg.threshold = 0.5;
  return cfg;
}

static SCS_LossConfig make_linear(double sensitivity = 1.0) {
  SCS_LossConfig cfg{};
  cfg.loss_type = SCS_LOSS_LINEAR;
  cfg.sensitivity = sensitivity;
  cfg.max_loss = 1.0;
  cfg.steepness = 1.0;
  cfg.threshold = 0.5;
  return cfg;
}

// ---------------------------------------------------------------------------
// Distance tests
// ---------------------------------------------------------------------------

TEST(CApi_Distance, EuclideanTrivial) {
  double ideal[2] = {0.0, 0.0};
  double alt[2] = {3.0, 4.0};
  double weights[2] = {1.0, 1.0};
  SCS_DistanceConfig dc{};
  dc.distance_type = SCS_DIST_EUCLIDEAN;
  dc.order_p = 2.0;
  dc.salience_weights = weights;
  dc.n_weights = 2;

  double d = 0.0;
  char err[256] = {};
  int rc = scs_calculate_distance(ideal, alt, 2, &dc, &d, err, 256);
  EXPECT_EQ(rc, SCS_OK) << err;
  EXPECT_NEAR(d, 5.0, 1e-12);
}

TEST(CApi_Distance, ManhattanUnit) {
  double ideal[2] = {1.0, 1.0};
  double alt[2] = {3.0, 5.0};
  double weights[2] = {1.0, 1.0};
  SCS_DistanceConfig dc{};
  dc.distance_type = SCS_DIST_MANHATTAN;
  dc.order_p = 1.0;
  dc.salience_weights = weights;
  dc.n_weights = 2;

  double d = 0.0;
  int rc = scs_calculate_distance(ideal, alt, 2, &dc, &d, nullptr, 0);
  EXPECT_EQ(rc, SCS_OK);
  EXPECT_NEAR(d, 6.0, 1e-12);
}

TEST(CApi_Distance, ChebyshevUnit) {
  double ideal[2] = {0.0, 0.0};
  double alt[2] = {2.0, 5.0};
  double weights[2] = {1.0, 1.0};
  SCS_DistanceConfig dc{};
  dc.distance_type = SCS_DIST_CHEBYSHEV;
  dc.salience_weights = weights;
  dc.n_weights = 2;

  double d = 0.0;
  int rc = scs_calculate_distance(ideal, alt, 2, &dc, &d, nullptr, 0);
  EXPECT_EQ(rc, SCS_OK);
  EXPECT_NEAR(d, 5.0, 1e-12);
}

TEST(CApi_Distance, MinkowskiP3) {
  double ideal[2] = {0.0, 0.0};
  double alt[2] = {1.0, 0.0};
  double weights[2] = {1.0, 1.0};
  SCS_DistanceConfig dc{};
  dc.distance_type = SCS_DIST_MINKOWSKI;
  dc.order_p = 3.0;
  dc.salience_weights = weights;
  dc.n_weights = 2;

  double d = 0.0;
  int rc = scs_calculate_distance(ideal, alt, 2, &dc, &d, nullptr, 0);
  EXPECT_EQ(rc, SCS_OK);
  EXPECT_NEAR(d, 1.0, 1e-12);
}

TEST(CApi_Distance, ErrorOnDimensionMismatch) {
  double ideal[2] = {0.0, 0.0};
  double alt[2] = {1.0, 1.0};
  double weights[3] = {1.0, 1.0, 1.0};
  SCS_DistanceConfig dc{};
  dc.distance_type = SCS_DIST_EUCLIDEAN;
  dc.salience_weights = weights;
  dc.n_weights = 3;  // mismatch: n_dims=2, n_weights=3

  double d = 0.0;
  char err[256] = {};
  int rc = scs_calculate_distance(ideal, alt, 2, &dc, &d, err, 256);
  EXPECT_NE(rc, SCS_OK);
  EXPECT_GT(std::strlen(err), 0u);
}

TEST(CApi_Distance, ErrorOnNullPointer) {
  SCS_DistanceConfig dc{};
  double d = 0.0;
  char err[256] = {};
  int rc = scs_calculate_distance(nullptr, nullptr, 2, &dc, &d, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
}

// ---------------------------------------------------------------------------
// Loss / utility tests
// ---------------------------------------------------------------------------

TEST(CApi_Loss, QuadraticAtIdeal) {
  SCS_LossConfig cfg = make_quadratic();
  double u = 0.0;
  int rc = scs_distance_to_utility(0.0, &cfg, &u, nullptr, 0);
  EXPECT_EQ(rc, SCS_OK);
  EXPECT_NEAR(u, 0.0, 1e-12);
}

TEST(CApi_Loss, LinearAtDistance2) {
  SCS_LossConfig cfg = make_linear(2.0);
  double u = 0.0;
  int rc = scs_distance_to_utility(3.0, &cfg, &u, nullptr, 0);
  EXPECT_EQ(rc, SCS_OK);
  EXPECT_NEAR(u, -6.0, 1e-12);
}

TEST(CApi_Loss, NormalizeUtilityRoundTrip) {
  SCS_LossConfig cfg = make_quadratic(1.0);
  // At distance=0 utility=0, at distance=1 utility=-1.
  // normalize_utility(-0.5, max_distance=1.0) should be 0.5.
  double u_raw = -0.5;
  double norm = 0.0;
  int rc = scs_normalize_utility(u_raw, 1.0, &cfg, &norm, nullptr, 0);
  EXPECT_EQ(rc, SCS_OK);
  EXPECT_NEAR(norm, 0.5, 1e-12);
}

TEST(CApi_Loss, ErrorOnNullConfig) {
  double u = 0.0;
  char err[256] = {};
  int rc = scs_distance_to_utility(1.0, nullptr, &u, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
  EXPECT_GT(std::strlen(err), 0u);
}

// ---------------------------------------------------------------------------
// Level set 1D tests
// ---------------------------------------------------------------------------

TEST(CApi_LevelSet1d, TwoPointsQuadratic) {
  SCS_LossConfig lcfg = make_quadratic(1.0);
  // u = -d^2 = -1.0 → d = 1.0; weight=1 → points at ideal ± 1.
  double ideal = 3.0;
  double pts[2] = {0.0, 0.0};
  int n = 0;
  char err[256] = {};
  int rc = scs_level_set_1d(ideal, 1.0, -1.0, &lcfg, pts, &n, err, 256);
  EXPECT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(n, 2);
  EXPECT_NEAR(pts[0], 2.0, 1e-12);
  EXPECT_NEAR(pts[1], 4.0, 1e-12);
}

TEST(CApi_LevelSet1d, ZeroPointsUnreachable) {
  SCS_LossConfig lcfg = make_quadratic(1.0);
  // Utility > 0 is unreachable for quadratic (level_set returns {}).
  double pts[2] = {0.0, 0.0};
  int n = 99;
  int rc = scs_level_set_1d(0.0, 1.0, 1.0, &lcfg, pts, &n, nullptr, 0);
  EXPECT_EQ(rc, SCS_OK);
  EXPECT_EQ(n, 0);
}

TEST(CApi_LevelSet1d, ErrorOnNullPointer) {
  SCS_LossConfig lcfg = make_quadratic();
  int n = 0;
  char err[256] = {};
  int rc = scs_level_set_1d(0.0, 1.0, -1.0, &lcfg, nullptr, &n, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
}

// ---------------------------------------------------------------------------
// Level set 2D tests
// ---------------------------------------------------------------------------

TEST(CApi_LevelSet2d, EuclideanEqualWeightsIsCircle) {
  SCS_LossConfig lcfg = make_quadratic(1.0);
  double weights[2] = {1.0, 1.0};
  SCS_DistanceConfig dc{};
  dc.distance_type = SCS_DIST_EUCLIDEAN;
  dc.order_p = 2.0;
  dc.salience_weights = weights;
  dc.n_weights = 2;

  SCS_LevelSet2d out{};
  char err[256] = {};
  // u = -d^2 = -4.0 → d = 2.0; equal weights → circle with radius 2.
  int rc = scs_level_set_2d(1.0, 2.0, -4.0, &lcfg, &dc, &out, err, 256);
  EXPECT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(out.type, SCS_LEVEL_SET_CIRCLE);
  EXPECT_NEAR(out.center_x, 1.0, 1e-12);
  EXPECT_NEAR(out.center_y, 2.0, 1e-12);
  EXPECT_NEAR(out.param0, 2.0, 1e-12);
}

TEST(CApi_LevelSet2d, EuclideanUnequalWeightsIsEllipse) {
  SCS_LossConfig lcfg = make_quadratic(1.0);
  double weights[2] = {1.0, 2.0};
  SCS_DistanceConfig dc{};
  dc.distance_type = SCS_DIST_EUCLIDEAN;
  dc.salience_weights = weights;
  dc.n_weights = 2;

  SCS_LevelSet2d out{};
  char err[256] = {};
  int rc = scs_level_set_2d(0.0, 0.0, -1.0, &lcfg, &dc, &out, err, 256);
  EXPECT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(out.type, SCS_LEVEL_SET_ELLIPSE);
  EXPECT_NEAR(out.param0, 1.0, 1e-12);  // r/w0 = 1/1
  EXPECT_NEAR(out.param1, 0.5, 1e-12);  // r/w1 = 1/2
}

TEST(CApi_LevelSet2d, ManhattanIsPolygon4Vertices) {
  SCS_LossConfig lcfg = make_quadratic(1.0);
  double weights[2] = {1.0, 1.0};
  SCS_DistanceConfig dc{};
  dc.distance_type = SCS_DIST_MANHATTAN;
  dc.salience_weights = weights;
  dc.n_weights = 2;

  SCS_LevelSet2d out{};
  int rc = scs_level_set_2d(0.0, 0.0, -1.0, &lcfg, &dc, &out, nullptr, 0);
  EXPECT_EQ(rc, SCS_OK);
  EXPECT_EQ(out.type, SCS_LEVEL_SET_POLYGON);
  EXPECT_EQ(out.n_vertices, 4);
}

TEST(CApi_LevelSet2d, ChebyshevIsPolygon4Vertices) {
  SCS_LossConfig lcfg = make_quadratic(1.0);
  double weights[2] = {1.0, 1.0};
  SCS_DistanceConfig dc{};
  dc.distance_type = SCS_DIST_CHEBYSHEV;
  dc.salience_weights = weights;
  dc.n_weights = 2;

  SCS_LevelSet2d out{};
  int rc = scs_level_set_2d(0.0, 0.0, -1.0, &lcfg, &dc, &out, nullptr, 0);
  EXPECT_EQ(rc, SCS_OK);
  EXPECT_EQ(out.type, SCS_LEVEL_SET_POLYGON);
  EXPECT_EQ(out.n_vertices, 4);
}

TEST(CApi_LevelSet2d, MinkowskiP3IsSuperellipse) {
  SCS_LossConfig lcfg = make_quadratic(1.0);
  double weights[2] = {1.0, 1.0};
  SCS_DistanceConfig dc{};
  dc.distance_type = SCS_DIST_MINKOWSKI;
  dc.order_p = 3.0;
  dc.salience_weights = weights;
  dc.n_weights = 2;

  SCS_LevelSet2d out{};
  int rc = scs_level_set_2d(0.0, 0.0, -1.0, &lcfg, &dc, &out, nullptr, 0);
  EXPECT_EQ(rc, SCS_OK);
  EXPECT_EQ(out.type, SCS_LEVEL_SET_SUPERELLIPSE);
  EXPECT_NEAR(out.exponent_p, 3.0, 1e-12);
}

TEST(CApi_LevelSet2d, ErrorOnWrongWeightCount) {
  SCS_LossConfig lcfg = make_quadratic();
  double weights[3] = {1.0, 1.0, 1.0};
  SCS_DistanceConfig dc{};
  dc.distance_type = SCS_DIST_EUCLIDEAN;
  dc.salience_weights = weights;
  dc.n_weights = 3;

  SCS_LevelSet2d out{};
  char err[256] = {};
  int rc = scs_level_set_2d(0.0, 0.0, -1.0, &lcfg, &dc, &out, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
  EXPECT_GT(std::strlen(err), 0u);
}

// ---------------------------------------------------------------------------
// API version tests  (C0.1)
// ---------------------------------------------------------------------------

TEST(CApi_Version, ReturnsCompiledVersion) {
  int major = -1, minor = -1, patch = -1;
  char err[256] = {};
  int rc = scs_api_version(&major, &minor, &patch, err, 256);
  EXPECT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(major, SCS_API_VERSION_MAJOR);
  EXPECT_EQ(minor, SCS_API_VERSION_MINOR);
  EXPECT_EQ(patch, SCS_API_VERSION_PATCH);
}

TEST(CApi_Version, ErrorOnNullOutput) {
  char err[256] = {};
  int rc = scs_api_version(nullptr, nullptr, nullptr, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
  EXPECT_GT(std::strlen(err), 0u);
}

// ---------------------------------------------------------------------------
// Error code distinctness tests  (C0.2)
// ---------------------------------------------------------------------------

TEST(CApi_ErrorCodes, AllDistinct) {
  EXPECT_NE(SCS_OK, SCS_ERROR_INVALID_ARGUMENT);
  EXPECT_NE(SCS_OK, SCS_ERROR_INTERNAL);
  EXPECT_NE(SCS_OK, SCS_ERROR_BUFFER_TOO_SMALL);
  EXPECT_NE(SCS_OK, SCS_ERROR_OUT_OF_MEMORY);
  EXPECT_NE(SCS_ERROR_INVALID_ARGUMENT, SCS_ERROR_INTERNAL);
  EXPECT_NE(SCS_ERROR_INVALID_ARGUMENT, SCS_ERROR_BUFFER_TOO_SMALL);
  EXPECT_NE(SCS_ERROR_INTERNAL, SCS_ERROR_BUFFER_TOO_SMALL);
  EXPECT_NE(SCS_ERROR_BUFFER_TOO_SMALL, SCS_ERROR_OUT_OF_MEMORY);
}

TEST(CApi_ErrorCodes, SentinelValues) {
  EXPECT_EQ(SCS_OK, 0);
  EXPECT_EQ(SCS_ERROR_INVALID_ARGUMENT, 1);
  EXPECT_EQ(SCS_ERROR_INTERNAL, 2);
  EXPECT_EQ(SCS_ERROR_BUFFER_TOO_SMALL, 3);
  EXPECT_EQ(SCS_ERROR_OUT_OF_MEMORY, 4);
}

// ---------------------------------------------------------------------------
// SCS_PairwiseResult domain tests  (C0.3)
// ---------------------------------------------------------------------------

TEST(CApi_Pairwise, ConstantValues) {
  EXPECT_EQ(SCS_PAIRWISE_LOSS, static_cast<SCS_PairwiseResult>(-1));
  EXPECT_EQ(SCS_PAIRWISE_TIE, static_cast<SCS_PairwiseResult>(0));
  EXPECT_EQ(SCS_PAIRWISE_WIN, static_cast<SCS_PairwiseResult>(1));
}

TEST(CApi_Pairwise, AllDistinct) {
  EXPECT_NE(SCS_PAIRWISE_LOSS, SCS_PAIRWISE_TIE);
  EXPECT_NE(SCS_PAIRWISE_TIE, SCS_PAIRWISE_WIN);
  EXPECT_NE(SCS_PAIRWISE_LOSS, SCS_PAIRWISE_WIN);
}

// ---------------------------------------------------------------------------
// Level set → polygon sampling tests
// ---------------------------------------------------------------------------

TEST(CApi_ToPolygon, CircleSamples64) {
  SCS_LevelSet2d ls{};
  ls.type = SCS_LEVEL_SET_CIRCLE;
  ls.center_x = 0.0;
  ls.center_y = 0.0;
  ls.param0 = 1.0;

  constexpr int N = 64;
  double xy[2 * N] = {};
  int n = 0;
  char err[256] = {};
  int rc = scs_level_set_to_polygon(&ls, N, xy, N, &n, err, 256);
  EXPECT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(n, N);
  // Every sampled point should be ~1 unit from origin.
  for (int i = 0; i < n; ++i) {
    double rx = xy[2 * i];
    double ry = xy[2 * i + 1];
    EXPECT_NEAR(std::sqrt(rx * rx + ry * ry), 1.0, 1e-12) << "vertex " << i;
  }
}

TEST(CApi_ToPolygon, PolygonPassthroughVertices) {
  SCS_LevelSet2d ls{};
  ls.type = SCS_LEVEL_SET_POLYGON;
  ls.n_vertices = 4;
  ls.vertices[0] = 2.0;
  ls.vertices[1] = 0.0;
  ls.vertices[2] = 0.0;
  ls.vertices[3] = 2.0;
  ls.vertices[4] = -2.0;
  ls.vertices[5] = 0.0;
  ls.vertices[6] = 0.0;
  ls.vertices[7] = -2.0;

  double xy[8] = {};
  int n = 0;
  int rc = scs_level_set_to_polygon(&ls, 64, xy, 4, &n, nullptr, 0);
  EXPECT_EQ(rc, SCS_OK);
  EXPECT_EQ(n, 4);
  EXPECT_NEAR(xy[0], 2.0, 1e-12);
  EXPECT_NEAR(xy[1], 0.0, 1e-12);
}

TEST(CApi_ToPolygon, ErrorOnFewSamplesForSmoothShape) {
  SCS_LevelSet2d ls{};
  ls.type = SCS_LEVEL_SET_CIRCLE;
  ls.param0 = 1.0;

  double xy[20] = {};
  int n = 0;
  char err[256] = {};
  int rc = scs_level_set_to_polygon(&ls, 2, xy, 20, &n, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
}

TEST(CApi_ToPolygon, SizeQueryNullBuffer) {
  // Size-query mode: pass null buffer, receive required capacity in *out_n.
  SCS_LevelSet2d ls{};
  ls.type = SCS_LEVEL_SET_CIRCLE;
  ls.center_x = 0.0;
  ls.center_y = 0.0;
  ls.param0 = 1.0;

  constexpr int N = 32;
  int n = 0;
  char err[256] = {};
  int rc = scs_level_set_to_polygon(&ls, N, nullptr, 0, &n, err, 256);
  EXPECT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(n, N);
}

TEST(CApi_ToPolygon, BufferTooSmallReturnsError) {
  SCS_LevelSet2d ls{};
  ls.type = SCS_LEVEL_SET_CIRCLE;
  ls.center_x = 0.0;
  ls.center_y = 0.0;
  ls.param0 = 1.0;

  constexpr int N = 64;
  double xy[2 * 8] = {};  // only 8 pairs — far too small for 64 samples
  int n = 0;
  char err[256] = {};
  int rc = scs_level_set_to_polygon(&ls, N, xy, 8, &n, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_BUFFER_TOO_SMALL);
  // *out_n must report the required capacity so the caller can resize.
  EXPECT_EQ(n, N);
  EXPECT_GT(std::strlen(err), 0u);
}

// ---------------------------------------------------------------------------
// scs_ic_polygon_2d tests
// ---------------------------------------------------------------------------

static SCS_DistanceConfig make_euclidean_2d() {
  static double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc{};
  dc.distance_type = SCS_DIST_EUCLIDEAN;
  dc.order_p = 2.0;
  dc.salience_weights = w;
  dc.n_weights = 2;
  return dc;
}

TEST(CApi_IcPolygon2d, EuclideanCircleMatchesLevelSetPath) {
  // Voter at (0,0), SQ at (3,4) → distance = 5 (3-4-5 triangle).
  // The IC polygon should be a circle of radius 5 centred at (0,0).
  SCS_LossConfig lc = make_linear();
  SCS_DistanceConfig dc = make_euclidean_2d();
  constexpr int N = 64;
  double xy[2 * N] = {};
  int out_n = 0;
  char err[256] = {};
  int rc = scs_ic_polygon_2d(0.0, 0.0, 3.0, 4.0, &lc, &dc, N, xy, N, &out_n,
                             err, 256);
  ASSERT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(out_n, N);
  // Every vertex should be at radius ~5 from (0,0).
  for (int i = 0; i < out_n; ++i) {
    double r = std::sqrt(xy[2 * i] * xy[2 * i] + xy[2 * i + 1] * xy[2 * i + 1]);
    EXPECT_NEAR(r, 5.0, 1e-9) << "vertex " << i;
  }
}

TEST(CApi_IcPolygon2d, ManhattanPolygon4Vertices) {
  static double w[2] = {1.0, 1.0};
  SCS_LossConfig lc = make_linear();
  SCS_DistanceConfig dc{};
  dc.distance_type = SCS_DIST_MANHATTAN;
  dc.order_p = 1.0;
  dc.salience_weights = w;
  dc.n_weights = 2;
  // Voter at (0,0), SQ at (1,0) → Manhattan distance = 1.
  // IC should be a diamond (4 vertices) with semi-axes 1.
  constexpr int N = 32;
  double xy[2 * N] = {};
  int out_n = 0;
  char err[256] = {};
  int rc = scs_ic_polygon_2d(0.0, 0.0, 1.0, 0.0, &lc, &dc, N, xy, N, &out_n,
                             err, 256);
  ASSERT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(out_n, 4);
}

TEST(CApi_IcPolygon2d, SizeQueryNullBuffer) {
  SCS_LossConfig lc = make_linear();
  SCS_DistanceConfig dc = make_euclidean_2d();
  int out_n = 0;
  char err[256] = {};
  int rc = scs_ic_polygon_2d(0.0, 0.0, 1.0, 0.0, &lc, &dc, 48, nullptr, 0,
                             &out_n, err, 256);
  ASSERT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(out_n, 48);
}

TEST(CApi_IcPolygon2d, BufferTooSmallReturnsError) {
  SCS_LossConfig lc = make_linear();
  SCS_DistanceConfig dc = make_euclidean_2d();
  double xy[2 * 4] = {};
  int out_n = 0;
  char err[256] = {};
  int rc = scs_ic_polygon_2d(0.0, 0.0, 1.0, 0.0, &lc, &dc, 64, xy, 4, &out_n,
                             err, 256);
  EXPECT_EQ(rc, SCS_ERROR_BUFFER_TOO_SMALL);
  EXPECT_EQ(out_n, 64);
}

TEST(CApi_IcPolygon2d, ErrorOnNullPointer) {
  SCS_LossConfig lc = make_linear();
  SCS_DistanceConfig dc = make_euclidean_2d();
  int out_n = 0;
  char err[256] = {};
  // Null loss_cfg.
  EXPECT_NE(scs_ic_polygon_2d(0.0, 0.0, 1.0, 0.0, nullptr, &dc, 32, nullptr, 0,
                              &out_n, err, 256),
            SCS_OK);
  // Null dist_cfg.
  EXPECT_NE(scs_ic_polygon_2d(0.0, 0.0, 1.0, 0.0, &lc, nullptr, 32, nullptr, 0,
                              &out_n, err, 256),
            SCS_OK);
  // Null out_n.
  EXPECT_NE(scs_ic_polygon_2d(0.0, 0.0, 1.0, 0.0, &lc, &dc, 32, nullptr, 0,
                              nullptr, err, 256),
            SCS_OK);
}

TEST(CApi_IcPolygon2d, ResultMatchesFourCallPath) {
  // Verify compound function produces identical output to the 4-call sequence.
  SCS_LossConfig lc = make_linear();
  SCS_DistanceConfig dc = make_euclidean_2d();
  constexpr int N = 32;

  // Four-call path.
  double dist = 0.0;
  char err[256] = {};
  ASSERT_EQ(scs_calculate_distance((double[]){1.0, 2.0}, (double[]){4.0, 6.0},
                                   2, &dc, &dist, err, 256),
            SCS_OK)
      << err;
  double ul = 0.0;
  ASSERT_EQ(scs_distance_to_utility(dist, &lc, &ul, err, 256), SCS_OK) << err;
  SCS_LevelSet2d ls{};
  ASSERT_EQ(scs_level_set_2d(1.0, 2.0, ul, &lc, &dc, &ls, err, 256), SCS_OK)
      << err;
  double xy4[2 * N] = {};
  int n4 = 0;
  ASSERT_EQ(scs_level_set_to_polygon(&ls, N, xy4, N, &n4, err, 256), SCS_OK)
      << err;

  // Compound path.
  double xy1[2 * N] = {};
  int n1 = 0;
  ASSERT_EQ(
      scs_ic_polygon_2d(1.0, 2.0, 4.0, 6.0, &lc, &dc, N, xy1, N, &n1, err, 256),
      SCS_OK)
      << err;

  ASSERT_EQ(n1, n4);
  for (int i = 0; i < n1 * 2; ++i) {
    EXPECT_NEAR(xy1[i], xy4[i], 1e-12) << "index " << i;
  }
}

// ---------------------------------------------------------------------------
// StreamManager tests
// ---------------------------------------------------------------------------

TEST(CApi_StreamManager, CreateAndDestroy) {
  char err[256] = {};
  SCS_StreamManager* mgr = scs_stream_manager_create(42u, err, 256);
  ASSERT_NE(mgr, nullptr) << err;
  scs_stream_manager_destroy(mgr);
}

TEST(CApi_StreamManager, DestroyNullIsNoOp) {
  scs_stream_manager_destroy(nullptr);  // must not crash
}

TEST(CApi_StreamManager, ReproducibleUniformReal) {
  SCS_StreamManager* m1 = scs_stream_manager_create(12345u, nullptr, 0);
  SCS_StreamManager* m2 = scs_stream_manager_create(12345u, nullptr, 0);
  ASSERT_NE(m1, nullptr);
  ASSERT_NE(m2, nullptr);

  double v1 = 0.0, v2 = 0.0;
  scs_uniform_real(m1, "voters", 0.0, 1.0, &v1, nullptr, 0);
  scs_uniform_real(m2, "voters", 0.0, 1.0, &v2, nullptr, 0);
  EXPECT_EQ(v1, v2);

  scs_stream_manager_destroy(m1);
  scs_stream_manager_destroy(m2);
}

TEST(CApi_StreamManager, RegisterAllowlist) {
  char err[256] = {};
  SCS_StreamManager* mgr = scs_stream_manager_create(1u, err, 256);
  ASSERT_NE(mgr, nullptr) << err;

  const char* names[] = {"voters", "candidates"};
  int rc = scs_register_streams(mgr, names, 2, err, 256);
  EXPECT_EQ(rc, SCS_OK) << err;

  // Allowed stream should work.
  double v = 0.0;
  rc = scs_uniform_real(mgr, "voters", 0.0, 1.0, &v, err, 256);
  EXPECT_EQ(rc, SCS_OK) << err;

  // Unknown stream should fail.
  rc = scs_uniform_real(mgr, "unknown_stream", 0.0, 1.0, &v, err, 256);
  EXPECT_NE(rc, SCS_OK);
  EXPECT_GT(std::strlen(err), 0u);

  scs_stream_manager_destroy(mgr);
}

TEST(CApi_StreamManager, ResetAllRestoresSeed) {
  SCS_StreamManager* mgr = scs_stream_manager_create(99u, nullptr, 0);
  ASSERT_NE(mgr, nullptr);

  double v1 = 0.0;
  scs_uniform_real(mgr, "voters", 0.0, 1.0, &v1, nullptr, 0);

  scs_reset_all(mgr, 99u, nullptr, 0);  // reset to same seed

  double v2 = 0.0;
  scs_uniform_real(mgr, "voters", 0.0, 1.0, &v2, nullptr, 0);

  EXPECT_EQ(v1, v2);
  scs_stream_manager_destroy(mgr);
}

TEST(CApi_Competition, RunTraceAndExportRoundData) {
  double competitor_positions[2] = {0.0, 3.0};
  double competitor_headings[2] = {0.0, 0.0};
  int strategies[2] = {SCS_COMPETITION_STRATEGY_STICKER,
                       SCS_COMPETITION_STRATEGY_STICKER};
  double voters[3] = {0.1, 0.2, 2.9};
  double weights[1] = {1.0};
  SCS_DistanceConfig dc{};
  dc.distance_type = SCS_DIST_EUCLIDEAN;
  dc.order_p = 2.0;
  dc.salience_weights = weights;
  dc.n_weights = 1;

  SCS_CompetitionEngineConfig cfg{};
  cfg.seat_count = 1;
  cfg.seat_rule = SCS_COMPETITION_SEAT_RULE_PLURALITY_TOP_K;
  cfg.step_config.kind = SCS_COMPETITION_STEP_FIXED;
  cfg.step_config.fixed_step_size = 1.0;
  cfg.boundary_policy = SCS_COMPETITION_BOUNDARY_PROJECT_TO_BOX;
  cfg.objective_kind = SCS_COMPETITION_OBJECTIVE_VOTE_SHARE;
  cfg.max_rounds = 2;

  char err[256] = {};
  SCS_CompetitionTrace* trace =
      scs_competition_run(competitor_positions, competitor_headings, strategies,
                          2, voters, 3, 1, &dc, &cfg, nullptr, err, 256);
  ASSERT_NE(trace, nullptr) << err;

  int n_rounds = 0, n_competitors = 0, n_dims = 0;
  EXPECT_EQ(scs_competition_trace_dims(trace, &n_rounds, &n_competitors,
                                       &n_dims, err, 256),
            SCS_OK)
      << err;
  EXPECT_EQ(n_rounds, 2);
  EXPECT_EQ(n_competitors, 2);
  EXPECT_EQ(n_dims, 1);

  int terminated_early = 0;
  SCS_CompetitionTerminationReason reason = SCS_COMPETITION_TERM_CONVERGED;
  EXPECT_EQ(scs_competition_trace_termination(trace, &terminated_early, &reason,
                                              err, 256),
            SCS_OK)
      << err;
  EXPECT_EQ(terminated_early, 0);
  EXPECT_EQ(reason, SCS_COMPETITION_TERM_MAX_ROUNDS);

  double round_positions[2] = {};
  EXPECT_EQ(scs_competition_trace_round_positions(trace, 0, round_positions, 2,
                                                  err, 256),
            SCS_OK)
      << err;
  EXPECT_DOUBLE_EQ(round_positions[0], 0.0);
  EXPECT_DOUBLE_EQ(round_positions[1], 3.0);

  double vote_shares[2] = {};
  EXPECT_EQ(scs_competition_trace_round_vote_shares(trace, 0, vote_shares, 2,
                                                    err, 256),
            SCS_OK)
      << err;
  EXPECT_DOUBLE_EQ(vote_shares[0], 2.0 / 3.0);
  EXPECT_DOUBLE_EQ(vote_shares[1], 1.0 / 3.0);

  int vote_totals[2] = {};
  EXPECT_EQ(scs_competition_trace_round_vote_totals(trace, 0, vote_totals, 2,
                                                    err, 256),
            SCS_OK)
      << err;
  EXPECT_EQ(vote_totals[0], 2);
  EXPECT_EQ(vote_totals[1], 1);

  int seat_totals[2] = {};
  EXPECT_EQ(scs_competition_trace_round_seat_totals(trace, 0, seat_totals, 2,
                                                    err, 256),
            SCS_OK)
      << err;
  EXPECT_EQ(seat_totals[0], 1);
  EXPECT_EQ(seat_totals[1], 0);

  double final_vote_shares[2] = {};
  EXPECT_EQ(scs_competition_trace_final_vote_shares(trace, final_vote_shares, 2,
                                                    err, 256),
            SCS_OK)
      << err;
  EXPECT_DOUBLE_EQ(final_vote_shares[0], 2.0 / 3.0);
  EXPECT_DOUBLE_EQ(final_vote_shares[1], 1.0 / 3.0);

  double final_seat_shares[2] = {};
  EXPECT_EQ(scs_competition_trace_final_seat_shares(trace, final_seat_shares, 2,
                                                    err, 256),
            SCS_OK)
      << err;
  EXPECT_DOUBLE_EQ(final_seat_shares[0], 1.0);
  EXPECT_DOUBLE_EQ(final_seat_shares[1], 0.0);

  int strategy_kinds[2] = {-1, -1};
  EXPECT_EQ(
      scs_competition_trace_strategy_kinds(trace, strategy_kinds, 2, err, 256),
      SCS_OK)
      << err;
  EXPECT_EQ(strategy_kinds[0], SCS_COMPETITION_STRATEGY_STICKER);
  EXPECT_EQ(strategy_kinds[1], SCS_COMPETITION_STRATEGY_STICKER);

  scs_competition_trace_destroy(trace);
}

TEST(CApi_Competition, RunExperimentAndExportSummary) {
  double competitor_positions[2] = {0.0, 3.0};
  int strategies[2] = {SCS_COMPETITION_STRATEGY_STICKER,
                       SCS_COMPETITION_STRATEGY_STICKER};
  double voters[3] = {0.1, 0.2, 2.9};
  double weights[1] = {1.0};
  SCS_DistanceConfig dc{};
  dc.distance_type = SCS_DIST_EUCLIDEAN;
  dc.order_p = 2.0;
  dc.salience_weights = weights;
  dc.n_weights = 1;

  SCS_CompetitionEngineConfig cfg{};
  cfg.seat_count = 1;
  cfg.seat_rule = SCS_COMPETITION_SEAT_RULE_PLURALITY_TOP_K;
  cfg.step_config.kind = SCS_COMPETITION_STEP_FIXED;
  cfg.step_config.fixed_step_size = 1.0;
  cfg.boundary_policy = SCS_COMPETITION_BOUNDARY_PROJECT_TO_BOX;
  cfg.objective_kind = SCS_COMPETITION_OBJECTIVE_VOTE_SHARE;
  cfg.max_rounds = 3;
  cfg.termination.stop_on_convergence = 1;
  cfg.termination.position_epsilon = 1e-12;

  char err[256] = {};
  SCS_CompetitionExperiment* experiment = scs_competition_run_experiment(
      competitor_positions, nullptr, strategies, 2, voters, 3, 1, &dc, &cfg,
      2026u, 3, 0, err, 256);
  ASSERT_NE(experiment, nullptr) << err;

  int num_runs = 0, n_competitors = 0, n_dims = 0;
  EXPECT_EQ(scs_competition_experiment_dims(experiment, &num_runs,
                                            &n_competitors, &n_dims, err, 256),
            SCS_OK)
      << err;
  EXPECT_EQ(num_runs, 3);
  EXPECT_EQ(n_competitors, 2);
  EXPECT_EQ(n_dims, 1);

  double mean_rounds = 0.0;
  double early_rate = 0.0;
  EXPECT_EQ(scs_competition_experiment_summary(experiment, &mean_rounds,
                                               &early_rate, err, 256),
            SCS_OK)
      << err;
  EXPECT_DOUBLE_EQ(mean_rounds, 1.0);
  EXPECT_DOUBLE_EQ(early_rate, 1.0);

  double mean_vote_shares[2] = {};
  EXPECT_EQ(scs_competition_experiment_mean_final_vote_shares(
                experiment, mean_vote_shares, 2, err, 256),
            SCS_OK)
      << err;
  EXPECT_DOUBLE_EQ(mean_vote_shares[0], 2.0 / 3.0);

  int round_counts[3] = {};
  EXPECT_EQ(scs_competition_experiment_run_round_counts(
                experiment, round_counts, 3, err, 256),
            SCS_OK)
      << err;
  EXPECT_EQ(round_counts[0], 1);
  EXPECT_EQ(round_counts[1], 1);
  EXPECT_EQ(round_counts[2], 1);

  SCS_CompetitionTerminationReason reasons[3] = {};
  EXPECT_EQ(scs_competition_experiment_run_termination_reasons(
                experiment, reasons, 3, err, 256),
            SCS_OK)
      << err;
  EXPECT_EQ(reasons[0], SCS_COMPETITION_TERM_CONVERGED);
  EXPECT_EQ(reasons[1], SCS_COMPETITION_TERM_CONVERGED);
  EXPECT_EQ(reasons[2], SCS_COMPETITION_TERM_CONVERGED);

  int early_flags[3] = {};
  EXPECT_EQ(scs_competition_experiment_run_terminated_early_flags(
                experiment, early_flags, 3, err, 256),
            SCS_OK)
      << err;
  EXPECT_EQ(early_flags[0], 1);
  EXPECT_EQ(early_flags[1], 1);
  EXPECT_EQ(early_flags[2], 1);

  double run_vote_shares[6] = {};
  EXPECT_EQ(scs_competition_experiment_run_final_vote_shares(
                experiment, run_vote_shares, 6, err, 256),
            SCS_OK)
      << err;
  EXPECT_DOUBLE_EQ(run_vote_shares[0], 2.0 / 3.0);
  EXPECT_DOUBLE_EQ(run_vote_shares[1], 1.0 / 3.0);
  EXPECT_DOUBLE_EQ(run_vote_shares[2], 2.0 / 3.0);
  EXPECT_DOUBLE_EQ(run_vote_shares[3], 1.0 / 3.0);

  double run_seat_shares[6] = {};
  EXPECT_EQ(scs_competition_experiment_run_final_seat_shares(
                experiment, run_seat_shares, 6, err, 256),
            SCS_OK)
      << err;
  EXPECT_DOUBLE_EQ(run_seat_shares[0], 1.0);
  EXPECT_DOUBLE_EQ(run_seat_shares[1], 0.0);
  EXPECT_DOUBLE_EQ(run_seat_shares[2], 1.0);
  EXPECT_DOUBLE_EQ(run_seat_shares[3], 0.0);

  double run_final_positions[6] = {};
  EXPECT_EQ(scs_competition_experiment_run_final_positions(
                experiment, run_final_positions, 6, err, 256),
            SCS_OK)
      << err;
  EXPECT_DOUBLE_EQ(run_final_positions[0], 0.0);
  EXPECT_DOUBLE_EQ(run_final_positions[1], 3.0);
  EXPECT_DOUBLE_EQ(run_final_positions[2], 0.0);
  EXPECT_DOUBLE_EQ(run_final_positions[3], 3.0);

  scs_competition_experiment_destroy(experiment);
}

TEST(CApi_StreamManager, ResetStreamRestoresSeed) {
  SCS_StreamManager* mgr = scs_stream_manager_create(1u, nullptr, 0);
  ASSERT_NE(mgr, nullptr);

  double v1 = 0.0;
  scs_uniform_real(mgr, "voters", 0.0, 1.0, &v1, nullptr, 0);
  double stream_seed = 0.0;
  // Get the seed that was used the first time by resetting to a known value.
  scs_reset_stream(mgr, "voters", 777u, nullptr, 0);
  double va = 0.0;
  scs_uniform_real(mgr, "voters", 0.0, 1.0, &va, nullptr, 0);
  scs_reset_stream(mgr, "voters", 777u, nullptr, 0);
  double vb = 0.0;
  scs_uniform_real(mgr, "voters", 0.0, 1.0, &vb, nullptr, 0);
  EXPECT_EQ(va, vb);
  (void)v1;
  (void)stream_seed;

  scs_stream_manager_destroy(mgr);
}

TEST(CApi_StreamManager, NormalDraw) {
  SCS_StreamManager* mgr = scs_stream_manager_create(42u, nullptr, 0);
  ASSERT_NE(mgr, nullptr);

  double v = 0.0;
  int rc = scs_normal(mgr, "voters", 0.0, 1.0, &v, nullptr, 0);
  EXPECT_EQ(rc, SCS_OK);
  // Normal draw: basic sanity (finite; |value| < 10 is almost certain).
  EXPECT_TRUE(std::isfinite(v));
  EXPECT_LT(std::abs(v), 10.0);

  scs_stream_manager_destroy(mgr);
}

TEST(CApi_StreamManager, BernoulliDraw) {
  SCS_StreamManager* mgr = scs_stream_manager_create(5u, nullptr, 0);
  ASSERT_NE(mgr, nullptr);

  int v = -1;
  int rc = scs_bernoulli(mgr, "tiebreak", 0.5, &v, nullptr, 0);
  EXPECT_EQ(rc, SCS_OK);
  EXPECT_TRUE(v == 0 || v == 1);

  scs_stream_manager_destroy(mgr);
}

TEST(CApi_StreamManager, UniformIntInRange) {
  SCS_StreamManager* mgr = scs_stream_manager_create(7u, nullptr, 0);
  ASSERT_NE(mgr, nullptr);

  for (int i = 0; i < 20; ++i) {
    int64_t v = 0;
    int rc = scs_uniform_int(mgr, "candidates", 1, 10, &v, nullptr, 0);
    EXPECT_EQ(rc, SCS_OK);
    EXPECT_GE(v, 1);
    EXPECT_LE(v, 10);
  }

  scs_stream_manager_destroy(mgr);
}

TEST(CApi_StreamManager, UniformChoiceInRange) {
  SCS_StreamManager* mgr = scs_stream_manager_create(8u, nullptr, 0);
  ASSERT_NE(mgr, nullptr);

  for (int i = 0; i < 20; ++i) {
    int64_t v = -1;
    int rc = scs_uniform_choice(mgr, "voters", 5, &v, nullptr, 0);
    EXPECT_EQ(rc, SCS_OK);
    EXPECT_GE(v, 0);
    EXPECT_LT(v, 5);
  }

  scs_stream_manager_destroy(mgr);
}

TEST(CApi_StreamManager, SkipAdvancesState) {
  SCS_StreamManager* mgr1 = scs_stream_manager_create(100u, nullptr, 0);
  SCS_StreamManager* mgr2 = scs_stream_manager_create(100u, nullptr, 0);
  ASSERT_NE(mgr1, nullptr);
  ASSERT_NE(mgr2, nullptr);

  // mgr1: draw 1, then skip 4, then draw 1.
  double v1_a = 0.0, v1_b = 0.0;
  scs_uniform_real(mgr1, "voters", 0.0, 1.0, &v1_a, nullptr, 0);
  scs_skip(mgr1, "voters", 4u, nullptr, 0);
  scs_uniform_real(mgr1, "voters", 0.0, 1.0, &v1_b, nullptr, 0);

  // mgr2: draw 6 sequentially, collect #1 and #6.
  double v2_a = 0.0, v2_b = 0.0;
  scs_uniform_real(mgr2, "voters", 0.0, 1.0, &v2_a, nullptr, 0);
  for (int i = 0; i < 4; ++i) {
    double tmp = 0.0;
    scs_uniform_real(mgr2, "voters", 0.0, 1.0, &tmp, nullptr, 0);
  }
  scs_uniform_real(mgr2, "voters", 0.0, 1.0, &v2_b, nullptr, 0);

  EXPECT_EQ(v1_a, v2_a);
  EXPECT_EQ(v1_b, v2_b);

  scs_stream_manager_destroy(mgr1);
  scs_stream_manager_destroy(mgr2);
}

TEST(CApi_StreamManager, ErrorOnNullStream) {
  SCS_StreamManager* mgr = scs_stream_manager_create(1u, nullptr, 0);
  ASSERT_NE(mgr, nullptr);

  double v = 0.0;
  char err[256] = {};
  int rc = scs_uniform_real(nullptr, "voters", 0.0, 1.0, &v, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);

  scs_stream_manager_destroy(mgr);
}

TEST(CApi_StreamManager, ErrorOnInvalidUniformRealRange) {
  SCS_StreamManager* mgr = scs_stream_manager_create(1u, nullptr, 0);
  ASSERT_NE(mgr, nullptr);

  double v = 0.0;
  char err[256] = {};
  // min >= max is invalid for uniform_real.
  int rc = scs_uniform_real(mgr, "voters", 1.0, 0.0, &v, err, 256);
  EXPECT_NE(rc, SCS_OK);
  EXPECT_GT(std::strlen(err), 0u);

  scs_stream_manager_destroy(mgr);
}

// ---------------------------------------------------------------------------
// scs_convex_hull_2d
// ---------------------------------------------------------------------------

TEST(CApi_ConvexHull, Triangle_ThreeVertices) {
  double pts[] = {0.0, 0.0, 1.0, 0.0, 0.0, 1.0};
  double out[6];
  int n = 0;
  char err[256] = {};
  ASSERT_EQ(scs_convex_hull_2d(pts, 3, out, &n, err, 256), SCS_OK);
  EXPECT_EQ(n, 3);
}

TEST(CApi_ConvexHull, InteriorPointExcluded) {
  double pts[] = {0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, 0.5, 0.5};
  double out[10];
  int n = 0;
  char err[256] = {};
  ASSERT_EQ(scs_convex_hull_2d(pts, 5, out, &n, err, 256), SCS_OK);
  EXPECT_EQ(n, 4);
}

TEST(CApi_ConvexHull, FiorinaPlott1978S1) {
  double pts[] = {
    0.2166667, 0.3777778, 0.1666667, 0.2888889, 0.1388889,
    0.4000000, 0.3444444, 0.6055556, 0.9166667, 0.1777778,
  };
  double out[10];
  int n = 0;
  char err[256] = {};
  ASSERT_EQ(scs_convex_hull_2d(pts, 5, out, &n, err, 256), SCS_OK);
  EXPECT_GE(n, 3);
  EXPECT_LE(n, 5);
  for (int i = 0; i < n; ++i) {
    EXPECT_GE(out[2 * i], 0.1);
    EXPECT_LE(out[2 * i], 1.0);
    EXPECT_GE(out[2 * i + 1], 0.1);
    EXPECT_LE(out[2 * i + 1], 0.7);
  }
}

TEST(CApi_ConvexHull, Error_NullPoints) {
  double out[6];
  int n = 0;
  char err[256] = {};
  EXPECT_NE(scs_convex_hull_2d(nullptr, 3, out, &n, err, 256), SCS_OK);
  EXPECT_GT(std::strlen(err), 0u);
}

TEST(CApi_ConvexHull, Error_ZeroPoints) {
  double pts[] = {0.0, 0.0};
  double out[2];
  int n = 0;
  char err[256] = {};
  EXPECT_NE(scs_convex_hull_2d(pts, 0, out, &n, err, 256), SCS_OK);
}

// ---------------------------------------------------------------------------
// scs_majority_prefers_2d, scs_pairwise_matrix_2d,
// scs_weighted_majority_prefers_2d  (C1)
// ---------------------------------------------------------------------------

static SCS_DistanceConfig make_euclidean_dist(double* weights, int n) {
  SCS_DistanceConfig dc{};
  dc.distance_type = SCS_DIST_EUCLIDEAN;
  dc.order_p = 2.0;
  dc.salience_weights = weights;
  dc.n_weights = n;
  return dc;
}

TEST(CApi_Majority, SimpleMajorityPrefers) {
  // 3 voters at (0,0), (1,0), (10,0).
  // A=(0.4,0), B=(5,0): voters 0 and 1 are closer to A; voter 2 to B.
  // Simple majority (k=2 of 3) → A wins.
  double voters[] = {0.0, 0.0, 1.0, 0.0, 10.0, 0.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  int out = -1;
  char err[256] = {};
  int rc = scs_majority_prefers_2d(0.4, 0.0, 5.0, 0.0, voters, 3, &dc,
                                   SCS_MAJORITY_SIMPLE, &out, err, 256);
  EXPECT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(out, 1);
}

TEST(CApi_Majority, SimpleMajorityDoesNotPrefer) {
  // 3 voters at (0,0), (1,0), (10,0).
  // A=(8,0), B=(0.5,0): voter 2 is closer to A; voters 0 and 1 to B.
  // Simple majority → B wins (A does not have majority).
  double voters[] = {0.0, 0.0, 1.0, 0.0, 10.0, 0.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  int out = -1;
  int rc = scs_majority_prefers_2d(8.0, 0.0, 0.5, 0.0, voters, 3, &dc,
                                   SCS_MAJORITY_SIMPLE, &out, nullptr, 0);
  EXPECT_EQ(rc, SCS_OK);
  EXPECT_EQ(out, 0);
}

TEST(CApi_Majority, UnanimityThreshold) {
  // 3 voters at (0,0), (1,0), (2,0). A=(0,0), B=(10,0).
  // All three voters are closer to A than to B, so k=3 (unanimity) holds.
  double voters[] = {0.0, 0.0, 1.0, 0.0, 2.0, 0.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  int out = -1;
  char err[256] = {};
  int rc = scs_majority_prefers_2d(0.0, 0.0, 10.0, 0.0, voters, 3, &dc, 3, &out,
                                   err, 256);
  EXPECT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(out, 1);
}

TEST(CApi_Majority, Error_NullVoters) {
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  int out = -1;
  char err[256] = {};
  int rc = scs_majority_prefers_2d(0.0, 0.0, 1.0, 0.0, nullptr, 3, &dc,
                                   SCS_MAJORITY_SIMPLE, &out, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
}

TEST(CApi_Majority, Error_ZeroVoters) {
  double voters[] = {0.0, 0.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  int out = -1;
  char err[256] = {};
  int rc = scs_majority_prefers_2d(0.0, 0.0, 1.0, 0.0, voters, 0, &dc,
                                   SCS_MAJORITY_SIMPLE, &out, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
  EXPECT_GT(std::strlen(err), 0u);
}

TEST(CApi_Majority, Error_WrongWeightCount) {
  double voters[] = {0.0, 0.0};
  double w[3] = {1.0, 1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 3);  // wrong: 3 weights for 2D
  int out = -1;
  char err[256] = {};
  int rc = scs_majority_prefers_2d(0.0, 0.0, 1.0, 0.0, voters, 1, &dc,
                                   SCS_MAJORITY_SIMPLE, &out, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
  EXPECT_GT(std::strlen(err), 0u);
}

TEST(CApi_PairwiseMatrix, CondorcetCycle_AntiSymmetry) {
  // 3-voter Condorcet cycle: A=(0,0), B=(1,0), C=(0.5,1).
  // Voters: (0,0) prefers A; (1,0) prefers B; (0.5,1) prefers C.
  // M is anti-symmetric and diagonal is TIE.
  double alts[] = {0.0, 0.0, 1.0, 0.0, 0.5, 1.0};
  double voters[] = {0.0, 0.0, 1.0, 0.0, 0.5, 1.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  SCS_PairwiseResult M[9] = {};
  char err[256] = {};
  int rc = scs_pairwise_matrix_2d(alts, 3, voters, 3, &dc, SCS_MAJORITY_SIMPLE,
                                  M, 9, err, 256);
  EXPECT_EQ(rc, SCS_OK) << err;

  // Diagonal must be TIE.
  EXPECT_EQ(M[0], SCS_PAIRWISE_TIE);
  EXPECT_EQ(M[4], SCS_PAIRWISE_TIE);
  EXPECT_EQ(M[8], SCS_PAIRWISE_TIE);

  // Anti-symmetry: M[i,j] = -M[j,i].
  EXPECT_EQ(M[1], -M[3]);
  EXPECT_EQ(M[2], -M[6]);
  EXPECT_EQ(M[5], -M[7]);
}

TEST(CApi_PairwiseMatrix, CondorcetWinner) {
  // 3 voters at (0,0), (1,0), (2,0) and alternatives A=(1,0), B=(5,0).
  // All 3 voters prefer A — A is Condorcet winner.
  double alts[] = {1.0, 0.0, 5.0, 0.0};
  double voters[] = {0.0, 0.0, 1.0, 0.0, 2.0, 0.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  SCS_PairwiseResult M[4] = {};
  char err[256] = {};
  int rc = scs_pairwise_matrix_2d(alts, 2, voters, 3, &dc, SCS_MAJORITY_SIMPLE,
                                  M, 4, err, 256);
  EXPECT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(M[0], SCS_PAIRWISE_TIE);   // A vs A
  EXPECT_EQ(M[1], SCS_PAIRWISE_WIN);   // A beats B
  EXPECT_EQ(M[2], SCS_PAIRWISE_LOSS);  // B loses to A
  EXPECT_EQ(M[3], SCS_PAIRWISE_TIE);   // B vs B
}

TEST(CApi_PairwiseMatrix, Error_BufferTooSmall) {
  double alts[] = {0.0, 0.0, 1.0, 0.0, 2.0, 0.0};  // 3 alts → need 9
  double voters[] = {0.0, 0.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  SCS_PairwiseResult M[4] = {};  // only 4 — too small
  char err[256] = {};
  int rc = scs_pairwise_matrix_2d(alts, 3, voters, 1, &dc, SCS_MAJORITY_SIMPLE,
                                  M, 4, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_BUFFER_TOO_SMALL);
  EXPECT_GT(std::strlen(err), 0u);
}

TEST(CApi_WeightedMajority, HighWeightVoterDominates) {
  // 3 voters: 2 prefer B, 1 (with huge weight) prefers A.
  // voter 0 at (0,0) weight=100 prefers A=(0.1,0)
  // voter 1 at (5,0) weight=1 prefers B=(5,0) = ideal
  // voter 2 at (5,0) weight=1 prefers B
  // Weighted: A gets weight 100, B gets weight 2; threshold=0.5 → A wins.
  double voters[] = {0.0, 0.0, 5.0, 0.0, 5.0, 0.0};
  double weights[] = {100.0, 1.0, 1.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  int out = -1;
  char err[256] = {};
  int rc = scs_weighted_majority_prefers_2d(0.1, 0.0, 5.0, 0.0, voters, 3,
                                            weights, &dc, 0.5, &out, err, 256);
  EXPECT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(out, 1);
}

TEST(CApi_WeightedMajority, UniformWeightsMatchesSimpleMajority) {
  // With equal weights and threshold=0.5, result must agree with simple
  // majority (ignoring the ⌊n/2⌋+1 vs ≥ half distinction for odd n).
  double voters[] = {0.0, 0.0, 1.0, 0.0, 10.0, 0.0};
  double weights[] = {1.0, 1.0, 1.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);

  int out_weighted = -1, out_majority = -1;
  scs_weighted_majority_prefers_2d(0.4, 0.0, 5.0, 0.0, voters, 3, weights, &dc,
                                   0.5, &out_weighted, nullptr, 0);
  scs_majority_prefers_2d(0.4, 0.0, 5.0, 0.0, voters, 3, &dc,
                          SCS_MAJORITY_SIMPLE, &out_majority, nullptr, 0);
  EXPECT_EQ(out_weighted, out_majority);
}

TEST(CApi_WeightedMajority, Error_NullWeights) {
  double voters[] = {0.0, 0.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  int out = -1;
  char err[256] = {};
  int rc = scs_weighted_majority_prefers_2d(0.0, 0.0, 1.0, 0.0, voters, 1,
                                            nullptr, &dc, 0.5, &out, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
}

TEST(CApi_WeightedMajority, Error_InvalidThreshold) {
  double voters[] = {0.0, 0.0};
  double weights[] = {1.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  int out = -1;
  char err[256] = {};
  // threshold=0.0 is invalid (must be in (0,1]).
  int rc = scs_weighted_majority_prefers_2d(0.0, 0.0, 1.0, 0.0, voters, 1,
                                            weights, &dc, 0.0, &out, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
  EXPECT_GT(std::strlen(err), 0u);
}

TEST(CApi_WeightedMajority, Error_NegativeWeight) {
  double voters[] = {0.0, 0.0, 1.0, 0.0};
  double weights[] = {1.0, -1.0};  // negative weight
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  int out = -1;
  char err[256] = {};
  int rc = scs_weighted_majority_prefers_2d(0.0, 0.0, 1.0, 0.0, voters, 2,
                                            weights, &dc, 0.5, &out, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
  EXPECT_GT(std::strlen(err), 0u);
}

// ---------------------------------------------------------------------------
// scs_winset_2d and related winset functions  (C2)
// ---------------------------------------------------------------------------

// 3 voters forming a triangle; SQ at centroid gives a non-empty winset.
static void make_triangle_winset(SCS_Winset** out_ws, char* err, int err_len) {
  double voters[] = {0.0, 0.0, 2.0, 0.0, 1.0, 2.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  *out_ws = scs_winset_2d(1.0, 0.667, voters, 3, &dc, SCS_MAJORITY_SIMPLE,
                          SCS_DEFAULT_WINSET_SAMPLES, err, err_len);
}

TEST(CApi_Winset, CreateAndDestroy) {
  char err[256] = {};
  SCS_Winset* ws = nullptr;
  make_triangle_winset(&ws, err, 256);
  ASSERT_NE(ws, nullptr) << err;
  scs_winset_destroy(ws);
}

TEST(CApi_Winset, DestroyNullIsNoOp) { scs_winset_destroy(nullptr); }

TEST(CApi_Winset, IsEmpty_NonEmpty) {
  char err[256] = {};
  SCS_Winset* ws = nullptr;
  make_triangle_winset(&ws, err, 256);
  ASSERT_NE(ws, nullptr) << err;
  int is_empty = -1;
  EXPECT_EQ(scs_winset_is_empty(ws, &is_empty, err, 256), SCS_OK) << err;
  EXPECT_EQ(is_empty, 0);
  scs_winset_destroy(ws);
}

TEST(CApi_Winset, IsEmpty_AtCondorcetWinner) {
  // Single voter at SQ — no point beats it; winset is empty.
  double voters[] = {1.0, 1.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  char err[256] = {};
  SCS_Winset* ws = scs_winset_2d(1.0, 1.0, voters, 1, &dc, SCS_MAJORITY_SIMPLE,
                                 SCS_DEFAULT_WINSET_SAMPLES, err, 256);
  ASSERT_NE(ws, nullptr) << err;
  int is_empty = -1;
  EXPECT_EQ(scs_winset_is_empty(ws, &is_empty, err, 256), SCS_OK) << err;
  EXPECT_EQ(is_empty, 1);
  scs_winset_destroy(ws);
}

TEST(CApi_Winset, ContainsPoint_OutsideIsZero) {
  char err[256] = {};
  SCS_Winset* ws = nullptr;
  make_triangle_winset(&ws, err, 256);
  ASSERT_NE(ws, nullptr) << err;
  int contains = -1;
  EXPECT_EQ(scs_winset_contains_point_2d(ws, 100.0, 100.0, &contains, err, 256),
            SCS_OK);
  EXPECT_EQ(contains, 0);
  scs_winset_destroy(ws);
}

TEST(CApi_Winset, BoundingBox_NonEmpty) {
  char err[256] = {};
  SCS_Winset* ws = nullptr;
  make_triangle_winset(&ws, err, 256);
  ASSERT_NE(ws, nullptr) << err;
  int found = 0;
  double xmin = 0.0, ymin = 0.0, xmax = 0.0, ymax = 0.0;
  EXPECT_EQ(
      scs_winset_bbox_2d(ws, &found, &xmin, &ymin, &xmax, &ymax, err, 256),
      SCS_OK)
      << err;
  EXPECT_EQ(found, 1);
  EXPECT_LT(xmin, xmax);
  EXPECT_LT(ymin, ymax);
  EXPECT_GE(xmin, -0.5);
  EXPECT_LE(xmax, 2.5);
  scs_winset_destroy(ws);
}

TEST(CApi_Winset, BoundingBox_Empty) {
  double voters[] = {1.0, 1.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  char err[256] = {};
  SCS_Winset* ws = scs_winset_2d(1.0, 1.0, voters, 1, &dc, SCS_MAJORITY_SIMPLE,
                                 SCS_DEFAULT_WINSET_SAMPLES, err, 256);
  ASSERT_NE(ws, nullptr) << err;
  int found = -1;
  double xmin = 0.0, ymin = 0.0, xmax = 0.0, ymax = 0.0;
  EXPECT_EQ(
      scs_winset_bbox_2d(ws, &found, &xmin, &ymin, &xmax, &ymax, err, 256),
      SCS_OK);
  EXPECT_EQ(found, 0);
  scs_winset_destroy(ws);
}

TEST(CApi_Winset, BoundarySizeQuery) {
  char err[256] = {};
  SCS_Winset* ws = nullptr;
  make_triangle_winset(&ws, err, 256);
  ASSERT_NE(ws, nullptr) << err;
  int xy_pairs = -1, n_paths = -1;
  EXPECT_EQ(scs_winset_boundary_size_2d(ws, &xy_pairs, &n_paths, err, 256),
            SCS_OK)
      << err;
  EXPECT_GT(xy_pairs, 0);
  EXPECT_GT(n_paths, 0);
  scs_winset_destroy(ws);
}

TEST(CApi_Winset, BoundarySampleFill) {
  char err[256] = {};
  SCS_Winset* ws = nullptr;
  make_triangle_winset(&ws, err, 256);
  ASSERT_NE(ws, nullptr) << err;
  int xy_pairs = 0, n_paths = 0;
  ASSERT_EQ(scs_winset_boundary_size_2d(ws, &xy_pairs, &n_paths, err, 256),
            SCS_OK)
      << err;
  ASSERT_GT(xy_pairs, 0);
  ASSERT_GT(n_paths, 0);
  std::vector<double> xy(static_cast<std::size_t>(2 * xy_pairs), 0.0);
  std::vector<int> starts(static_cast<std::size_t>(n_paths), -1);
  std::vector<int> holes(static_cast<std::size_t>(n_paths), -1);
  int out_xy_n = 0, out_n_paths = 0;
  int rc = scs_winset_sample_boundary_2d(ws, xy.data(), xy_pairs, &out_xy_n,
                                         starts.data(), n_paths, holes.data(),
                                         &out_n_paths, err, 256);
  EXPECT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(out_xy_n, xy_pairs);
  EXPECT_EQ(out_n_paths, n_paths);
  EXPECT_EQ(starts[0], 0);
  EXPECT_GE(holes[0], 0);
  scs_winset_destroy(ws);
}

TEST(CApi_Winset, BoundarySampleNullBufferSizeQuery) {
  char err[256] = {};
  SCS_Winset* ws = nullptr;
  make_triangle_winset(&ws, err, 256);
  ASSERT_NE(ws, nullptr) << err;
  int out_xy_n = -1, out_n_paths = -1;
  int rc = scs_winset_sample_boundary_2d(ws, nullptr, 0, &out_xy_n, nullptr, 0,
                                         nullptr, &out_n_paths, err, 256);
  EXPECT_EQ(rc, SCS_OK) << err;
  EXPECT_GT(out_xy_n, 0);
  EXPECT_GT(out_n_paths, 0);
  scs_winset_destroy(ws);
}

// ---------------------------------------------------------------------------
// Voronoi cells 2D (C2.8)
// ---------------------------------------------------------------------------

TEST(CApi_Voronoi2d, SizeQuery) {
  char err[256] = {};
  double sites[] = {0.0, 0.0, 2.0, 0.0};
  int total_pairs = -1, n_cells = -1;
  int rc = scs_voronoi_cells_2d_size(sites, 2, -0.5, -0.5, 2.5, 0.5,
                                     &total_pairs, &n_cells, err, 256);
  ASSERT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(n_cells, 2);
  EXPECT_GT(total_pairs, 0);
}

TEST(CApi_Voronoi2d, ExportTwoSites) {
  char err[256] = {};
  double sites[] = {0.0, 0.0, 2.0, 0.0};
  int total_pairs = 0, n_cells = 0;
  ASSERT_EQ(scs_voronoi_cells_2d_size(sites, 2, -0.5, -0.5, 2.5, 0.5,
                                      &total_pairs, &n_cells, err, 256),
            SCS_OK)
      << err;
  std::vector<double> xy(static_cast<std::size_t>(2 * total_pairs), 0.0);
  std::vector<int> cell_start(static_cast<std::size_t>(n_cells + 1), -1);
  int out_xy_n = 0;
  int rc = scs_voronoi_cells_2d(sites, 2, -0.5, -0.5, 2.5, 0.5, xy.data(),
                                total_pairs, &out_xy_n, cell_start.data(),
                                n_cells + 1, err, 256);
  EXPECT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(out_xy_n, total_pairs);
  EXPECT_EQ(cell_start[0], 0);
  EXPECT_EQ(cell_start[2], out_xy_n);
  EXPECT_GT(cell_start[1] - cell_start[0], 0);
  EXPECT_GT(cell_start[2] - cell_start[1], 0);
}

TEST(CApi_Voronoi2d, InvalidNSites) {
  char err[256] = {};
  double sites[] = {0.0, 0.0};
  int total_pairs = -1, n_cells = -1;
  int rc = scs_voronoi_cells_2d_size(sites, 0, 0, 0, 1, 1, &total_pairs,
                                     &n_cells, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
  EXPECT_NE(err[0], '\0');
}

TEST(CApi_Voronoi2d, InvalidBbox) {
  char err[256] = {};
  double sites[] = {0.0, 0.0};
  int total_pairs = -1, n_cells = -1;
  int rc = scs_voronoi_cells_2d_size(sites, 1, 1.0, 0, 0.5, 1, &total_pairs,
                                     &n_cells, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
}

// ---------------------------------------------------------------------------
// WinSet interval 1D (C2.9)
// ---------------------------------------------------------------------------

TEST(CApi_WinsetInterval1d, NonEmptyResult) {
  char err[256] = {};
  double voters[] = {-1.0, 0.0, 1.0};
  double lo = 0.0, hi = 0.0;
  int rc = scs_winset_interval_1d(voters, 3, 2.0, &lo, &hi, err, 256);
  EXPECT_EQ(rc, SCS_OK);
  EXPECT_FALSE(std::isnan(lo));
  EXPECT_FALSE(std::isnan(hi));
  // voters {-1,0,1}, seat 2: WinSet = (-2, 2)
  EXPECT_NEAR(lo, -2.0, 1e-9);
  EXPECT_NEAR(hi, 2.0, 1e-9);
}

TEST(CApi_WinsetInterval1d, MedianSeatIsEmpty) {
  char err[256] = {};
  double voters[] = {-1.0, 0.0, 1.0};
  double lo = 99.0, hi = 99.0;
  int rc = scs_winset_interval_1d(voters, 3, 0.0, &lo, &hi, err, 256);
  EXPECT_EQ(rc, SCS_OK);
  EXPECT_TRUE(std::isnan(lo));
  EXPECT_TRUE(std::isnan(hi));
}

TEST(CApi_WinsetInterval1d, NullArgError) {
  char err[256] = {};
  double lo, hi;
  int rc = scs_winset_interval_1d(nullptr, 3, 0.0, &lo, &hi, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
}

TEST(CApi_WinsetInterval1d, ZeroVotersError) {
  char err[256] = {};
  double voters[] = {0.0};
  double lo, hi;
  int rc = scs_winset_interval_1d(voters, 0, 0.0, &lo, &hi, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
}

TEST(CApi_Winset, Clone) {
  char err[256] = {};
  SCS_Winset* ws = nullptr;
  make_triangle_winset(&ws, err, 256);
  ASSERT_NE(ws, nullptr) << err;
  SCS_Winset* clone = scs_winset_clone(ws, err, 256);
  ASSERT_NE(clone, nullptr) << err;
  int e1 = -1, e2 = -1;
  EXPECT_EQ(scs_winset_is_empty(ws, &e1, err, 256), SCS_OK);
  EXPECT_EQ(scs_winset_is_empty(clone, &e2, err, 256), SCS_OK);
  EXPECT_EQ(e1, e2);
  scs_winset_destroy(ws);
  scs_winset_destroy(clone);
}

TEST(CApi_Winset, BooleanUnion_NonEmpty) {
  double voters[] = {0.0, 0.0, 2.0, 0.0, 1.0, 2.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  char err[256] = {};
  SCS_Winset* ws1 = scs_winset_2d(0.5, 0.3, voters, 3, &dc, SCS_MAJORITY_SIMPLE,
                                  SCS_DEFAULT_WINSET_SAMPLES, err, 256);
  SCS_Winset* ws2 = scs_winset_2d(1.5, 0.3, voters, 3, &dc, SCS_MAJORITY_SIMPLE,
                                  SCS_DEFAULT_WINSET_SAMPLES, err, 256);
  ASSERT_NE(ws1, nullptr) << err;
  ASSERT_NE(ws2, nullptr) << err;
  SCS_Winset* u = scs_winset_union(ws1, ws2, err, 256);
  ASSERT_NE(u, nullptr) << err;
  int is_empty = -1;
  EXPECT_EQ(scs_winset_is_empty(u, &is_empty, err, 256), SCS_OK);
  EXPECT_EQ(is_empty, 0);
  scs_winset_destroy(ws1);
  scs_winset_destroy(ws2);
  scs_winset_destroy(u);
}

TEST(CApi_Winset, BooleanIntersection_SelfIsNonEmpty) {
  char err[256] = {};
  SCS_Winset* ws = nullptr;
  make_triangle_winset(&ws, err, 256);
  ASSERT_NE(ws, nullptr) << err;
  SCS_Winset* inter = scs_winset_intersection(ws, ws, err, 256);
  ASSERT_NE(inter, nullptr) << err;
  int is_empty = -1;
  EXPECT_EQ(scs_winset_is_empty(inter, &is_empty, err, 256), SCS_OK);
  EXPECT_EQ(is_empty, 0);
  scs_winset_destroy(ws);
  scs_winset_destroy(inter);
}

TEST(CApi_Winset, BooleanDifference_SelfIsEmpty) {
  char err[256] = {};
  SCS_Winset* ws = nullptr;
  make_triangle_winset(&ws, err, 256);
  ASSERT_NE(ws, nullptr) << err;
  SCS_Winset* diff = scs_winset_difference(ws, ws, err, 256);
  ASSERT_NE(diff, nullptr) << err;
  int is_empty = -1;
  EXPECT_EQ(scs_winset_is_empty(diff, &is_empty, err, 256), SCS_OK);
  EXPECT_EQ(is_empty, 1);
  scs_winset_destroy(ws);
  scs_winset_destroy(diff);
}

TEST(CApi_Winset, Error_NullVoters) {
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  char err[256] = {};
  SCS_Winset* ws = scs_winset_2d(0.0, 0.0, nullptr, 3, &dc, SCS_MAJORITY_SIMPLE,
                                 SCS_DEFAULT_WINSET_SAMPLES, err, 256);
  EXPECT_EQ(ws, nullptr);
  EXPECT_GT(std::strlen(err), 0u);
}

TEST(CApi_Winset, WeightedWinset_NonEmpty) {
  double voters[] = {0.0, 0.0, 2.0, 0.0, 1.0, 2.0};
  double weights[] = {1.0, 1.0, 1.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  char err[256] = {};
  SCS_Winset* ws =
      scs_weighted_winset_2d(1.0, 0.667, voters, 3, weights, &dc, 0.5,
                             SCS_DEFAULT_WINSET_SAMPLES, err, 256);
  ASSERT_NE(ws, nullptr) << err;
  int is_empty = -1;
  EXPECT_EQ(scs_winset_is_empty(ws, &is_empty, err, 256), SCS_OK);
  EXPECT_EQ(is_empty, 0);
  scs_winset_destroy(ws);
}

TEST(CApi_Winset, VetoWinset_VetoAtSQGivesEmpty) {
  // Veto player at SQ: their preferred region is empty → result is empty.
  double voters[] = {0.0, 0.0, 2.0, 0.0, 1.0, 2.0};
  double veto[] = {1.0, 0.667};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  char err[256] = {};
  SCS_Winset* ws = scs_winset_with_veto_2d(
      1.0, 0.667, voters, 3, &dc, veto, 1, SCS_MAJORITY_SIMPLE,
      SCS_DEFAULT_WINSET_SAMPLES, err, 256);
  ASSERT_NE(ws, nullptr) << err;
  int is_empty = -1;
  EXPECT_EQ(scs_winset_is_empty(ws, &is_empty, err, 256), SCS_OK);
  EXPECT_EQ(is_empty, 1);
  scs_winset_destroy(ws);
}

// ---------------------------------------------------------------------------
// scs_yolk_2d  (C3)
// ---------------------------------------------------------------------------

TEST(CApi_Yolk, EquilateralTriangle_CenterAndRadius) {
  // 3 voters at equilateral triangle: (0,0), (2,0), (1,√3).
  // Known: center ≈ centroid (1, √3/3), radius ≈ √3/3 ≈ 0.577.
  const double sq3 = std::sqrt(3.0);
  double voters[] = {0.0, 0.0, 2.0, 0.0, 1.0, sq3};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  SCS_Yolk2d yolk{};
  char err[256] = {};
  int rc =
      scs_yolk_2d(voters, 3, &dc, SCS_MAJORITY_SIMPLE, 720, &yolk, err, 256);
  EXPECT_EQ(rc, SCS_OK) << err;
  EXPECT_NEAR(yolk.center_x, 1.0, 0.05);
  EXPECT_NEAR(yolk.center_y, sq3 / 3.0, 0.05);
  EXPECT_NEAR(yolk.radius, sq3 / 3.0, 0.05);
  EXPECT_GE(yolk.radius, 0.0);
}

TEST(CApi_Yolk, CollinearVoters_RadiusNearZero) {
  // Median voter at (1,0) is Condorcet winner → Yolk radius ≈ 0.
  double voters[] = {0.0, 0.0, 1.0, 0.0, 2.0, 0.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  SCS_Yolk2d yolk{};
  char err[256] = {};
  int rc =
      scs_yolk_2d(voters, 3, &dc, SCS_MAJORITY_SIMPLE, 720, &yolk, err, 256);
  EXPECT_EQ(rc, SCS_OK) << err;
  EXPECT_LT(yolk.radius, 0.05);
  EXPECT_GE(yolk.radius, 0.0);
}

TEST(CApi_Yolk, Error_TooFewVoters) {
  double voters[] = {0.0, 0.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  SCS_Yolk2d yolk{};
  char err[256] = {};
  int rc =
      scs_yolk_2d(voters, 1, &dc, SCS_MAJORITY_SIMPLE, 720, &yolk, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
  EXPECT_GT(std::strlen(err), 0u);
}

TEST(CApi_Yolk, Error_NullVoters) {
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  SCS_Yolk2d yolk{};
  char err[256] = {};
  int rc =
      scs_yolk_2d(nullptr, 3, &dc, SCS_MAJORITY_SIMPLE, 720, &yolk, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
}

// ==========================================================================
// C4 — Copeland scores and winner
// ==========================================================================

// 3 voters on x-axis; A=(1,0) is the Condorcet winner.
// Scores: A=+2, B=0, C=-2.
static const double kC4Alts[] = {1.0, 0.0,   // idx 0 = A (Condorcet winner)
                                 0.0, 0.0,   // idx 1 = B
                                 3.0, 0.0};  // idx 2 = C
static const double kC4Voters[] = {0.0, 0.0, 1.0, 0.0, 3.0, 0.0};

static SCS_DistanceConfig make_euc2d() {
  static double w[2] = {1.0, 1.0};
  return make_euclidean_dist(w, 2);
}

TEST(CApi_Copeland, Scores_CondorcetWinnerHasMaxScore) {
  SCS_DistanceConfig dc = make_euc2d();
  int scores[3] = {};
  char err[256] = {};
  int rc = scs_copeland_scores_2d(kC4Alts, 3, kC4Voters, 3, &dc,
                                  SCS_MAJORITY_SIMPLE, scores, 3, err, 256);
  ASSERT_EQ(rc, SCS_OK) << err;
  EXPECT_GT(scores[0], scores[1]);
  EXPECT_GT(scores[0], scores[2]);
}

TEST(CApi_Copeland, WinnerIndex_CondorcetWinner) {
  SCS_DistanceConfig dc = make_euc2d();
  int idx = -1;
  char err[256] = {};
  int rc = scs_copeland_winner_2d(kC4Alts, 3, kC4Voters, 3, &dc,
                                  SCS_MAJORITY_SIMPLE, &idx, err, 256);
  ASSERT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(idx, 0);
}

TEST(CApi_Copeland, Error_NullAlts) {
  SCS_DistanceConfig dc = make_euc2d();
  int scores[3] = {};
  char err[256] = {};
  int rc = scs_copeland_scores_2d(nullptr, 3, kC4Voters, 3, &dc,
                                  SCS_MAJORITY_SIMPLE, scores, 3, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
}

// ==========================================================================
// C4 — Condorcet winner and core
// ==========================================================================

TEST(CApi_Condorcet, HasWinner_CondorcetWinnerExists) {
  SCS_DistanceConfig dc = make_euc2d();
  int found = -1;
  char err[256] = {};
  int rc = scs_has_condorcet_winner_2d(kC4Alts, 3, kC4Voters, 3, &dc,
                                       SCS_MAJORITY_SIMPLE, &found, err, 256);
  ASSERT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(found, 1);
}

TEST(CApi_Condorcet, CondorcetWinner_CorrectIndex) {
  SCS_DistanceConfig dc = make_euc2d();
  int found = -1, idx = -1;
  char err[256] = {};
  int rc = scs_condorcet_winner_2d(kC4Alts, 3, kC4Voters, 3, &dc,
                                   SCS_MAJORITY_SIMPLE, &found, &idx, err, 256);
  ASSERT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(found, 1);
  EXPECT_EQ(idx, 0);
}

TEST(CApi_Condorcet, Error_NullVoters) {
  SCS_DistanceConfig dc = make_euc2d();
  int found = -1;
  char err[256] = {};
  int rc = scs_has_condorcet_winner_2d(kC4Alts, 3, nullptr, 3, &dc,
                                       SCS_MAJORITY_SIMPLE, &found, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
}

TEST(CApi_Core, CollinearVoters_CoreFound) {
  SCS_DistanceConfig dc = make_euc2d();
  int found = -1;
  double cx = 0.0, cy = 0.0;
  char err[256] = {};
  // Voters on x-axis; median voter at (1,0) is Condorcet winner.
  int rc = scs_core_2d(kC4Voters, 3, &dc, SCS_MAJORITY_SIMPLE, &found, &cx, &cy,
                       err, 256);
  ASSERT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(found, 1);
  EXPECT_NEAR(cx, 1.0, 0.1);
  EXPECT_NEAR(cy, 0.0, 0.1);
}

TEST(CApi_Core, Error_NullFound) {
  SCS_DistanceConfig dc = make_euc2d();
  double cx = 0.0, cy = 0.0;
  char err[256] = {};
  int rc = scs_core_2d(kC4Voters, 3, &dc, SCS_MAJORITY_SIMPLE, nullptr, &cx,
                       &cy, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
}

// ==========================================================================
// C4 — Uncovered set (discrete)
// ==========================================================================

TEST(CApi_UncoveredSet, CondorcetWinner_SizeQuery) {
  SCS_DistanceConfig dc = make_euc2d();
  int out_n = -1;
  char err[256] = {};
  int rc =
      scs_uncovered_set_2d(kC4Alts, 3, kC4Voters, 3, &dc, SCS_MAJORITY_SIMPLE,
                           nullptr, 0, &out_n, err, 256);
  ASSERT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(out_n, 1);
}

TEST(CApi_UncoveredSet, CondorcetWinner_Index) {
  SCS_DistanceConfig dc = make_euc2d();
  int indices[3] = {-1, -1, -1};
  int out_n = -1;
  char err[256] = {};
  int rc =
      scs_uncovered_set_2d(kC4Alts, 3, kC4Voters, 3, &dc, SCS_MAJORITY_SIMPLE,
                           indices, 3, &out_n, err, 256);
  ASSERT_EQ(rc, SCS_OK) << err;
  ASSERT_EQ(out_n, 1);
  EXPECT_EQ(indices[0], 0);
}

TEST(CApi_UncoveredSet, BufferTooSmall) {
  SCS_DistanceConfig dc = make_euc2d();
  int tiny[0] = {};
  int out_n = -1;
  char err[256] = {};
  int rc = scs_uncovered_set_2d(kC4Alts, 3, kC4Voters, 3, &dc,
                                SCS_MAJORITY_SIMPLE, tiny, 0, &out_n, err, 256);
  // size-query with capacity 0 is not an error
  EXPECT_EQ(rc, SCS_OK);
  EXPECT_EQ(out_n, 1);
}

// ==========================================================================
// C4 — Uncovered set boundary (continuous)
// ==========================================================================

TEST(CApi_UncoveredSetBoundary, SizeQuery_NonEmpty) {
  // 5-voter pentagon provides a non-trivial uncovered set region.
  const double pi = 3.14159265358979323846;
  double voters[10];
  for (int i = 0; i < 5; ++i) {
    voters[2 * i] = std::cos(2.0 * pi * i / 5.0);
    voters[2 * i + 1] = std::sin(2.0 * pi * i / 5.0);
  }
  SCS_DistanceConfig dc = make_euc2d();
  int pairs = -1;
  char err[256] = {};
  int rc = scs_uncovered_set_boundary_size_2d(
      voters, 5, &dc, 5, SCS_MAJORITY_SIMPLE, &pairs, err, 256);
  ASSERT_EQ(rc, SCS_OK) << err;
  EXPECT_GT(pairs, 0);
}

TEST(CApi_UncoveredSetBoundary, FillBuffer) {
  const double pi = 3.14159265358979323846;
  double voters[10];
  for (int i = 0; i < 5; ++i) {
    voters[2 * i] = std::cos(2.0 * pi * i / 5.0);
    voters[2 * i + 1] = std::sin(2.0 * pi * i / 5.0);
  }
  SCS_DistanceConfig dc = make_euc2d();
  int pairs = -1;
  char err[256] = {};
  scs_uncovered_set_boundary_size_2d(voters, 5, &dc, 5, SCS_MAJORITY_SIMPLE,
                                     &pairs, err, 256);
  std::vector<double> buf(static_cast<size_t>(pairs * 2));
  int out_n = -1;
  int rc = scs_uncovered_set_boundary_2d(voters, 5, &dc, 5, SCS_MAJORITY_SIMPLE,
                                         buf.data(), pairs, &out_n, err, 256);
  ASSERT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(out_n, pairs);
}

// ==========================================================================
// C4 — Heart (discrete)
// ==========================================================================

TEST(CApi_Heart, CondorcetWinner_SizeQuery) {
  SCS_DistanceConfig dc = make_euc2d();
  int out_n = -1;
  char err[256] = {};
  int rc = scs_heart_2d(kC4Alts, 3, kC4Voters, 3, &dc, SCS_MAJORITY_SIMPLE,
                        nullptr, 0, &out_n, err, 256);
  ASSERT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(out_n, 1);
}

TEST(CApi_Heart, CondorcetWinner_Index) {
  SCS_DistanceConfig dc = make_euc2d();
  int indices[3] = {-1, -1, -1};
  int out_n = -1;
  char err[256] = {};
  int rc = scs_heart_2d(kC4Alts, 3, kC4Voters, 3, &dc, SCS_MAJORITY_SIMPLE,
                        indices, 3, &out_n, err, 256);
  ASSERT_EQ(rc, SCS_OK) << err;
  ASSERT_EQ(out_n, 1);
  EXPECT_EQ(indices[0], 0);
}

TEST(CApi_Heart, Error_NullAlts) {
  SCS_DistanceConfig dc = make_euc2d();
  int out_n = -1;
  char err[256] = {};
  int rc = scs_heart_2d(nullptr, 3, kC4Voters, 3, &dc, SCS_MAJORITY_SIMPLE,
                        nullptr, 0, &out_n, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
}

// ==========================================================================
// C4 — Heart boundary (continuous)
// ==========================================================================

TEST(CApi_HeartBoundary, SizeQuery_NonEmpty) {
  const double pi = 3.14159265358979323846;
  double voters[10];
  for (int i = 0; i < 5; ++i) {
    voters[2 * i] = std::cos(2.0 * pi * i / 5.0);
    voters[2 * i + 1] = std::sin(2.0 * pi * i / 5.0);
  }
  SCS_DistanceConfig dc = make_euc2d();
  int pairs = -1;
  char err[256] = {};
  int rc = scs_heart_boundary_size_2d(voters, 5, &dc, 5, SCS_MAJORITY_SIMPLE,
                                      &pairs, err, 256);
  ASSERT_EQ(rc, SCS_OK) << err;
  EXPECT_GT(pairs, 0);
}

TEST(CApi_HeartBoundary, FillBuffer) {
  const double pi = 3.14159265358979323846;
  double voters[10];
  for (int i = 0; i < 5; ++i) {
    voters[2 * i] = std::cos(2.0 * pi * i / 5.0);
    voters[2 * i + 1] = std::sin(2.0 * pi * i / 5.0);
  }
  SCS_DistanceConfig dc = make_euc2d();
  int pairs = -1;
  char err[256] = {};
  scs_heart_boundary_size_2d(voters, 5, &dc, 5, SCS_MAJORITY_SIMPLE, &pairs,
                             err, 256);
  std::vector<double> buf(static_cast<size_t>(pairs * 2));
  int out_n = -1;
  int rc = scs_heart_boundary_2d(voters, 5, &dc, 5, SCS_MAJORITY_SIMPLE,
                                 buf.data(), pairs, &out_n, err, 256);
  ASSERT_EQ(rc, SCS_OK) << err;
  EXPECT_EQ(out_n, pairs);
}

// ==========================================================================
// C5 — Profile: build from spatial model
// ==========================================================================

TEST(CApi_Profile, BuildSpatial_CorrectRankings) {
  // Voter 0=(0,0) is closer to Alt 0=(0,1); Voter 1=(2,0) to Alt 1=(2,1).
  double alts[] = {0.0, 1.0, 2.0, 1.0};
  double voters[] = {0.0, 0.0, 2.0, 0.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  char err[256] = {};
  SCS_Profile* p = scs_profile_build_spatial(alts, 2, voters, 2, &dc, err, 256);
  ASSERT_NE(p, nullptr) << err;

  int n_voters = 0, n_alts = 0;
  ASSERT_EQ(scs_profile_dims(p, &n_voters, &n_alts, err, 256), SCS_OK) << err;
  EXPECT_EQ(n_voters, 2);
  EXPECT_EQ(n_alts, 2);

  int ranking0[2] = {}, ranking1[2] = {};
  ASSERT_EQ(scs_profile_get_ranking(p, 0, ranking0, 2, err, 256), SCS_OK)
      << err;
  ASSERT_EQ(scs_profile_get_ranking(p, 1, ranking1, 2, err, 256), SCS_OK)
      << err;
  EXPECT_EQ(ranking0[0], 0);  // voter 0 top-ranked alt is 0
  EXPECT_EQ(ranking1[0], 1);  // voter 1 top-ranked alt is 1

  scs_profile_destroy(p);
}

TEST(CApi_Profile, BuildSpatial_Error_NullAlts) {
  double voters[] = {0.0, 0.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  char err[256] = {};
  SCS_Profile* p =
      scs_profile_build_spatial(nullptr, 1, voters, 1, &dc, err, 256);
  EXPECT_EQ(p, nullptr);
  EXPECT_GT(std::strlen(err), 0u);
}

// ==========================================================================
// C5 — Profile: from utility matrix
// ==========================================================================

TEST(CApi_Profile, FromUtilityMatrix_CorrectRankings) {
  // utilities[voter * n_alts + alt]: voter 0 prefers alt 0, voter 1 prefers
  // alt 1.
  double utils[] = {2.0, 1.0, 1.0, 2.0};
  char err[256] = {};
  SCS_Profile* p = scs_profile_from_utility_matrix(utils, 2, 2, err, 256);
  ASSERT_NE(p, nullptr) << err;

  int ranking0[2] = {}, ranking1[2] = {};
  ASSERT_EQ(scs_profile_get_ranking(p, 0, ranking0, 2, err, 256), SCS_OK);
  ASSERT_EQ(scs_profile_get_ranking(p, 1, ranking1, 2, err, 256), SCS_OK);
  EXPECT_EQ(ranking0[0], 0);
  EXPECT_EQ(ranking1[0], 1);

  scs_profile_destroy(p);
}

TEST(CApi_Profile, FromUtilityMatrix_Error_NullPointer) {
  char err[256] = {};
  SCS_Profile* p = scs_profile_from_utility_matrix(nullptr, 2, 2, err, 256);
  EXPECT_EQ(p, nullptr);
  EXPECT_GT(std::strlen(err), 0u);
}

// ==========================================================================
// C5 — Profile: impartial culture generator
// ==========================================================================

TEST(CApi_Profile, ImpartialCulture_CorrectDims) {
  SCS_StreamManager* mgr = scs_stream_manager_create(42u, nullptr, 0);
  ASSERT_NE(mgr, nullptr);
  char err[256] = {};
  SCS_Profile* p = scs_profile_impartial_culture(5, 4, mgr, "test", err, 256);
  ASSERT_NE(p, nullptr) << err;

  int n_voters = 0, n_alts = 0;
  ASSERT_EQ(scs_profile_dims(p, &n_voters, &n_alts, err, 256), SCS_OK);
  EXPECT_EQ(n_voters, 5);
  EXPECT_EQ(n_alts, 4);

  scs_profile_destroy(p);
  scs_stream_manager_destroy(mgr);
}

TEST(CApi_Profile, ImpartialCulture_Error_NullManager) {
  char err[256] = {};
  SCS_Profile* p = scs_profile_impartial_culture(3, 3, nullptr, "x", err, 256);
  EXPECT_EQ(p, nullptr);
  EXPECT_GT(std::strlen(err), 0u);
}

// ==========================================================================
// C5 — Profile: uniform and gaussian spatial generators
// ==========================================================================

TEST(CApi_Profile, UniformSpatial_CorrectDims) {
  SCS_StreamManager* mgr = scs_stream_manager_create(7u, nullptr, 0);
  ASSERT_NE(mgr, nullptr);
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  char err[256] = {};
  SCS_Profile* p = scs_profile_uniform_spatial(10, 3, 2, -1.0, 1.0, &dc, mgr,
                                               "gen", err, 256);
  ASSERT_NE(p, nullptr) << err;

  int nv = 0, na = 0;
  ASSERT_EQ(scs_profile_dims(p, &nv, &na, err, 256), SCS_OK);
  EXPECT_EQ(nv, 10);
  EXPECT_EQ(na, 3);

  scs_profile_destroy(p);
  scs_stream_manager_destroy(mgr);
}

TEST(CApi_Profile, UniformSpatial_Error_BadDims) {
  SCS_StreamManager* mgr = scs_stream_manager_create(1u, nullptr, 0);
  ASSERT_NE(mgr, nullptr);
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  char err[256] = {};
  SCS_Profile* p =
      scs_profile_uniform_spatial(5, 3, 3, 0.0, 1.0, &dc, mgr, "g", err, 256);
  EXPECT_EQ(p, nullptr);
  EXPECT_GT(std::strlen(err), 0u);
  scs_stream_manager_destroy(mgr);
}

TEST(CApi_Profile, GaussianSpatial_CorrectDims) {
  SCS_StreamManager* mgr = scs_stream_manager_create(99u, nullptr, 0);
  ASSERT_NE(mgr, nullptr);
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  char err[256] = {};
  SCS_Profile* p = scs_profile_gaussian_spatial(8, 4, 2, 0.0, 1.0, &dc, mgr,
                                                "gauss", err, 256);
  ASSERT_NE(p, nullptr) << err;

  int nv = 0, na = 0;
  ASSERT_EQ(scs_profile_dims(p, &nv, &na, err, 256), SCS_OK);
  EXPECT_EQ(nv, 8);
  EXPECT_EQ(na, 4);

  scs_profile_destroy(p);
  scs_stream_manager_destroy(mgr);
}

// ==========================================================================
// C5.6 — Voter sampling: scs_draw_voters
// ==========================================================================

TEST(CApi_VoterSampler, Uniform_CorrectLength) {
  SCS_StreamManager* mgr = scs_stream_manager_create(42u, nullptr, 0);
  ASSERT_NE(mgr, nullptr);
  SCS_VoterSamplerConfig cfg{SCS_VOTER_SAMPLER_UNIFORM, -1.0, 1.0};
  double out[20] = {};
  char err[256] = {};
  ASSERT_EQ(scs_draw_voters(10, 2, &cfg, mgr, "v", out, 20, err, 256), SCS_OK)
      << err;
  scs_stream_manager_destroy(mgr);
}

TEST(CApi_VoterSampler, Uniform_AllInRange) {
  SCS_StreamManager* mgr = scs_stream_manager_create(7u, nullptr, 0);
  ASSERT_NE(mgr, nullptr);
  SCS_VoterSamplerConfig cfg{SCS_VOTER_SAMPLER_UNIFORM, -1.0, 1.0};
  double out[400] = {};
  char err[256] = {};
  ASSERT_EQ(scs_draw_voters(200, 2, &cfg, mgr, "v", out, 400, err, 256), SCS_OK)
      << err;
  for (int i = 0; i < 400; ++i) {
    EXPECT_GE(out[i], -1.0);
    EXPECT_LE(out[i], 1.0);
  }
  scs_stream_manager_destroy(mgr);
}

TEST(CApi_VoterSampler, Gaussian_AllFinite) {
  SCS_StreamManager* mgr = scs_stream_manager_create(3u, nullptr, 0);
  ASSERT_NE(mgr, nullptr);
  SCS_VoterSamplerConfig cfg{SCS_VOTER_SAMPLER_GAUSSIAN, 0.0, 0.5};
  double out[200] = {};
  char err[256] = {};
  ASSERT_EQ(scs_draw_voters(100, 2, &cfg, mgr, "v", out, 200, err, 256), SCS_OK)
      << err;
  for (int i = 0; i < 200; ++i) EXPECT_TRUE(std::isfinite(out[i]));
  scs_stream_manager_destroy(mgr);
}

TEST(CApi_VoterSampler, Error_NullConfig) {
  SCS_StreamManager* mgr = scs_stream_manager_create(1u, nullptr, 0);
  ASSERT_NE(mgr, nullptr);
  double out[4] = {};
  char err[256] = {};
  int rc = scs_draw_voters(2, 2, nullptr, mgr, "v", out, 4, err, 256);
  EXPECT_NE(rc, SCS_OK);
  EXPECT_GT(std::strlen(err), 0u);
  scs_stream_manager_destroy(mgr);
}

TEST(CApi_VoterSampler, Error_NullManager) {
  SCS_VoterSamplerConfig cfg{SCS_VOTER_SAMPLER_UNIFORM, -1.0, 1.0};
  double out[4] = {};
  char err[256] = {};
  int rc = scs_draw_voters(2, 2, &cfg, nullptr, "v", out, 4, err, 256);
  EXPECT_NE(rc, SCS_OK);
  EXPECT_GT(std::strlen(err), 0u);
}

TEST(CApi_VoterSampler, Error_UnsupportedNDims) {
  SCS_StreamManager* mgr = scs_stream_manager_create(1u, nullptr, 0);
  ASSERT_NE(mgr, nullptr);
  SCS_VoterSamplerConfig cfg{SCS_VOTER_SAMPLER_UNIFORM, -1.0, 1.0};
  double out[6] = {};
  char err[256] = {};
  int rc = scs_draw_voters(2, 3, &cfg, mgr, "v", out, 6, err, 256);
  EXPECT_NE(rc, SCS_OK);
  EXPECT_GT(std::strlen(err), 0u);
  scs_stream_manager_destroy(mgr);
}

// ==========================================================================
// C5 — Profile: bulk export and clone
// ==========================================================================

TEST(CApi_Profile, ExportRankings_MatchesGetRanking) {
  double alts[] = {0.0, 1.0, 2.0, 1.0, 4.0, 1.0};
  double voters[] = {0.0, 0.0, 2.0, 0.0, 4.0, 0.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  char err[256] = {};
  SCS_Profile* p = scs_profile_build_spatial(alts, 3, voters, 3, &dc, err, 256);
  ASSERT_NE(p, nullptr) << err;

  int bulk[9] = {};
  ASSERT_EQ(scs_profile_export_rankings(p, bulk, 9, err, 256), SCS_OK) << err;

  for (int v = 0; v < 3; ++v) {
    int row[3] = {};
    ASSERT_EQ(scs_profile_get_ranking(p, v, row, 3, err, 256), SCS_OK);
    for (int r = 0; r < 3; ++r) {
      EXPECT_EQ(bulk[v * 3 + r], row[r]) << "voter " << v << " rank " << r;
    }
  }
  scs_profile_destroy(p);
}

TEST(CApi_Profile, DestroyNull_IsNoOp) {
  scs_profile_destroy(nullptr);  // must not crash
}

TEST(CApi_Profile, Clone_SameDimsAndRankings) {
  double alts[] = {0.0, 1.0, 2.0, 1.0};
  double voters[] = {0.0, 0.0, 2.0, 0.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  char err[256] = {};
  SCS_Profile* orig =
      scs_profile_build_spatial(alts, 2, voters, 2, &dc, err, 256);
  ASSERT_NE(orig, nullptr) << err;

  SCS_Profile* copy = scs_profile_clone(orig, err, 256);
  ASSERT_NE(copy, nullptr) << err;

  int nv = 0, na = 0;
  ASSERT_EQ(scs_profile_dims(copy, &nv, &na, err, 256), SCS_OK);
  EXPECT_EQ(nv, 2);
  EXPECT_EQ(na, 2);

  int r_orig[2] = {}, r_copy[2] = {};
  for (int v = 0; v < 2; ++v) {
    ASSERT_EQ(scs_profile_get_ranking(orig, v, r_orig, 2, err, 256), SCS_OK);
    ASSERT_EQ(scs_profile_get_ranking(copy, v, r_copy, 2, err, 256), SCS_OK);
    EXPECT_EQ(r_orig[0], r_copy[0]);
    EXPECT_EQ(r_orig[1], r_copy[1]);
  }

  scs_profile_destroy(orig);
  scs_profile_destroy(copy);
}

// ===========================================================================
// Phase C6 — Voting rules
//
// Test fixture profile: 3 voters, 3 alternatives (A=0, B=1, C=2).
//   V0 ranking: [A, B, C]   (prefers A)
//   V1 ranking: [B, A, C]   (prefers B)
//   V2 ranking: [A, B, C]   (prefers A)
//
// Derived facts:
//   Plurality scores:     A=2, B=1, C=0   → winner A
//   Borda scores:         A=5, B=4, C=0   → winner A
//   Antiplurality scores: A=0, B=0, C=3   → C placed last 3×;
//                         both A and B are tied winners
//   Borda ranking: [A, B, C]   (all ties broken by kSmallestIndex below)
//   Scoring rule [2,1,0] == Borda (double output)
//   Top-2 approval:       A=3, B=3, C=0   → tie between A and B
// ===========================================================================

namespace {

// Build the shared 3-voter / 3-alt profile used by C6 tests.
// Caller must destroy the returned handle.
SCS_Profile* make_three_voter_profile(char* err, int err_len) {
  // Utility matrix (row-major, 3 voters × 3 alts):
  //   V0: [3, 2, 1]   V1: [2, 3, 1]   V2: [3, 2, 1]
  double u[] = {3, 2, 1,   // V0
                2, 3, 1,   // V1
                3, 2, 1};  // V2
  return scs_profile_from_utility_matrix(u, 3, 3, err, err_len);
}

}  // namespace

// ---------------------------------------------------------------------------
// C6.1 — Plurality
// ---------------------------------------------------------------------------

TEST(CApi_Plurality, Scores_Correct) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int scores[3] = {};
  ASSERT_EQ(scs_plurality_scores(p, scores, 3, err, 256), SCS_OK) << err;
  EXPECT_EQ(scores[0], 2);  // A
  EXPECT_EQ(scores[1], 1);  // B
  EXPECT_EQ(scores[2], 0);  // C

  scs_profile_destroy(p);
}

TEST(CApi_Plurality, AllWinners_SizeQuery) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int n = 0;
  ASSERT_EQ(scs_plurality_all_winners(p, nullptr, 0, &n, err, 256), SCS_OK)
      << err;
  EXPECT_EQ(n, 1);

  scs_profile_destroy(p);
}

TEST(CApi_Plurality, AllWinners_Correct) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int winners[3] = {};
  int n = 0;
  ASSERT_EQ(scs_plurality_all_winners(p, winners, 3, &n, err, 256), SCS_OK)
      << err;
  ASSERT_EQ(n, 1);
  EXPECT_EQ(winners[0], 0);  // A

  scs_profile_destroy(p);
}

TEST(CApi_Plurality, OneWinner_SmallestIndex) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int winner = -1;
  ASSERT_EQ(scs_plurality_one_winner(p, SCS_TIEBREAK_SMALLEST_INDEX, nullptr,
                                     nullptr, &winner, err, 256),
            SCS_OK)
      << err;
  EXPECT_EQ(winner, 0);  // A

  scs_profile_destroy(p);
}

TEST(CApi_Plurality, NullProfile_ReturnsError) {
  char err[256] = {};
  int scores[3] = {};
  EXPECT_NE(scs_plurality_scores(nullptr, scores, 3, err, 256), SCS_OK);
}

TEST(CApi_Plurality, RandomTieBreak_NullMgr_ReturnsError) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int winner = -1;
  EXPECT_NE(scs_plurality_one_winner(p, SCS_TIEBREAK_RANDOM, nullptr, nullptr,
                                     &winner, err, 256),
            SCS_OK);
  scs_profile_destroy(p);
}

// ---------------------------------------------------------------------------
// C6.2 — Borda Count
// ---------------------------------------------------------------------------

TEST(CApi_Borda, Scores_Correct) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int scores[3] = {};
  ASSERT_EQ(scs_borda_scores(p, scores, 3, err, 256), SCS_OK) << err;
  EXPECT_EQ(scores[0], 5);  // A
  EXPECT_EQ(scores[1], 4);  // B
  EXPECT_EQ(scores[2], 0);  // C

  scs_profile_destroy(p);
}

TEST(CApi_Borda, AllWinners_SizeQuery) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int n = 0;
  ASSERT_EQ(scs_borda_all_winners(p, nullptr, 0, &n, err, 256), SCS_OK) << err;
  EXPECT_EQ(n, 1);

  scs_profile_destroy(p);
}

TEST(CApi_Borda, OneWinner_SmallestIndex) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int winner = -1;
  ASSERT_EQ(scs_borda_one_winner(p, SCS_TIEBREAK_SMALLEST_INDEX, nullptr,
                                 nullptr, &winner, err, 256),
            SCS_OK)
      << err;
  EXPECT_EQ(winner, 0);  // A

  scs_profile_destroy(p);
}

TEST(CApi_Borda, Ranking_Correct) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int ranking[3] = {};
  ASSERT_EQ(scs_borda_ranking(p, SCS_TIEBREAK_SMALLEST_INDEX, nullptr, nullptr,
                              ranking, 3, err, 256),
            SCS_OK)
      << err;
  // Borda: A(5) > B(4) > C(0)
  EXPECT_EQ(ranking[0], 0);  // A is best
  EXPECT_EQ(ranking[1], 1);  // B is second
  EXPECT_EQ(ranking[2], 2);  // C is worst

  scs_profile_destroy(p);
}

TEST(CApi_Borda, NullProfile_ReturnsError) {
  char err[256] = {};
  int scores[3] = {};
  EXPECT_NE(scs_borda_scores(nullptr, scores, 3, err, 256), SCS_OK);
}

// ---------------------------------------------------------------------------
// C6.3 — Anti-plurality
// ---------------------------------------------------------------------------

TEST(CApi_Antiplurality, Scores_Correct) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  // Antiplurality score = number of voters who do NOT rank the alternative
  // last (higher is better). C is ranked last by all 3 voters so gets 0;
  // A and B are never ranked last, so both get 3.
  int scores[3] = {};
  ASSERT_EQ(scs_antiplurality_scores(p, scores, 3, err, 256), SCS_OK) << err;
  EXPECT_EQ(scores[0], 3);  // A: not last for any voter
  EXPECT_EQ(scores[1], 3);  // B: not last for any voter
  EXPECT_EQ(scores[2], 0);  // C: last for all 3 voters

  scs_profile_destroy(p);
}

TEST(CApi_Antiplurality, AllWinners_TiedAAndB) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int n = 0;
  ASSERT_EQ(scs_antiplurality_all_winners(p, nullptr, 0, &n, err, 256), SCS_OK)
      << err;
  EXPECT_EQ(n, 2);  // A and B are tied (both have 0 last-place votes)

  int winners[3] = {};
  ASSERT_EQ(scs_antiplurality_all_winners(p, winners, 3, &n, err, 256), SCS_OK)
      << err;
  EXPECT_EQ(winners[0], 0);  // A
  EXPECT_EQ(winners[1], 1);  // B

  scs_profile_destroy(p);
}

TEST(CApi_Antiplurality, OneWinner_SmallestIndex) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int winner = -1;
  ASSERT_EQ(scs_antiplurality_one_winner(p, SCS_TIEBREAK_SMALLEST_INDEX,
                                         nullptr, nullptr, &winner, err, 256),
            SCS_OK)
      << err;
  EXPECT_EQ(winner, 0);  // A has lowest index among tied winners

  scs_profile_destroy(p);
}

TEST(CApi_Antiplurality, NullProfile_ReturnsError) {
  char err[256] = {};
  int scores[3] = {};
  EXPECT_NE(scs_antiplurality_scores(nullptr, scores, 3, err, 256), SCS_OK);
}

// ---------------------------------------------------------------------------
// C6.4 — Generic positional scoring rule
// ---------------------------------------------------------------------------

TEST(CApi_ScoringRule, Scores_BordaEquivalent) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  // Weights [2, 1, 0] reproduce Borda for 3 alternatives.
  double weights[] = {2.0, 1.0, 0.0};
  double scores[3] = {};
  ASSERT_EQ(scs_scoring_rule_scores(p, weights, 3, scores, 3, err, 256), SCS_OK)
      << err;
  EXPECT_DOUBLE_EQ(scores[0], 5.0);  // A
  EXPECT_DOUBLE_EQ(scores[1], 4.0);  // B
  EXPECT_DOUBLE_EQ(scores[2], 0.0);  // C

  scs_profile_destroy(p);
}

TEST(CApi_ScoringRule, AllWinners_SizeQuery) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  double weights[] = {2.0, 1.0, 0.0};
  int n = 0;
  ASSERT_EQ(
      scs_scoring_rule_all_winners(p, weights, 3, nullptr, 0, &n, err, 256),
      SCS_OK)
      << err;
  EXPECT_EQ(n, 1);  // only A

  scs_profile_destroy(p);
}

TEST(CApi_ScoringRule, OneWinner_SmallestIndex) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  double weights[] = {2.0, 1.0, 0.0};
  int winner = -1;
  ASSERT_EQ(
      scs_scoring_rule_one_winner(p, weights, 3, SCS_TIEBREAK_SMALLEST_INDEX,
                                  nullptr, nullptr, &winner, err, 256),
      SCS_OK)
      << err;
  EXPECT_EQ(winner, 0);  // A

  scs_profile_destroy(p);
}

TEST(CApi_ScoringRule, NonIncreasingWeights_ReturnsError) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  double bad_weights[] = {1.0, 2.0, 0.0};  // not non-increasing
  double scores[3] = {};
  EXPECT_NE(scs_scoring_rule_scores(p, bad_weights, 3, scores, 3, err, 256),
            SCS_OK);
  scs_profile_destroy(p);
}

TEST(CApi_ScoringRule, NullWeights_ReturnsError) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;
  double scores[3] = {};
  EXPECT_NE(scs_scoring_rule_scores(p, nullptr, 3, scores, 3, err, 256),
            SCS_OK);
  scs_profile_destroy(p);
}

// ---------------------------------------------------------------------------
// C6.5 — Approval voting
// ---------------------------------------------------------------------------

TEST(CApi_ApprovalSpatial, Scores_Correct) {
  // Two alternatives: A=(0,0), B=(10,0).
  // Three voters with ideals at: (0,0), (0.5,0), (5,0).
  // Threshold = 1.0 (Euclidean).
  // V0 approves A (dist=0), not B (dist=10).
  // V1 approves A (dist=0.5), not B (dist=9.5).
  // V2 approves neither (dist(A)=5 > 1, dist(B)=5 > 1).
  // Expected scores: A=2, B=0.
  char err[256] = {};
  double alts[] = {0.0, 0.0, 10.0, 0.0};
  double voters[] = {0.0, 0.0, 0.5, 0.0, 5.0, 0.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);

  int scores[2] = {};
  ASSERT_EQ(scs_approval_scores_spatial(alts, 2, voters, 3, 1.0, &dc, scores, 2,
                                        err, 256),
            SCS_OK)
      << err;
  EXPECT_EQ(scores[0], 2);  // A
  EXPECT_EQ(scores[1], 0);  // B
}

TEST(CApi_ApprovalSpatial, AllWinners_SizeQuery) {
  char err[256] = {};
  double alts[] = {0.0, 0.0, 10.0, 0.0};
  double voters[] = {0.0, 0.0, 0.5, 0.0, 5.0, 0.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);

  int n = 0;
  ASSERT_EQ(scs_approval_all_winners_spatial(alts, 2, voters, 3, 1.0, &dc,
                                             nullptr, 0, &n, err, 256),
            SCS_OK)
      << err;
  EXPECT_EQ(n, 1);  // only A
}

TEST(CApi_ApprovalSpatial, NegativeThreshold_ReturnsError) {
  char err[256] = {};
  double alts[] = {0.0, 0.0, 1.0, 0.0};
  double voters[] = {0.0, 0.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  int scores[2] = {};
  EXPECT_NE(scs_approval_scores_spatial(alts, 2, voters, 1, -1.0, &dc, scores,
                                        2, err, 256),
            SCS_OK);
}

TEST(CApi_ApprovalTopK, Scores_Top1_EqualToPlurality) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  // Top-1 approval == plurality: each voter approves only their first choice.
  int ap_scores[3] = {};
  ASSERT_EQ(scs_approval_scores_topk(p, 1, ap_scores, 3, err, 256), SCS_OK)
      << err;

  int pl_scores[3] = {};
  ASSERT_EQ(scs_plurality_scores(p, pl_scores, 3, err, 256), SCS_OK) << err;

  EXPECT_EQ(ap_scores[0], pl_scores[0]);
  EXPECT_EQ(ap_scores[1], pl_scores[1]);
  EXPECT_EQ(ap_scores[2], pl_scores[2]);

  scs_profile_destroy(p);
}

TEST(CApi_ApprovalTopK, Scores_Top2_TieAB) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  // Top-2: every voter approves their top 2 alternatives.
  // A is in top-2 for all 3 voters → score=3.
  // B is in top-2 for all 3 voters → score=3.
  // C is never in top-2 → score=0.
  int scores[3] = {};
  ASSERT_EQ(scs_approval_scores_topk(p, 2, scores, 3, err, 256), SCS_OK) << err;
  EXPECT_EQ(scores[0], 3);  // A
  EXPECT_EQ(scores[1], 3);  // B
  EXPECT_EQ(scores[2], 0);  // C

  scs_profile_destroy(p);
}

TEST(CApi_ApprovalTopK, AllWinners_Top2_SizeQuery) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int n = 0;
  ASSERT_EQ(scs_approval_all_winners_topk(p, 2, nullptr, 0, &n, err, 256),
            SCS_OK)
      << err;
  EXPECT_EQ(n, 2);  // A and B tied

  scs_profile_destroy(p);
}

TEST(CApi_ApprovalTopK, InvalidK_ReturnsError) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int scores[3] = {};
  EXPECT_NE(scs_approval_scores_topk(p, 0, scores, 3, err, 256), SCS_OK);
  EXPECT_NE(scs_approval_scores_topk(p, 4, scores, 3, err, 256), SCS_OK);

  scs_profile_destroy(p);
}

// ===========================================================================
// Phase C7 — Social rankings and properties
//
// All tests reuse the 3-voter / 3-alt profile from C6:
//   V0: [A, B, C]    V1: [B, A, C]    V2: [A, B, C]
//
// Derived facts:
//   Pairwise majorities: A>B (2-1), A>C (3-0), B>C (3-0)
//   Condorcet winner:    A (index 0)
//   Pareto set:          {A, B}  (C is dominated by both A and B)
//   Pareto-efficient:    A=yes, B=yes, C=no
//   Condorcet-consistent: A=yes, B=no (CW exists and B≠CW), C=no
//   Majority-consistent:  A=yes only (A is the CW)
//   Borda scores [5,4,0] ranked → [A, B, C] = [0, 1, 2]
// ===========================================================================

// ---------------------------------------------------------------------------
// C7.1 — Rank by scores
// ---------------------------------------------------------------------------

TEST(CApi_RankByScores, CorrectOrder) {
  char err[256] = {};
  double scores[] = {5.0, 4.0, 0.0};  // Borda scores: A=5, B=4, C=0
  int ranking[3] = {};
  ASSERT_EQ(scs_rank_by_scores(scores, 3, SCS_TIEBREAK_SMALLEST_INDEX, nullptr,
                               nullptr, ranking, 3, err, 256),
            SCS_OK)
      << err;
  EXPECT_EQ(ranking[0], 0);  // A is best
  EXPECT_EQ(ranking[1], 1);  // B is second
  EXPECT_EQ(ranking[2], 2);  // C is worst
}

TEST(CApi_RankByScores, TiedScoresSmallestIndex) {
  char err[256] = {};
  double scores[] = {3.0, 3.0, 1.0};  // A and B tied
  int ranking[3] = {};
  ASSERT_EQ(scs_rank_by_scores(scores, 3, SCS_TIEBREAK_SMALLEST_INDEX, nullptr,
                               nullptr, ranking, 3, err, 256),
            SCS_OK)
      << err;
  EXPECT_EQ(ranking[0], 0);  // A before B (smaller index)
  EXPECT_EQ(ranking[1], 1);
  EXPECT_EQ(ranking[2], 2);
}

TEST(CApi_RankByScores, NullScores_ReturnsError) {
  char err[256] = {};
  int ranking[3] = {};
  EXPECT_NE(scs_rank_by_scores(nullptr, 3, SCS_TIEBREAK_SMALLEST_INDEX, nullptr,
                               nullptr, ranking, 3, err, 256),
            SCS_OK);
}

TEST(CApi_RankByScores, RandomTieBreak_NullMgr_ReturnsError) {
  char err[256] = {};
  double scores[] = {1.0, 1.0, 0.0};
  int ranking[3] = {};
  EXPECT_NE(scs_rank_by_scores(scores, 3, SCS_TIEBREAK_RANDOM, nullptr, nullptr,
                               ranking, 3, err, 256),
            SCS_OK);
}

// ---------------------------------------------------------------------------
// C7.2 — Pairwise ranking from matrix
// ---------------------------------------------------------------------------

TEST(CApi_PairwiseRanking, CorrectOrder) {
  // Pairwise matrix for our 3-alt example:
  //       A    B    C
  //  A  [ 0,  +1,  +1 ]   (A beats B and C)
  //  B  [-1,   0,  +1 ]   (B loses to A, beats C)
  //  C  [-1,  -1,   0 ]   (C loses to everyone)
  char err[256] = {};
  SCS_PairwiseResult mat[9] = {
    SCS_PAIRWISE_TIE,  SCS_PAIRWISE_WIN,  SCS_PAIRWISE_WIN,  // A
    SCS_PAIRWISE_LOSS, SCS_PAIRWISE_TIE,  SCS_PAIRWISE_WIN,  // B
    SCS_PAIRWISE_LOSS, SCS_PAIRWISE_LOSS, SCS_PAIRWISE_TIE   // C
  };
  int ranking[3] = {};
  ASSERT_EQ(
      scs_pairwise_ranking_from_matrix(mat, 3, SCS_TIEBREAK_SMALLEST_INDEX,
                                       nullptr, nullptr, ranking, 3, err, 256),
      SCS_OK)
      << err;
  // Copeland scores: A=2, B=1, C=0
  EXPECT_EQ(ranking[0], 0);  // A first
  EXPECT_EQ(ranking[1], 1);  // B second
  EXPECT_EQ(ranking[2], 2);  // C last
}

TEST(CApi_PairwiseRanking, NullMatrix_ReturnsError) {
  char err[256] = {};
  int ranking[3] = {};
  EXPECT_NE(
      scs_pairwise_ranking_from_matrix(nullptr, 3, SCS_TIEBREAK_SMALLEST_INDEX,
                                       nullptr, nullptr, ranking, 3, err, 256),
      SCS_OK);
}

// ---------------------------------------------------------------------------
// C7.3 — Pareto efficiency
// ---------------------------------------------------------------------------

TEST(CApi_Pareto, ParetoSet_SizeQuery) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int n = 0;
  ASSERT_EQ(scs_pareto_set(p, nullptr, 0, &n, err, 256), SCS_OK) << err;
  EXPECT_EQ(n, 2);  // A and B (C is dominated)

  scs_profile_destroy(p);
}

TEST(CApi_Pareto, ParetoSet_Correct) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int indices[3] = {};
  int n = 0;
  ASSERT_EQ(scs_pareto_set(p, indices, 3, &n, err, 256), SCS_OK) << err;
  ASSERT_EQ(n, 2);
  EXPECT_EQ(indices[0], 0);  // A
  EXPECT_EQ(indices[1], 1);  // B

  scs_profile_destroy(p);
}

TEST(CApi_Pareto, IsParetoEfficient_Correct) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int out = -1;
  ASSERT_EQ(scs_is_pareto_efficient(p, 0, &out, err, 256), SCS_OK) << err;
  EXPECT_EQ(out, 1);  // A is efficient

  ASSERT_EQ(scs_is_pareto_efficient(p, 1, &out, err, 256), SCS_OK) << err;
  EXPECT_EQ(out, 1);  // B is efficient

  ASSERT_EQ(scs_is_pareto_efficient(p, 2, &out, err, 256), SCS_OK) << err;
  EXPECT_EQ(out, 0);  // C is dominated

  scs_profile_destroy(p);
}

TEST(CApi_Pareto, OutOfRange_ReturnsError) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;
  int out = -1;
  EXPECT_NE(scs_is_pareto_efficient(p, 3, &out, err, 256), SCS_OK);
  scs_profile_destroy(p);
}

// ---------------------------------------------------------------------------
// C7.4 — Condorcet and majority-selection predicates
// ---------------------------------------------------------------------------

TEST(CApi_CondorcetProfile, HasWinner_True) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int found = 0;
  ASSERT_EQ(scs_has_condorcet_winner_profile(p, &found, err, 256), SCS_OK)
      << err;
  EXPECT_EQ(found, 1);  // A beats B (2-1) and C (3-0)

  scs_profile_destroy(p);
}

TEST(CApi_CondorcetProfile, Winner_Correct) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int found = 0, winner = -1;
  ASSERT_EQ(scs_condorcet_winner_profile(p, &found, &winner, err, 256), SCS_OK)
      << err;
  EXPECT_EQ(found, 1);
  EXPECT_EQ(winner, 0);  // A

  scs_profile_destroy(p);
}

TEST(CApi_CondorcetProfile, NoWinner_OutWinnerUntouched) {
  // Build a Condorcet cycle: V0:A>B>C, V1:B>C>A, V2:C>A>B → no CW.
  // A>B (V0,V2), B>C (V0,V1), C>A (V1,V2) — each 2-1, no Condorcet winner.
  char err[256] = {};
  double u_cycle[] = {3, 2, 1,   // V0: A>B>C  (utils 3,2,1)
                      1, 3, 2,   // V1: B>C>A  (utils 1,3,2)
                      2, 1, 3};  // V2: C>A>B  (utils 2,1,3)
  SCS_Profile* p = scs_profile_from_utility_matrix(u_cycle, 3, 3, err, 256);
  ASSERT_NE(p, nullptr) << err;

  int found = 1;  // set to 1 to ensure it is overwritten
  int winner = 99;
  ASSERT_EQ(scs_condorcet_winner_profile(p, &found, &winner, err, 256), SCS_OK)
      << err;
  EXPECT_EQ(found, 0);
  EXPECT_EQ(winner, 99);  // untouched when no winner found

  scs_profile_destroy(p);
}

TEST(CApi_CondorcetProfile, CondorcetConsistent_OnlyWinnerPasses) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int out = -1;
  ASSERT_EQ(scs_is_selected_by_condorcet_consistent_rules(p, 0, &out, err, 256),
            SCS_OK)
      << err;
  EXPECT_EQ(out, 1);  // A is the CW → consistent

  ASSERT_EQ(scs_is_selected_by_condorcet_consistent_rules(p, 1, &out, err, 256),
            SCS_OK)
      << err;
  EXPECT_EQ(out, 0);  // B is not the CW → inconsistent

  scs_profile_destroy(p);
}

TEST(CApi_CondorcetProfile, MajorityConsistent_OnlyCWPasses) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int out = -1;
  ASSERT_EQ(scs_is_selected_by_majority_consistent_rules(p, 0, &out, err, 256),
            SCS_OK)
      << err;
  EXPECT_EQ(out, 1);  // A beats all by majority

  ASSERT_EQ(scs_is_selected_by_majority_consistent_rules(p, 1, &out, err, 256),
            SCS_OK)
      << err;
  EXPECT_EQ(out, 0);  // B does not beat A by majority

  scs_profile_destroy(p);
}

TEST(CApi_CondorcetProfile, NullProfile_ReturnsError) {
  char err[256] = {};
  int found = 0;
  EXPECT_NE(scs_has_condorcet_winner_profile(nullptr, &found, err, 256),
            SCS_OK);
}

// ===========================================================================
// Phase C8 — Additional coverage sweep
// ===========================================================================

// ---------------------------------------------------------------------------
// C0 scalar domain sentinel values (SCS_TieBreak + SCS_MAJORITY_SIMPLE)
// ---------------------------------------------------------------------------

TEST(CApi_ScalarDomains, TieBreak_SentinelValues) {
  // These are ABI-stable constants; a binding layer may hard-code them.
  EXPECT_EQ(SCS_TIEBREAK_RANDOM, static_cast<SCS_TieBreak>(0));
  EXPECT_EQ(SCS_TIEBREAK_SMALLEST_INDEX, static_cast<SCS_TieBreak>(1));
  EXPECT_NE(SCS_TIEBREAK_RANDOM, SCS_TIEBREAK_SMALLEST_INDEX);
}

TEST(CApi_ScalarDomains, MajoritySimple_SentinelValue) {
  // SCS_MAJORITY_SIMPLE = -1 by convention; binding layers must not treat it
  // as a valid voter count.
  EXPECT_EQ(SCS_MAJORITY_SIMPLE, -1);
}

// ---------------------------------------------------------------------------
// Error path: null / short err_buf does not crash and preserves error code
// ---------------------------------------------------------------------------

TEST(CApi_ErrorPaths, NullErrBuf_StillReturnsCode) {
  // Pass null err_buf + 0 length.  The function must still return the correct
  // error code; it must not crash trying to write the message.
  int major = -1;
  int rc = scs_api_version(nullptr, nullptr, nullptr, nullptr, 0);
  (void)major;
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
}

TEST(CApi_ErrorPaths, ErrBufLen0_StillReturnsCode) {
  char dummy[4] = {'X', 'X', 'X', '\0'};
  int rc = scs_api_version(nullptr, nullptr, nullptr, dummy, 0);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
  // With len=0, set_error skips writing entirely; dummy should be unchanged.
  EXPECT_EQ(dummy[0], 'X');
}

TEST(CApi_ErrorPaths, ErrBufLen1_WritesEmptyString) {
  char tiny[1] = {'X'};
  int rc = scs_api_version(nullptr, nullptr, nullptr, tiny, 1);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
  // set_error writes '\0' at tiny[0] (copies 0 chars, then null-terminates).
  EXPECT_EQ(tiny[0], '\0');
}

// ---------------------------------------------------------------------------
// C2 — Winset: X ∪ X = X (self-union is non-empty and same as original)
// ---------------------------------------------------------------------------

TEST(CApi_Winset, UnionSelf_NonEmpty) {
  char err[256] = {};
  SCS_Winset* ws = nullptr;
  make_triangle_winset(&ws, err, 256);
  ASSERT_NE(ws, nullptr) << err;

  // Union ws with a clone so the two pointer-level inputs are distinct.
  SCS_Winset* ws2 = scs_winset_clone(ws, err, 256);
  ASSERT_NE(ws2, nullptr) << err;

  SCS_Winset* u = scs_winset_union(ws, ws2, err, 256);
  ASSERT_NE(u, nullptr) << err;

  int is_empty = -1;
  EXPECT_EQ(scs_winset_is_empty(u, &is_empty, err, 256), SCS_OK);
  EXPECT_EQ(is_empty, 0);  // X ∪ X cannot be empty if X is non-empty

  scs_winset_destroy(ws);
  scs_winset_destroy(ws2);
  scs_winset_destroy(u);
}

// ---------------------------------------------------------------------------
// C6 — all_winners respects out_capacity → SCS_ERROR_BUFFER_TOO_SMALL
// ---------------------------------------------------------------------------

TEST(CApi_Antiplurality, AllWinners_BufferTooSmall_ReturnsError) {
  // Antiplurality has 2 tied winners (A and B) in the 3-voter profile.
  // Passing capacity=1 when 2 winners exist must return BUFFER_TOO_SMALL.
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int winners[1] = {};
  int n = 0;
  int rc = scs_antiplurality_all_winners(p, winners, 1, &n, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_BUFFER_TOO_SMALL);
  EXPECT_EQ(n, 2);  // out_n still reports the required count

  scs_profile_destroy(p);
}

TEST(CApi_Plurality, AllWinners_ExactCapacity_Succeeds) {
  // Providing capacity == n_winners (= 1) must succeed, not error.
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  int winners[1] = {};
  int n = 0;
  EXPECT_EQ(scs_plurality_all_winners(p, winners, 1, &n, err, 256), SCS_OK)
      << err;
  EXPECT_EQ(n, 1);
  EXPECT_EQ(winners[0], 0);

  scs_profile_destroy(p);
}

// ---------------------------------------------------------------------------
// C6 — scoring rule with fractional weights
// ---------------------------------------------------------------------------

TEST(CApi_ScoringRule, FractionalWeights_Correct) {
  // Weights [1.5, 0.5, 0.0] for 3-voter profile:
  //   A: V0 gives 1.5, V1 gives 0.5, V2 gives 1.5  →  total 3.5
  //   B: V0 gives 0.5, V1 gives 1.5, V2 gives 0.5  →  total 2.5
  //   C: all get 0.0                                →  total 0.0
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;

  double weights[] = {1.5, 0.5, 0.0};
  double scores[3] = {};
  ASSERT_EQ(scs_scoring_rule_scores(p, weights, 3, scores, 3, err, 256), SCS_OK)
      << err;
  EXPECT_DOUBLE_EQ(scores[0], 3.5);  // A
  EXPECT_DOUBLE_EQ(scores[1], 2.5);  // B
  EXPECT_DOUBLE_EQ(scores[2], 0.0);  // C

  // Winner must still be A.
  int winner = -1;
  ASSERT_EQ(
      scs_scoring_rule_one_winner(p, weights, 3, SCS_TIEBREAK_SMALLEST_INDEX,
                                  nullptr, nullptr, &winner, err, 256),
      SCS_OK)
      << err;
  EXPECT_EQ(winner, 0);

  scs_profile_destroy(p);
}

// ---------------------------------------------------------------------------
// C8 — Non-finite input rejection (NaN/Inf)
// ---------------------------------------------------------------------------

TEST(CApi_NonFinite, Distance_NaNInIdeal_ReturnsInvalidArgument) {
  double ideal[2] = {std::nan(""), 0.0};
  double alt[2] = {1.0, 0.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  double d = 0.0;
  char err[256] = {};
  int rc = scs_calculate_distance(ideal, alt, 2, &dc, &d, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
  EXPECT_GT(std::strlen(err), 0u);
}

TEST(CApi_NonFinite, Distance_InfInAlt_ReturnsInvalidArgument) {
  double ideal[2] = {0.0, 0.0};
  double alt[2] = {1.0, std::numeric_limits<double>::infinity()};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  double d = 0.0;
  char err[256] = {};
  int rc = scs_calculate_distance(ideal, alt, 2, &dc, &d, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
  EXPECT_GT(std::strlen(err), 0u);
}

TEST(CApi_NonFinite, Winset2d_NaNInVoterIdeals_ReturnsNull) {
  double voters[] = {0.0, 0.0, std::nan(""), 0.0, 1.0, 2.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  char err[256] = {};
  SCS_Winset* ws =
      scs_winset_2d(1.0, 0.667, voters, 3, &dc, SCS_MAJORITY_SIMPLE,
                    SCS_DEFAULT_WINSET_SAMPLES, err, 256);
  EXPECT_EQ(ws, nullptr);
  EXPECT_GT(std::strlen(err), 0u);
}

TEST(CApi_NonFinite, ProfileFromUtilityMatrix_NaN_ReturnsNull) {
  double utilities[] = {1.0, 0.0, std::nan(""), 0.5, 1.0, 0.0};
  char err[256] = {};
  SCS_Profile* p = scs_profile_from_utility_matrix(utilities, 2, 3, err, 256);
  EXPECT_EQ(p, nullptr);
  EXPECT_GT(std::strlen(err), 0u);
}

TEST(CApi_NonFinite, ApprovalSpatial_InfThreshold_ReturnsInvalidArgument) {
  double alts[] = {0.0, 0.0, 1.0, 0.0};
  double voters[] = {0.0, 0.0, 1.0, 0.0, 0.5, 0.0};
  double w[2] = {1.0, 1.0};
  SCS_DistanceConfig dc = make_euclidean_dist(w, 2);
  int scores[2] = {};
  char err[256] = {};
  int rc = scs_approval_scores_spatial(alts, 2, voters, 3,
                                       std::numeric_limits<double>::infinity(),
                                       &dc, scores, 2, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
  EXPECT_GT(std::strlen(err), 0u);
}

TEST(CApi_NonFinite, ScoringRule_NaNWeights_ReturnsInvalidArgument) {
  char err[256] = {};
  SCS_Profile* p = make_three_voter_profile(err, 256);
  ASSERT_NE(p, nullptr) << err;
  double weights[] = {1.0, std::nan(""), 0.0};
  double scores[3] = {};
  int rc = scs_scoring_rule_scores(p, weights, 3, scores, 3, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
  EXPECT_GT(std::strlen(err), 0u);
  scs_profile_destroy(p);
}

TEST(CApi_NonFinite, RankByScores_InfScores_ReturnsInvalidArgument) {
  double scores[] = {1.0, std::numeric_limits<double>::infinity(), 0.0};
  int ranking[3] = {};
  char err[256] = {};
  int rc = scs_rank_by_scores(scores, 3, SCS_TIEBREAK_SMALLEST_INDEX, nullptr,
                              nullptr, ranking, 3, err, 256);
  EXPECT_EQ(rc, SCS_ERROR_INVALID_ARGUMENT);
  EXPECT_GT(std::strlen(err), 0u);
}
