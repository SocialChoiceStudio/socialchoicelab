// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <socialchoicelab/aggregation/seat_allocation.h>

#include <numeric>
#include <vector>

using socialchoicelab::aggregation::hare_largest_remainder_allocation;
using socialchoicelab::aggregation::plurality_top_k_seat_allocation;

TEST(SeatAllocation, PluralityTopKAssignsSeatsToHighestVoteTotals) {
  const auto seats = plurality_top_k_seat_allocation({10, 7, 7, 2}, 2);
  EXPECT_EQ(seats, (std::vector<int>{1, 1, 0, 0}));
}

TEST(SeatAllocation, HareLargestRemainderMatchesWorkedExample) {
  const auto seats = hare_largest_remainder_allocation({100, 60, 40}, 5);
  EXPECT_EQ(seats, (std::vector<int>{3, 1, 1}));
}

TEST(SeatAllocation, HareLargestRemainderPreservesSeatCount) {
  const auto seats = hare_largest_remainder_allocation({12, 9, 4, 1}, 7);
  EXPECT_EQ(std::accumulate(seats.begin(), seats.end(), 0), 7);
}

TEST(SeatAllocation, HareLargestRemainderHandlesZeroVoteCompetitors) {
  const auto seats = hare_largest_remainder_allocation({10, 0, 5}, 3);
  EXPECT_EQ(seats, (std::vector<int>{2, 0, 1}));
}

TEST(SeatAllocation,
     HareLargestRemainderAllZeroVotesFallsBackDeterministically) {
  const auto seats = hare_largest_remainder_allocation({0, 0, 0}, 5);
  EXPECT_EQ(seats, (std::vector<int>{2, 2, 1}));
}
