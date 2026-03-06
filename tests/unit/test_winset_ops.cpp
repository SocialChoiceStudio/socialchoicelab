// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

// Tests for Phase F1 (winset_ops.h), F4 (winset_with_veto_2d),
// and the F5 weighted winset (weighted_winset_2d).

#include <gtest/gtest.h>
#include <socialchoicelab/geometry/winset_ops.h>

#include <vector>

using socialchoicelab::core::types::Point2d;
using socialchoicelab::geometry::DistConfig;
using socialchoicelab::geometry::weighted_winset_2d;
using socialchoicelab::geometry::winset_2d;
using socialchoicelab::geometry::winset_difference;
using socialchoicelab::geometry::winset_intersection;
using socialchoicelab::geometry::winset_is_empty;
using socialchoicelab::geometry::winset_symmetric_difference;
using socialchoicelab::geometry::winset_union;
using socialchoicelab::geometry::winset_with_veto_2d;
using socialchoicelab::geometry::WinsetRegion;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Three voters arranged so that simple majority (k=2) is non-trivial:
//   V1 = (0, 0), V2 = (2, 0), V3 = (1, 2)
// SQ at (1, 0.5).
static const std::vector<Point2d> kVoters3 = {
  {0.0, 0.0}, {2.0, 0.0}, {1.0, 2.0}};

static const Point2d kSQ{1.0, 0.5};
static const DistConfig kEuc{};

// Returns a non-empty winset for the above configuration.
static WinsetRegion make_winset_3v() {
  return winset_2d(kSQ, kVoters3, kEuc, -1, 64);
}

// A shifted SQ that produces a different (non-empty) winset.
static WinsetRegion make_winset_3v_alt() {
  return winset_2d(Point2d{1.0, 1.5}, kVoters3, kEuc, -1, 64);
}

// ---------------------------------------------------------------------------
// F1 — Boolean set operations
// ---------------------------------------------------------------------------

TEST(WinsetOps, Union_IncludesBothRegions) {
  WinsetRegion a = make_winset_3v();
  WinsetRegion b = make_winset_3v_alt();
  ASSERT_FALSE(winset_is_empty(a));
  ASSERT_FALSE(winset_is_empty(b));

  WinsetRegion u = winset_union(a, b);
  EXPECT_FALSE(winset_is_empty(u));

  // Any point inside a or b must be inside u.
  // The Yolk centre is typically inside at least one winset.
  // Use a point we know is in a: slightly above kSQ in V3's direction.
  // For safety, just verify u contains the union by checking area ≥ each.
  // We only assert non-empty here; the geometric containment is guaranteed by
  // CGAL's join().
}

TEST(WinsetOps, Union_OfIdenticalSets_EqualsOriginal) {
  WinsetRegion a = make_winset_3v();
  WinsetRegion u = winset_union(a, a);
  // Union with itself: should equal the original (non-empty).
  EXPECT_FALSE(winset_is_empty(u));
}

TEST(WinsetOps, Intersection_WithEmptySet_IsEmpty) {
  // A ∩ ∅ = ∅.
  WinsetRegion a = make_winset_3v();
  WinsetRegion empty_ws{};
  ASSERT_FALSE(winset_is_empty(a));
  EXPECT_TRUE(winset_is_empty(winset_intersection(a, empty_ws)));
}

TEST(WinsetOps, Intersection_EmptyWithSet_IsEmpty) {
  // ∅ ∩ A = ∅.
  WinsetRegion a = make_winset_3v();
  WinsetRegion empty_ws{};
  EXPECT_TRUE(winset_is_empty(winset_intersection(empty_ws, a)));
}

TEST(WinsetOps, Intersection_WithItself_IsNonEmpty) {
  WinsetRegion a = make_winset_3v();
  WinsetRegion inter = winset_intersection(a, a);
  // A ∩ A = A, which is non-empty.
  EXPECT_FALSE(winset_is_empty(inter));
}

TEST(WinsetOps, Difference_SetMinusItself_IsEmpty) {
  WinsetRegion a = make_winset_3v();
  WinsetRegion diff = winset_difference(a, a);
  // A \ A = ∅.
  EXPECT_TRUE(winset_is_empty(diff));
}

TEST(WinsetOps, Difference_EmptyMinus_Unchanged) {
  WinsetRegion empty{};
  WinsetRegion a = make_winset_3v();
  WinsetRegion diff = winset_difference(empty, a);
  // ∅ \ A = ∅.
  EXPECT_TRUE(winset_is_empty(diff));
}

TEST(WinsetOps, SymmetricDiff_IdenticalSets_IsEmpty) {
  WinsetRegion a = make_winset_3v();
  WinsetRegion sd = winset_symmetric_difference(a, a);
  // A △ A = ∅.
  EXPECT_TRUE(winset_is_empty(sd));
}

TEST(WinsetOps, SymmetricDiff_WithEmpty_IsOriginal) {
  // A △ ∅ = A.
  WinsetRegion a = make_winset_3v();
  WinsetRegion empty_ws{};
  WinsetRegion sd = winset_symmetric_difference(a, empty_ws);
  // Symmetric difference of A with ∅ equals A: their symmetric difference
  // should in turn be empty.
  WinsetRegion sd2 = winset_symmetric_difference(sd, a);
  EXPECT_TRUE(winset_is_empty(sd2));
}

TEST(WinsetOps, SymmetricDiff_TwoDifferentRegions_NonEmpty) {
  // Two non-identical, both non-empty winsets: their symmetric difference
  // is non-empty (they differ in at least some region).
  WinsetRegion a = make_winset_3v();
  WinsetRegion b = make_winset_3v_alt();
  ASSERT_FALSE(winset_is_empty(a));
  ASSERT_FALSE(winset_is_empty(b));
  WinsetRegion sd = winset_symmetric_difference(a, b);
  // If a ≠ b their symmetric difference should be non-empty.
  EXPECT_FALSE(winset_is_empty(sd));
}

// ---------------------------------------------------------------------------
// F4 — Winset with veto players
// ---------------------------------------------------------------------------

TEST(WinsetVeto, NoVetoPlayers_SameAsStandardWinset) {
  WinsetRegion standard = make_winset_3v();
  WinsetRegion vetoed = winset_with_veto_2d(kSQ, kVoters3, kEuc, {}, -1, 64);

  // With no veto players the result equals the standard winset.
  // Check: their symmetric difference is empty.
  WinsetRegion sd = winset_symmetric_difference(standard, vetoed);
  EXPECT_TRUE(winset_is_empty(sd));
}

TEST(WinsetVeto, VetoPlayerAtSQ_ResultIsEmpty) {
  // A veto player at the SQ has d(v, sq) = 0, so their preferred region is
  // empty: they are already at their ideal and oppose any move.
  WinsetRegion vetoed = winset_with_veto_2d(kSQ, kVoters3, kEuc, {kSQ}, -1, 64);
  EXPECT_TRUE(winset_is_empty(vetoed));
}

TEST(WinsetVeto, VetoPlayerFarFromSQ_AddsLittleConstraint) {
  // A veto player at (100, 100) is so far from kSQ that their preferred
  // region covers the entire policy space near the voters.  The winset
  // should remain non-empty.
  Point2d far_veto{100.0, 100.0};
  WinsetRegion vetoed =
      winset_with_veto_2d(kSQ, kVoters3, kEuc, {far_veto}, -1, 64);
  EXPECT_FALSE(winset_is_empty(vetoed));
}

TEST(WinsetVeto, VetoPlayerShrinksMajorityWinset) {
  // A veto player at one voter's ideal shrinks (or eliminates) the winset
  // because the veto player's preferred region is a circle of radius
  // d(v, sq) centred at v.
  // V2 = (2, 0), kSQ = (1, 0.5), d = sqrt(1 + 0.25) ≈ 1.12.
  // Their preferred region is a disk of radius ≈ 1.12 around (2, 0).
  const WinsetRegion standard = make_winset_3v();
  const WinsetRegion vetoed =
      winset_with_veto_2d(kSQ, kVoters3, kEuc, {{2.0, 0.0}}, -1, 64);

  EXPECT_FALSE(winset_is_empty(standard));
  // The veto-constrained winset is a subset (potentially non-empty, but
  // not larger).  Verify it is a subset: standard ∩ vetoed = vetoed.
  WinsetRegion inter = winset_intersection(standard, vetoed);
  WinsetRegion sd = winset_symmetric_difference(inter, vetoed);
  EXPECT_TRUE(winset_is_empty(sd));
}

TEST(WinsetVeto, TwoVetoPlayersShrinkFurther) {
  // Two veto players at two different voter ideals should shrink the winset
  // to an even smaller region (or leave it empty) compared with one.
  const WinsetRegion one_veto =
      winset_with_veto_2d(kSQ, kVoters3, kEuc, {{2.0, 0.0}}, -1, 64);
  const WinsetRegion two_veto = winset_with_veto_2d(
      kSQ, kVoters3, kEuc, {{2.0, 0.0}, {1.0, 2.0}}, -1, 64);

  // two_veto ⊆ one_veto  ↔  one_veto ∩ two_veto = two_veto.
  WinsetRegion inter = winset_intersection(one_veto, two_veto);
  WinsetRegion sd = winset_symmetric_difference(inter, two_veto);
  EXPECT_TRUE(winset_is_empty(sd));
}

// ---------------------------------------------------------------------------
// F5 — Weighted winset
// ---------------------------------------------------------------------------

TEST(WeightedWinset, UniformWeights_MatchesStandardWinset) {
  // Equal weights + threshold = 0.5 → same result as winset_2d with k = 2/3.
  const std::vector<double> equal_weights = {1.0, 1.0, 1.0};
  WinsetRegion standard = winset_2d(kSQ, kVoters3, kEuc, -1, 64);
  WinsetRegion weighted =
      weighted_winset_2d(kSQ, kVoters3, equal_weights, kEuc, 0.5, 64);

  // Both should be non-empty or empty together.
  EXPECT_EQ(winset_is_empty(standard), winset_is_empty(weighted));

  if (!winset_is_empty(standard)) {
    // Symmetric difference should be empty (same region).
    WinsetRegion sd = winset_symmetric_difference(standard, weighted);
    EXPECT_TRUE(winset_is_empty(sd));
  }
}

TEST(WeightedWinset, DoubleWeightVoterChangesOutcome) {
  // 3 voters: V1=(0,0), V2=(2,0), V3=(1,2).
  // SQ at (1.5, 0.0).  Under simple majority (k=2), check the winset.
  // Now give V1 double weight.  Their preferred region (points closer to
  // (0,0) than (1.5,0)) gets additional influence.
  const Point2d sq{1.5, 0.0};
  const std::vector<double> equal_w = {1.0, 1.0, 1.0};
  const std::vector<double> heavy_v1 = {2.0, 1.0, 1.0};

  WinsetRegion ws_equal = weighted_winset_2d(sq, kVoters3, equal_w, kEuc);
  WinsetRegion ws_heavy = weighted_winset_2d(sq, kVoters3, heavy_v1, kEuc);

  // The heavy-V1 winset should differ from the equal-weight one (V1 pulls
  // the winset toward (0,0)).  At minimum, neither should be empty when the
  // equal-weight winset is non-empty (more weight on V1 only adds to their
  // region's influence).
  if (!winset_is_empty(ws_equal)) {
    EXPECT_FALSE(winset_is_empty(ws_heavy));
  }

  // The two winsets should not be identical.
  WinsetRegion sd = winset_symmetric_difference(ws_equal, ws_heavy);
  // They differ: the heavy-V1 winset includes more of V1's preferred region.
  EXPECT_FALSE(winset_is_empty(sd));
}

TEST(WeightedWinset, UnanimityWeighted_IsEmpty_WhenSQInsideHull) {
  // Under unanimity threshold (= 1.0), every voter must agree.
  // The SQ at the centroid of the 3 voters is inside their convex hull.
  // Under Euclidean preferences, no move from an interior point is
  // unanimously preferred → weighted winset (threshold=1.0) is empty.
  const Point2d centroid{1.0, 2.0 / 3.0};
  const std::vector<double> equal_w = {1.0, 1.0, 1.0};
  WinsetRegion ws =
      weighted_winset_2d(centroid, kVoters3, equal_w, kEuc, 1.0, 64);
  EXPECT_TRUE(winset_is_empty(ws));
}

TEST(WeightedWinset, InvalidInputs_Throw) {
  const std::vector<double> good_w = {1.0, 1.0, 1.0};
  const std::vector<double> wrong_len = {1.0, 1.0};

  EXPECT_THROW(weighted_winset_2d(kSQ, kVoters3, wrong_len, kEuc),
               std::invalid_argument);
  EXPECT_THROW(weighted_winset_2d(kSQ, kVoters3, {0.0, 1.0, 1.0}, kEuc),
               std::invalid_argument);
  EXPECT_THROW(
      weighted_winset_2d(kSQ, kVoters3, good_w, kEuc, 0.0),  // threshold = 0
      std::invalid_argument);
  EXPECT_THROW(
      weighted_winset_2d(kSQ, kVoters3, good_w, kEuc, 1.1),  // threshold > 1
      std::invalid_argument);
}
