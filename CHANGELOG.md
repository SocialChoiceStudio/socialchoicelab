# Changelog

All notable changes to SocialChoiceLab are documented in this file. The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Changed

#### Static spatial plots (R and Python) — canvas migration

- **Renderer:** `plot_spatial_voting()` and all `layer_*()` now build a **canvas**
  payload (`spatial_voting_canvas` htmlwidget in R; `dict` in Python) using shared
  JS (`scs_canvas_core.js`, `spatial_voting_canvas.js`). **Plotly** removed from
  package dependencies; **`finalize_plot()`** removed.
- **`save_plot()`:** writes **HTML** for static figures; **PNG/SVG** raise a clear
  error (use HTML + browser print/screenshot).
- **`dist_config`:** `layer_ic()`, `layer_preferred_regions()`, and `layer_winset()`
  (auto-compute) still accept `DistanceConfig` / `dist_config` for non-Euclidean
  metrics where the core supports them.
- **Polygon stroke:** level-set polygons remain explicitly closed so strokes match fills.
- **Centroid / marginal median:** symbols and colours aligned with the competition canvas.
- **Status quo:** no on-plot `"SQ"` text; legend and hover only.
- **Removed:** `animate_competition_trajectories`, `plot_competition_trajectories`
  (R and Python) and their tests — use `animate_competition_canvas` for animation.

---

## [0.3.0] — 2026-03-17

### Added

#### Candidate competition engine (Layer 7)
- Multi-candidate spatial competition engine with synchronous round updates and full trace recording.
- Competitor strategies: Sticker, Hunter, Aggregator, Predator — as introduced by Laver (2005) and extended by Laver & Sergenti (2011).
- Step-size and boundary policies: fixed step, random step, project-to-bounds, stay-put, reflect.
- Electoral evaluation: plurality support assignment with distance-tie breaking; plurality top-k and Hare largest remainder seat conversion.
- Convergence and cycle diagnostics: position-epsilon convergence, rounded-signature cycle detection, no-improvement windows.
- Experiment runner: serial, per-run deterministic via `StreamManager::reset_for_run`; per-run summaries and optional full traces; labeled scenario sweeps.

#### Competition C API (stable at `v0.3.0`)
- Full `scs_competition_*` surface: handle lifecycle, experiment config, voter setup, single-run execution, trace export (positions, seats, votes, strategy kinds).
- Experiment runner C API: batch runs with per-run summaries and optional trace export.

#### R and Python bindings — competition layer
- `CompetitionExperiment`, `CompetitionTrace`, `make_competition_step_config` in both R and Python with full API parity.
- `animate_competition_canvas`: canvas-based animated player with trail modes, fade control, and layout polish. Supersedes the former frame-based `animate_competition_trajectories` helper (**removed** in the static canvas migration; see `[Unreleased]`).
- Voter sampling: `make_voter_sampler` and `draw_voters` in R and Python.

#### Documentation
- `docs/architecture/competition_design.md`: verified citations (Laver 2005, Laver & Sergenti 2011), stream map, and updated status.
- Competition C API declared stable in `docs/architecture/c_api_design.md`.

---

## [0.2.0] — 2026-03-08

### Added

#### Centrality measures — 2026-03-08
- `scs_marginal_median_2d`: coordinate-wise median of voter ideal points (issue-by-issue median voter; Black 1948). Exposed in C API, R (`.Call()`), and Python (`cffi`).
- `scs_centroid_2d`: coordinate-wise arithmetic mean (barycenter / centre of mass). Exposed in C API, R, and Python.
- `scs_geometric_mean_2d`: coordinate-wise geometric mean (`exp(mean(log(xᵢ)))`). Requires strictly positive coordinates; returns `SCS_ERROR_INVALID_ARGUMENT` with a descriptive message for non-positive inputs (e.g., NOMINATE-scale data). Exposed in C API, R, and Python.
- `layer_centroid()`: Plotly diamond marker layer for the centroid, in R and Python.
- `layer_marginal_median()`: Plotly cross marker layer for the marginal median, in R and Python.
- 27 new tests across C++ (`test_centrality.cpp`), R (`test_geometry.R`), and Python (`test_b5.py`, `test_plots.py`).

#### User-facing documentation — 2026-03-08
- `r/inst/doc/spatial_voting.html`: pre-rendered R vignette covering voter ideal points, StreamManager, majority preference, winsets, Copeland/Condorcet, centrality, ordinal profiles, voting rules, built-in scenarios, and the full visualization layer. All code shown with `eval = FALSE` (no compiled library needed for rendering).
- `python/README.md`: expanded to a full getting-started guide with a quick-start code block covering all major API areas and a module reference table.
- 5 new Roxygen man pages: `centroid_2d.Rd`, `geometric_mean_2d.Rd`, `marginal_median_2d.Rd`, `layer_centroid.Rd`, `layer_marginal_median.Rd`.

#### Visualization layer (C10–C13) — 2026-03-07/08
- `plot_spatial_voting()`: base interactive Plotly figure for voter ideal points and alternatives; auto-computes 12%-padded axis ranges; `xlim`/`ylim` override.
- `layer_winset()`, `layer_uncovered_set()`: accept either a pre-computed object or `voters` + `sq` for auto-compute.
- `layer_ic()`: individual voter indifference curves (circles centred at ideal points, radius = distance to SQ); `color_by_voter` flag; dotted-dash style.
- `layer_preferred_regions()`: filled voter-preferred-to-SQ circles; `color_by_voter` flag.
- `layer_convex_hull()`, `layer_yolk()` (approximate; see Known Issues): geometry layers.
- `save_plot()`: HTML export (`htmlwidgets::saveWidget()` / `fig.write_html()`); image export via `plotly::save_image()` / `fig.write_image()`.
- `finalize_plot()`: applies `zorder`-based layer stacking and legend layout.
- `scl_palette(name, n, alpha)`: colorblind-safe palette utility. Available themes: `dark2`, `set2`, `okabe_ito`, `paired`, `bw`.
- `scl_theme_colors(layer_type, theme)`: canonical color for a named layer type from the active theme.
- All `layer_*()` functions accept a `theme=` argument (default `"dark2"`).
- Full test coverage in `test_plots.R`, `test_palette.R` (R) and `test_plots.py`, `test_palette.py` (Python).

#### Built-in scenarios (C13.A) — 2026-03-07
- 33 canonical spatial voting scenarios in JSON, bundled in R (`inst/extdata/scenarios/`) and Python (`src/socialchoicelab/data/scenarios/`).
- `load_scenario(name)` and `list_scenarios()` in both languages.

#### R and Python bindings (B0–B9) — 2026-03-07
- Full R package (`r/`) with `.Call()` → C ABI bindings for all geometry, aggregation, profile, voting rules, social rankings, and visualization functions. No C++ compilation in the package; links against pre-built `libscs_api`.
- Full Python package (`python/`) with `cffi` ABI-mode bindings for the same surface. `_loader.py` discovers `libscs_api` via `SCS_LIB_PATH`.
- R exception hierarchy: `SCSError`, `SCSInvalidArgumentError`, `SCSBufferError`, `SCSInternalError`, `SCSOutOfMemoryError`.
- Python exception hierarchy mirrors R.
- CI: single `.github/workflows/ci.yml` with separate jobs for C++ (Ubuntu + macOS), R, and Python.

#### C API extensions (C1–C9) — 2026-03-05/07
- **C1** — Majority / pairwise: `scs_majority_prefers_2d`, `scs_pairwise_matrix_2d`, `scs_weighted_majority_prefers_2d`.
- **C2** — Winset opaque handle: `SCS_Winset*` lifecycle (create/destroy/clone), `scs_winset_is_empty`, `scs_winset_contains_point_2d`, `scs_winset_bbox_2d`, `scs_winset_boundary_size_2d`, `scs_winset_sample_boundary_2d`, boolean ops (union/intersection/difference/symmetric_difference).
- **C3** — Yolk: `SCS_Yolk2d` POD struct, `scs_yolk_2d` (LP-yolk approximation; labelled as such).
- **C4** — Geometry: Copeland scores/winner, Condorcet winner (finite), core detection, discrete/continuous uncovered set and Heart.
- **C5** — Profile opaque handle: `SCS_Profile*` lifecycle, `scs_profile_build_spatial`, `scs_profile_from_utility_matrix`, `scs_profile_impartial_culture`, `scs_profile_uniform_spatial`, `scs_profile_gaussian_spatial`.
- **C6** — Voting rules: plurality, Borda, anti-plurality, approval (spatial + top-k), generic scoring rule; all with scores, all-winners, and one-winner variants.
- **C7** — Social rankings: `scs_rank_by_scores`, `scs_pairwise_ranking`, `scs_pareto_set`, `scs_has_condorcet_winner_profile`, `scs_condorcet_winner_profile`, `scs_is_condorcet_consistent`, `scs_is_majority_consistent`, `scs_is_selected_by_majority_consistent_rules`.
- **C8** — Non-finite input validation: all double inputs validated for finiteness at the C API boundary; `SCS_ERROR_INVALID_ARGUMENT` + message on NaN/Inf.
- **C9** — Docs: `docs/architecture/c_api_design.md` updated with all types, lifecycle contracts, function signatures, mapping table, and usage examples.
- Version constants `SCS_API_VERSION_MAJOR/MINOR/PATCH` (0.1.0) and `scs_api_version()`.

#### Profiles & Aggregation (Layers 4–5) — 2026-03-06
- `Profile` struct (`n_voters`, `n_alts`, `rankings`): 0-indexed preference rankings over alternatives, most preferred first.
- `build_spatial_profile`, `profile_from_utility_matrix`, `is_well_formed`.
- Synthetic generators: `uniform_spatial_profile`, `gaussian_spatial_profile`, `impartial_culture` (Fisher-Yates).
- `TieBreak` enum (`kRandom`, `kSmallestIndex`).
- Positional voting rules: plurality, Borda (+ `borda_ranking`), anti-plurality, generic scoring rule; each with `*_scores`, `*_all_winners`, `*_one_winner`.
- Approval voting: `approval_scores_spatial` / `approval_scores_topk` and corresponding winner functions.
- `rank_by_scores`, `pairwise_ranking`, `pareto_set`, Condorcet/majority consistency checks.
- 100 unit and integration tests; `docs/architecture/aggregation_design.md`.

#### Geometry Layer 3 — 2026-03-05/06
- CGAL EPEC/EPIC kernel integration; exact 2D type layer (`Point2E`, `Segment2E`, `Polygon2E`).
- `convex_hull_2d`, majority preference, pairwise matrix, weighted majority.
- Winset (continuous exact), weighted winset, veto-player winset, boolean winset ops.
- Yolk and yolk radius (LP-yolk approximation via directional sampling + subgradient descent).
- Covering relation, discrete uncovered set, continuous uncovered set boundary.
- Condorcet winner (finite), spatial core detection, Copeland scores/winner.
- Heart (finite, iterated fixed-point) and Heart boundary (continuous, grid approximation).
- 250+ unit and integration tests; `docs/architecture/geometry_design.md`.

#### c_api minimal — 2026-03-05
- `include/scs_api.h` and `src/c_api/scs_api.cpp`: initial C ABI surface for distance, loss, utility, level sets, convex hull, and `SCS_StreamManager`.
- 35 GTest cases; `docs/architecture/c_api_design.md`.

### Changed

#### 2026-03-08
- **Citations corrected** in `aggregation_design.md`: Borda standard year is 1784; Fishburn (1977) claim made conservative (Condorcet winner → also Borda winner for 3 alternatives; converse does not hold); Guilbaud citation updated to English translation (Lazarsfeld & Henry (eds.), MIT Press, 1966, pp. 262–307).
- **C++ standard confirmed**: `CMAKE_CXX_STANDARD 20` retained; no action needed before tagging.
- `devScript_MAIN.R` and `devScript_MAIN.py`: `layer_yolk` calls removed (deferred); `layer_centroid` + `layer_marginal_median` added; Laing-Olmsted-Bear scenario used throughout.
- `r/DESCRIPTION`: `htmlwidgets` added to Suggests; `VignetteBuilder: knitr` restored.
- `r/.Rbuildignore`: dev scripts and `doc/` excluded from built package tarball.

#### 2026-03-06/07
- Archived completed plans: `geometry_plan.md`, `profiles_and_aggregation_plan.md`, `c_api_extensions_plan.md`, `visualization_plan.md` → `docs/status/archive/`.
- ROADMAP, `WHERE_WE_ARE.md`, `milestone_gates.md` updated to reflect completed milestones and deferred known issues (Yolk LP-approximation, Heart grid approximation).

### Known Issues (deferred post-0.2.0)

- **Yolk computation**: `yolk_2d` computes the LP-yolk (smallest circle through limiting median lines), which may differ from the true yolk (Stone & Tovey 1992). Labelled as approximate in `scs_api.h`. Candidate replacement: Gudmundsson & Wong (2019).
- **Heart boundary**: `heart_boundary_2d` is a grid + convex-hull approximation; the continuous Heart has no known exact algorithm. Labelled as approximate in API and docs.

---

When cutting a release, move `[Unreleased]` entries into a new `[Version]` section and add the release date. The end-of-milestone script (`scripts/end-of-milestone.sh`) reminds you to update this file before tagging.
