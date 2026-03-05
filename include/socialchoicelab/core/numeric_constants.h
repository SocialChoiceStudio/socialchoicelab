// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace socialchoicelab::core {

/**
 * @brief Single source of truth for "negligible" in floating-point comparisons
 *
 * Use these constants whenever code must treat two floats as "equal for
 * practical purposes" (e.g. degenerate range, zero denominator). Scale is
 * computed locally at the call site from the values being compared (e.g.
 * max(|a|, |b|) or max(|max_utility|, |min_utility|, 1)); do not use a global
 * "space size." See docs/development/development.md § Numeric tolerance.
 *
 * Convention (PEP 485 / math.isclose style): treat a and b as equal when
 *   |a - b| <= max(k_near_zero_rel * scale, k_near_zero_abs)
 * where scale = max(|a|, |b|) or similar, and at least 1 to avoid division by
 * zero when both are zero.
 */
namespace near_zero {

/// Relative tolerance: negligible = this fraction of the local scale.
constexpr double k_near_zero_rel = 1e-9;

/// Absolute tolerance: fallback when scale is tiny (e.g. values near zero).
constexpr double k_near_zero_abs = 1e-12;

}  // namespace near_zero

}  // namespace socialchoicelab::core
