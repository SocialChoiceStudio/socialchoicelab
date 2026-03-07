# SocialChoiceLab Roadmap

High-level direction for the project. This document does not duplicate detail; it points to the authoritative sources.

| For full detail on … | See |
|----------------------|-----|
| Architecture, layers, and integration pattern | [Design Document](../architecture/design_document.md) |
| Feature and algorithm prioritization (Yolk, voting rules, etc.) | [Implementation Priority Guide](../references/implementation_priority.md) |
| Current position and recent work | [where_we_are.md](where_we_are.md) |
| Archived plans (consensus reviews, core completion) | [archive/](archive/README.md) |
| Definition of done per milestone (features, tests, docs, API stability) | [milestone_gates.md](milestone_gates.md) |

**Dependency order** (from design): core C++ and foundation first → stable **c_api** → geometry primitives (e.g. CGAL 2D) → voting rules and outcome concepts → **then** R/Python bindings and GUI. Language bindings and advanced electoral methods depend on the C API and core geometry.

### Dependency sequencing (what must come first)

- **c_api before language bindings:** R and Python packages call the C API only; do not bind C++ directly. Implement and stabilize the c_api surface before building R/Python packages.
- **Geometry primitives before advanced electoral methods:** Implement exact 2D geometry (CGAL), voting rules, and core outcome concepts (Yolk, Heart, etc.) before simulation engines, electoral competition models, or advanced applications that depend on them.
- **Foundation before new layers:** Keep the existing core (distance, loss, PRNG, StreamManager) stable; add new layers (geometry, aggregation, c_api) on top without breaking existing behaviour.

---

## Near-term (1–2 months)

- **Core and c_api:** Complete ✅. C++ core (distance, loss, PRNG, indifference) and stable C API (`scs_api`) are done and CI green.
- **Geometry Layer 3:** Complete ✅ *(Yolk and Heart need revisiting — see below)*. CGAL EPEC integration, exact 2D types, convex hull, majority preference, winsets, Yolk, uncovered set, Heart, Copeland, veto players, weighted voting. See [archive/geometry_plan.md](archive/geometry_plan.md). **⚠️ Note:** `yolk_2d` is currently an LP-yolk approximation (subgradient over sampled directions), not the exact yolk — Stone & Tovey (1992) showed limiting median lines alone do not determine the yolk. `heart_boundary_2d` is a coarse grid approximation. Both must be addressed before the `geometry-complete` milestone is considered fully valid. See [where_we_are.md § Known Issues](where_we_are.md) and [milestone_gates.md § Revisit before release](milestone_gates.md).
- **Profiles & Aggregation Layers 4–5:** Complete ✅. Profile struct, spatial/utility/synthetic profile construction, plurality/Borda/approval/anti-plurality/scoring rules, social rankings, Pareto, Condorcet/majority consistency. See [archive/profiles_and_aggregation_plan.md](archive/profiles_and_aggregation_plan.md).
- **c_api geometry + aggregation extensions (active):** Expose geometry and aggregation services through the stable C API so R/Python can call them.

---

## Mid-term (3–6 months)

- **First R or Python binding:** Start `socialchoicelab` R (cpp11) or Python (pybind11) package calling the c_api. Geometry and preference services available from R/Python.
- **Visualization layer:** Plot helpers in R and Python for spatial voting output: voter ideal points, status quo, winsets (with individual voter preferred regions drawn as overlapping circles), Yolk circle, uncovered set boundary, convex hull. Identical API across R and Python; Plotly output.

---

## Long-term (6+ months)

- **R and Python packages:** Full-featured `socialchoicelab` packages with ModelConfig-driven repro, `export_script(config, lang="R|python")`, and documentation.
- **GUI and web:** "GUI-lite" (R Shiny / Shiny for Python) and optional web app (Shiny for Python deployment) as in the [Design Document](../architecture/design_document.md).
- **Advanced features:** 3D/N-D geometry, simulation engines (adaptive candidates, experiment runner), empirical profiles, preference estimation — per [implementation priority](../references/implementation_priority.md) Phases 3–4.
- **Contributor C API wrapper tooling:** When the project opens to external contributors, any new C++ functionality (preference generation, voting rules, candidate/party strategy, etc.) will require a corresponding C API wrapper. Provide either: (a) documented wrapper templates so contributors know the expected pattern, (b) a template generator script, or (c) a PR-triggered agent that drafts wrapper boilerplate for review. Without this, C API coverage will fall behind the C++ surface. See expanded note at the bottom of this file.

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
| **Near** | c_api geometry + aggregation extensions (active). |
| **Mid**  | First R/Python binding, visualization layer. |
| **Long** | Full packages, GUI-lite, web app, advanced spatial and empirical features. |

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

## C API wrapper tooling for external contributors

**When relevant:** When the project opens for external contributions.

The C API (`scs_api.h` / `scs_api.cpp`) is the sole stable boundary between C++ and language bindings (R, Python). Every new C++ function callable from R/Python must have a corresponding `extern "C"` wrapper. The current pattern — input validation, `try/catch`, error-buffer output, opaque handles for non-trivial return types — is consistent across C0–C5 and documented in `docs/architecture/c_api_design.md`.

When external contributors add C++ functionality they may not know this pattern, may skip the wrapper, or may write one that doesn't conform (wrong error codes, exceptions escaping, STL types crossing the boundary). Options:

- **Wrapper template / contributor guide:** A documented skeleton (`scs_my_function.h.template`, `scs_my_function.cpp.template`) showing the required pattern, referenced from `CONTRIBUTING.md`.
- **Template generator script:** A script that takes a C++ function signature and emits a conforming C wrapper stub for the contributor to fill in.
- **PR-triggered agent:** A CI or review-bot step that detects new public C++ API surface (new functions in `include/socialchoicelab/`) and either auto-drafts a wrapper stub or posts a checklist comment flagging that a C API wrapper is required.

Start with option one when contributor onboarding begins; revisit options two and three when contributor volume warrants it.
