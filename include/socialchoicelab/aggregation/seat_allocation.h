// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace socialchoicelab::aggregation {

namespace detail {

[[nodiscard]] inline std::vector<int> sorted_indices_desc_then_index(
    const std::vector<int>& values) {
  std::vector<int> order(values.size());
  std::iota(order.begin(), order.end(), 0);
  std::sort(order.begin(), order.end(), [&](int a, int b) {
    if (values[a] != values[b]) return values[a] > values[b];
    return a < b;
  });
  return order;
}

}  // namespace detail

[[nodiscard]] inline std::vector<int> plurality_top_k_seat_allocation(
    const std::vector<int>& vote_totals, int seat_count) {
  if (seat_count < 0) {
    throw std::invalid_argument(
        "plurality_top_k_seat_allocation: seat_count must be non-negative.");
  }
  if (seat_count > static_cast<int>(vote_totals.size())) {
    throw std::invalid_argument(
        "plurality_top_k_seat_allocation: seat_count must not exceed the "
        "number of competitors.");
  }
  for (int votes : vote_totals) {
    if (votes < 0) {
      throw std::invalid_argument(
          "plurality_top_k_seat_allocation: vote totals must be "
          "non-negative.");
    }
  }

  std::vector<int> seats(vote_totals.size(), 0);
  const auto order = detail::sorted_indices_desc_then_index(vote_totals);
  for (int i = 0; i < seat_count; ++i) {
    ++seats[order[i]];
  }
  return seats;
}

// Hare Largest Remainder proportional seat allocation.
//
// Note on seat_count vs. competitor count: unlike plurality_top_k_seat_
// allocation, this function intentionally does not require seat_count <=
// vote_totals.size(). In a PR system a single party can legitimately win
// multiple seats (e.g., 2 parties, 10 seats — one party may receive 8).
// Callers using kHareLargestRemainder with seat_count > n_competitors will
// see one or more competitors receive multiple seats, which is correct PR
// behaviour. The inconsistency with plurality_top_k is by design: the two
// rules have fundamentally different natural constraints.
[[nodiscard]] inline std::vector<int> hare_largest_remainder_allocation(
    const std::vector<int>& vote_totals, int seat_count) {
  if (seat_count < 0) {
    throw std::invalid_argument(
        "hare_largest_remainder_allocation: seat_count must be non-negative.");
  }
  for (int votes : vote_totals) {
    if (votes < 0) {
      throw std::invalid_argument(
          "hare_largest_remainder_allocation: vote totals must be "
          "non-negative.");
    }
  }

  const int n = static_cast<int>(vote_totals.size());
  std::vector<int> seats(n, 0);
  if (seat_count == 0 || n == 0) return seats;

  const int total_votes =
      std::accumulate(vote_totals.begin(), vote_totals.end(), 0);

  if (total_votes == 0) {
    const int base = seat_count / n;
    const int remainder = seat_count % n;
    std::fill(seats.begin(), seats.end(), base);
    for (int i = 0; i < remainder; ++i) {
      ++seats[i];
    }
    return seats;
  }

  const double quota = static_cast<double>(total_votes) / seat_count;
  std::vector<double> remainders(n, 0.0);
  int allocated = 0;

  for (int i = 0; i < n; ++i) {
    const double exact = static_cast<double>(vote_totals[i]) / quota;
    seats[i] = static_cast<int>(std::floor(exact));
    remainders[i] = exact - seats[i];
    allocated += seats[i];
  }

  std::vector<int> order(n);
  std::iota(order.begin(), order.end(), 0);
  std::sort(order.begin(), order.end(), [&](int a, int b) {
    if (remainders[a] != remainders[b]) return remainders[a] > remainders[b];
    if (vote_totals[a] != vote_totals[b])
      return vote_totals[a] > vote_totals[b];
    return a < b;
  });

  for (int i = 0; i < seat_count - allocated; ++i) {
    ++seats[order[i]];
  }

  return seats;
}

}  // namespace socialchoicelab::aggregation
