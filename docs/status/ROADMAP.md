# SocialChoiceLab Roadmap

High-level direction for the project. This document does not duplicate detail; it points to the authoritative sources.

| For full detail on … | See |
|----------------------|-----|
| Architecture, layers, and integration pattern | [Design Document](../architecture/design_document.md) |
| Feature and algorithm prioritization (Yolk, voting rules, etc.) | [Implementation Priority Guide](../references/implementation_priority.md) |
| Current position and recent work | [where_we_are.md](where_we_are.md) |
| Layer 7 candidate-competition implementation plan | [competition_plan.md](competition_plan.md) |
| Archived plans (consensus reviews, core completion) | [archive/](archive/README.md) |
| Definition of done per milestone (features, tests, docs, API stability) | [milestone_gates.md](milestone_gates.md) |

**Dependency order** (from design): core C++ and foundation first → stable **c_api** → geometry primitives (e.g. CGAL 2D) → voting rules and outcome concepts → **then** R/Python bindings and GUI. Language bindings and advanced electoral methods depend on the C API and core geometry.

**Release numbering plan:** the next public release is `0.2.0`, reflecting that the core library, C API, bindings, and visualization layer exist but the major planned feature tracks are not complete. `0.3.0` is reserved for the candidate-competition / Layer 7 milestone, which is now substantially implemented locally and in active refinement. `1.0.0` is reserved for the point at which the remaining major feature track after candidate competition — currently described as *Characteristics of Voting Rules* (working title; rename later) — is also complete.

### Dependency sequencing (what must come first)

- **c_api before language bindings:** R and Python packages call the C API only; do not bind C++ directly. Implement and stabilize the c_api surface before building R/Python packages.
- **Geometry primitives before advanced electoral methods:** Implement exact 2D geometry (CGAL), voting rules, and core outcome concepts (Yolk, Heart, etc.) before simulation engines, electoral competition models, or advanced applications that depend on them.
- **Foundation before new layers:** Keep the existing core (distance, loss, PRNG, StreamManager) stable; add new layers (geometry, aggregation, c_api) on top without breaking existing behaviour.
- **For Layer 7, stabilize the one-run competition engine before the sweep/experiment runner:** adaptive candidate competition is the foundation; parameter sweeps, replications, and animated trajectories should consume the engine's stable trace format rather than define it indirectly. See [competition_plan.md](competition_plan.md).

---

## Near-term (1–2 months)

- **Core and c_api:** Complete ✅. C++ core (distance, loss, PRNG, indifference) and stable C API (`scs_api`) are done and CI green.
- **Geometry Layer 3:** Complete ✅ *(Yolk and Heart need revisiting — see below)*. CGAL EPEC integration, exact 2D types, convex hull, majority preference, winsets, Yolk, uncovered set, Heart, Copeland, veto players, weighted voting. See [archive/geometry_plan.md](archive/geometry_plan.md). **⚠️ Note:** `yolk_2d` is currently an LP-yolk approximation (subgradient over sampled directions), not the exact yolk — Stone & Tovey (1992) showed limiting median lines alone do not determine the yolk. `heart_boundary_2d` is a coarse grid approximation. Both must be addressed before the `geometry-complete` milestone is considered fully valid. See [where_we_are.md § Known Issues](where_we_are.md) and [milestone_gates.md § Revisit before release](milestone_gates.md).
- **Profiles & Aggregation Layers 4–5:** Complete ✅. Profile struct, spatial/utility/synthetic profile construction, plurality/Borda/approval/anti-plurality/scoring rules, social rankings, Pareto, Condorcet/majority consistency. See [archive/profiles_and_aggregation_plan.md](archive/profiles_and_aggregation_plan.md).
- **c_api geometry + aggregation extensions:** Complete ✅. All geometry and aggregation services exposed through the stable C API. See [archive/c_api_extensions_plan.md](archive/c_api_extensions_plan.md).
- **Preparing for `0.2.0` (active):** Work through "Revisit before release" items and final release-doc checks before tagging `0.2.0`.

---

## Mid-term (3–6 months)

- **First R and Python bindings:** Complete ✅. `socialchoicelab` R (`.Call()`) and Python (cffi) packages calling the pre-built `libscs_api` via the C ABI. See [archive/binding_plan_completed.md](archive/binding_plan_completed.md).
- **Visualization layer:** Complete ✅. Plotly-based spatial voting plot helpers in R and Python. Composable layers, colorblind-safe theme system, built-in scenario datasets, indifference curves, preferred-region overlays, `save_plot()`. See [archive/visualization_plan.md](archive/visualization_plan.md).
- **`0.2.0` tag:** Pending final release-gate confirmation; see [milestone_gates.md](milestone_gates.md).

---

## Long-term (6+ months)

- **R and Python packages:** Full-featured `socialchoicelab` packages with ModelConfig-driven repro, `export_script(config, lang="R|python")`, and documentation.
- **GUI and web:** "GUI-lite" (R Shiny / Shiny for Python) and optional web app (Shiny for Python deployment) as in the [Design Document](../architecture/design_document.md).
- **Layer 7 simulation engines (`0.3.0` track, now in active implementation/refinement):**
  - **Adaptive candidate / party competition:** multi-candidate spatial competition with Sticker, Hunter, Aggregator, and Predator heuristics; plurality and PR seat conversion; deterministic trace recording; convergence/cycle diagnostics; C API wrappers; R/Python bindings; Plotly trajectory animation. Authoritative plan: [competition_plan.md](competition_plan.md).
  - **Experiment runner:** reproducible parameter sweeps and parallel replications built on top of the stable competition engine, using named streams and per-run seeds.
  - **Remaining near-term Layer 7 blocker:** R animated competition plots still show oversized first jumps in some runs; once solved, the next work item is further animation refinement (trail toggles, fade tuning, layout polish, R/Python parity).
- **Next major feature track after candidate competition:** working title **Characteristics of Voting Rules**. This will be renamed when its scope is formalized, but it is currently the feature family intended to take the project from `0.3.0` to `1.0.0`.
- **Advanced features beyond Layer 7:** 3D/N-D geometry, empirical profiles, preference estimation — per [implementation priority](../references/implementation_priority.md) Phases 3–4.
- **Contributor C API wrapper tooling:** When the project opens to external contributors, any new C++ functionality (preference generation, voting rules, candidate/party strategy, etc.) will require a corresponding C API wrapper. Provide either: (a) documented wrapper templates so contributors know the expected pattern, (b) a template generator script, or (c) a PR-triggered agent that drafts wrapper boilerplate for review. Without this, C API coverage will fall behind the C++ surface. See expanded note at the bottom of this file.

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

Substantial Layer 7 work has landed locally:

- competition core engine
- strategy layer
- step and boundary policies
- election feedback and seat conversion
- experiment runner
- competition C API
- R and Python wrappers
- first static and animated trajectory plots

The remaining work is no longer "build Layer 7 from scratch." It is now:

1. Resolve the remaining R animation jump issue.
2. Refine the competition animation UX.
3. Finish docs/polish/release-gate decisions for the `0.3.0` line.

### Known limitation: animation implementation

The current animation approach (Plotly frame-based HTML export) generates one complete data payload per animation frame. This scales poorly with round count: a 1000-round trace with fade trails produces a very large HTML file that is slow to write and slow to load in the browser. The current approach was chosen for availability and cross-language consistency, not performance.

**Revisit before `0.3.0` release:** investigate a more efficient animation backend — candidates include streaming/chunked approaches, canvas-based rendering, or a lightweight custom viewer — that can handle long competitions (hundreds to thousands of rounds) without prohibitive file sizes or generation times.

### Release ladder

| Version | Meaning |
|---------|---------|
| `0.2.0` | First cohesive public package release: core + c_api + geometry + aggregation + bindings + visualization. |
| `0.3.0` | Candidate competition / Layer 7 release: adaptive candidate engine + trace/C API/bindings + experiment runner baseline. |
| `1.0.0` | Major components complete: `0.3.0` scope plus the next major feature track (currently "Characteristics of Voting Rules", working title). |

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
