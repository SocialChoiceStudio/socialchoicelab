// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// tie_break.h — TieBreak policy enum and break_tie() helper.
//
// Used by all Category-3 voting rules (rules whose definition does not specify
// what to do when multiple alternatives share the top score).
//
// Two policies:
//   kRandom        — select uniformly at random among tied alternatives.
//                    Requires a non-null PRNG pointer.  This is the DEFAULT
//                    for all *_one_winner() functions because it is what users
//                    expect in simulations and interactive applications.
//   kSmallestIndex — select the tied alternative with the lowest index.
//                    Deterministic; pass this explicitly in tests that need
//                    reproducibility without a seeded PRNG.
//
// The burden of passing kSmallestIndex is on the test author, not the user.

#include <socialchoicelab/core/rng/prng.h>

#include <algorithm>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace socialchoicelab::aggregation {

enum class TieBreak {
  kRandom,         ///< Uniform random among tied winners (requires PRNG).
  kSmallestIndex,  ///< Lowest index among tied winners (deterministic).
};

/**
 * @brief Select one element from a non-empty vector of tied alternative
 *        indices.
 *
 * @param tied       Non-empty vector of tied alternative indices.
 * @param tie_break  Policy to apply.
 * @param prng       Required when tie_break == kRandom; ignored otherwise.
 * @return           Selected alternative index.
 * @throws std::invalid_argument if tied is empty, or if kRandom and prng
 *         is nullptr.
 */
[[nodiscard]] inline int break_tie(const std::vector<int>& tied,
                                   TieBreak tie_break, core::rng::PRNG* prng) {
  if (tied.empty()) {
    throw std::invalid_argument("break_tie: tied set must not be empty.");
  }
  if (tied.size() == 1) return tied[0];

  if (tie_break == TieBreak::kSmallestIndex) {
    return *std::min_element(tied.begin(), tied.end());
  }

  // kRandom
  if (prng == nullptr) {
    throw std::invalid_argument(
        "break_tie: TieBreak::kRandom requires a non-null PRNG. "
        "Pass a PRNG& to *_one_winner(), or use TieBreak::kSmallestIndex "
        "for deterministic results.");
  }
  std::uniform_int_distribution<int> dist(0, static_cast<int>(tied.size()) - 1);
  return tied[static_cast<std::size_t>(dist(prng->engine()))];
}

}  // namespace socialchoicelab::aggregation
