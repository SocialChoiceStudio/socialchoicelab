// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// winset.h — 2D winset computation.
//
// Provides two implementations:
//
//   winset_2d(sq, voter_ideals, cfg, k, num_samples)
//     Production function.  Incremental depth-update algorithm: O(nk) polygon
//     boolean operations, where n is the number of voters and k is the majority
//     threshold.  Scales to large n (tested with n up to several hundred).
//
//   winset_2d_petal(sq, voter_ideals, cfg, k, num_samples, max_voters)
//     Reference / verification function.  Explicit petal method: enumerates
//     all C(n,k) minimal winning coalitions, intersects k preferred-to
//     polygons per coalition (one petal), then unions all non-empty petals.
//     Only feasible for small n (default guard: n ≤ 15).  Use to cross-check
//     winset_2d on small test cases.
//
// Algorithm details — incremental depth update:
//   Maintain sets S[0]…S[k-1] where S[j] is the region where at least j+1
//   preferred-to sets overlap.  For each voter i (PTS polygon Dᵢ):
//     for j = k-1 down to 1:  S[j] = S[j] ∪ (S[j-1] ∩ Dᵢ)
//     S[0] = S[0] ∪ Dᵢ
//   After all voters: winset = S[k-1].
//   Complexity: O(nk) polygon boolean ops, each O(m log m) for m-vertex polys.
//   Total: O(nkm log m) ≈ O(n²m log m) for k = n/2.
//
// Polygon representation:
//   Preferred-to sets are approximated as convex polygons (true circles /
//   Lp balls sampled at num_samples points).  All preferred-to sets are convex
//   for any Minkowski p ≥ 1.  The winset boundary is approximate but the
//   boolean operations are performed with CGAL exact arithmetic, so there are
//   no robustness failures.  Increase num_samples to improve boundary accuracy.
//
// Scalability note:
//   winset_2d scales to large n but performance degrades for very large n × k.
//   For n = 435 (Congress), k = 218: O(94,830) polygon ops — feasible.
//   Benchmark and revisit when optimisation is needed.
//
// References:
//   Plott (1967), "A Notion of Equilibrium and Its Possibility Under Majority
//   Rule", American Economic Review.
//   McKelvey (1979), "General Conditions for Global Intransitivities in Formal
//   Voting Models", Econometrica.
//   CGAL, 2D Boolean Set-Operations package.

#include <CGAL/Boolean_set_operations_2.h>
#include <CGAL/General_polygon_set_2.h>
#include <CGAL/Gps_segment_traits_2.h>
#include <socialchoicelab/core/kernels.h>
#include <socialchoicelab/geometry/geom2d.h>
#include <socialchoicelab/geometry/majority.h>
#include <socialchoicelab/preference/indifference/level_set.h>

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace socialchoicelab::geometry {

// ---------------------------------------------------------------------------
// CGAL types for winset polygon sets
// ---------------------------------------------------------------------------

// Short kernel alias following CGAL's own tutorial convention (see CGAL §2D
// Boolean Set-Operations, "Kernel" section): defining `using K = Kernel;`
// keeps all downstream CGAL template instantiations on a single line, which
// avoids the cpplint whitespace/indent_namespace false-positive that fires on
// indented line-continuation inside a namespace block.
using EK = socialchoicelab::core::EpecKernel;

using WinsetTraits = CGAL::Gps_segment_traits_2<EK>;

/// A region of 2D policy space represented as a possibly non-convex,
/// possibly disconnected union of polygons (with holes).  This is the
/// return type of winset_2d and winset_2d_petal.
using WinsetRegion = CGAL::General_polygon_set_2<WinsetTraits>;

using PolygonWithHoles2E = CGAL::Polygon_with_holes_2<EK>;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace detail {

namespace indiff = socialchoicelab::preference::indifference;
namespace dist_ = socialchoicelab::preference::distance;

/// Convert a floating-point Polygon2d to a CGAL exact Polygon2E.
/// The polygon must be simple; CCW orientation is enforced.
inline Polygon2E to_exact_polygon(const indiff::Polygon2d& poly) {
  if (poly.vertices.size() < 3) return Polygon2E{};
  Polygon2E p;
  for (const auto& v : poly.vertices) p.push_back(to_exact(v));
  if (p.is_clockwise_oriented()) p.reverse_orientation();
  return p;
}

/// Compute the preferred-to-set boundary polygon for voter with ideal point p
/// and status quo sq under distance config cfg, sampled at num_samples points.
/// Returns an empty polygon if the voter is at the SQ (distance ≈ 0).
inline Polygon2E pts_polygon(const core::types::Point2d& voter_ideal,
                             const core::types::Point2d& sq,
                             const DistConfig& cfg, int num_samples) {
  const double r = dist_::calculate_distance(voter_ideal, sq, cfg.type,
                                             cfg.order_p, cfg.salience_weights);
  if (r < 1e-12) return Polygon2E{};  // voter at SQ — empty PTS

  const double w0 = cfg.salience_weights[0];
  const double w1 = cfg.salience_weights[1];
  const double a0 = r / w0;
  const double a1 = r / w1;

  // Build the level-set shape and sample it.
  using indiff::Circle2d;
  using indiff::Ellipse2d;
  using indiff::LevelSet2d;
  using indiff::Polygon2d;
  using indiff::Superellipse2d;

  LevelSet2d ls;
  switch (cfg.type) {
    case dist_::DistanceType::EUCLIDEAN:
      if (std::abs(w0 - w1) < 1e-10 * std::max(w0, w1)) {
        ls = Circle2d{voter_ideal, r};
      } else {
        ls = Ellipse2d{voter_ideal, a0, a1};
      }
      break;
    case dist_::DistanceType::MANHATTAN: {
      Polygon2d poly;
      poly.vertices = {voter_ideal + core::types::Point2d(a0, 0),
                       voter_ideal + core::types::Point2d(0, a1),
                       voter_ideal + core::types::Point2d(-a0, 0),
                       voter_ideal + core::types::Point2d(0, -a1)};
      ls = poly;
      break;
    }
    case dist_::DistanceType::CHEBYSHEV: {
      Polygon2d poly;
      poly.vertices = {voter_ideal + core::types::Point2d(a0, a1),
                       voter_ideal + core::types::Point2d(-a0, a1),
                       voter_ideal + core::types::Point2d(-a0, -a1),
                       voter_ideal + core::types::Point2d(a0, -a1)};
      ls = poly;
      break;
    }
    case dist_::DistanceType::MINKOWSKI: {
      const double p = cfg.order_p;
      if (p <= 1.0 + 1e-9) {
        Polygon2d poly;
        poly.vertices = {voter_ideal + core::types::Point2d(a0, 0),
                         voter_ideal + core::types::Point2d(0, a1),
                         voter_ideal + core::types::Point2d(-a0, 0),
                         voter_ideal + core::types::Point2d(0, -a1)};
        ls = poly;
      } else if (p >= 100.0) {
        Polygon2d poly;
        poly.vertices = {voter_ideal + core::types::Point2d(a0, a1),
                         voter_ideal + core::types::Point2d(-a0, a1),
                         voter_ideal + core::types::Point2d(-a0, -a1),
                         voter_ideal + core::types::Point2d(a0, -a1)};
        ls = poly;
      } else {
        ls = Superellipse2d{voter_ideal, a0, a1, p};
      }
      break;
    }
    default:
      throw std::invalid_argument("pts_polygon: unsupported DistanceType");
  }

  return to_exact_polygon(
      indiff::to_polygon(ls, static_cast<std::size_t>(num_samples)));
}

/// Insert p into gps if p is a valid (simple, non-degenerate) polygon.
inline void gps_insert_if_valid(WinsetRegion& gps, const Polygon2E& p) {
  if (p.size() < 3 || !p.is_simple()) return;
  gps.insert(p);
}

}  // namespace detail

// ---------------------------------------------------------------------------
// Helpers for querying WinsetRegion
// ---------------------------------------------------------------------------

/// Returns true if the winset region is empty (i.e. a Condorcet winner exists
/// for the given SQ and k-majority threshold).
[[nodiscard]] inline bool winset_is_empty(const WinsetRegion& ws) {
  return ws.is_empty();
}

/// Returns true if point p lies strictly inside the winset region.
[[nodiscard]] inline bool winset_contains(const WinsetRegion& ws,
                                          const core::types::Point2d& p) {
  if (ws.is_empty()) return false;
  const Point2E ep = to_exact(p);
  // Collect polygons-with-holes and test bounded_side on each.
  std::vector<PolygonWithHoles2E> pwhs;
  ws.polygons_with_holes(std::back_inserter(pwhs));
  for (const auto& pwh : pwhs) {
    // Check outer boundary.
    const auto side =
        CGAL::bounded_side_2(pwh.outer_boundary().vertices_begin(),
                             pwh.outer_boundary().vertices_end(), ep,
                             socialchoicelab::core::EpecKernel{});
    if (side == CGAL::ON_BOUNDED_SIDE) {
      // Also check holes: point must not be inside a hole.
      bool in_hole = false;
      for (auto h = pwh.holes_begin(); h != pwh.holes_end(); ++h) {
        if (CGAL::bounded_side_2(h->vertices_begin(), h->vertices_end(), ep,
                                 socialchoicelab::core::EpecKernel{}) ==
            CGAL::ON_BOUNDED_SIDE) {
          in_hole = true;
          break;
        }
      }
      if (!in_hole) return true;
    }
  }
  return false;
}

// ---------------------------------------------------------------------------
// B3 — Production: incremental depth-update algorithm
// ---------------------------------------------------------------------------

/**
 * @brief Compute the 2D k-majority winset of a status quo.
 *
 * Returns the region of policy space where at least k voters prefer the
 * alternative to the status quo under the given distance metric.
 *
 * Algorithm: incremental depth update — O(nk) polygon boolean operations.
 * Scales to large n (e.g. n = 435 for Congress).
 *
 * @param sq           Status quo point.
 * @param voter_ideals Voter ideal points (n ≥ 1).
 * @param cfg          Distance configuration.
 * @param k            Majority threshold. -1 = ⌊n/2⌋ + 1 (simple majority).
 * @param num_samples  Polygon approximation quality for curved boundaries
 *                     (circle/superellipse).  Default 64.  Increase for higher
 *                     accuracy at the cost of speed.
 * @return WinsetRegion — possibly non-convex, possibly disconnected polygon
 *         region. Empty iff a k-majority Condorcet winner exists at sq.
 * @throws std::invalid_argument if inputs are invalid.
 */
[[nodiscard]] inline WinsetRegion winset_2d(
    const core::types::Point2d& sq,
    const std::vector<core::types::Point2d>& voter_ideals,
    const DistConfig& cfg, int k = -1, int num_samples = 64) {
  const int n = static_cast<int>(voter_ideals.size());
  if (n == 0) {
    throw std::invalid_argument("winset_2d: voter_ideals must not be empty.");
  }
  if (num_samples < 4) {
    throw std::invalid_argument("winset_2d: num_samples must be >= 4 (got " +
                                std::to_string(num_samples) + ").");
  }
  const int threshold = detail::resolve_k(k, n);

  // S[j] = polygon set for "depth >= j+1".
  // S[0] = depth >= 1, ..., S[threshold-1] = depth >= threshold = winset.
  std::vector<WinsetRegion> S(static_cast<std::size_t>(threshold));

  for (int i = 0; i < n; ++i) {
    Polygon2E Di = detail::pts_polygon(voter_ideals[i], sq, cfg, num_samples);
    if (Di.size() < 3) continue;  // voter at SQ or degenerate — skip

    WinsetRegion gps_Di;
    detail::gps_insert_if_valid(gps_Di, Di);
    if (gps_Di.is_empty()) continue;

    // Update from high j to low j so we use old S[j-1] values.
    for (int j = threshold - 1; j >= 1; --j) {
      WinsetRegion temp = S[static_cast<std::size_t>(j - 1)];
      temp.intersection(gps_Di);
      S[static_cast<std::size_t>(j)].join(temp);
    }
    S[0].join(gps_Di);
  }

  return S[static_cast<std::size_t>(threshold - 1)];
}

// ---------------------------------------------------------------------------
// B3 — Reference: petal method (small n only)
// ---------------------------------------------------------------------------

/**
 * @brief Compute the 2D k-majority winset using the explicit petal method.
 *
 * Reference implementation for verifying winset_2d on small inputs.
 * Enumerates all C(n,k) minimal winning coalitions, intersects k preferred-to
 * polygons per coalition (one convex petal), then unions all non-empty petals.
 *
 * Complexity: O(C(n,k) · k · m) where m = num_samples.  Infeasible for large
 * n: C(20,11) ≈ 168k, C(50,26) ≈ 10¹⁴.  Guarded by max_voters.
 *
 * @param sq           Status quo point.
 * @param voter_ideals Voter ideal points (n ≤ max_voters).
 * @param cfg          Distance configuration.
 * @param k            Majority threshold. -1 = ⌊n/2⌋ + 1.
 * @param num_samples  Polygon approximation quality. Default 64.
 * @param max_voters   Maximum n before throwing. Default 15.
 * @return WinsetRegion — union of all non-empty petals.
 * @throws std::invalid_argument if n > max_voters or other inputs invalid.
 */
[[nodiscard]] inline WinsetRegion winset_2d_petal(
    const core::types::Point2d& sq,
    const std::vector<core::types::Point2d>& voter_ideals,
    const DistConfig& cfg, int k = -1, int num_samples = 64,
    int max_voters = 15) {
  const int n = static_cast<int>(voter_ideals.size());
  if (n == 0) {
    throw std::invalid_argument(
        "winset_2d_petal: voter_ideals must not be empty.");
  }
  if (n > max_voters) {
    throw std::invalid_argument(
        "winset_2d_petal: n=" + std::to_string(n) + " exceeds max_voters=" +
        std::to_string(max_voters) + ". Use winset_2d for large inputs.");
  }
  if (num_samples < 4) {
    throw std::invalid_argument(
        "winset_2d_petal: num_samples must be >= 4 (got " +
        std::to_string(num_samples) + ").");
  }
  const int threshold = detail::resolve_k(k, n);

  // Pre-compute all n PTS polygons.
  std::vector<Polygon2E> pts(static_cast<std::size_t>(n));
  std::vector<bool> valid(static_cast<std::size_t>(n), false);
  for (int i = 0; i < n; ++i) {
    pts[static_cast<std::size_t>(i)] =
        detail::pts_polygon(voter_ideals[i], sq, cfg, num_samples);
    if (pts[static_cast<std::size_t>(i)].size() >= 3 &&
        pts[static_cast<std::size_t>(i)].is_simple()) {
      valid[static_cast<std::size_t>(i)] = true;
    }
  }

  // Enumerate all C(n, threshold) coalitions via bitmask.
  WinsetRegion winset;
  std::vector<int> coalition(static_cast<std::size_t>(threshold));
  std::iota(coalition.begin(), coalition.end(), 0);

  // Iterate through all combinations using next_combination logic.
  while (true) {
    // Compute the petal for this coalition: intersection of k PTS polygons.
    WinsetRegion petal;
    bool first = true;
    bool skip = false;

    for (int idx : coalition) {
      if (!valid[static_cast<std::size_t>(idx)]) {
        skip = true;
        break;
      }
      if (first) {
        detail::gps_insert_if_valid(petal, pts[static_cast<std::size_t>(idx)]);
        first = false;
      } else {
        WinsetRegion other;
        detail::gps_insert_if_valid(other, pts[static_cast<std::size_t>(idx)]);
        petal.intersection(other);
        if (petal.is_empty()) {
          skip = true;
          break;
        }
      }
    }

    if (!skip && !petal.is_empty()) {
      winset.join(petal);
    }

    // Advance to the next combination (Gosper's hack / std::next_permutation
    // equivalent for combinations).
    int i = threshold - 1;
    while (i >= 0 &&
           coalition[static_cast<std::size_t>(i)] == n - threshold + i) {
      --i;
    }
    if (i < 0) break;  // all combinations exhausted
    ++coalition[static_cast<std::size_t>(i)];
    for (int j = i + 1; j < threshold; ++j) {
      coalition[static_cast<std::size_t>(j)] =
          coalition[static_cast<std::size_t>(j - 1)] + 1;
    }
  }

  return winset;
}

}  // namespace socialchoicelab::geometry
