# Changelog

All notable changes to SocialChoiceLab are documented in this file. The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added

#### Profiles & Aggregation (Layers 4–5) — 2026-03-06
- `Profile` struct (`n_voters`, `n_alts`, `rankings`): 0-indexed preference rankings over alternatives, most preferred first.
- `build_spatial_profile`: ranks alternatives by Lp-distance from voter ideal points; ties broken by index.
- `profile_from_utility_matrix`: ranks alternatives by descending utility; ties broken by index.
- `is_well_formed`: validates a `Profile` for internal consistency.
- `uniform_spatial_profile`, `gaussian_spatial_profile`, `impartial_culture` (Fisher-Yates): synthetic profile generators; all seeded via `PRNG`.
- `TieBreak` enum (`kRandom`, `kSmallestIndex`) and `break_tie` helper. Default for user-facing `*_one_winner` functions is `kRandom`; `kSmallestIndex` is used explicitly in tests.
- Positional voting rules — Category 3 (each provides `*_scores`, `*_all_winners`, `*_one_winner`):
  - `plurality.h`: plurality rule.
  - `borda.h`: Borda count, plus `borda_ranking` (full social ordering).
  - `antiplurality.h`: anti-plurality rule.
  - `scoring_rule.h`: generic non-increasing positional scoring rule.
- Approval voting — Category 1 (provides `*_all_winners` only; no tie-breaking needed):
  - `approval_scores_spatial` / `approval_all_winners_spatial`: distance-threshold approval.
  - `approval_scores_topk` / `approval_all_winners_topk`: ordinal top-k approval.
- `rank_by_scores`: sorts alternatives by score vector (descending) with configurable tie-breaking.
- `pairwise_ranking`: computes Copeland scores from a pairwise matrix and returns a full social ordering.
- `pareto_dominates`, `pareto_set`, `is_pareto_efficient`: Pareto efficiency checks over a profile.
- `has_condorcet_winner_profile`, `condorcet_winner_profile` (returns `std::optional<int>`), `is_condorcet_consistent`, `is_majority_consistent`: Condorcet and majority consistency checks — Category 2 (no tie-breaking).
- `socialchoicelab_aggregation` INTERFACE library in CMake; four new test targets.
- 100 new unit tests across `test_profile.cpp` (38), `test_voting_rules.cpp` (28), `test_aggregation_properties.cpp` (22), `test_aggregation_integration.cpp` (12).
- `docs/architecture/aggregation_design.md`: full API, implementation notes, tie-breaking policy, and citation table.

#### Geometry Layer 3 — 2026-03-05/06
- CGAL EPEC/EPIC kernel integration (`kernels.h`); CI updated for Ubuntu (`libcgal-dev`, `libgmp-dev`, `libmpfr-dev`) and macOS (`brew install cgal`).
- Exact 2D type layer (`geom2d.h`): `Point2E`, `Segment2E`, `Polygon2E` (EPEC kernel); `to_exact`/`to_numeric` conversions; `orientation` and `bounded_side` predicates with `Orientation` and `BoundedSide` enums.
- `convex_hull_2d`: exact convex hull of 2D point sets (CCW vertex order).
- `majority_prefers`: k-majority preference relation between two 2D points given voter ideal points.
- `pairwise_matrix`: n×n majority-preference matrix for a finite alternative set.
- `weighted_majority_prefers`: weighted voting majority preference with configurable threshold.
- `winset_2d`: continuous 2D winset as CGAL `General_polygon_set_2` (exact boolean union of preferred-to-SQ sets).
- `weighted_winset_2d`: winset under weighted majority.
- `winset_with_veto_2d`: winset intersected with each veto player's preferred-to-SQ set.
- `winset_union`, `winset_intersection`, `winset_difference`, `winset_symmetric_difference`: exact boolean set operations on winsets.
- `winset_is_empty`: emptiness check.
- `yolk_2d` / `yolk_radius`: Yolk computation via dense directional sampling and subgradient minimax descent.
- `covers`: covering relation on a finite alternative set.
- `uncovered_set`: uncovered set of a finite alternative set.
- `uncovered_set_boundary_2d`: continuous uncovered set boundary as a convex polygon (grid sampling).
- `has_condorcet_winner` / `condorcet_winner` (finite alternatives) / `core_2d` (continuous): Condorcet winner and spatial core detection.
- `copeland_scores` / `copeland_winner`: Copeland scores and strong-point detection.
- `heart`: iterated fixed-point Heart computation over a finite alternative set.
- `heart_boundary_2d`: continuous Heart boundary as a convex polygon (grid sampling).
- `socialchoicelab_geometry` INTERFACE library in CMake; geometry test targets.
- 250+ unit and integration tests across 14 test files.
- `docs/architecture/geometry_design.md`: full API, kernel policy, design rationale, and citation table (14 verified concepts).

#### c_api minimal — 2026-03-05
- `include/scs_api.h`: public C header — `SCS_LossConfig`, `SCS_DistanceConfig`, `SCS_LevelSet2d`, `SCS_StreamManager` opaque handle; all function declarations.
- `src/c_api/scs_api.cpp`: `extern "C"` implementation; `try/catch` → return-code + error buffer; `Eigen::Map` for array inputs; `SCS_StreamManagerImpl` owns the `StreamManager`.
- Functions: `scs_calculate_distance`, `scs_distance_to_utility`, `scs_normalize_utility`, `scs_level_set_1d`, `scs_level_set_2d`, `scs_level_set_to_polygon`, `scs_convex_hull_2d`, `scs_stream_manager_create/destroy`, `scs_register_streams`, `scs_reset_all`, `scs_reset_stream`, `scs_skip`, `scs_uniform_real`, `scs_normal`, `scs_bernoulli`, `scs_uniform_int`, `scs_uniform_choice`.
- 35 GTest cases in `test_c_api.cpp` covering all functions and key error paths.
- `docs/architecture/c_api_design.md`: constraints table, error contract, POD struct layout, function signatures, C usage example.

### Changed

#### 2026-03-06/07
- `docs/status/geometry_plan.md` and `docs/status/profiles_and_aggregation_plan.md` archived to `docs/status/archive/`.
- ROADMAP near-term updated: geometry and aggregation complete ✅; active work is c_api geometry + aggregation extensions.
- `where_we_are.md` updated to current phase (c_api extensions).
- All test files: replaced `using namespace` directives with explicit `using`-declarations; added missing `#include <vector>` and `#include <utility>` (cpplint `build/namespaces` and `build/include_what_you_use` violations fixed).

#### 2026-03-05
- Fourth review consensus plan (30 items) all completed.
- `docs/status/consensus_plan_3.md`, `consensus_plan_4.md`, `core_completion_plan.md`, `c_api_plan.md` archived to `docs/status/archive/`.
- File naming conventions enforced: renamed 10 doc files to `snake_case.md`; cross-references updated in 16 files; `.cursor/rules/File-Naming-Conventions.mdc` rule added.
- CI workflow (`.github/workflows/ci.yml`): build + test on Ubuntu and macOS; format check; strict cpplint; excludes benchmark tests.

---

When cutting a release, move `[Unreleased]` entries into a new `[Version]` section (e.g. `[1.0.0]`) and add the release date. The end-of-milestone script (`scripts/end-of-milestone.sh`) reminds you to update this file before tagging.
