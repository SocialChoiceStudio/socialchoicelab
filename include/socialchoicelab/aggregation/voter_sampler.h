// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// voter_sampler.h — Standalone voter ideal-point generators.
//
// Unlike the profile generators in profile_generators.h, these functions
// produce only voter coordinates as a flat double vector [x0, y0, x1, y1, ...],
// not full preference profiles.  This makes them directly usable as
// voter_ideals in competition_run() or profile_build_spatial().
//
// Supported distributions:
//   Uniform  — each coordinate drawn from U([lo, hi])
//   Gaussian — each coordinate drawn from N(mean, stddev²)
//
// All generators accept a PRNG reference for reproducibility.

#include <socialchoicelab/core/rng/prng.h>

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace socialchoicelab::aggregation {

enum class VoterSamplerKind { Uniform = 0, Gaussian = 1 };

struct VoterSamplerConfig {
  VoterSamplerKind kind;
  double param1;  // lo (Uniform) or mean (Gaussian)
  double param2;  // hi (Uniform) or stddev (Gaussian)
};

[[nodiscard]] inline VoterSamplerConfig make_voter_sampler_uniform(double lo,
                                                                   double hi) {
  if (!std::isfinite(lo) || !std::isfinite(hi))
    throw std::invalid_argument(
        "make_voter_sampler_uniform: lo and hi must be finite.");
  if (lo >= hi)
    throw std::invalid_argument(
        "make_voter_sampler_uniform: lo must be < hi (got lo=" +
        std::to_string(lo) + ", hi=" + std::to_string(hi) + ").");
  return {VoterSamplerKind::Uniform, lo, hi};
}

[[nodiscard]] inline VoterSamplerConfig make_voter_sampler_gaussian(
    double mean, double stddev) {
  if (!std::isfinite(mean) || !std::isfinite(stddev))
    throw std::invalid_argument(
        "make_voter_sampler_gaussian: mean and stddev must be finite.");
  if (stddev <= 0.0)
    throw std::invalid_argument(
        "make_voter_sampler_gaussian: stddev must be positive (got " +
        std::to_string(stddev) + ").");
  return {VoterSamplerKind::Gaussian, mean, stddev};
}

/**
 * @brief Draw voter ideal points as a flat row-major vector
 *        [x0, y0, x1, y1, ...] of length n_voters * n_dims.
 *
 * @param n_voters  Number of voters to generate (>= 1).
 * @param n_dims    Number of spatial dimensions (>= 1).
 * @param cfg       Sampler configuration (kind and parameters).
 * @param prng      PRNG used for sampling.
 * @return          Flat vector of n_voters * n_dims coordinates.
 * @throws std::invalid_argument if n_voters < 1, n_dims < 1, or cfg is
 *         invalid (e.g. stddev <= 0 for Gaussian).
 */
[[nodiscard]] inline std::vector<double> draw_voters(
    int n_voters, int n_dims, const VoterSamplerConfig& cfg,
    core::rng::PRNG& prng) {
  if (n_voters < 1)
    throw std::invalid_argument("draw_voters: n_voters must be >= 1 (got " +
                                std::to_string(n_voters) + ").");
  if (n_dims < 1)
    throw std::invalid_argument("draw_voters: n_dims must be >= 1 (got " +
                                std::to_string(n_dims) + ").");

  const auto total =
      static_cast<std::size_t>(n_voters) * static_cast<std::size_t>(n_dims);
  std::vector<double> out;
  out.reserve(total);

  switch (cfg.kind) {
    case VoterSamplerKind::Uniform:
      for (std::size_t i = 0; i < total; ++i)
        out.push_back(prng.uniform_real(cfg.param1, cfg.param2));
      break;
    case VoterSamplerKind::Gaussian:
      for (std::size_t i = 0; i < total; ++i)
        out.push_back(prng.normal(cfg.param1, cfg.param2));
      break;
    default:
      throw std::invalid_argument("draw_voters: unknown VoterSamplerKind.");
  }
  return out;
}

}  // namespace socialchoicelab::aggregation
