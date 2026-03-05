# Changelog

All notable changes to SocialChoiceLab are documented in this file. The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- Fourth review consensus plan (`docs/status/consensus_plan_4.md`): 30 actionable items (4 High, 6 Medium, 7 Low, 8 test gaps, 5 doc follow-ups) identified by two-agent review (Claude Opus + ChatGPT Codex) in 5 rounds.
- Input validation: distance (finite order_p/coordinates/salience, weights ≥ 0), loss (sensitivity, max_loss, steepness, threshold, finite inputs), PRNG (min/max, stddev, lambda, gamma, bernoulli).
- Optional stream allowlist: `StreamManager::register_streams(names)`; get_stream/reset_stream throw for unregistered names when set.
- Numeric tolerance: `numeric_constants.h` (k_near_zero_rel/abs); `normalize_utility` degenerate range uses epsilon and returns 1.0.
- SplitMix64-style finalizer in stream seed derivation (combine_seed, generate_stream_seed).

### Changed
- Third review consensus plan (26 items) all completed; `where_we_are.md` and `docs/README.md` updated to point to fourth review plan.
- Consensus plan (second review) completed: all items 1–35 done.
- CMake: compiler-ID guard for flags (MSVC vs GCC/Clang); single SocialChoiceLab_compile_options for core and tests.
- Design doc: c_api design inputs table updated (29–31); stream_manager_design notes optional allowlist and SplitMix64 finalizer.

---

When cutting a release, move `[Unreleased]` entries into a new `[Version]` section (e.g. `[1.0.0]`) and add the release date. The end-of-milestone script (`scripts/end-of-milestone.sh`) reminds you to update this file before tagging.
