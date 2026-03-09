// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <socialchoicelab/competition/competitor.h>

#include <cmath>
#include <sstream>
#include <string>
#include <vector>

namespace socialchoicelab::competition {

[[nodiscard]] inline std::string state_signature(
    const std::vector<CompetitorState>& competitors, double resolution) {
  if (resolution <= 0.0) {
    throw std::invalid_argument(
        "state_signature: resolution must be positive.");
  }

  std::ostringstream out;
  out.imbue(std::locale::classic());
  for (const auto& competitor : competitors) {
    out << competitor.id << ':';
    for (int d = 0; d < competitor.position.size(); ++d) {
      const double rounded =
          std::round(competitor.position[d] / resolution) * resolution;
      out << rounded << ',';
    }
    out << '|';
  }
  return out.str();
}

}  // namespace socialchoicelab::competition
