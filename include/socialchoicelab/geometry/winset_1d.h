// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// winset_1d.h — Euclidean 1D WinSet interval computation.
//
// For a seat holder at seat_x and voters at voter_x[0..n_voters-1], the
// WinSet is the set of challenger positions x such that a strict majority of
// voters prefer x over seat_x under Euclidean (L2) distance.
//
// Voter i prefers x to seat_x iff |x - voter_x[i]| < |voter_x[i] - seat_x|,
// which defines an open interval (voter_x[i] - d_i, voter_x[i] + d_i) where
// d_i = |voter_x[i] - seat_x|.
//
// Algorithm: O(n log n + n^2 / n) ≈ O(n^2) sweep over the 2n critical
// x-values (interval endpoints). Between consecutive critical values, the
// majority-preference count is constant; a midpoint probe counts the active
// intervals in O(n). The total cost is O(n^2), acceptable for typical voter
// counts (n ≤ a few hundred per frame).
//
// Returns a WinSetInterval1d{lo, hi}. If the WinSet is empty (e.g. seat_x
// sits at the median voter), both lo and hi are NaN.
//
// References:
//   Black (1948), "On the Rationale of Group Decision-Making", JPE.
//   Plott (1967), "A Notion of Equilibrium", AER.

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace socialchoicelab::geometry {

struct WinSetInterval1d {
  double lo;  // lower bound; NaN if WinSet is empty
  double hi;  // upper bound; NaN if WinSet is empty

  [[nodiscard]] bool empty() const { return std::isnan(lo); }
};

[[nodiscard]] inline WinSetInterval1d winset_interval_1d(const double* voter_x,
                                                         int n_voters,
                                                         double seat_x) {
  constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

  if (n_voters <= 0) return {kNaN, kNaN};

  // Collect the 2n critical x-values (open-interval endpoints).
  std::vector<double> xs;
  xs.reserve(static_cast<size_t>(2 * n_voters) + 1);
  xs.push_back(seat_x);  // anchor: seat position is always a boundary
  for (int i = 0; i < n_voters; ++i) {
    double d = std::abs(voter_x[i] - seat_x);
    if (d > 0.0) {
      xs.push_back(voter_x[i] - d);
      xs.push_back(voter_x[i] + d);
    }
  }
  std::sort(xs.begin(), xs.end());
  xs.erase(std::unique(xs.begin(), xs.end()), xs.end());

  if (xs.size() < 2) return {kNaN, kNaN};

  // For each gap between consecutive critical values, probe the midpoint.
  // A strict majority prefers midpoint x to seat_x iff count(x)*2 > n_voters.
  auto count_preferring = [&](double x) -> int {
    int cnt = 0;
    for (int i = 0; i < n_voters; ++i) {
      if (std::abs(x - voter_x[i]) < std::abs(voter_x[i] - seat_x)) ++cnt;
    }
    return cnt;
  };

  double ws_lo = kNaN;
  double ws_hi = kNaN;
  bool in_ws = false;

  for (std::size_t j = 0; j + 1 < xs.size(); ++j) {
    double mid = (xs[j] + xs[j + 1]) * 0.5;
    bool majority = count_preferring(mid) * 2 > n_voters;

    if (!in_ws && majority) {
      ws_lo = xs[j];
      in_ws = true;
    } else if (in_ws && !majority) {
      ws_hi = xs[j];
      in_ws = false;
      // In 1D Euclidean with simple majority the WinSet is at most one
      // connected interval; stop after the first segment closes.
      break;
    }
  }

  if (in_ws) ws_hi = xs.back();
  if (std::isnan(ws_lo)) return {kNaN, kNaN};
  return {ws_lo, ws_hi};
}

}  // namespace socialchoicelab::geometry
