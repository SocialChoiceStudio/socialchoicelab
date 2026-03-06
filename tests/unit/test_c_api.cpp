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
