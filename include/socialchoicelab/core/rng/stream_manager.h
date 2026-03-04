// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "socialchoicelab/core/rng/prng.h"

namespace socialchoicelab {
namespace core {
namespace rng {

/**
 * @brief Manages multiple independent PRNG streams
 *
 * Provides named streams for different components (voters, candidates, etc.)
 * to ensure reproducible and independent random number generation.
 *
 * OWNERSHIP CONTRACT (single-owner policy):
 * Each StreamManager instance must be owned and used by exactly one thread.
 * Do not share a StreamManager instance across concurrent threads. For
 * multi-threaded programs, construct a separate StreamManager per thread or
 * per run rather than sharing one instance.
 *
 * The global get_global_stream_manager() is safe for single-threaded use and
 * tests. In multi-threaded programs, construct local StreamManager instances
 * with per-run seeds derived from (master_seed, run_index) via reset_for_run().
 *
 * Stream naming convention (see docs/governance/StreamManager_Design.md):
 *   "voters"        - voter initialization
 *   "candidates"    - candidate initialization
 *   "tiebreak"      - preference and vote tie-breaking
 *   "movement"      - candidate movement strategies
 *   "memory_update" - agent memory updates
 *   "analysis"      - model output / analytical computations
 */
class StreamManager {
 public:
  /**
   * @brief Construct with master seed
   * @param master_seed Master seed for all streams
   */
  explicit StreamManager(uint64_t master_seed = 12345)
      : master_seed_(master_seed) {}

  /**
   * @brief Get or create a named stream
   * @param name Stream name (e.g., "voters", "candidates", "tiebreak")
   * @return Reference to the PRNG for this stream
   */
  PRNG& get_stream(const std::string& name) {
    auto it = streams_.find(name);
    if (it == streams_.end()) {
      uint64_t stream_seed = generate_stream_seed(name);
      streams_[name] = std::make_unique<PRNG>(stream_seed);
      it = streams_.find(name);
    }

    return *it->second;
  }

  /**
   * @brief Reset all streams with new master seed
   * @param master_seed New master seed
   */
  void reset_all(uint64_t master_seed) {
    master_seed_ = master_seed;
    streams_.clear();
  }

  /**
   * @brief Reset for a new run using a deterministic per-run seed
   *
   * Derives the run seed from (global_master_seed, run_index) so that each
   * run is independently replayable and run N does not depend on draws from
   * run N-1. Use this at the start of each simulation run.
   *
   * @param global_master_seed Experiment-level master seed
   * @param run_index Zero-based index of this run
   */
  void reset_for_run(uint64_t global_master_seed, uint64_t run_index) {
    uint64_t run_seed = combine_seed(global_master_seed, run_index);
    reset_all(run_seed);
  }

  /**
   * @brief Reset a specific stream
   * @param name Stream name
   * @param seed New seed for this stream
   */
  void reset_stream(const std::string& name, uint64_t seed) {
    auto it = streams_.find(name);
    if (it != streams_.end()) {
      it->second->reset(seed);
    } else {
      streams_[name] = std::make_unique<PRNG>(seed);
    }
  }

  /**
   * @brief Get master seed
   * @return Master seed value
   */
  uint64_t master_seed() const { return master_seed_; }

  /**
   * @brief Get list of all stream names
   * @return Vector of stream names
   */
  std::vector<std::string> stream_names() const {
    std::vector<std::string> names;
    names.reserve(streams_.size());
    for (const auto& pair : streams_) {
      names.push_back(pair.first);
    }
    return names;
  }

  /**
   * @brief Check if a stream exists
   * @param name Stream name
   * @return True if stream exists
   */
  bool has_stream(const std::string& name) const {
    return streams_.find(name) != streams_.end();
  }

  /**
   * @brief Remove a stream
   * @param name Stream name
   */
  void remove_stream(const std::string& name) { streams_.erase(name); }

  /**
   * @brief Clear all streams
   */
  void clear() { streams_.clear(); }

  /**
   * @brief Get total number of streams
   * @return Number of active streams
   */
  size_t size() const { return streams_.size(); }

  /**
   * @brief Get debug information
   * @return String with stream information
   */
  std::string debug_info() const {
    std::string info =
        "StreamManager(master_seed=" + std::to_string(master_seed_) +
        ", streams=" + std::to_string(streams_.size()) + "):\n";

    for (const auto& pair : streams_) {
      info += "  " + pair.first + ": " + pair.second->state_string() + "\n";
    }

    return info;
  }

 private:
  /**
   * @brief Generate deterministic seed for a stream based on name
   *
   * Uses FNV-1a (64-bit) to hash the stream name. We do not use std::hash
   * here because std::hash<std::string> is implementation-defined and
   * produces different values across compilers (GCC, Clang, MSVC) and
   * platforms. That would break reproducibility: the same master seed and
   * stream names would yield different RNG sequences on different toolchains.
   * FNV-1a is deterministic and portable, so stream seeds are identical
   * everywhere for the same inputs.
   *
   * @param name Stream name
   * @return Seed for this stream
   */
  /**
   * @brief Combine a master seed and a run index into a single run seed
   *
   * Uses FNV-1a mixing so adjacent run indices produce unrelated seeds.
   *
   * @param master_seed Experiment-level seed
   * @param run_index Zero-based run index
   * @return Deterministic seed for this run
   */
  static uint64_t combine_seed(uint64_t master_seed, uint64_t run_index) {
    constexpr uint64_t kFnvPrime = 1099511628211ULL;
    uint64_t h = master_seed;
    h ^= run_index;
    h *= kFnvPrime;
    h ^= (run_index >> 17);
    h *= kFnvPrime;
    return h;
  }

  uint64_t generate_stream_seed(const std::string& name) const {
    // FNV-1a 64-bit constants (see e.g.
    // http://www.isthe.com/chongo/tech/comp/fnv/)
    constexpr uint64_t kFnvOffsetBasis = 14695981039346656037ULL;
    constexpr uint64_t kFnvPrime = 1099511628211ULL;

    uint64_t name_hash = kFnvOffsetBasis;
    for (unsigned char c : name) {
      name_hash ^= static_cast<uint64_t>(c);
      name_hash *= kFnvPrime;
    }

    // Combine master seed with name hash to get unique seed for each stream
    return master_seed_ ^ (name_hash << 1) ^ (name_hash >> 1);
  }

  uint64_t master_seed_;
  std::unordered_map<std::string, std::unique_ptr<PRNG>> streams_;
};

// Global stream manager instance
extern StreamManager& get_global_stream_manager();
extern void set_global_stream_manager_seed(uint64_t seed);

// Convenience functions for the standard stream names
// (single-threaded use via the global manager only — see ownership contract
// above)
inline PRNG& voters_rng() {
  return get_global_stream_manager().get_stream("voters");
}
inline PRNG& candidates_rng() {
  return get_global_stream_manager().get_stream("candidates");
}
inline PRNG& tiebreak_rng() {
  return get_global_stream_manager().get_stream("tiebreak");
}
inline PRNG& movement_rng() {
  return get_global_stream_manager().get_stream("movement");
}
inline PRNG& memory_update_rng() {
  return get_global_stream_manager().get_stream("memory_update");
}
inline PRNG& analysis_rng() {
  return get_global_stream_manager().get_stream("analysis");
}

}  // namespace rng
}  // namespace core
}  // namespace socialchoicelab
