// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "socialchoicelab/core/rng/stream_manager.h"

#include <memory>

namespace socialchoicelab {
namespace core {
namespace rng {

namespace {

struct GlobalStreamManagerState {
  std::mutex mutex;
  std::unique_ptr<StreamManager> manager;
};

// Meyers' singleton: initialized on first call, avoiding Static Initialization
// Order Fiasco (SIOF). The mutex here guards construction of the global
// StreamManager only — not concurrent access to the StreamManager itself.
// The StreamManager's own single-owner contract means callers must not call
// get_global_stream_manager() from multiple threads concurrently.
GlobalStreamManagerState& get_global_state() {
  static GlobalStreamManagerState state;
  return state;
}

}  // namespace

StreamManager& get_global_stream_manager() {
  GlobalStreamManagerState& state = get_global_state();
  std::lock_guard<std::mutex> lock(state.mutex);

  if (!state.manager) {
    state.manager = std::make_unique<StreamManager>(12345);  // Default seed
  }

  return *state.manager;
}

void set_global_stream_manager_seed(uint64_t seed) {
  GlobalStreamManagerState& state = get_global_state();
  std::lock_guard<std::mutex> lock(state.mutex);

  if (!state.manager) {
    state.manager = std::make_unique<StreamManager>(seed);
  } else {
    state.manager->reset_all(seed);
  }
}

}  // namespace rng
}  // namespace core
}  // namespace socialchoicelab
