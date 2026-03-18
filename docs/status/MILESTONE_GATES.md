# Milestone Gates: Definition of Done

For each milestone we tag (e.g. `phase-3`, `c-api-minimal`), these are the gates that must be satisfied before we consider it "done". See [ROADMAP.md](ROADMAP.md) for time horizons; this document defines *what* must be true.

---

## Phase 3 complete (tag: `phase-3`) ✅ 2026-03-04

**Scope:** Developer experience and process. No new product features; process and docs only.

| Gate | Criteria |
|------|----------|
| **Features** | No new feature requirements. Existing core (distance, loss, PRNG, StreamManager) unchanged and working. |
| **Tests** | CI green on `main`: build + test (unit tests, exclude benchmarks) on Ubuntu and macOS; format check passes; `./lint.sh --strict lint` passes. |
| **Docs** | Phase 3 complete (CI, roadmap, gates, CONTRIBUTING/SECURITY/CHANGELOG). `ROADMAP.md` and this file exist. `where_we_are.md` and docs index point to them. `CONTRIBUTING.md`, `SECURITY.md`, `CHANGELOG.md` added. |
| **API stability** | N/A — we are not promising stability yet. Public C++ API may still change; no c_api exists. |

**Check before tagging:** Run `scripts/end-of-milestone.sh <tag>`; it will prompt for CHANGELOG and ROADMAP.

---

## c_api minimal (tag: `c-api-minimal`) ✅ 2026-03-05

**Scope:** A stable C ABI surface over the current C++ core so that R/Python bindings can depend on it. See [Design Document](../architecture/design_document.md) § FFI & Interfaces.

| Gate | Criteria |
|------|----------|
| **Features** | c_api exists: `extern "C"` entry points for distance, loss, utility, level sets, convex hull, and StreamManager/PRNG. No STL across the boundary; error codes + message buffer instead of exceptions. `SCS_LossConfig`, `SCS_DistanceConfig`, `SCS_LevelSet2d` POD structs. `SCS_StreamManager` opaque handle. |
| **Tests** | CI green. 35 GTest cases in `test_c_api.cpp` calling the c_api using only C types. No regressions in existing C++ unit tests. |
| **Docs** | `docs/architecture/c_api_design.md` documents constraints, error contract, struct layout, all function signatures, and a C usage example. |
| **API stability** | c_api surface declared stable for this tag: no breaking changes to function signatures or struct layout without a new milestone/tag. C++ internals may still change behind the c_api. |

---

## Geometry Layer 3 (tag: `geometry-complete`) ✅ 2026-03-06

**Scope:** Exact 2D geometry via CGAL EPEC: convex hull, majority preference, winsets, Yolk, uncovered set, Heart, Copeland, core detection, veto players, weighted voting.

| Gate | Criteria |
|------|----------|
| **Features** | All of Phases A–G and E complete: CGAL integration, exact 2D types, convex hull, majority/pairwise/weighted, winset + boolean ops + veto + weighted, Yolk, uncovered set (discrete + boundary), core detection, Copeland, Heart (discrete + boundary). |
| **Tests** | CI green. 250+ tests across 14 test files. Integration tests chain full pipeline. All correctness assertions verified against theory. |
| **Docs** | `docs/architecture/geometry_design.md` with full API, kernel policy, and 14 verified citations. Design doc Layer 3 updated. |
| **API stability** | Geometry C++ API stable behind the c_api boundary. |

---

## Profiles & Aggregation Layers 4–5 (tag: `aggregation-complete`) ✅ 2026-03-06

**Scope:** Profile construction, synthetic generators, positional voting rules, approval voting, social rankings, Pareto efficiency, Condorcet/majority consistency.

| Gate | Criteria |
|------|----------|
| **Features** | All of Phases P1–P2, V1–V5, W1–W3, I1–I2 complete: `Profile`, spatial/utility/synthetic construction, plurality/Borda/approval/anti-plurality/scoring rules, `rank_by_scores`, `pairwise_ranking`, Pareto, Condorcet/majority consistency. `TieBreak` enum with `kRandom` default and `kSmallestIndex` for testing. |
| **Tests** | CI green. 100 tests across 4 test files (unit + integration). All pass. |
| **Docs** | `docs/architecture/aggregation_design.md` with full API, tie-breaking policy, and citation table. Design doc Layers 4–5 updated. |
| **API stability** | Aggregation C++ API stable behind the c_api boundary. |

---

## c_api extensions (tag: `c-api-extensions`) ✅ 2026-03-07

**Scope:** Expose all geometry and aggregation C++ services through the stable C API. After this milestone, R/Python bindings can call every implemented feature. See [archive/c_api_extensions_plan.md](archive/c_api_extensions_plan.md).

| Gate | Criteria |
|------|----------|
| **Features** | All of C1–C7 complete: majority/pairwise (C1), `SCS_Winset*` opaque handle + boolean ops (C2), `SCS_Yolk2d` + `scs_yolk_2d` (C3), uncovered set/Copeland/core/Heart (C4), `SCS_Profile*` + generators (C5), all voting rules (C6), social rankings + Pareto + Condorcet properties (C7). |
| **Tests** | CI green. `test_c_api.cpp` extended to cover all new functions (C8). Key error paths tested. |
| **Docs** | `docs/architecture/c_api_design.md` updated with all new types and functions; lifecycle contracts for `SCS_Winset*` and `SCS_Profile*`; updated mapping table and usage example (C9). |
| **API stability** | Extended c_api surface declared stable for this tag. No breaking changes to existing function signatures or struct layouts. |

---

## Visualization layer (tag: `visualization-complete`) ✅ 2026-03-08

**Scope:** Plotly-based spatial voting plot helpers in R and Python. Identical API across both languages; composable layers; colorblind-safe theme system; built-in scenario datasets.

| Gate | Criteria | Status |
|------|----------|--------|
| **Features** | C10–C13 complete: `plot_spatial_voting`, `layer_winset`, `layer_yolk`, `layer_uncovered_set`, `layer_convex_hull`, `layer_ic`, `layer_preferred_regions`, `save_plot`, `load_scenario`, `list_scenarios`, `scl_palette`, `scl_theme_colors`. Auto-compute in `layer_winset`/`layer_uncovered_set`. | ✅ |
| **Tests** | All R and Python tests pass. `test_plots.R`, `test_palette.R`, `test_plots.py`, `test_palette.py`. | ✅ |
| **Docs** | `visualization_design.md` updated (color system, layer stack); `visualization_plan.md` complete. | ✅ |
| **API stability** | R/Python plotting API stable for this tag. | ✅ |

---

## First public release / `0.2.0` (tag: `v0.2.0`) 🔲 Next

**Scope:** First cohesive public release with the C++ core, stable C API, at least one language binding, and the visualization layer. This is a feature-complete pre-1.0 release, not the final semver-major boundary.

| Gate | Criteria |
|------|----------|
| **Features** | At least one of: R package (.Call() → c_api) or Python package (cffi → c_api) that builds, installs, and exposes core + geometry + aggregation operations. Binding calls pre-built `libscs_api` via C ABI; no C++ compilation in the binding package. Visualization layer shipped in both bindings remains part of the release line. |
| **Tests** | CI green for core and for the binding(s). Binding tests (R or Python) run in CI. |
| **Docs** | User-facing docs for the binding (README, vignette or equivalent, API reference). Design doc and ROADMAP updated. |
| **API stability** | Pre-1.0 API stability promise: use semver-style versioning within the `0.x` line, document breaking changes clearly in CHANGELOG, and avoid unnecessary churn in the published C API and binding surfaces. |

---

## Competition Layer 7 / `0.3.0` (tag: `v0.3.0`) ✅ Ready to tag

**Scope:** Multi-candidate spatial competition engine (adaptive candidates + trace export) with stable C API and R/Python access. Detailed phase plan: [competition_plan.md](competition_plan.md).

| Gate | Criteria |
|------|----------|
| **Features** | Baseline Layer 7 scope complete: typed competitor/config/state model; fixed and generated voters; Sticker/Hunter/Aggregator/Predator strategies; plurality and proportional seat conversion; synchronous round engine; step-size and boundary policies; convergence/cycle diagnostics; full trace recording/export. |
| **Tests** | CI green. C++ unit/integration tests cover strategy behavior, one-run engine correctness, reproducibility by seed/stream map, and termination diagnostics. C API tests cover handle lifecycle, size-query trace export, and failure paths. R/Python tests cover at least one end-to-end competition run and trace inspection. |
| **Docs** | `docs/architecture/competition_design.md` complete with verified citations and stream map; `docs/status/competition_plan.md` updated to reflect delivered scope; ROADMAP and where_we_are updated. Remaining visual-polish blocker explicitly recorded, including the current R animation jump issue and the follow-on animation-refinement work. |
| **API stability** | Competition C API surface declared stable for the tag; binding APIs for competition runs/trace access stable behind that boundary. |

**Current status note (2026-03-17):**

All Layer 7 scope is complete: core engine, experiment runner, C API, R/Python bindings with full parity, static and canvas-based animation, animation refinement (trail modes, fade, layout polish). CI is green. The R animation jump bug is resolved. All gate criteria are satisfied; pending tag.

---

## Major components complete / `1.0.0` (tag: `v1.0.0`) 🔲 Future

**Scope:** All planned major feature families are in place: the `0.2.0` base release, the `0.3.0` candidate-competition layer, and the next major feature track currently referred to as **Characteristics of Voting Rules** (working title; may be renamed before planning is finalized).

| Gate | Criteria |
|------|----------|
| **Features** | All major planned components are implemented for the first stable era of the project: core + geometry + aggregation + c_api + bindings + visualization + competition layer + the post-competition major feature track. |
| **Tests** | CI green across C++, C API, bindings, and major end-to-end workflows for every major feature family. |
| **Docs** | User-facing and architecture docs cover the full 1.0 surface; roadmap and milestone docs reflect the post-1.0 development model. |
| **API stability** | Semver 1.0 promise takes effect: breaking API changes require a major version bump. |

---

## Summary

| Milestone | Features | Tests | Docs | API stability | Status |
|-----------|----------|-------|------|----------------|--------|
| Phase 3 complete | None new | CI green, format, lint | Plan + ROADMAP + gates | N/A | ✅ 2026-03-04 |
| c_api minimal | C ABI over core | CI + c_api tests | Design + c_api docs | c_api frozen for tag | ✅ 2026-03-05 |
| Geometry Layer 3 | CGAL 2D, winset, Yolk, uncovered set, Heart, Copeland | CI + 250+ tests | geometry_design.md | C++ API stable | ✅ 2026-03-06 |
| Profiles & Aggregation | Profile, voting rules, Pareto, Condorcet | CI + 100 tests | aggregation_design.md | C++ API stable | ✅ 2026-03-06 |
| c_api extensions | Full c_api for geometry + aggregation | CI + extended c_api tests | c_api_design.md updated | Extended c_api frozen | ✅ 2026-03-07 |
| Visualization layer | Plotly layers, theme system, built-in scenarios | R + Python test suites | visualization_design.md | Plotting API stable | ✅ 2026-03-08 |
| First public release / `0.2.0` | Core + c_api + bindings + visualization | CI + binding tests | User docs | Pre-1.0 stability in `0.x` line | 🔲 Next |
| Competition Layer 7 / `0.3.0` | Adaptive candidate engine + stable trace/C API/bindings | CI + engine/C API/binding tests | competition_design.md + roadmap updates | Competition API frozen for tag | ✅ Ready to tag |
| Major components complete / `1.0.0` | All major feature families complete | Full-stack CI + end-to-end workflows | Full 1.0 docs | Semver 1.0 promise | 🔲 Future |

When in doubt, tighten the gate rather than ship: "done" means the criteria above are satisfied, not "we moved on".

---

## Revisit before release / before opening to others

Single list of items to revisit before we tag a release or open the project to other contributors. Add items here when we defer or skip something that we want to decide before launch.

| Item | Source | Note |
|------|--------|------|
| ~~**C++20 vs C++17**~~ | Resolved | **Keep C++20.** Decision: stay at `CMAKE_CXX_STANDARD 20` — the codebase has substantial development ahead and C++20 features will be used as the project grows. No action required before `0.2.0`. |
| ~~**Citations to verify**~~ | Resolved | Borda: standard year is **1784** (cite as "Borda (1784)"). Fishburn (1977): conservative claim — a Condorcet winner, when it exists, is also the Borda winner for 3 alternatives (converse does not hold generally). Guilbaud: corrected to English translation citation (Lazarsfeld & Henry (eds.), MIT Press, 1966, pp. 262–307). All updated in `aggregation_design.md`. |

---

## Deferred known issues (not blocking `0.2.0`)

These are documented approximations that are clearly labelled in the codebase and API headers. They are not blocking the `0.2.0` release. Full technical detail and candidate algorithms are in `ROADMAP.md` and `where_we_are.md § Known Issues`.

| Item | Status | Detail |
|------|--------|--------|
| **Yolk computation (LP-yolk approximation)** | ⏭ Deferred post-`0.2.0` | `yolk_2d` computes the LP-yolk (smallest ball through limiting median lines), not the true yolk. Clearly labelled as approximate in `scs_api.h` (`SCS_Yolk2d` comment) and `where_we_are.md`. `layer_yolk` removed from example scripts. Candidate replacement: Gudmundsson & Wong (2019). See ROADMAP.md for full investigation notes. |
| **Heart boundary (grid approximation)** | ⏭ Deferred post-`0.2.0` | `heart_boundary_2d` is a grid + convex-hull approximation. The continuous Heart has no known exact algorithm (open research problem). Labelled as approximate in API and docs. Not shown in example scripts. See ROADMAP.md and `where_we_are.md`. |
