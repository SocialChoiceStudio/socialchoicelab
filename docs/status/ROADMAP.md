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
- **Geometry foundation (active):** CGAL EPEC integration, exact 2D types, convex hull, majority preference (k-majority + weighted), winsets, Yolk, uncovered set. See [geometry_plan.md](geometry_plan.md) Phases A–D.
- **Extended winset services (active):** Winset set operations, core detection, Copeland winner, veto players, weighted voting, transaction costs. See [geometry_plan.md](geometry_plan.md) Phase F.

---

## Mid-term (3–6 months)

- **Voting and aggregation:** Implement voting rules (plurality, Borda, Condorcet, approval) and aggregation properties (transitivity, Pareto, IIA). Plan: [profiles_and_aggregation_plan.md](profiles_and_aggregation_plan.md). See design doc layers 4–5 and [implementation priority](../references/implementation_priority.md) Phase 2.
- **c_api geometry extensions:** Expose geometry services (winset, Yolk, uncovered set, Copeland, veto players) through the stable C API so R/Python can call them.
- **First R or Python binding:** Start `socialchoicelab` R (cpp11) or Python (pybind11) package calling the c_api. Geometry and preference services available from R/Python.
- **Visualization layer:** Plot helpers in R and Python for spatial voting output: voter ideal points, status quo, winsets (with individual voter preferred regions drawn as overlapping circles), Yolk circle, uncovered set boundary, convex hull. Identical API across R and Python; Plotly output.

---

## Long-term (6+ months)

- **R and Python packages:** Full-featured `socialchoicelab` packages with ModelConfig-driven repro, `export_script(config, lang="R|python")`, and documentation.
- **GUI and web:** "GUI-lite" (R Shiny / Shiny for Python) and optional web app (Shiny for Python deployment) as in the [Design Document](../architecture/design_document.md).
- **Advanced features:** 3D/N-D geometry, simulation engines (adaptive candidates, experiment runner), empirical profiles, preference estimation — per [implementation priority](../references/implementation_priority.md) Phases 3–4.

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
| **Near** | CGAL 2D geometry (foundation + extended winset services). |
| **Mid**  | Voting rules, c_api geometry extensions, first R/Python binding, visualization layer. |
| **Long** | Full packages, GUI-lite, web app, advanced spatial and empirical features. |

---

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
