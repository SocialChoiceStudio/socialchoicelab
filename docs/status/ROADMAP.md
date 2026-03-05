# SocialChoiceLab Roadmap

High-level direction for the project. This document does not duplicate detail; it points to the authoritative sources.

| For full detail on … | See |
|----------------------|-----|
| Architecture, layers, and integration pattern | [Design Document](../architecture/Design_Document.md) |
| Feature and algorithm prioritization (Yolk, voting rules, etc.) | [Implementation Priority Guide](../references/social_choice/implementation_priority.md) |
| Current position and next backlog item | [WHERE_WE_ARE.md](WHERE_WE_ARE.md) |
| Item-by-item backlog and status | [CONSENSUS_PLAN.md](CONSENSUS_PLAN.md) |
| Definition of done per milestone (features, tests, docs, API stability) | [MILESTONE_GATES.md](MILESTONE_GATES.md) |

**Dependency order** (from design and consensus plan): core C++ and foundation first → stable **c_api** → geometry primitives (e.g. CGAL 2D) → voting rules and outcome concepts → **then** R/Python bindings and GUI. Language bindings and advanced electoral methods depend on the C API and core geometry.

### Dependency sequencing (what must come first)

- **c_api before language bindings:** R and Python packages call the C API only; do not bind C++ directly. Implement and stabilize the c_api surface before building R/Python packages.
- **Geometry primitives before advanced electoral methods:** Implement exact 2D geometry (CGAL), voting rules, and core outcome concepts (Yolk, Heart, etc.) before simulation engines, electoral competition models, or advanced applications that depend on them.
- **Foundation before new layers:** Keep the existing core (distance, loss, PRNG, StreamManager) stable; add new layers (geometry, aggregation, c_api) on top without breaking existing behaviour.

---

## Near-term (1–2 months)

- **Process and docs:** Complete Phase 3 of the [consensus plan](CONSENSUS_PLAN.md): ROADMAP (this file), milestone gates (Item 30), CONTRIBUTING/SECURITY/CHANGELOG (Item 31), optional .clang-tidy and pre-commit (Item 32).
- **Core stability:** Address any remaining Phase 3 backlog items; keep CI green and tests passing.
- **Next technical step:** Begin **c_api** design and a minimal stable C surface over the existing C++ core (per [Design Document](../architecture/Design_Document.md) § FFI & Interfaces), so future R/Python bindings depend on a single ABI rather than the C++ API directly.

---

## Mid-term (3–6 months)

- **Geometry services:** Introduce exact 2D geometry via CGAL (EPEC) where correctness matters; keep Eigen for fast numeric work. See design doc layers 1–3 and [implementation priority](../references/social_choice/implementation_priority.md) Phase 1 (Yolk, uncovered set, convex hull).
- **Voting and aggregation:** Implement voting rules (plurality, Borda, Condorcet, approval) and aggregation properties (transitivity, Pareto, IIA). See design doc layers 4–5 and implementation priority Phase 2.
- **Outcome concepts:** Add outcome concepts (e.g. Copeland, Yolk, Heart, uncovered set) as in the [README](../../README.md) “Planned” list and implementation priority.
- **c_api and bindings:** Finalize c_api for the above; start R (cpp11) and/or Python (pybind11) packages that call the c_api only (design doc § Integration Pattern).

---

## Long-term (6+ months)

- **R and Python packages:** Full-featured `socialchoicelab` packages for R and Python with plot helpers (e.g. Plotly), ModelConfig-driven repro, and documentation.
- **GUI and web:** “GUI-lite” (R Shiny / Shiny for Python) and optional web app (Shiny for Python deployment) as in the [Design Document](../architecture/Design_Document.md) (Code-first packages, GUI-lite, Web app).
- **Advanced features:** 3D/N-D geometry, simulation engines (adaptive candidates, experiment runner), empirical profiles, preference estimation — per design doc “Advanced Features” and [implementation priority](../references/social_choice/implementation_priority.md) Phases 3–4. These are lower priority and may depend on research and validation.

---

## Summary

| Horizon   | Focus |
|----------|--------|
| **Near** | Phase 3 process, docs, start c_api. |
| **Mid**  | CGAL 2D geometry, voting rules, outcome concepts, c_api + first bindings. |
| **Long** | R/Python packages, GUI-lite, web app, advanced spatial and empirical features. |

When in doubt, follow the [Design Document](../architecture/Design_Document.md) for architecture and the [Implementation Priority Guide](../references/social_choice/implementation_priority.md) for ordering of social-choice and geometric features.
