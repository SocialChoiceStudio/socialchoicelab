# Changelog

All notable changes to SocialChoiceLab are documented in this file. The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- Input validation: distance (finite order_p/coordinates/salience, weights ≥ 0), loss (sensitivity, max_loss, steepness, threshold, finite inputs), PRNG (min/max, stddev, lambda, gamma, bernoulli).
- Optional stream allowlist: `StreamManager::register_streams(names)`; get_stream/reset_stream throw for unregistered names when set.
- Numeric tolerance: `numeric_constants.h` (k_near_zero_rel/abs); `normalize_utility` degenerate range uses epsilon and returns 1.0.
- SplitMix64-style finalizer in stream seed derivation (combine_seed, generate_stream_seed).

### Changed
- Consensus plan (second review) completed: all items 1–35 done; WHERE_WE_ARE points to roadmap.md for next work.
- CMake: compiler-ID guard for flags (MSVC vs GCC/Clang); single SocialChoiceLab_compile_options for core and tests.
- Design doc: c_api design inputs table updated (29–31); stream_manager_design notes optional allowlist and SplitMix64 finalizer.

### Fixed
- (Add entries here as relevant.)

---

When cutting a release, move `[Unreleased]` entries into a new `[Version]` section (e.g. `[1.0.0]`) and add the release date. The end-of-milestone script (`scripts/end-of-milestone.sh`) reminds you to update this file before tagging.
