# SocialChoiceStudio – Living Design Document

## Overview
SocialChoiceStudio is a modular, high-performance simulation and analysis platform for **spatial** and **general social choice models**.  
It is designed for reproducible research, educational use, and exploratory simulations.  
The system is built around a **C++ core** with bindings for **R** and **Python**, plus an optional GUI.

## Project Goals
- Model separable and non-separable voter preferences.
- Perform exact geometry for spatial computations using CGAL.
- Provide modular plug-and-play architecture for preference modeling, aggregation, and outcome logic.
- Support reproducible workflows in R and Python, and interactive work in the GUI.

---

## What to Build

### A) Code-first packages
- **Core C++**: pure C++ (CGAL/Eigen), no binding headers.
- **Stable façade**: thin c_api (C ABI) with POD structs/opaque handles; no STL across boundary.
- **R package (socialchoicelab)**: thin C registration layer (`.Call()`) that calls c_api via C ABI. No C++ compilation in the R package.
- **Python package (socialchoicelab)**: cffi adapter that calls c_api via C ABI. No C++ compilation in the Python package.
- **Plot helpers**: HTML5 Canvas widgets (R `htmlwidgets`, Python self-contained HTML) with identical APIs; static spatial plots use `spatial_voting_canvas.js` + `scs_canvas_core.js` (shared with the competition player).
- **Repro**: ModelConfig JSON/YAML everywhere; export_script(config, lang="R|python").

### B) "GUI-lite"
- **R Shiny modules** and **Shiny for Python apps** call their language package (which calls c_api).

### C) Web app
- **Shiny for Python deployment**; talks to Python package (cffi → c_api).

---

## Layered Architecture

### 1. Foundation
- **core::types** – Canonical point/vector types: `core/types.h` defines `Point2d`, `Point3d`, `PointNd` (Eigen::Vector2d, Vector3d, VectorXd). Use these for ideal points, alternatives, and level-set points. CGAL exact types when needed (Layer 3).
- **core::kernels** – CGAL kernel policy, numeric kernels
- **core::linalg** – Eigen linear algebra; see `core/linalg.h` (index type, single include). Vectors, matrices, and operations used in distance/loss/indifference come from Eigen.
- **core::rng** – Seeded PRNG, multiple streams
- **core::serialization** – Deferred. Save/load of RNG or core config may be added in a later milestone when there is a concrete need (e.g. simulation checkpoint/resume). Config and repro use JSON/YAML (ModelConfig); no binary serialization in core for now.

### 2. Preference Services
- **distance** – Minkowski (p≥1), Euclidean, Manhattan, Chebyshev, custom. Salience weights per dimension (finite, ≥0; zero = dimension masking). Invalid inputs throw `std::invalid_argument`. **Weighting convention (Convention B — dimension pre-scaling):** `d = (Σ (wᵢ |xᵢ - yᵢ|)^p)^(1/p)`. Weight is applied *before* exponentiation; doubling wᵢ halves the effective unit length in dimension i. This differs from Convention A (`d = (Σ wᵢ |xᵢ - yᵢ|^p)^(1/p)`) where weight is outside the exponent. All wrappers (Euclidean, Manhattan, Chebyshev) use Convention B via `minkowski_distance`.
- **loss** – Linear, quadratic, Gaussian, and threshold loss functions; `distance_to_utility` and `normalize_utility`. Parameter domains enforced (sensitivity, max_loss, steepness, threshold, finite inputs); invalid inputs throw `std::invalid_argument`.
- **utility** – Applies a loss function to a distance metric to produce utility values
- **indifference** – Level-set construction; stateless service. Given ideal point, utility level, loss config, and distance config, returns points where u(x) = level: in 1D, 0/1/2 points; in 2D, an exact shape (circle, ellipse, superellipse, or polygon). See [Indifference design](indifference_design.md); implementation in `preference/indifference/level_set.h`.

### 3. Geometry Services *(implemented — Phases A–G complete)*

Full design: [geometry_design.md](geometry_design.md).

- **kernels** – CGAL kernel aliases (`EpecKernel`, `EpicKernel`) in `core/kernels.h` ✅
- **geom2d** – Exact 2D type layer: `Point2E`, `Segment2E`, `Polygon2E`, `WinsetRegion`; conversions `to_exact`/`to_numeric`; predicates `orientation`, `bounded_side` ✅
- **convex_hull** – `convex_hull_2d`: exact CCW hull of voter ideal points; equals the Pareto set under Euclidean preferences ✅
- **majority** – `majority_prefers`, `pairwise_matrix`, `preference_margin`, `weighted_majority_prefers` ✅
- **winset** – `winset_2d` (Euclidean + Minkowski), `winset_is_empty`, `winset_area`, `winset_with_veto_2d`, `weighted_winset_2d` ✅
- **winset_ops** – `winset_union`, `winset_intersection`, `winset_difference`, `winset_symmetric_difference` ✅
- **yolk** – `yolk_2d`: smallest circle intersecting all k-quantile lines ✅
- **uncovered_set** – `covers`, `uncovered_set`, `uncovered_set_boundary_2d` (finite + continuous approximation) ✅
- **core** – `has_condorcet_winner`, `condorcet_winner`, `core_2d` ✅
- **copeland** – `copeland_scores`, `copeland_winner` ✅
- **heart** – `heart` (fixed-point over finite alternatives), `heart_boundary_2d` (continuous approximation) ✅
- **geom3d / geomND** – 3D and N-D geometry *(out of scope for this plan)*

### 4. Profiles and Aggregation *(implemented — Layers 4–5 complete)*

All headers are header-only in `include/socialchoicelab/aggregation/`.
See [aggregation_design.md](aggregation_design.md) for full API and citations.

- **`profile.h`** — `Profile` struct (n voters × m alternatives, 0-indexed, most-preferred
  first); `build_spatial_profile` (Lp-distance ranking); `profile_from_utility_matrix`
  (sort each utility row descending); `is_well_formed`.
- **`profile_generators.h`** — `uniform_spatial_profile`, `gaussian_spatial_profile`,
  `impartial_culture` (Fisher-Yates; all seeded via `PRNG`).
- **`tie_break.h`** — `enum class TieBreak { kRandom, kSmallestIndex }` and `break_tie()`.
  Default is `kRandom`; tests use `kSmallestIndex` explicitly.

### 5. Voting Rules & Aggregation Properties *(implemented — Layers 4–5 complete)*

- **`plurality.h`** — `plurality_scores`, `plurality_all_winners`, `plurality_one_winner`.
- **`borda.h`** — `borda_scores`, `borda_all_winners`, `borda_one_winner`, `borda_ranking`.
- **`approval.h`** — Spatial (`approval_scores_spatial`, `approval_all_winners_spatial`)
  and ordinal top-k (`approval_scores_topk`, `approval_all_winners_topk`) variants.
  Category 1 rule: no `_one_winner` variant.
- **`antiplurality.h`** — `antiplurality_scores`, `antiplurality_all_winners`,
  `antiplurality_one_winner`.
- **`scoring_rule.h`** — `scoring_rule_scores`, `scoring_rule_all_winners`,
  `scoring_rule_one_winner`. Recovers plurality, Borda, and anti-plurality as
  special cases.
- **`social_ranking.h`** — `rank_by_scores` (sort any score vector with tie-breaking);
  `pairwise_ranking` (Copeland scores from geometry-layer pairwise matrix).
- **`pareto.h`** — `pareto_dominates`, `pareto_set`, `is_pareto_efficient`.
- **`condorcet_consistency.h`** — `has_condorcet_winner_profile`,
  `condorcet_winner_profile` (Category 2: `std::optional<int>`),
  `is_condorcet_consistent`, `is_majority_consistent`.

### 6. Outcome Concepts *(planned, not yet built)*
- **outcome_concepts** – Copeland, Strong Point, Heart, Yolk, Uncovered sets, k-majority winsets

### 7. Simulation Engines (stateful) *(planned, not yet built)*
- **adaptive_candidates** – Round-based candidate movement (Sticker, Aggregator, Hunter, Predator)
- **experiment_runner** – Replications, parameter sweeps

### 8. FFI & Interfaces *(planned, not yet built)*
- **c_api** – Stable surface for all bindings
- **python** – cffi (C ABI; no C++ compilation in the Python package)
- **r** – `.Call()` via thin C registration layer (C ABI; no C++ compilation in the R package)
- **web** – gRPC service
- **ios** – C bridge (low priority)

---

## Integration Pattern

### Boundaries
- **Core engine**: header-only exposure limited to your C++ domain types; no R/Python includes.
- **c_api**: stable C interface implemented in `include/scs_api.h` + `src/c_api/scs_api.cpp`.
  Full specification: [c_api_design.md](c_api_design.md).
  - Opaque handle: `SCS_StreamManager*` (wraps `core::rng::StreamManager`).
  - POD config/result structs (`SCS_LossConfig`, `SCS_DistanceConfig`, `SCS_LevelSet2d`); lengths + pointers for arrays; error codes + message buffer.
  - No exceptions cross the boundary (`catch` → return code).
  - **Status:** c_api minimal milestone complete (Items 29–31 implemented).

### R binding (.Call())
- **Files**: `r/src/scs_bindings.c` (thin C registration), `r/src/init.c` (R_registerRoutines / useDynLib).
- **Marshalling**: translate R `SEXP` objects ↔ C scalars/arrays/POD structs in C; translate 0-indexed C results → 1-indexed R at this boundary.
- **Errors**: map non-zero c_api return codes to `Rf_error(msg)` with the err_buf message.
- **Build**: `src/Makevars{,.win}` links against the pre-built `libscs_api` shared library. No C++20 compiler, no CGAL, no Eigen in the R package sources.
- **CRAN-friendly**: pure C registration; no C++ compilation step; only a C compiler required.

### Python binding (cffi)
- **Files**: `python/socialchoicelab/_bindings.py` (cffi ABI-mode loader), `python/socialchoicelab/scs_api.h` (C header declarations).
- **Build**: standard `pyproject.toml`; cffi ABI mode loads `libscs_api` at import time — no compilation step at `pip install`.
- **Errors**: map c_api return codes to Python exceptions with the err_buf message; expose NumPy views for bulk double arrays where helpful.

### c_api design inputs (consensus plan Items 29–31)

Reference these when designing the c_api surface; they are not implementation tasks for current C++ code.

| Item | Topic | Guidance |
|------|--------|----------|
| **29** | `distance_to_utility` parameter explosion | C has no default arguments; six parameters (most conditionally used) do not map cleanly. **Use:** a single `SCS_LossConfig` POD struct with a type discriminant and a parameter union (or equivalent) so the c_api takes one config object instead of six optional parameters. |
| **30** | Stream auto-creation vs pre-registration | **Implemented in C++:** `StreamManager::register_streams(names)`. When the allowlist is set, `get_stream(name)` and `reset_stream(name, seed)` throw `std::invalid_argument` for names not in the set; call `register_streams({})` to clear and restore get-or-create for any name. **c_api:** Enforce the same policy at the boundary (registered streams only when the client has called the registration equivalent). |
| **31** | Do not expose `PRNG::engine()` through c_api | **Decision:** Do not expose in the c_api. `PRNG::engine()` remains in the C++ API with a Doxygen note (internal/test use only; do not expose via c_api). Tests use it for `skip()` verification. Revisit removing it from the public C++ API only if no internal or test code needs it. |

---

## Design Principles
- **Performance first**: Use optimized libraries (Eigen, CGAL) for all geometric and numerical operations.
- **Speed by default**: Eigen vectors for fast numeric operations, CGAL only when exact precision is absolutely necessary.
- **Use "engines" only for stateful or iterative processes** (e.g., adaptive candidates, simulations).
- **Keep stateless math as "services"** (e.g., distance, loss, geometry computations).
- Separation of **distance** and **loss** functions for maximum flexibility in utility modeling.
- Modular boundaries so each part can be swapped or extended.
- Exact geometry (CGAL EPEC) only where correctness is critical and performance can be sacrificed.
- **All social-choice computation in the C++ core.** Distance calculations, cutlines, Voronoi partitions, winsets, majority comparisons, and any other geometry that depends on the distance/loss configuration must be computed in the C++ core and exposed via the C API. Visualization layers (JS canvas widgets, R/Python marshalling) are strictly renderers: they receive pre-computed results and draw them. They must not approximate, re-derive, or short-circuit social-choice geometry, even when a Euclidean special case would appear trivial — the distance metric is configurable and the "trivial" case is not general.

---

## Determinism & Geometry
- **ModelConfig** carries master seed + named streams; geometry precision policy (numeric | exact2d) selects between Eigen-only ops and CGAL EPEC backends.

---

## Visualization Contract
- **Plot helpers** (R, Python, JS canvas player) consume pre-computed results and produce visual output. They do not perform social-choice computation.
- Any geometry that depends on the distance/loss configuration (cutlines, Voronoi cells, winsets, indifference curves, preferred-to sets) must arrive in the visualization layer as vertex lists, polygon boundaries, or other render-ready data produced by the C++ core via the C API.
- The JS canvas player may perform purely visual computations (coordinate transforms, KDE heatmaps from raw voter positions, trail rendering, zoom/pan) but must not compute social-choice geometry.
- R and Python plot helpers produce identical themes and share the same canonical JS files (`scs_canvas_core.js`, `spatial_voting_canvas.js`, `competition_canvas.js`).

---

## Licensing
- **GPL v3** for full compatibility with CGAL EPEC.
- **Revisit:** Once most functionality is built out, reconsider GPL v3 vs LGPL v3 for library adoption (LGPL allows proprietary code to link without relicensing; CGAL Voronoi/Delaunay and some other algorithm packages are GPL-only). See project history in `docs/status/project_log.md` for context.

---

## Naming
- **Project**: SocialChoiceStudio
- **Core package name**: `socialchoicelab` (R and Python)
- **GUI name**: SocialChoiceStudio (alias SCS)

---

## ModelConfig Structure
- **Master seed**: Global random number generator seed
- **Named streams**: Separate PRNG streams for different components (voters, candidates, etc.)
- **Geometry precision**: `numeric` (Eigen-only) or `exact2d` (CGAL EPEC)
- **Plot settings**: Theme preferences, color schemes, export formats
- **Performance limits**: Memory caps, CPU timeouts, iteration limits
- **Output formats**: JSON/YAML export options, precision settings

---

## Performance & Scalability
- **Memory management**: Smart pointers for large profiles, memory pools for temporary objects
- **Parallelization**: Multi-threading for independent computations (profile generation, voting rule evaluation)
- **Streaming**: Process large profiles in chunks to avoid memory issues
- **Caching**: Cache expensive geometric computations (convex hulls, intersections)


---

## Testing & Quality Assurance
- **Unit tests**: Test each service independently (distance, loss, geometry)
- **Integration tests**: Test complete workflows end-to-end
- **Geometric validation**: Compare against reference implementations (CGAL examples, published results)
- **Performance benchmarks**: Timing reports only (no hard assertions); labeled `benchmark` in CTest so they can be excluded from CI (`ctest -LE benchmark`)
- **Continuous integration**: Automated builds on multiple platforms
- **Test data**: Synthetic profiles, real-world examples, edge cases

---

## Advanced Features (Future Research)

### Empirical Data Integration
- **empirical_profiles** – Functions for generating preference profiles based on real-world data
  - Import from survey data, voting records, or other empirical sources
  - Statistical validation and cleaning of empirical preference data
  - Conversion between different empirical data formats

### Preference Estimation
- **preference_estimation** – Machine learning and statistical methods for estimating voter preferences
  - **loss_function_estimation** – Estimate optimal loss functions from observed behavior
  - **utility_function_estimation** – Recover complete utility functions from partial preference data
  - **parameter_estimation** – Fit model parameters to empirical data
  - **validation_methods** – Cross-validation and goodness-of-fit measures

*Note: These features require significant research and validation before implementation. Priority: Low - implement after core functionality is complete.*
