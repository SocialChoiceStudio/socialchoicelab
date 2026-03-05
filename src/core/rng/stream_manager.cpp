// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "socialchoicelab/core/rng/stream_manager.h"

#include <memory>
#include <mutex>

namespace socialchoicelab::core::rng {

namespace {

struct GlobalStreamManagerState {
  std::mutex mutex;
  std::unique_ptr<StreamManager> manager;
};

// Meyers' singleton: initialized on first call, avoiding Static Initialization
// Order Fiasco (SIOF). The mutex here guards construction of the global
// StreamManager only — not concurrent access to the StreamManager itself.
// See get_global_stream_manager() for the single-owner contract.
GlobalStreamManagerState& get_global_state() {
  static GlobalStreamManagerState state;
  return state;
}

}  // namespace

/// The lock guards lazy construction only. The returned StreamManager& is not
/// thread-safe; the single-owner contract applies — use from one thread only.
StreamManager& get_global_stream_manager() {
  GlobalStreamManagerState& state = get_global_state();
  std::lock_guard<std::mutex> lock(state.mutex);

  if (!state.manager) {
    state.manager = std::make_unique<StreamManager>(k_default_master_seed);
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

}  // namespace socialchoicelab::core::rng
