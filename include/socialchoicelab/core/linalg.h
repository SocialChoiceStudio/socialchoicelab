// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <Eigen/Dense>

namespace socialchoicelab::core {

/**
 * @brief Linear algebra for the core (Eigen).
 *
 * core::linalg is satisfied by Eigen: vectors, matrices, norms, and other
 * operations used in distance, loss, and future indifference/geometry come from
 * Eigen. This header provides a single include and index type for code that
 * needs to refer to "core linear algebra."
 *
 * Use Eigen::MatrixBase<Derived> for generic vector/matrix parameters (e.g. in
 * distance APIs) so both fixed-size and dynamic-size vectors are accepted.
 */
namespace linalg {

/// Index type for dimensions and sizes (Eigen::Index, typically
/// std::ptrdiff_t).
using Index = Eigen::Index;

}  // namespace linalg

}  // namespace socialchoicelab::core
