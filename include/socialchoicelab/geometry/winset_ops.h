// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// winset_ops.h — Boolean set operations on WinsetRegion (F1).
//
// Provides free-function wrappers around CGAL::General_polygon_set_2's
// in-place boolean operations, returning a new WinsetRegion result:
//
//   winset_union(a, b)                → WinsetRegion  (a ∪ b)
//   winset_intersection(a, b)        → WinsetRegion  (a ∩ b)
//   winset_difference(a, b)          → WinsetRegion  (a \ b)
//   winset_symmetric_difference(a,b) → WinsetRegion  (a △ b)
//
// Note: winset_is_empty() and winset_contains() for WinsetRegion already
// live in winset.h and are not duplicated here.
//
// All operations use CGAL exact arithmetic (EPEC kernel) and inherit the
// same robustness guarantees as winset_2d.

#include <socialchoicelab/geometry/winset.h>

namespace socialchoicelab::geometry {

// ===========================================================================
// F1 — Boolean set operations on WinsetRegion
// ===========================================================================

/**
 * @brief Return the union of two winset regions (a ∪ b).
 *
 * A policy point x is in the result iff it is in a, in b, or in both.
 * The operation uses CGAL::General_polygon_set_2::join(), which runs in
 * O((m₁ + m₂) log(m₁ + m₂)) for m-vertex inputs.
 */
[[nodiscard]] inline WinsetRegion winset_union(WinsetRegion a,
                                               const WinsetRegion& b) {
  a.join(b);
  return a;
}

/**
 * @brief Return the intersection of two winset regions (a ∩ b).
 *
 * A policy point x is in the result iff it is in both a and b.
 * Returns an empty WinsetRegion when the two regions are disjoint.
 */
[[nodiscard]] inline WinsetRegion winset_intersection(WinsetRegion a,
                                                      const WinsetRegion& b) {
  a.intersection(b);
  return a;
}

/**
 * @brief Return the set difference of two winset regions (a \ b).
 *
 * A policy point x is in the result iff it is in a but not in b.
 * Returns an empty WinsetRegion when a ⊆ b.
 */
[[nodiscard]] inline WinsetRegion winset_difference(WinsetRegion a,
                                                    const WinsetRegion& b) {
  a.difference(b);
  return a;
}

/**
 * @brief Return the symmetric difference of two winset regions (a △ b).
 *
 * A policy point x is in the result iff it is in exactly one of a or b
 * (i.e. in a ∪ b but not in a ∩ b).
 * Returns an empty WinsetRegion when a = b.
 */
[[nodiscard]] inline WinsetRegion winset_symmetric_difference(
    WinsetRegion a, const WinsetRegion& b) {
  a.symmetric_difference(b);
  return a;
}

}  // namespace socialchoicelab::geometry
