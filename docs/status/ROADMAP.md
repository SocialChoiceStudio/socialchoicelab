# SocialChoiceLab Roadmap

High-level direction for the project. This document does not duplicate detail; it points to the authoritative sources.

| For full detail on … | See |
|----------------------|-----|
| Architecture, layers, and integration pattern | [Design Document](../architecture/design_document.md) |
| Feature and algorithm prioritization (Yolk, voting rules, etc.) | [Implementation Priority Guide](../references/implementation_priority.md) |
| Current position and recent work | [WHERE_WE_ARE.md](WHERE_WE_ARE.md) |
| Layer 7 candidate-competition implementation plan | [competition_plan.md](competition_plan.md) |
| Archived plans (consensus reviews, core completion) | [archive/](archive/README.md) |
| Definition of done per milestone (features, tests, docs, API stability) | [MILESTONE_GATES.md](MILESTONE_GATES.md) |
| Rendering backend consolidation analysis (Plotly vs. Canvas) | [rendering_consolidation_evaluation.md](../architecture/rendering_consolidation_evaluation.md) |

**Dependency order** (from design): core C++ and foundation first → stable **c_api** → geometry primitives (e.g. CGAL 2D) → voting rules and outcome concepts → **then** R/Python bindings and GUI. Language bindings and advanced electoral methods depend on the C API and core geometry.

**Release numbering plan:** `v0.2.0` and `v0.3.0` are tagged. The next semver bump may be a patch/minor release to ship post-`v0.3.0` static visualization improvements on `main`, or the next feature-led release after **Characteristics of Voting Rules** (working title) progresses. `1.0.0` is reserved for the point at which that major feature track (post-competition) is also complete.

### Dependency sequencing (what must come first)

- **c_api before language bindings:** R and Python packages call the C API only; do not bind C++ directly. Implement and stabilize the c_api surface before building R/Python packages.
- **Geometry primitives before advanced electoral methods:** Implement exact 2D geometry (CGAL), voting rules, and core outcome concepts (Yolk, Heart, etc.) before simulation engines, electoral competition models, or advanced applications that depend on them.
- **Foundation before new layers:** Keep the existing core (distance, loss, PRNG, StreamManager) stable; add new layers (geometry, aggregation, c_api) on top without breaking existing behaviour.
- **For Layer 7, stabilize the one-run competition engine before the sweep/experiment runner:** adaptive candidate competition is the foundation; parameter sweeps, replications, and animated trajectories should consume the engine's stable trace format rather than define it indirectly. See [competition_plan.md](competition_plan.md).

---

## Near-term (1–2 months)

- **Core and c_api:** Complete ✅. C++ core (distance, loss, PRNG, indifference) and stable C API (`scs_api`) are done and CI green.
- **Geometry Layer 3:** Complete ✅ *(Yolk and Heart need revisiting — see below)*. CGAL EPEC integration, exact 2D types, convex hull, majority preference, winsets, Yolk, uncovered set, Heart, Copeland, veto players, weighted voting. See [archive/geometry_plan.md](archive/geometry_plan.md). **⚠️ Note:** `yolk_2d` is currently an LP-yolk approximation (subgradient over sampled directions), not the exact yolk — Stone & Tovey (1992) showed limiting median lines alone do not determine the yolk. `heart_boundary_2d` is a coarse grid approximation. Both must be addressed before the `geometry-complete` milestone is considered fully valid. See [WHERE_WE_ARE.md § Known Issues](WHERE_WE_ARE.md) and [MILESTONE_GATES.md § Revisit before release](MILESTONE_GATES.md).
- **Profiles & Aggregation Layers 4–5:** Complete ✅. Profile struct, spatial/utility/synthetic profile construction, plurality/Borda/approval/anti-plurality/scoring rules, social rankings, Pareto, Condorcet/majority consistency. See [archive/profiles_and_aggregation_plan.md](archive/profiles_and_aggregation_plan.md).
- **c_api geometry + aggregation extensions:** Complete ✅. All geometry and aggregation services exposed through the stable C API. See [archive/c_api_extensions_plan.md](archive/c_api_extensions_plan.md).
- **`0.2.0` / `0.3.0` releases:** Tagged; see [MILESTONE_GATES.md](MILESTONE_GATES.md).

---

## Mid-term (3–6 months)

- **First R and Python bindings:** Complete ✅. `socialchoicelab` R (`.Call()`) and Python (cffi) packages calling the pre-built `libscs_api` via the C ABI. See [archive/binding_plan_completed.md](archive/binding_plan_completed.md).
- **Visualization layer:** Complete ✅. Plotly-based spatial voting plot helpers in R and Python. Composable layers, colorblind-safe theme system, built-in scenario datasets, indifference curves, preferred-region overlays, `save_plot()`. See [archive/visualization_plan.md](archive/visualization_plan.md).
- **`0.2.0` / `0.3.0` tags:** Complete ✅. See [MILESTONE_GATES.md](MILESTONE_GATES.md).

---

## Long-term (6+ months)

- **R and Python packages:** Full-featured `socialchoicelab` packages with ModelConfig-driven repro, `export_script(config, lang="R|python")`, and documentation.
- **GUI and web:** "GUI-lite" (R Shiny / Shiny for Python) and optional web app (Shiny for Python deployment) as in the [Design Document](../architecture/design_document.md).
- **Layer 7 simulation engines (`0.3.0` track):** ✅ Shipped in `v0.3.0`.
  - **Adaptive candidate / party competition:** multi-candidate spatial competition with Sticker, Hunter, Aggregator, and Predator heuristics; plurality and PR seat conversion; deterministic trace recording; convergence/cycle diagnostics; C API wrappers; R/Python bindings; canvas-based animation (`animate_competition_canvas`). Authoritative plan: [competition_plan.md](competition_plan.md).
  - **Experiment runner:** reproducible parameter sweeps and parallel replications built on top of the stable competition engine, using named streams and per-run seeds.
  - **Follow-on (post-tag):** optional engine extensions (e.g. per-run position re-randomization in [competition_plan.md](competition_plan.md)); retire Plotly frame animation before `1.0.0` per release ladder below.
- **Next major feature track after candidate competition:** working title **Characteristics of Voting Rules**. This will be renamed when its scope is formalized, but it is currently the feature family intended to take the project from `0.3.0` to `1.0.0`.
- **Advanced features beyond Layer 7:** 3D/N-D geometry, empirical profiles, preference estimation — per [implementation priority](../references/implementation_priority.md) Phases 3–4.
- **Contributor C API wrapper tooling:** When the project opens to external contributors, any new C++ functionality (preference generation, voting rules, candidate/party strategy, etc.) will require a corresponding C API wrapper. Provide either: (a) documented wrapper templates so contributors know the expected pattern, (b) a template generator script, or (c) a PR-triggered agent that drafts wrapper boilerplate for review. Without this, C API coverage will fall behind the C++ surface. See expanded note at the bottom of this file.

### Tiered convenience entry points (static analysis & competition) — future

**Intent:** Add **user-facing, one-stop helpers** in R and Python that produce common workflows (figures, profiles, competition runs, experiment batches) without forcing new users to assemble every low-level struct and stream name on day one. Each helper must still map cleanly onto the real parameter model: sensible defaults, optional “common variants,” and an escape hatch to the full surface.

**Scope (two families, parallel design):**

1. **Static / analytical spatial workflows** — e.g. voter ideals and alternatives, distance and loss, indifference and winsets, profiles and voting rules, centrality and scenario loading; bundles that reflect **traditional textbook defaults** where applicable (e.g. Euclidean / circular indifference in the spatial story, standard spatial profile construction, familiar culture assumptions such as IC / IAC where we expose them, single-peaked or other canonical structures only when the core supports them clearly).
2. **Candidate competition workflows** — e.g. one-call setup for a competition run or a small experiment matrix, still backed by the existing engine, C API, and trace format.

**Tiering (illustrative — final count and names TBD):**

- **Tier A — “Traditional defaults”:** One function (or small family) per major workflow with **high-opinion defaults** aligned with common teaching and papers (Euclidean metric, standard tie-breaking, default strategies/policies where relevant, etc.).
- **Tier B — “Common variants”:** A companion entry point (or overloaded/prefixed variant) that surfaces **frequently changed options** (e.g. non-Euclidean `DistanceConfig`, alternative loss, seat rules, key competition policies) without exposing the entire config graph.
- **Tier C — “Advanced / full control”:** Thin wrapper or documented pattern that **exposes all underlying parameters** (existing constructors + `StreamManager` + C API–faithful objects), for reproducibility and extensions.

We may ship **two or three** tiers per major functionality, or merge/split tiers once usage and maintenance cost are clear. **R and Python stay in parity:** same public behavior, options, and defaults across both packages unless explicitly documented otherwise.

**Timing:** **Defer implementation** until the relevant cores (static geometry/aggregation pipeline, competition engine and experiment runner, and the post-`0.3.0` feature track) are **further built out and stable enough** that these helpers are not churning every sprint. When scoped, add a short design note (e.g. under `docs/architecture/` or `docs/development/`) listing function names, default tables, and parity tests.

**Relation to other roadmap items:** Complements the long-term goal of **ModelConfig-driven repro** and scripted export; convenience functions may later **emit or consume** that config story rather than duplicating it.

### Long-term sequencing for candidate competition

The broad order for Layer 7 work is:

1. Single-run competition engine in C++.
2. Stable competition trace/result model.
3. C API exposure of runs and trace export.
4. R/Python wrappers over that stable C API.
5. Experiment runner and parameter sweeps.
6. Visualization and trajectory animation.

This sequencing mirrors how the geometry and aggregation layers were delivered: core first, then stable C API, then bindings and user-facing helpers.

### Current Layer 7 status snapshot

Layer 7 is complete and tagged as `v0.3.0`. Ongoing work is **extensions and maintenance** (experiment-runner options, docs, visualization consistency), not greenfield engine build.

### Post-`v0.3.0` static visualization (on `main`, not yet in a tagged release)

These changes landed after the `v0.3.0` tag; they tighten parity between **static Plotly** spatial plots and the **competition canvas** player:

- **`dist_config` on static layers:** `layer_ic()`, `layer_preferred_regions()`, and `layer_winset()` (auto-compute path) accept `DistanceConfig` so indifference boundaries, preferred regions, and winsets match non-Euclidean metrics (e.g. Manhattan, Chebyshev, Minkowski \(p\neq 2\)) where the core already supports them.
- **Polygon closure:** level-set polygons are explicitly closed in Plotly coordinate arrays so stroke outlines match fills for non-Euclidean ICs and preferred regions.
- **Centroid / marginal median markers:** Plotly symbols and theme colours aligned with the canvas (centroid: crimson cross; marginal median: filled triangle-up with outline).
- **Status quo:** no on-plot `"SQ"` text label; identification is via legend (`Status Quo`) and hover only (even when `show_labels=TRUE` for alternatives).

**Release:** Cut `0.3.1` (or next appropriate semver) when you want these on a tagged line; until then they are `[Unreleased]` in [CHANGELOG.md](../../CHANGELOG.md).

### Animation implementation

The Plotly frame-based approach (`animate_competition_trajectories`) has been superseded by a canvas-based player (`animate_competition_canvas`) that stores data once and draws frames on demand. The canvas version handles long runs (hundreds to thousands of rounds) without prohibitive file sizes or generation times, and supports additional features (KDE heatmap, vote-share bar, movement arrows, ghost position, keyboard scrubbing, loop toggle) that are impractical to implement cleanly in Plotly. R and Python share the same JS player file for consistency.

The Plotly implementation is retained with its tests for now because the project is not yet open to external users. It should be revisited and retired before `1.0.0`.

### Rendering backend consolidation (evaluation)

The project currently maintains two independent rendering backends: **Plotly** (SVG-based via D3.js, used for all static spatial voting plots in R and Python) and the **JS Canvas 2D** player (immediate-mode raster rendering, used for competition animations). Evaluate consolidating on a single JS Canvas backend for both static and animated visualizations, with the goal of eliminating Plotly as a required dependency.

**Motivation:**

- The canvas widget already renders most of the geometric primitives the static plots require (points, polygons, circles, axes, grid, labels). Extending it to cover the static spatial voting use case is an incremental step, not a ground-up build.
- Removing Plotly eliminates a runtime dependency from both R (`plotly >= 4.10.0`) and Python (`plotly >= 5.0`), plus `kaleido` for static image export.
- Reduces the R-Python parity maintenance surface: both layers would share a single canonical JS renderer with only data marshalling differing per language, instead of maintaining two parallel sets of Plotly API calls that must stay in sync.
- Gives full control over rendering (z-ordering, styling, interaction) without working around a third-party API.

**Key technical finding:** Plotly renders to SVG natively (via D3.js); its easy SVG export comes from serializing the DOM, not from a Canvas-to-vector conversion. Our canvas widget uses the HTML5 Canvas 2D API (raster/immediate-mode). For vector export from Canvas, the off-the-shelf library **[canvas2svg](https://github.com/gliffy/canvas2svg)** (MIT license) provides a mock Canvas 2D context that records standard drawing calls (`arc`, `lineTo`, `fill`, etc.) and builds an SVG scene graph. PNG export is native via `canvas.toDataURL()`.

**Phase 1 — Spike / proof-of-concept:** Port `plot_spatial_voting` + `layer_ic` + `layer_winset` to a canvas-based static widget. Evaluate: output quality, interactivity (hover/tooltips), composability (layer stacking via JSON spec analogous to `overlays_static`), and export (PNG via `toDataURL`, SVG via canvas2svg). Decision gate: proceed to full migration, keep Plotly as optional secondary format, or abandon.

**Phase 2 — Full migration (contingent on Phase 1):** Port all remaining `layer_*()` functions, `finalize_plot()`, and `save_plot()`. Retire Plotly imports and dependencies from both R and Python packages.

**When:** Evaluation spike after `0.3.0` is stable; full migration decision before `1.0.0`.

Detail and technical analysis: [rendering_consolidation_evaluation.md](../architecture/rendering_consolidation_evaluation.md).

### Parallel processing in IC construction (investigation)

Indifference curves (ICs) are computed during both static spatial voting visualizations (geometry layer) and dynamic candidate competition trace generation. Investigate whether parallel processing is currently utilized in IC construction for both paths:

1. **Static ICs** (e.g. in `plot_spatial_voting` overlays): Do current implementations construct multiple ICs in parallel, or sequentially?
2. **Candidate competition ICs** (during trace generation in the competition engine): Are ICs for multiple candidates computed in parallel during each round, or sequentially?

**Goal:** Identify whether parallelization is already deployed, and if not, evaluate the potential performance gain (wallclock time, memory usage) from parallel construction of independent ICs. This is particularly relevant for large profiles (hundreds of voters) and long competition runs (many rounds × many candidates).

**Scope:** Audit both Python and R code paths; check whether any parallelization occurs at the C++ core layer or is left to the language bindings.

**When:** After `0.3.0` is stable; appropriate for a performance sprint before `1.0.0`.

### Release ladder

| Version | Meaning |
|---------|---------|
| `0.2.0` | First cohesive public package release: core + c_api + geometry + aggregation + bindings + visualization. |
| `0.3.0` | Candidate competition / Layer 7 release: adaptive candidate engine + trace/C API/bindings + experiment runner baseline. |
| pre-`1.0.0` | **Clean up deprecated animation code:** retire `animate_competition_trajectories` (R and Python) and its tests now that the canvas player is the sole animation backend. **Rendering backend decision:** complete the Plotly-vs-Canvas consolidation evaluation (see § Rendering backend consolidation) and act on the result — either migrate static plots to Canvas or document the decision to retain Plotly. |
| `1.0.0` | Major components complete: `0.3.0` scope plus the next major feature track (currently "Characteristics of Voting Rules", working title). |

---

## Non-Euclidean geometry (deferred)

**Static Plotly overlays (partially addressed on `main`, post-`v0.3.0`):** `layer_ic()`, `layer_preferred_regions()`, and `layer_winset()` auto-compute accept `dist_config` and use core level-set / winset machinery for non-Euclidean metrics. This does **not** resolve the deferred items below (Pareto proxy, Voronoi, Yolk/Heart/etc. under alternate metrics).

Several geometry services are currently valid only under Euclidean distance (or
have not been verified for other metrics). These must be addressed before the
project is used with non-Euclidean `DistanceConfig` settings in production:

- **Pareto set under non-Euclidean metrics:** The Pareto set is a
  metric-independent concept, but a correct general implementation does not yet
  exist. Currently the convex hull is used as a proxy — it equals the Pareto set
  only under Euclidean distance with uniform salience. Under other metrics the
  convex hull is an outer bound, not the Pareto set. The overlay must not be
  labelled "Pareto Set" when a non-Euclidean `DistanceConfig` is active.
  See `geometry_design.md` open question.
- **Candidate Regions (generalised Voronoi) under non-Euclidean metrics:** The
  **Euclidean** Voronoi (candidate regions) is implemented for the 2D competition
  canvas behind a `compute_voronoi` flag and a "Voronoi" toggle in the player UI.
  Generalisation to non-Euclidean metrics remains planned. Under uniform
  Euclidean the partition boundary coincides with the indifference boundaries
  (perpendicular bisectors). Under other metrics it curves.
- **Yolk, Heart, Uncovered Set, Core under non-Euclidean metrics:** These are
  defined via median hyperplanes and pairwise majority over winsets. Their
  behaviour under non-Euclidean `DistanceConfig` has not been verified. Until
  verified, the R/Python layer must warn or error when these overlays are
  requested with a non-Euclidean config.

**Guard requirement:** `animate_competition_canvas()` in R and Python must
check the `DistanceConfig` from the trace and suppress or relabel any overlay
that is not valid for the active metric, with a clear user-facing error message.
This guard must be implemented before any of the overlay pipeline goes into
production use.

Once verified/corrected, add Yolk, Heart, Uncovered Set, and Core as static canvas overlays following the centroid/marginal median/Pareto Set pattern.

**When:** After the canvas overlay pipeline is built and before `1.0.0`.

---

## Possible future extensions

These are not on the active roadmap but are worth keeping in view as the project matures:

- **Directional preference model:** Voters care about the direction of change from the status quo, not just distance. Requires a different geometric structure (angle/projection-based rather than distance-based) for majority preference and winsets.
- **Complementarity / tradeoff metric:** Non-separable preferences where policy dimensions interact. Adds a complementarity angle parameter to the distance metric, allowing voters to prefer balanced movement across dimensions.
- **Bounded rationality:** Voters do not perfectly compare alternatives; the cutline is "blurred" by a noise or uncertainty model. Can represent analyst uncertainty about ideal point placement as well as voter inattention. Probabilistic rather than exact geometric operation.
- **Issue lines vs. cleave lines:** Election models sometimes distinguish between issue lines (projections of issue dimensions) and cleave lines (cutlines between candidates). Relevant when modeling party competition rather than individual voter choice.
- **Transaction costs:** Only proposals within a given distance of the status quo can be considered, due to agenda constraints or implementation costs. Geometrically this restricts the effective policy space to a ball around the SQ, intersected with the winset. Worth revisiting once the core geometry services are stable and there is a clearer sense of how this constraint should be modeled.
- **Banks set:** The set of alternatives that are the top element of some maximal chain in the majority preference tournament. Always a subset of the uncovered set; always contains the Condorcet winner if one exists. Defined by Banks (1985). Computationally harder than the uncovered set — finding it requires enumerating maximal chains in the majority tournament. In the 3-voter spatial case, Banks set = uncovered set (Feld et al. 1987, Theorem 10).
- **Schattschneider set:** The set of points at the perpendicular intersection of two median lines — equivalently, the set of median voter projections on median lines (Grofman and Feld 1985; Feld et al. 1987, Definition 7). In 2D it forms a continuous closed curve of arcs of circles through pairs of voter ideal points. It is a subset of both the uncovered set and the Banks set. Characterises the locus of outcomes under open agenda processes with a germaneness rule (Feld et al. 1987, Theorem 11).
- **Bipartisan set:** The support of the unique mixed-strategy Nash equilibrium of the symmetric zero-sum game where two candidates simultaneously choose platforms and each voter votes for the nearer one (Laffond, Laslier & Le Breton 1993). Always a subset of the uncovered set. Requires solving a linear program over the pairwise comparison matrix.
- **Tri-Median set:** The set of all possible multidimensional medians — points that are the median voter projection in every direction simultaneously. Related to the Yolk and the Schattschneider set. Relevant for understanding the structure of issue-by-issue voting outcomes (Feld and Grofman 1988).
- **Shapley-Owen Power Index (SOV):** The spatial analogue of the Shapley-Shubik power index. For each voter, the SOV is the proportion of directions (angles of rotation of a median line) on which that voter is the pivot (the median projection). SOVs sum to 1 across all voters. Key property: the strong point (spatial Copeland winner) is the SOV-weighted average of voter ideal points. Win-set area increases with the square of distance from the strong point along any ray (Shapley and Owen 1989; Godfrey, Grofman & Feld 2011).
- **Radar charts for voting rules:** Comparative visualization of voting rules across multiple criteria (e.g., Condorcet efficiency, monotonicity, strategic robustness, computational complexity). Allows side-by-side assessment of rule properties and trade-offs. Useful for educational scenarios and rule selection workflows.

---

## Summary

| Horizon  | Focus |
|----------|-------|
| **Near** | Close the current release gate → tag `0.2.0`. |
| **Mid**  | Finish Layer 7 candidate competition refinements and iterate to `0.3.0`. |
| **Long** | Complete the next major feature track, then tag `1.0.0`; after that, GUI-lite/web and other advanced spatial features. |

---

## Yolk and Heart algorithm investigation (needed before geometry milestone re-certification)

The current `yolk_2d` implementation (subgradient descent over sampled directions including n(n−1)/2 critical directions) computes an **LP yolk** — the smallest ball intersecting all *limiting* median lines — rather than the true yolk which must intersect *all* median lines. These are different concepts:

- **Stone, R.E. & Tovey, C.A. (1992).** "Limiting median lines do not suffice to determine the yolk." *Social Choice and Welfare*, 9(1), 33–35. — Proves by counter-example that the LP yolk can be strictly smaller than the true yolk.
- **Hu, R. & Bailey, J.P. (2024).** "On the approximability of the yolk in the spatial model of voting." arXiv:2410.10788. — For odd n in ℝ², LP yolk radius ≥ ½ × true yolk radius (tight bound), but centre can be arbitrarily far; for even n or dimension ≥ 3 the LP yolk can be arbitrarily small relative to the true yolk.

Candidate algorithms to investigate and potentially adopt:

1. **Gudmundsson, J. & Wong, S. (2019).** "Computing the yolk in spatial voting games without computing median lines." arXiv:1902.04735. (AAAI 2019.) — O(n log⁷ n) near-linear time for exact L1/L∞ yolk; O(n log⁷ n · log⁴(1/ε)) time for (1+ε)-approximation of L2 yolk. Avoids computing limiting median lines entirely (breaks prior O(n^{4/3}) bottleneck). Uses Megiddo's parametric search. **Most promising near-term option for 2D L2.**
2. **Liu, Y. & Tovey, C.A. (2023).** "Polynomial-time algorithm for computing the yolk in fixed dimension." (Poster/preprint.) — Extends Tovey (1992)'s exact algorithm, O(n^{m+1}²) in m dimensions. A possible counter-example in 3D is under investigation; 2D case believed sound.
3. **Tovey, C.A. (1992).** "A polynomial-time algorithm for computing the yolk in fixed dimension." *Mathematical Programming*, 57(1), 259–277. — Original O(n⁴) exact algorithm in 2D; known correct but slow.

**For the `heart_boundary_2d`:** The continuous Heart in policy space has no known exact algorithm or closed-form characterisation. The current grid + convex-hull approach is a visualisation aid, not a rigorous solution concept. Mark as approximate in all documentation and c_api exposure.

## Test correctness review

At a suitable milestone boundary (suggested: end of geometry Phase B–D), conduct a systematic review of all unit tests for **substantive correctness** — not just that tests pass, but that the expected outcomes are geometrically and theoretically correct.

Specifically, for every test that asserts a spatial-voting or social-choice claim (e.g. "this winset is empty", "this voter prefers x to y", "this is a Condorcet winner"), verify:

1. The claimed result follows from the stated theory (cite the relevant theorem or result).
2. The voter configuration and SQ actually instantiate the conditions of that theorem.
3. The expected value (empty/non-empty, inside/outside, margin sign) is correct under those conditions.

The trigger for adding this item: a test asserting that the centroid of an equilateral triangle is a Condorcet winner under simple majority was written and passed through review. This is false — an empty majority-rule winset requires Plott's radial symmetry conditions (Plott 1967), which the equilateral centroid does not satisfy. Both algorithms correctly returned non-empty winsets; the test was wrong. The error was caught only by domain knowledge, not by the test framework.

Files to review: `test_majority.cpp`, `test_winset.cpp`, `test_geom2d.cpp`, `test_convex_hull.cpp`, `test_c_api.cpp`.

**Additional item (from paper reference review, March 2026):** The paper
(`introducing_socialchoicelab_PCS2026.tex`, Testing and Reliability paragraph)
claims: "for profiles with three alternatives, the Condorcet winner (when one
exists) is verified to also be the Borda winner (Fishburn 1977)." This claim is
**false**. Counterexample: 3 voters rank A > C > B, 2 voters rank C > B > A —
A is the Condorcet winner (beats B 3–2, beats C 3–2) but C is the Borda winner
(score 7 vs. A's 6). The correct result (Fishburn 1973) is that the Borda count
never ranks the Condorcet winner *last* among 3 alternatives, which is weaker.
If a unit test asserts Condorcet-winner = Borda-winner for m = 3, that test
encodes a false theorem and must be corrected. The paper text must also be fixed
(the Fishburn 1977 reference is additionally missing from the bibliography).

---

## Documentation and citation standards

At each milestone boundary, before marking a milestone complete:

- Every implemented concept must have at least one citation to the original defining work.
- Citations should be checked for accuracy (author, year, title, publication). Inline citations in doc files are the source of truth; code comments may use short-form (e.g. "Feld et al. 1987").
- Where a primary source is old or hard to access, add a more recent secondary reference that restates the concept accessibly.
- Flag any concept whose citation is uncertain or missing — do not leave it as "TODO", record it explicitly so it is not lost.
- A dedicated citation verification step (analogous to E3 in the geometry plan) must appear in every future plan document.

When in doubt, follow the [Design Document](../architecture/design_document.md) for architecture and the [Implementation Priority Guide](../references/implementation_priority.md) for ordering of social-choice and geometric features.

---

## Symbolic computation via SymPy (post-open-source)

**When relevant:** Well after the project opens for external contributions.

Many social-choice results — median voter positions, yolk centers, Plott symmetry conditions, winset boundaries under rational ideal points — have exact closed-form expressions. Floating-point arithmetic means these are currently approximated, which can obscure structural properties and introduce numerical error in derived quantities.

Investigate threading SymPy (Python) and a compatible R symbolic library through as much of the computation stack as possible, so that results involving rationally-specified voter ideal points can be preserved symbolically rather than collapsed to floats at the first arithmetic step. The gain would be exact proofs-by-construction, cleaner test assertions (equality instead of tolerance checks), and academic output quality that matches the literature.

This is a significant architectural undertaking and only worthwhile once the API surface is stable and contributor capacity exists to maintain a parallel symbolic path. Record here so it is not forgotten.

---

## Data and I/O — Future features

### PrefLib and COMSOC ecosystem integration

Evaluate and build interoperability with the [PrefLib](https://preflib.org/) ecosystem and other Computational Social Choice (COMSOC) tools. PrefLib maintains a large collection of real and synthetic preference datasets in a well-specified ordinal format (SOC/SOI/TOC/TOI/CAT/WMD — see [FORMAT_SPECIFICATION.md](https://github.com/PrefLib/PrefLib-Data/blob/main/FORMAT_SPECIFICATION.md)).

PrefLib's format is purely ordinal (rankings, not spatial coordinates), so it maps to our `Profile` layer, not the spatial/geometric layer. The integration strategy:

1. **Import:** Read PrefLib `.soc`/`.soi`/`.toc`/`.toi` files into our `Profile` object. Allows users to run our voting rules and analysis on real election data.
2. **Export:** Write our `Profile` objects out in PrefLib format. Allows our synthetic profiles to be shared with the broader COMSOC research community.
3. **Survey other COMSOC tools** (e.g., PrefLib-Tools, abcvoting, pref_voting) for complementary functionality worth linking to or learning from.
4. **PrefSampling interop** ([COMSOC-Community/prefsampling](https://github.com/COMSOC-Community/prefsampling)): a lightweight Python library of point samplers (uniform ball, sphere, cube, Gaussian variants) and ordinal statistical cultures (Mallows, Pólya urn, single-peaked, etc.). For our spatial work, their point samplers return `np.ndarray` of shape `(num_points, num_dimensions)`, which converts to our flat interleaved format with a single `.flatten()`. Only the point samplers are relevant for spatial models; the ordinal cultures produce rankings with no underlying coordinates and are not applicable to our geometry/competition engine. Integration path: accept PrefSampling arrays directly as voter ideals in Python via a thin adapter; R users continue using our native generators or scenarios.

**When:** After the `0.3.0` competition milestone is stable and before `1.0.0`. The profile layer is already complete; this adds I/O and adapter utilities, not new computation.

### Canvas payload compression for .scscanvas files

Investigate compression strategies for `.scscanvas` files produced by
`save_competition_canvas()`. These files grow with the number of frames,
voters, and enabled overlays (ICs, WinSet, Cutlines); large runs can produce
multi-MB JSON. Candidates to evaluate:

- **gzip / brotli** wrapping of the JSON text — transparent to the format
  structure; supported natively in R (`memCompress`) and Python (`gzip`).
- **Binary payload** for the numeric arrays (positions, IC vertices, WinSet
  boundaries) — significantly reduces size but requires a format version bump
  and a more complex loader.
- **Deduplicated frames** — store only per-frame deltas for data that does not
  change between frames (e.g. voter positions, static overlays).

**When:** After the initial `.scscanvas` save/load feature stabilises. Lower
priority than the core serialisation work.

### C13.1: Load scenario from external file format

Support for user-provided external scenario files (complement to the bundled scenarios in C13.A). Design decisions:

- **Format:** CSV for voters (lightweight, spreadsheet-friendly). Metadata (SQ, decision rule, axis labels, etc.) can be specified via a companion JSON sidecar or simple conventions (e.g., comments in the CSV).
- **Convenience functions:** `read_scenario(path)` in R and Python.
- **When:** After C13.A is complete and bundled scenarios are stable. Lower priority than other visualization refinements.

---

## C API wrapper tooling for external contributors

**When relevant:** When the project opens for external contributions.

The C API (`scs_api.h` / `scs_api.cpp`) is the sole stable boundary between C++ and language bindings (R, Python). Every new C++ function callable from R/Python must have a corresponding `extern "C"` wrapper. The current pattern — input validation, `try/catch`, error-buffer output, opaque handles for non-trivial return types — is consistent across C0–C5 and documented in `docs/architecture/c_api_design.md`.

When external contributors add C++ functionality they may not know this pattern, may skip the wrapper, or may write one that doesn't conform (wrong error codes, exceptions escaping, STL types crossing the boundary). Options:

- **Wrapper template / contributor guide:** A documented skeleton (`scs_my_function.h.template`, `scs_my_function.cpp.template`) showing the required pattern, referenced from `CONTRIBUTING.md`.
- **Template generator script:** A script that takes a C++ function signature and emits a conforming C wrapper stub for the contributor to fill in.
- **PR-triggered agent:** A CI or review-bot step that detects new public C++ API surface (new functions in `include/socialchoicelab/`) and either auto-drafts a wrapper stub or posts a checklist comment flagging that a C API wrapper is required.

Start with option one when contributor onboarding begins; revisit options two and three when contributor volume warrants it.
