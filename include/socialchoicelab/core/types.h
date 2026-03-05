// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <Eigen/Dense>

namespace socialchoicelab::core {

/**
 * @brief Canonical point and vector types for the core and preference/geometry
 * layers.
 *
 * Use these types for ideal points, alternative points, level-set points, and
 * any N-dimensional point or vector in the policy space. They are backed by
 * Eigen; when Layer 3 (exact geometry) is added, CGAL types can be introduced
 * alongside or converted from these where needed.
 */
namespace types {

/// Fixed-dimension 2D point/vector (double). Use for 2D spatial models.
using Point2d = Eigen::Vector2d;

/// Fixed-dimension 3D point/vector (double). Use for 3D spatial models.
using Point3d = Eigen::Vector3d;

/// Dynamic-dimension point/vector (double). Use for 1D, N-D, or when dimension
/// is runtime.
using PointNd = Eigen::VectorXd;

}  // namespace types

}  // namespace socialchoicelab::core
