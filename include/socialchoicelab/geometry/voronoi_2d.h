// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// voronoi_2d.h — Euclidean 2D Voronoi diagram (candidate regions).
//
// Computes the nearest-site Voronoi partition for 2D points under Euclidean
// (L2) distance. Each cell is clipped to an axis-aligned bounding box so that
// all output polygons are bounded. Generalisation to non-Euclidean metrics
// is planned (see ROADMAP.md).

#include <CGAL/Line_2.h>
#include <socialchoicelab/core/kernels.h>
#include <socialchoicelab/core/types.h>
#include <socialchoicelab/geometry/geom2d.h>

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace socialchoicelab::geometry {

using EK = socialchoicelab::core::EpecKernel;

// ---------------------------------------------------------------------------
// Helpers: bbox polygon, perpendicular bisector, polygon clip by half-plane
// ---------------------------------------------------------------------------

/// Axis-aligned rectangle as a CCW polygon (4 vertices).
[[nodiscard]] inline Polygon2E bbox_to_polygon(double min_x, double min_y,
                                               double max_x, double max_y) {
  const Point2E a(min_x, min_y);
  const Point2E b(max_x, min_y);
  const Point2E c(max_x, max_y);
  const Point2E d(min_x, max_y);
  Polygon2E p;
  p.push_back(a);
  p.push_back(b);
  p.push_back(c);
  p.push_back(d);
  return p;
}

/// Perpendicular bisector of segment (a, b) as an oriented line. The
/// "positive" side of the line is the half-plane containing @p a.
[[nodiscard]] inline Line2E perp_bisector_line(const Point2E& a,
                                               const Point2E& b) {
  const double ax = CGAL::to_double(a.x());
  const double ay = CGAL::to_double(a.y());
  const double bx = CGAL::to_double(b.x());
  const double by = CGAL::to_double(b.y());
  const double mx = (ax + bx) / 2.0;
  const double my = (ay + by) / 2.0;
  // Direction perpendicular to (b - a): (ay - by, bx - ax).
  const double dx = ay - by;
  const double dy = bx - ax;
  const Point2E m(mx, my);
  const Point2E m2(mx + dx, my + dy);
  return Line2E(m, m2);
}

/// Intersection of segment (s, t) with line L. Returns the point if the
/// segment crosses L at a single point with 0 <= u <= 1, otherwise nullopt.
[[nodiscard]] inline std::optional<Point2E> segment_line_intersection(
    const Point2E& s, const Point2E& t, const Line2E& L) {
  const Point2E m = L.point();
  const auto dir = L.direction();
  auto dx = dir.dx();
  auto dy = dir.dy();
  auto seg_dx = t.x() - s.x();
  auto seg_dy = t.y() - s.y();
  // denom = (t-s) × direction = seg_dx*dy - seg_dy*dx
  auto denom = seg_dx * dy - seg_dy * dx;
  if (denom == 0) return std::nullopt;  // parallel
  // num = (m-s) × direction
  auto num = (m.x() - s.x()) * dy - (m.y() - s.y()) * dx;
  auto u = num / denom;
  if (u < 0 || u > 1) return std::nullopt;
  return Point2E(s.x() + u * seg_dx, s.y() + u * seg_dy);
}

/// Clip polygon P by the half-plane containing point keep_side_pt (the side
/// of line L where keep_side_pt lies). Returns the intersection polygon.
[[nodiscard]] inline Polygon2E clip_polygon_by_halfplane(
    const Polygon2E& P, const Line2E& L, const Point2E& keep_side_pt) {
  const auto side = L.oriented_side(keep_side_pt);
  Polygon2E out;
  if (P.size() < 3) return out;
  for (std::size_t i = 0; i < P.size(); ++i) {
    const Point2E& v_curr = P[i];
    const Point2E& v_prev = P[i == 0 ? P.size() - 1 : i - 1];
    const auto side_curr = L.oriented_side(v_curr);
    const auto side_prev = L.oriented_side(v_prev);
    const bool in_curr =
        (side_curr == side || side_curr == CGAL::ON_ORIENTED_BOUNDARY);
    const bool in_prev =
        (side_prev == side || side_prev == CGAL::ON_ORIENTED_BOUNDARY);

    if (in_curr) {
      if (!in_prev) {
        auto pt = segment_line_intersection(v_prev, v_curr, L);
        if (pt) out.push_back(*pt);
      }
      out.push_back(v_curr);
    } else {
      if (in_prev) {
        auto pt = segment_line_intersection(v_prev, v_curr, L);
        if (pt) out.push_back(*pt);
      }
    }
  }
  return out;
}

// ---------------------------------------------------------------------------
// Voronoi cell for one site
// ---------------------------------------------------------------------------

/// Compute the Euclidean Voronoi cell of site @p site_idx: the set of points
/// closer to sites[site_idx] than to any other site, clipped to the bbox.
/// Returns an exact polygon (possibly empty or degenerate).
[[nodiscard]] inline Polygon2E voronoi_cell_2d(
    const std::vector<Point2E>& sites, std::size_t site_idx, double bbox_min_x,
    double bbox_min_y, double bbox_max_x, double bbox_max_y) {
  if (sites.empty() || site_idx >= sites.size()) return Polygon2E{};
  Polygon2E cell =
      bbox_to_polygon(bbox_min_x, bbox_min_y, bbox_max_x, bbox_max_y);
  const Point2E& ai = sites[site_idx];
  for (std::size_t j = 0; j < sites.size(); ++j) {
    if (j == site_idx) continue;
    const Point2E& aj = sites[j];
    Line2E bisector = perp_bisector_line(ai, aj);
    cell = clip_polygon_by_halfplane(cell, bisector, ai);
    if (cell.size() < 3) return Polygon2E{};
  }
  return cell;
}

// ---------------------------------------------------------------------------
// Voronoi cells for all sites (Euclidean, clipped to bbox)
// ---------------------------------------------------------------------------

/**
 * @brief Compute Euclidean Voronoi cells for 2D sites clipped to a bbox.
 *
 * Each cell is the set of points closer to that site than to any other,
 * intersected with the axis-aligned box [bbox_min_x, bbox_max_x] ×
 * [bbox_min_y, bbox_max_y]. Output is one polygon per site; empty or
 * degenerate cells are returned as empty vectors.
 *
 * @param sites_xy  Interleaved x,y coordinates (length 2 * n_sites).
 * @param n_sites   Number of sites.
 * @param bbox_min_x, bbox_min_y, bbox_max_x, bbox_max_y  Clipping box.
 * @return          Vector of polygons; result[i] is the cell for site i as
 *                  a list of (x,y) pairs (closed ring; no repeated first
 * point).
 * @throws std::invalid_argument if n_sites < 1 or bbox is invalid.
 */
[[nodiscard]] inline std::vector<std::vector<core::types::Point2d>>
voronoi_cells_2d(const double* sites_xy, int n_sites, double bbox_min_x,
                 double bbox_min_y, double bbox_max_x, double bbox_max_y) {
  if (n_sites < 1) {
    throw std::invalid_argument("voronoi_cells_2d: n_sites must be >= 1 (got " +
                                std::to_string(n_sites) + ").");
  }
  if (bbox_min_x >= bbox_max_x || bbox_min_y >= bbox_max_y) {
    throw std::invalid_argument(
        "voronoi_cells_2d: bbox must have min < max for both dimensions.");
  }

  std::vector<Point2E> sites;
  sites.reserve(static_cast<std::size_t>(n_sites));
  for (int i = 0; i < n_sites; ++i) {
    sites.push_back(Point2E(sites_xy[2 * i], sites_xy[2 * i + 1]));
  }

  std::vector<std::vector<core::types::Point2d>> result;
  result.reserve(static_cast<std::size_t>(n_sites));
  for (int i = 0; i < n_sites; ++i) {
    Polygon2E cell =
        voronoi_cell_2d(sites, static_cast<std::size_t>(i), bbox_min_x,
                        bbox_min_y, bbox_max_x, bbox_max_y);
    std::vector<core::types::Point2d> verts;
    if (cell.size() >= 3 && cell.is_simple()) {
      verts.reserve(cell.size());
      for (auto it = cell.vertices_begin(); it != cell.vertices_end(); ++it) {
        verts.push_back(to_numeric(*it));
      }
    }
    result.push_back(std::move(verts));
  }
  return result;
}

}  // namespace socialchoicelab::geometry
