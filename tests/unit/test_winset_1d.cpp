// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <socialchoicelab/geometry/winset_1d.h>

#include <cmath>

using socialchoicelab::geometry::winset_interval_1d;

namespace {

// 3-voter case.  Voters at -1, 0, +1; seat at 2.
// Distances: d0=3, d1=2, d2=1.
// Preferred intervals:
//   voter -1: (-4, 2)   voter 0: (-2, 2)   voter 1: (0, 2)
// Majority (count*2 > 3, i.e. count >= 2):
//   x ∈ (-2, 0): voters -1 and 0 → count=2.
//   x ∈ (0, 2): all three → count=3.
//   → WinSet = (-2, 2).
TEST(WinSet1d, ThreeVotersOffMedian) {
  double voters[] = {-1.0, 0.0, 1.0};
  auto iv = winset_interval_1d(voters, 3, 2.0);
  ASSERT_FALSE(iv.empty()) << "WinSet should be non-empty";
  EXPECT_NEAR(iv.lo, -2.0, 1e-9);
  EXPECT_NEAR(iv.hi, 2.0, 1e-9);
}

// Seat at the median (voter at 0) — WinSet should be empty.
TEST(WinSet1d, SeatAtMedianIsEmpty) {
  double voters[] = {-1.0, 0.0, 1.0};
  auto iv = winset_interval_1d(voters, 3, 0.0);
  EXPECT_TRUE(iv.empty()) << "WinSet of a median seat must be empty";
}

// Single voter: majority means count*2 > 1, i.e. count >= 1.
// The one voter's preferred interval is the WinSet.
// voter at 0.5, seat at 1.0 → d=0.5, preferred interval = (0, 1).
TEST(WinSet1d, SingleVoterHasWinSet) {
  double voters[] = {0.5};
  auto iv = winset_interval_1d(voters, 1, 1.0);
  ASSERT_FALSE(iv.empty());
  EXPECT_NEAR(iv.lo, 0.0, 1e-9);
  EXPECT_NEAR(iv.hi, 1.0, 1e-9);
}

// Even voters: n=4, majority requires count*2 > 4, i.e. count >= 3.
// Voters at -1, 0, 1, 2; seat at 3.
// Distances: 4, 3, 2, 1.
// Preferred intervals:
//   voter -1: (-5, 3)   voter 0: (-3, 3)   voter 1: (-1, 3)   voter 2: (1, 3)
// count=3 starting at x=-1 (voters -1, 0, 1 all prefer from there).
// → WinSet = (-1, 3).
TEST(WinSet1d, FourVotersMajority) {
  double voters[] = {-1.0, 0.0, 1.0, 2.0};
  auto iv = winset_interval_1d(voters, 4, 3.0);
  ASSERT_FALSE(iv.empty());
  EXPECT_NEAR(iv.lo, -1.0, 1e-9);
  EXPECT_NEAR(iv.hi, 3.0, 1e-9);
}

// Zero voters — should return empty.
TEST(WinSet1d, ZeroVoters) {
  auto iv = winset_interval_1d(nullptr, 0, 0.0);
  EXPECT_TRUE(iv.empty());
}

// WinSet length is positive when non-empty.
TEST(WinSet1d, LengthIsPositive) {
  double voters[] = {0.0, 1.0, 2.0};
  auto iv = winset_interval_1d(voters, 3, 3.0);
  if (!iv.empty()) {
    EXPECT_GT(iv.hi - iv.lo, 0.0);
  }
}

}  // namespace
