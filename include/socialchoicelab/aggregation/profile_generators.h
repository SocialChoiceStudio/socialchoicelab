// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// profile_generators.h — Synthetic preference profile generators.
//
// Three generators:
//   uniform_spatial_profile   — voter ideals drawn from U([x,y]²), then
//                               ranked by distance.
//   gaussian_spatial_profile  — voter ideals drawn from N(center, σ²I),
//                               then ranked by distance.
//   impartial_culture          — each voter's ranking drawn uniformly from
//                               all m! orderings (Fisher-Yates shuffle).
//                               No spatial model; purely ordinal.
//
// All generators accept a PRNG reference for reproducibility.

#include <socialchoicelab/aggregation/profile.h>
#include <socialchoicelab/core/rng/prng.h>

#include <algorithm>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace socialchoicelab::aggregation {

/**
 * @brief Generate a spatial profile with voters drawn from a uniform
 *        distribution over [xmin,xmax] × [ymin,ymax].
 *
 * @param n_voters  Number of voters to generate.
 * @param alternatives  Fixed set of m alternative positions.
 * @param xmin,xmax,ymin,ymax  Bounding box for voter ideals.
 * @param cfg       Distance metric configuration.
 * @param prng      PRNG for voter ideal generation.
 * @return          Well-formed profile with n_voters voters and n_alts =
 *                  alternatives.size().
 * @throws std::invalid_argument if n_voters < 1 or xmin >= xmax or
 *         ymin >= ymax.
 */
[[nodiscard]] inline Profile uniform_spatial_profile(
    int n_voters, const std::vector<Point2d>& alternatives, double xmin,
    double xmax, double ymin, double ymax, const DistConfig& cfg,
    core::rng::PRNG& prng) {
  if (n_voters < 1) {
    throw std::invalid_argument(
        "uniform_spatial_profile: n_voters must be at least 1 (got " +
        std::to_string(n_voters) + ").");
  }
  if (xmin >= xmax) {
    throw std::invalid_argument(
        "uniform_spatial_profile: xmin must be less than xmax (got xmin=" +
        std::to_string(xmin) + ", xmax=" + std::to_string(xmax) + ").");
  }
  if (ymin >= ymax) {
    throw std::invalid_argument(
        "uniform_spatial_profile: ymin must be less than ymax (got ymin=" +
        std::to_string(ymin) + ", ymax=" + std::to_string(ymax) + ").");
  }

  std::uniform_real_distribution<double> x_dist(xmin, xmax);
  std::uniform_real_distribution<double> y_dist(ymin, ymax);

  std::vector<Point2d> voter_ideals;
  voter_ideals.reserve(n_voters);
  for (int i = 0; i < n_voters; ++i) {
    voter_ideals.emplace_back(x_dist(prng.engine()), y_dist(prng.engine()));
  }
  return build_spatial_profile(alternatives, voter_ideals, cfg);
}

/**
 * @brief Generate a spatial profile with voters drawn from a bivariate
 *        Gaussian distribution N(center, std_dev² I).
 *
 * Each coordinate is drawn independently from N(center[d], std_dev²).
 *
 * @param n_voters     Number of voters to generate.
 * @param alternatives Fixed set of m alternative positions.
 * @param center       Mean of the Gaussian.
 * @param std_dev      Standard deviation (same for both dimensions, > 0).
 * @param cfg          Distance metric configuration.
 * @param prng         PRNG for voter ideal generation.
 * @return             Well-formed profile.
 * @throws std::invalid_argument if n_voters < 1 or std_dev <= 0.
 */
[[nodiscard]] inline Profile gaussian_spatial_profile(
    int n_voters, const std::vector<Point2d>& alternatives,
    const Point2d& center, double std_dev, const DistConfig& cfg,
    core::rng::PRNG& prng) {
  if (n_voters < 1) {
    throw std::invalid_argument(
        "gaussian_spatial_profile: n_voters must be at least 1 (got " +
        std::to_string(n_voters) + ").");
  }
  if (std_dev <= 0.0) {
    throw std::invalid_argument(
        "gaussian_spatial_profile: std_dev must be positive (got " +
        std::to_string(std_dev) + ").");
  }

  std::normal_distribution<double> x_dist(center.x(), std_dev);
  std::normal_distribution<double> y_dist(center.y(), std_dev);

  std::vector<Point2d> voter_ideals;
  voter_ideals.reserve(n_voters);
  for (int i = 0; i < n_voters; ++i) {
    voter_ideals.emplace_back(x_dist(prng.engine()), y_dist(prng.engine()));
  }
  return build_spatial_profile(alternatives, voter_ideals, cfg);
}

/**
 * @brief Generate a profile under the impartial culture (IC) assumption.
 *
 * Each voter's ranking is drawn independently and uniformly from all m!
 * orderings using a Fisher-Yates shuffle.  There is no spatial model;
 * alternative indices 0 … n_alts-1 are used directly.
 *
 * @param n_voters  Number of voters.
 * @param n_alts    Number of alternatives.
 * @param prng      PRNG used for shuffling.
 * @return          Well-formed profile with n_voters voters and n_alts alts.
 * @throws std::invalid_argument if n_voters < 1 or n_alts < 1.
 */
[[nodiscard]] inline Profile impartial_culture(int n_voters, int n_alts,
                                               core::rng::PRNG& prng) {
  if (n_voters < 1) {
    throw std::invalid_argument(
        "impartial_culture: n_voters must be at least 1 (got " +
        std::to_string(n_voters) + ").");
  }
  if (n_alts < 1) {
    throw std::invalid_argument(
        "impartial_culture: n_alts must be at least 1 (got " +
        std::to_string(n_alts) + ").");
  }

  Profile profile;
  profile.n_voters = n_voters;
  profile.n_alts = n_alts;
  profile.rankings.resize(n_voters);

  std::vector<int> order(n_alts);
  for (auto& ranking : profile.rankings) {
    std::iota(order.begin(), order.end(), 0);
    std::shuffle(order.begin(), order.end(), prng.engine());
    ranking = order;
  }
  return profile;
}

}  // namespace socialchoicelab::aggregation
