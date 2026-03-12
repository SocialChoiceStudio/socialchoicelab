// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

// Tests for voter_sampler.h: make_voter_sampler_uniform,
// make_voter_sampler_gaussian, and draw_voters.

#include <gtest/gtest.h>
#include <socialchoicelab/aggregation/voter_sampler.h>

#include <algorithm>
#include <cmath>

using socialchoicelab::aggregation::draw_voters;
using socialchoicelab::aggregation::make_voter_sampler_gaussian;
using socialchoicelab::aggregation::make_voter_sampler_uniform;
using socialchoicelab::aggregation::VoterSamplerConfig;
using socialchoicelab::aggregation::VoterSamplerKind;
using socialchoicelab::core::rng::PRNG;

// ==========================================================================
// VS1 — make_voter_sampler_uniform
// ==========================================================================

TEST(MakeVoterSamplerUniform, ReturnsCorrectKindAndParams) {
  const auto cfg = make_voter_sampler_uniform(-1.0, 1.0);
  EXPECT_EQ(cfg.kind, VoterSamplerKind::Uniform);
  EXPECT_DOUBLE_EQ(cfg.param1, -1.0);
  EXPECT_DOUBLE_EQ(cfg.param2, 1.0);
}

TEST(MakeVoterSamplerUniform, ThrowsIfLoGeHi) {
  EXPECT_THROW(make_voter_sampler_uniform(1.0, 1.0), std::invalid_argument);
  EXPECT_THROW(make_voter_sampler_uniform(2.0, 1.0), std::invalid_argument);
}

TEST(MakeVoterSamplerUniform, ThrowsIfInfinite) {
  EXPECT_THROW(
      make_voter_sampler_uniform(-std::numeric_limits<double>::infinity(), 1.0),
      std::invalid_argument);
}

// ==========================================================================
// VS2 — make_voter_sampler_gaussian
// ==========================================================================

TEST(MakeVoterSamplerGaussian, ReturnsCorrectKindAndParams) {
  const auto cfg = make_voter_sampler_gaussian(0.5, 0.3);
  EXPECT_EQ(cfg.kind, VoterSamplerKind::Gaussian);
  EXPECT_DOUBLE_EQ(cfg.param1, 0.5);
  EXPECT_DOUBLE_EQ(cfg.param2, 0.3);
}

TEST(MakeVoterSamplerGaussian, ThrowsIfStddevNotPositive) {
  EXPECT_THROW(make_voter_sampler_gaussian(0.0, 0.0), std::invalid_argument);
  EXPECT_THROW(make_voter_sampler_gaussian(0.0, -1.0), std::invalid_argument);
}

TEST(MakeVoterSamplerGaussian, ThrowsIfInfinite) {
  EXPECT_THROW(
      make_voter_sampler_gaussian(std::numeric_limits<double>::infinity(), 1.0),
      std::invalid_argument);
}

// ==========================================================================
// VS3 — draw_voters: uniform
// ==========================================================================

TEST(DrawVoters, Uniform_CorrectLength) {
  PRNG prng(42);
  const auto cfg = make_voter_sampler_uniform(-1.0, 1.0);
  const auto coords = draw_voters(10, 2, cfg, prng);
  EXPECT_EQ(static_cast<int>(coords.size()), 20);
}

TEST(DrawVoters, Uniform_AllInRange) {
  PRNG prng(7);
  const auto cfg = make_voter_sampler_uniform(-1.0, 1.0);
  const auto coords = draw_voters(200, 2, cfg, prng);
  for (double v : coords) {
    EXPECT_GE(v, -1.0);
    EXPECT_LE(v, 1.0);
  }
}

TEST(DrawVoters, Uniform_Reproducible) {
  const auto cfg = make_voter_sampler_uniform(-1.0, 1.0);
  PRNG p1(99);
  PRNG p2(99);
  EXPECT_EQ(draw_voters(20, 2, cfg, p1), draw_voters(20, 2, cfg, p2));
}

// ==========================================================================
// VS4 — draw_voters: gaussian
// ==========================================================================

TEST(DrawVoters, Gaussian_CorrectLength) {
  PRNG prng(1);
  const auto cfg = make_voter_sampler_gaussian(0.0, 1.0);
  const auto coords = draw_voters(15, 2, cfg, prng);
  EXPECT_EQ(static_cast<int>(coords.size()), 30);
}

TEST(DrawVoters, Gaussian_AllFinite) {
  PRNG prng(3);
  const auto cfg = make_voter_sampler_gaussian(0.0, 0.5);
  const auto coords = draw_voters(100, 2, cfg, prng);
  for (double v : coords) EXPECT_TRUE(std::isfinite(v));
}

TEST(DrawVoters, Gaussian_Reproducible) {
  const auto cfg = make_voter_sampler_gaussian(0.0, 1.0);
  PRNG p1(55);
  PRNG p2(55);
  EXPECT_EQ(draw_voters(30, 2, cfg, p1), draw_voters(30, 2, cfg, p2));
}

// ==========================================================================
// VS5 — draw_voters: error cases
// ==========================================================================

TEST(DrawVoters, ThrowsIfNVotersTooSmall) {
  PRNG prng(1);
  const auto cfg = make_voter_sampler_uniform(-1.0, 1.0);
  EXPECT_THROW(draw_voters(0, 2, cfg, prng), std::invalid_argument);
}

TEST(DrawVoters, ThrowsIfNDimsTooSmall) {
  PRNG prng(1);
  const auto cfg = make_voter_sampler_uniform(-1.0, 1.0);
  EXPECT_THROW(draw_voters(10, 0, cfg, prng), std::invalid_argument);
}
