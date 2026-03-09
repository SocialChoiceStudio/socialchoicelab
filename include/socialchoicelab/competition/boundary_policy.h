// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <socialchoicelab/competition/competition_types.h>
#include <socialchoicelab/competition/initialization.h>

namespace socialchoicelab::competition {

namespace detail {

[[nodiscard]] inline double reflect_coordinate(double value, double lower,
                                               double upper) {
  while (value < lower || value > upper) {
    if (value < lower) {
      value = lower + (lower - value);
    }
    if (value > upper) {
      value = upper - (value - upper);
    }
  }
  return value;
}

}  // namespace detail

[[nodiscard]] inline PointNd apply_boundary_policy(
    const PointNd& current_position, const PointNd& proposed_position,
    const CompetitionBounds& bounds, BoundaryPolicyKind boundary_policy) {
  validate_bounds(bounds);
  const int dimension = competition_dimension(bounds);
  detail::validate_point_dimension(current_position, dimension,
                                   "apply_boundary_policy");
  detail::validate_point_dimension(proposed_position, dimension,
                                   "apply_boundary_policy");

  switch (boundary_policy) {
    case BoundaryPolicyKind::kProjectToBox: {
      PointNd projected = proposed_position;
      for (int d = 0; d < dimension; ++d) {
        if (projected[d] < bounds.lower[d]) projected[d] = bounds.lower[d];
        if (projected[d] > bounds.upper[d]) projected[d] = bounds.upper[d];
      }
      return projected;
    }
    case BoundaryPolicyKind::kStayPut: {
      for (int d = 0; d < dimension; ++d) {
        if (proposed_position[d] < bounds.lower[d] ||
            proposed_position[d] > bounds.upper[d]) {
          return current_position;
        }
      }
      return proposed_position;
    }
    case BoundaryPolicyKind::kReflect: {
      PointNd reflected = proposed_position;
      for (int d = 0; d < dimension; ++d) {
        reflected[d] = detail::reflect_coordinate(reflected[d], bounds.lower[d],
                                                  bounds.upper[d]);
      }
      return reflected;
    }
  }
  return current_position;
}

}  // namespace socialchoicelab::competition
