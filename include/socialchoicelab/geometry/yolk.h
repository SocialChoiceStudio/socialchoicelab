// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// yolk.h — k-quantile lines and k-Yolk for 2D spatial voting models.
//
// Phase C deliverables:
//   C1 (this file): quantile_lines_2d — exact k-quantile lines.
//   C2 (future):    yolk_2d — smallest circle intersecting all k-quantile
//   lines. C3 (future):    yolk_radius — convenience wrapper.
//
// Background — k-quantile lines:
//   In a 2D voter configuration with ideal points p₁…pₙ, a k-quantile line in
//   direction e (a unit vector) is the line perpendicular to e at the k-th
//   smallest voter projection onto e.  Formally, if proj_i = pᵢ · e, sorted
//   so proj_(1) ≤ … ≤ proj_(n), the k-quantile line is {x : x·e = proj_(k)}.
//
//   Under simple majority (k = ⌊n/2⌋ + 1) these are the classical median
//   lines studied by Feld, Grofman & Miller (1988).
//
// Background — k-Yolk:
//   The k-Yolk is the smallest disk whose centre c minimises the maximum
//   distance from c to any k-quantile line across all directions e:
//
//       Yolk centre c* = argmin_c  max_{all directions e} d(c, L_e)
//       Yolk radius r* = max_{e} d(c*, L_e)
//
//   A zero radius means a k-majority Condorcet winner exists at c*.  A larger
//   radius indicates more agenda-setter power / electoral instability.
//
// Algorithm — C1 (this file):
//   For each unordered voter pair {i, j} the critical direction where pᵢ and
//   pⱼ project to the same value is e_crit ⊥ (pⱼ − pᵢ).  These n(n−1)/2
//   critical directions generate the full combinatorial structure of the
//   k-quantile line family.  At each critical direction the k-quantile line is
//   computed exactly using CGAL EPEC arithmetic.
//
//   The returned O(n²) set of lines is sufficient for Yolk computation via
//   C2 (minimax optimisation), as the Yolk is determined by the most
//   constraining critical-direction lines.
//
// References:
//   Feld, Grofman & Miller (1988), "Centripetal Forces in Spatial Voting:
//     The Yolk and the Grid," American Journal of Political Science 32(2).
//   Grofman, Owen & Noviello (1987), "Stability and Centrality of Legislative
//     Choice in the Spatial Context," American Political Science Review 81(2).

#include <CGAL/Line_2.h>
#include <socialchoicelab/core/kernels.h>
#include <socialchoicelab/geometry/geom2d.h>
#include <socialchoicelab/geometry/majority.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>

namespace socialchoicelab::geometry {

// ---------------------------------------------------------------------------
// C1 — k-quantile lines (exact)
// ---------------------------------------------------------------------------

/**
 * @brief Compute the k-quantile lines for a 2D voter configuration.
 *
 * For each unordered voter pair {i, j}, computes the k-quantile line at the
 * critical direction perpendicular to (p_j − p_i).  At that direction voters
 * i and j project to identical values; sorting all projections and taking the
 * k-th defines the k-quantile line exactly.
 *
 * Under simple majority (k = ⌊n/2⌋ + 1) these are the classical median lines.
 * Under unanimity (k = n) the line separates all voters from the empty set,
 * so every k-quantile line passes beyond all voters.
 *
 * The returned vector has n(n−1)/2 entries (one per voter pair).  Duplicate
 * lines may occur for symmetric or collinear configurations; callers that
 * need a deduplicated set should filter by comparing coefficients.
 *
 * All computation uses CGAL EPEC exact arithmetic: results are never wrong
 * due to floating-point rounding.
 *
 * @param voter_ideals  At least 2 voter ideal points in 2D.
 * @param k             Majority threshold; -1 = ⌊n/2⌋ + 1 (simple majority).
 * @return              k-quantile lines as exact CGAL Line_2 objects.
 * @throws std::invalid_argument if n < 2 or k ∉ [1, n].
 */
[[nodiscard]] inline std::vector<Line2E> quantile_lines_2d(
    const std::vector<core::types::Point2d>& voter_ideals, int k = -1) {
  const int n = static_cast<int>(voter_ideals.size());
  if (n < 2) {
    throw std::invalid_argument(
        "quantile_lines_2d: need at least 2 voter ideals (got " +
        std::to_string(n) + ").");
  }
  const int threshold = detail::resolve_k(k, n);

  // Lift voter ideals to exact CGAL points.
  std::vector<Point2E> pts;
  pts.reserve(static_cast<std::size_t>(n));
  for (const auto& v : voter_ideals) pts.push_back(to_exact(v));

  using FT = core::EpecKernel::FT;

  std::vector<Line2E> lines;
  lines.reserve(static_cast<std::size_t>(n * (n - 1) / 2));

  for (int i = 0; i < n; ++i) {
    for (int j = i + 1; j < n; ++j) {
      // Direction vector from p_i to p_j: dv = (dx, dy).
      // The critical direction e ⊥ dv is e = (−dy, dx).
      // In direction e, voter projections are: p · e = −dy·p.x + dx·p.y.
      FT dx = pts[static_cast<std::size_t>(j)].x() -
              pts[static_cast<std::size_t>(i)].x();
      FT dy = pts[static_cast<std::size_t>(j)].y() -
              pts[static_cast<std::size_t>(i)].y();

      // Normal direction of the k-quantile line: (ndx, ndy) = (−dy, dx).
      FT ndx = -dy;
      FT ndy = dx;

      // Compute projections of all voters onto the normal direction.
      std::vector<FT> projs;
      projs.reserve(static_cast<std::size_t>(n));
      for (const auto& p : pts) {
        projs.push_back(ndx * p.x() + ndy * p.y());
      }
      std::sort(projs.begin(), projs.end());

      // k-quantile value: k-th smallest projection (1-indexed threshold).
      FT pk = projs[static_cast<std::size_t>(threshold - 1)];

      // k-quantile line: ndx·x + ndy·y = pk, i.e. ndx·x + ndy·y − pk = 0.
      lines.emplace_back(ndx, ndy, -pk);
    }
  }

  return lines;
}

// ---------------------------------------------------------------------------
// C1b — Annotated k-quantile lines (with metadata and limiting flag)
// ---------------------------------------------------------------------------

/**
 * @brief Metadata-carrying k-quantile line.
 *
 * Each entry records the exact line, the voter-pair indices that defined
 * the critical direction, and whether the line is "limiting" (passes
 * through both voters of the pair).
 *
 * A limiting median line is one where the defining pair's common projection
 * equals the k-th sorted projection value.  At such a direction the line
 * passes through two (or more) ideal points.  Non-limiting lines at a
 * critical direction pass through exactly one ideal point — whichever
 * voter provides the k-th projection.
 *
 * Reference:
 *   Miller, N. R. (2015), "The Spatial Model of Social Choice and Voting,"
 *     in Handbook of Social Choice and Voting, pp. 163–181.
 */
struct QuantileLine2d {
  Line2E line;       ///< Exact CGAL line (ax + by + c = 0).
  int voter_i;       ///< Index of first voter of the defining pair (0-based).
  int voter_j;       ///< Index of second voter of the defining pair (0-based).
  bool is_limiting;  ///< True iff the pair's projection is the k-th value.
};

/**
 * @brief Compute annotated k-quantile lines for a 2D voter configuration.
 *
 * Identical algorithm to quantile_lines_2d, but each returned entry carries
 * the voter-pair indices and a flag indicating whether the line is limiting.
 *
 * @param voter_ideals  At least 2 voter ideal points in 2D.
 * @param k             Majority threshold; -1 = ⌊n/2⌋ + 1 (simple majority).
 * @return              Annotated k-quantile lines (n(n−1)/2 entries).
 * @throws std::invalid_argument if n < 2 or k ∉ [1, n].
 */
[[nodiscard]] inline std::vector<QuantileLine2d> quantile_lines_2d_annotated(
    const std::vector<core::types::Point2d>& voter_ideals, int k = -1) {
  const int n = static_cast<int>(voter_ideals.size());
  if (n < 2) {
    throw std::invalid_argument(
        "quantile_lines_2d_annotated: need at least 2 voter ideals (got " +
        std::to_string(n) + ").");
  }
  const int threshold = detail::resolve_k(k, n);

  std::vector<Point2E> pts;
  pts.reserve(static_cast<std::size_t>(n));
  for (const auto& v : voter_ideals) pts.push_back(to_exact(v));

  using FT = core::EpecKernel::FT;

  std::vector<QuantileLine2d> result;
  result.reserve(static_cast<std::size_t>(n * (n - 1) / 2));

  for (int i = 0; i < n; ++i) {
    for (int j = i + 1; j < n; ++j) {
      FT dx = pts[static_cast<std::size_t>(j)].x() -
              pts[static_cast<std::size_t>(i)].x();
      FT dy = pts[static_cast<std::size_t>(j)].y() -
              pts[static_cast<std::size_t>(i)].y();

      FT ndx = -dy;
      FT ndy = dx;

      std::vector<FT> projs;
      projs.reserve(static_cast<std::size_t>(n));
      for (const auto& p : pts) {
        projs.push_back(ndx * p.x() + ndy * p.y());
      }
      std::sort(projs.begin(), projs.end());

      FT pk = projs[static_cast<std::size_t>(threshold - 1)];
      FT pair_proj = ndx * pts[static_cast<std::size_t>(i)].x() +
                     ndy * pts[static_cast<std::size_t>(i)].y();

      result.push_back({Line2E(ndx, ndy, -pk), i, j, pair_proj == pk});
    }
  }

  return result;
}

/**
 * @brief Return only the limiting k-quantile lines.
 *
 * Limiting median lines pass through two (or more) voter ideal points.
 * They are the lines where the defining pair's common projection is the
 * k-th sorted value.
 *
 * For n odd under simple majority, these form the finite set of boundary
 * lines that Miller (2015) calls "limiting median lines."  Non-limiting
 * median lines are sandwiched between pairs of limiting lines that pass
 * through each ideal point.
 *
 * @param voter_ideals  At least 2 voter ideal points in 2D.
 * @param k             Majority threshold; -1 = ⌊n/2⌋ + 1 (simple majority).
 * @return              Only the limiting lines (subset of n(n−1)/2).
 * @throws std::invalid_argument if n < 2 or k ∉ [1, n].
 */
[[nodiscard]] inline std::vector<Line2E> limiting_quantile_lines_2d(
    const std::vector<core::types::Point2d>& voter_ideals, int k = -1) {
  auto annotated = quantile_lines_2d_annotated(voter_ideals, k);
  std::vector<Line2E> result;
  result.reserve(annotated.size());
  for (const auto& ql : annotated) {
    if (ql.is_limiting) result.push_back(ql.line);
  }
  return result;
}

// ---------------------------------------------------------------------------
// C1 — helpers
// ---------------------------------------------------------------------------

/**
 * @brief Compute the signed distance from a point to a line.
 *
 * The signed distance from point p = (px, py) to line ax + by + c = 0 is
 *   (a·px + b·py + c) / sqrt(a² + b²)
 * and its absolute value is the Euclidean distance.
 *
 * @param p    Query point (numeric double coordinates).
 * @param line Exact CGAL line.
 * @return     Signed distance (positive on the left side of the line's
 *             orientation, negative on the right).
 */
[[nodiscard]] inline double signed_distance_to_line(
    const core::types::Point2d& p, const Line2E& line) {
  double a = CGAL::to_double(line.a());
  double b = CGAL::to_double(line.b());
  double c = CGAL::to_double(line.c());
  double denom = std::sqrt(a * a + b * b);
  if (denom < 1e-300) return 0.0;  // degenerate zero-length line
  return (a * p.x() + b * p.y() + c) / denom;
}

/**
 * @brief Compute the (unsigned) Euclidean distance from a point to a line.
 */
[[nodiscard]] inline double distance_to_line(const core::types::Point2d& p,
                                             const Line2E& line) {
  return std::abs(signed_distance_to_line(p, line));
}

// ---------------------------------------------------------------------------
// C2 — k-Yolk (smallest enclosing circle of the k-quantile line family)
// ---------------------------------------------------------------------------

/**
 * @brief Result of a k-Yolk computation.
 */
struct Yolk2d {
  core::types::Point2d center;  ///< Yolk centre — minimax point.
  double radius;                ///< Yolk radius (≥ 0). Zero iff a k-majority
                                ///< Condorcet winner exists at @p center.
};

/**
 * @brief Compute the k-Yolk for a 2D voter configuration.
 *
 * The k-Yolk is the smallest closed disk centred at c* that intersects every
 * k-quantile line in every direction:
 *
 *   c* = argmin_c  max_{θ ∈ [0,π)}  d(c, L(θ, k))
 *   r* = max_{θ}   d(c*, L(θ, k))
 *
 * Algorithm: generate k-quantile lines at @p n_sample uniformly-spaced
 * directions plus the n(n−1)/2 critical directions (one per voter pair), then
 * solve the resulting finite minimax problem via subgradient descent with
 * best-iterate tracking.
 *
 * Accuracy: error in radius is O(π/n_sample) from directional discretisation
 * and O(1/sqrt(n_iter)) from the optimiser.  Default parameters give radius
 * accuracy ≈ 0.005 × spread(voters) for typical configurations.
 *
 * References:
 *   Feld, Grofman & Miller (1988), "Centripetal Forces in Spatial Voting:
 *     The Yolk and the Grid," American Journal of Political Science 32(2),
 *     pp. 459–481.
 *
 * @param voter_ideals  At least 2 voter ideal points in 2D.
 * @param k             Majority threshold; -1 = ⌊n/2⌋+1 (simple majority).
 * @param n_sample      Number of uniformly-spaced directions (default 720).
 * @return              Yolk2d with centre and radius.
 * @throws std::invalid_argument if n < 2 or k ∉ [1, n].
 */
[[nodiscard]] inline Yolk2d yolk_2d(
    const std::vector<core::types::Point2d>& voter_ideals, int k = -1,
    int n_sample = 720) {
  const int n = static_cast<int>(voter_ideals.size());
  if (n < 2) {
    throw std::invalid_argument("yolk_2d: need at least 2 voter ideals (got " +
                                std::to_string(n) + ").");
  }
  const int threshold = detail::resolve_k(k, n);

  // -------------------------------------------------------------------------
  // Build normalised sample lines: a*x + b*y + c = 0, (a,b) unit normal.
  // Distance from point (px, py) to line = |a*px + b*py + c|.
  // -------------------------------------------------------------------------
  struct SampledLine {
    double a, b, c;
    [[nodiscard]] double dist(double px, double py) const {
      return std::abs(a * px + b * py + c);
    }
  };

  // Compute the k-quantile line for a given unit direction (cos_t, sin_t).
  auto make_line = [&](double cos_t, double sin_t) -> SampledLine {
    std::vector<double> projs;
    projs.reserve(static_cast<std::size_t>(n));
    for (const auto& v : voter_ideals) {
      projs.push_back(cos_t * v.x() + sin_t * v.y());
    }
    std::sort(projs.begin(), projs.end());
    double pk = projs[static_cast<std::size_t>(threshold - 1)];
    return {cos_t, sin_t, -pk};
  };

  std::vector<SampledLine> lines;
  lines.reserve(static_cast<std::size_t>(n_sample + n * (n - 1) / 2));

  // Uniform directional sample over [0, π).
  for (int i = 0; i < n_sample; ++i) {
    double theta = i * std::numbers::pi / n_sample;
    lines.push_back(make_line(std::cos(theta), std::sin(theta)));
  }

  // Critical directions: one per voter pair, already included via CGAL in C1
  // but recomputed here in double for the minimax solver.
  for (int i = 0; i < n; ++i) {
    for (int j = i + 1; j < n; ++j) {
      double dx = voter_ideals[static_cast<std::size_t>(j)].x() -
                  voter_ideals[static_cast<std::size_t>(i)].x();
      double dy = voter_ideals[static_cast<std::size_t>(j)].y() -
                  voter_ideals[static_cast<std::size_t>(i)].y();
      double len = std::hypot(dx, dy);
      if (len < 1e-12) continue;
      // Critical direction e ⊥ (dx, dy): normalise (−dy, dx).
      lines.push_back(make_line(-dy / len, dx / len));
    }
  }

  // -------------------------------------------------------------------------
  // Estimate voter spread for step-size scaling.
  // -------------------------------------------------------------------------
  double spread = 0.0;
  for (int i = 0; i < n; ++i) {
    for (int j = i + 1; j < n; ++j) {
      spread = std::max(
          spread,
          std::hypot(voter_ideals[static_cast<std::size_t>(i)].x() -
                         voter_ideals[static_cast<std::size_t>(j)].x(),
                     voter_ideals[static_cast<std::size_t>(i)].y() -
                         voter_ideals[static_cast<std::size_t>(j)].y()));
    }
  }
  if (spread < 1e-14) {
    // All voters collocated — trivial Yolk.
    return Yolk2d{voter_ideals[0], 0.0};
  }

  // -------------------------------------------------------------------------
  // Minimax solver: subgradient descent with best-iterate tracking.
  //
  // Objective: f(c) = max_i |a_i * cx + b_i * cy + c_i|  (all lines normalised)
  // Subgradient at c: g = sign(a_{i*} * cx + b_{i*} * cy + c_{i*}) * (a_{i*},
  // b_{i*})
  //   where i* = argmax_i |a_i * cx + b_i * cy + c_i|.
  // Step rule: Polyak-style diminishing step (f(c) - best_r + ε) / (iter + 1).
  // -------------------------------------------------------------------------

  // Initialise at voter centroid.
  double cx = 0.0, cy = 0.0;
  for (const auto& v : voter_ideals) {
    cx += v.x();
    cy += v.y();
  }
  cx /= n;
  cy /= n;

  double best_r = std::numeric_limits<double>::max();
  double best_cx = cx, best_cy = cy;
  const double alpha = spread * 0.6;
  const int n_iter = 10000;

  for (int iter = 1; iter <= n_iter; ++iter) {
    // Find the most distant (most violated) line.
    double max_d = 0.0;
    std::size_t max_idx = 0;
    double max_sign = 1.0;
    for (std::size_t li = 0; li < lines.size(); ++li) {
      double sd = lines[li].a * cx + lines[li].b * cy + lines[li].c;
      double d = std::abs(sd);
      if (d > max_d) {
        max_d = d;
        max_idx = li;
        max_sign = (sd >= 0.0) ? 1.0 : -1.0;
      }
    }

    // Track best iterate.
    if (max_d < best_r) {
      best_r = max_d;
      best_cx = cx;
      best_cy = cy;
    }

    // Polyak-like diminishing step: (f − best) / (iter + 1) with min floor.
    double gap = max_d - best_r + 1e-10 * spread;
    double step = alpha * gap / static_cast<double>(iter + 1);

    // Move toward the most violated line.
    cx -= step * max_sign * lines[max_idx].a;
    cy -= step * max_sign * lines[max_idx].b;
  }

  return Yolk2d{core::types::Point2d(best_cx, best_cy), std::max(0.0, best_r)};
}

// ---------------------------------------------------------------------------
// C3 — Yolk radius convenience function
// ---------------------------------------------------------------------------

/**
 * @brief Compute the k-Yolk radius for a 2D voter configuration.
 *
 * The Yolk radius measures electoral instability under the k-majority rule:
 * - Radius = 0: a k-majority Condorcet winner exists (rare in 2D, requires
 *               radial symmetry — Plott 1967 conditions).
 * - Larger radius: more agenda-setter power; the Yolk radius bounds the
 *                  expected cycling distance (McKelvey 1979).
 * - Supermajority k > ⌊n/2⌋+1 generally shifts and resizes the Yolk.
 *
 * References:
 *   Plott (1967), "A Notion of Equilibrium and Its Possibility Under Majority
 *     Rule," American Economic Review 57(4), pp. 787–806.
 *   McKelvey (1979), "General Conditions for Global Intransitivities in Formal
 *     Voting Models," Econometrica 47(5), pp. 1085–1112.
 *   Feld, Grofman & Miller (1988), "Centripetal Forces in Spatial Voting:
 *     The Yolk and the Grid," American Journal of Political Science 32(2).
 *
 * @param voter_ideals  At least 2 voter ideal points in 2D.
 * @param k             Majority threshold; -1 = ⌊n/2⌋+1 (simple majority).
 * @return              Yolk radius (≥ 0).
 * @throws std::invalid_argument if n < 2 or k ∉ [1, n].
 */
[[nodiscard]] inline double yolk_radius(
    const std::vector<core::types::Point2d>& voter_ideals, int k = -1) {
  return yolk_2d(voter_ideals, k).radius;
}

}  // namespace socialchoicelab::geometry
