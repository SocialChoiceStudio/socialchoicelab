// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

namespace socialchoicelab::core {

/**
 * @brief CGAL kernel aliases used throughout the geometry layer.
 *
 * EpecKernel (Exact_predicates_exact_constructions_kernel):
 *   - All geometric predicates AND constructions are exact.
 *   - Coordinates are multi-precision numbers backed by GMP/MPFR.
 *   - Use for output geometry (Polygon2E, Point2E, Segment2E) where
 *     the results of constructions (intersections, midpoints, etc.)
 *     must be exact for downstream correctness.
 *   - Slower than EpicKernel due to arbitrary-precision arithmetic.
 *
 * EpicKernel (Exact_predicates_inexact_constructions_kernel):
 *   - Predicates (orientation, in-circle, side-of-line, etc.) are exact
 *     via filtered interval arithmetic — no incorrect sign decisions.
 *   - Coordinates are floating-point (double); constructions may accumulate
 *     rounding error and are NOT exact.
 *   - Use when you only need to test geometric relationships and do not
 *     need exact constructed point coordinates.
 *   - Faster than EpecKernel for predicate-only work.
 *
 * Policy: use EpecKernel (via the Point2E / Polygon2E aliases in geom2d.h)
 * as the default for all geometry in this project. EpicKernel is available
 * for internal algorithmic use where exact constructions are not required.
 */
using EpecKernel = CGAL::Exact_predicates_exact_constructions_kernel;
using EpicKernel = CGAL::Exact_predicates_inexact_constructions_kernel;

}  // namespace socialchoicelab::core
