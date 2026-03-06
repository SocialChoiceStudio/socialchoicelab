// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <CGAL/Line_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Segment_2.h>
#include <socialchoicelab/core/kernels.h>
#include <socialchoicelab/core/types.h>

/**
 * @file geom2d.h
 * @brief Exact 2D geometry type layer and basic predicates.
 *
 * Provides type aliases for exact 2D CGAL types (Point2E, Segment2E,
 * Polygon2E), conversion functions between the numeric Eigen types used by
 * the core layer and the exact CGAL types used by the geometry layer, and
 * exact geometric predicates (orientation, bounded-side).
 *
 * All types use EpecKernel (Exact_predicates_exact_constructions_kernel) so
 * that both predicate tests and constructed coordinates are exact.
 *
 * Reference: CGAL manual § 2D Geometry Kernel.
 */

namespace socialchoicelab::geometry {

// ---------------------------------------------------------------------------
// Type aliases
// ---------------------------------------------------------------------------

/// Exact 2D point (EPEC). Use for voter ideal points, alternative points,
/// winset vertices, and any position that must survive exact constructions.
using Point2E = CGAL::Point_2<core::EpecKernel>;

/// Exact 2D line segment (EPEC). Use for median lines, cutlines,
/// perpendicular bisectors, and winset edges.
using Segment2E = CGAL::Segment_2<core::EpecKernel>;

/// Exact 2D simple polygon (EPEC), CCW orientation by convention.
/// Used for convex hulls, winsets, and all spatial solution concepts
/// (uncovered set boundary, Heart approximation, etc.).
using Polygon2E = CGAL::Polygon_2<core::EpecKernel>;

/// Exact 2D oriented line (EPEC). Used for k-quantile lines (Yolk computation),
/// median lines, and perpendicular bisectors.
using Line2E = CGAL::Line_2<core::EpecKernel>;

// ---------------------------------------------------------------------------
// Conversions between Eigen (numeric) and CGAL exact types
// ---------------------------------------------------------------------------

/// Convert a numeric Eigen 2D point to an exact CGAL point.
/// The double coordinates are lifted to EPEC exact numbers.  Small
/// floating-point representation errors in the input are preserved exactly
/// (not rounded further); downstream constructions remain exact from this
/// point on.
inline Point2E to_exact(const core::types::Point2d& p) {
  return Point2E(p.x(), p.y());
}

/// Convert an exact CGAL point back to a numeric Eigen point.
/// Uses CGAL::to_double() which returns the nearest representable double.
/// Precision is lost; use only for final output or display, not for further
/// exact computation.
inline core::types::Point2d to_numeric(const Point2E& p) {
  return core::types::Point2d(CGAL::to_double(p.x()), CGAL::to_double(p.y()));
}

// ---------------------------------------------------------------------------
// Geometric predicates
// ---------------------------------------------------------------------------

/// Orientation of the ordered triple (a, b, c).
enum class Orientation {
  kLeftTurn,   ///< Counter-clockwise turn; c is to the left of ray a→b.
  kRightTurn,  ///< Clockwise turn; c is to the right of ray a→b.
  kCollinear,  ///< a, b, c are collinear (on the same line).
};

/// Return the orientation of the ordered triple (a, b, c).
///
/// Uses exact EPEC arithmetic — the result is never wrong due to
/// floating-point cancellation, even for nearly-collinear inputs.
///
/// @param a  First point.
/// @param b  Second point (defines the direction of the ray a→b).
/// @param c  Third point being classified.
/// @return   kLeftTurn, kRightTurn, or kCollinear.
inline Orientation orientation(const Point2E& a, const Point2E& b,
                               const Point2E& c) {
  auto o = CGAL::orientation(a, b, c);
  if (o == CGAL::LEFT_TURN) return Orientation::kLeftTurn;
  if (o == CGAL::RIGHT_TURN) return Orientation::kRightTurn;
  return Orientation::kCollinear;
}

/// Which side of a simple polygon a point lies on.
enum class BoundedSide {
  kOnBoundedSide,    ///< Point is strictly inside the polygon.
  kOnBoundary,       ///< Point lies exactly on the polygon boundary.
  kOnUnboundedSide,  ///< Point is strictly outside the polygon.
};

/// Return which side of @p polygon the point @p pt lies on.
///
/// Uses exact EPEC arithmetic — boundary classification is never ambiguous.
/// @p polygon must be a simple (non-self-intersecting) polygon; behaviour is
/// undefined for self-intersecting inputs.
///
/// @param polygon  A simple polygon (CCW orientation by convention).
/// @param pt       The point to classify.
/// @return         kOnBoundedSide, kOnBoundary, or kOnUnboundedSide.
inline BoundedSide bounded_side(const Polygon2E& polygon, const Point2E& pt) {
  auto s = polygon.bounded_side(pt);
  if (s == CGAL::ON_BOUNDED_SIDE) return BoundedSide::kOnBoundedSide;
  if (s == CGAL::ON_BOUNDARY) return BoundedSide::kOnBoundary;
  return BoundedSide::kOnUnboundedSide;
}

}  // namespace socialchoicelab::geometry
