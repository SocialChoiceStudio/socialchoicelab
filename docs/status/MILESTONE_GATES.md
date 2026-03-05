# Milestone Gates: Definition of Done

For each milestone we tag (e.g. `phase-3`, `c-api-minimal`), these are the gates that must be satisfied before we consider it "done". See [roadmap.md](roadmap.md) for time horizons; this document defines *what* must be true.

---

## Phase 3 complete (tag: e.g. `phase-3`) ✅ Already reached (2026-03-04)

**Scope:** Developer experience and process. No new product features; process and docs only.

| Gate | Criteria |
|------|----------|
| **Features** | No new feature requirements. Existing core (distance, loss, PRNG, StreamManager) unchanged and working. |
| **Tests** | CI green on `main`: build + test (unit tests, exclude benchmarks) on Ubuntu and macOS; format check passes; `./lint.sh --strict lint` passes. |
| **Docs** | Phase 3 complete (CI, roadmap, gates, CONTRIBUTING/SECURITY/CHANGELOG). `roadmap.md` and this file exist. `where_we_are.md` and docs index point to them. `CONTRIBUTING.md`, `SECURITY.md`, `CHANGELOG.md` added. |
| **API stability** | N/A — we are not promising stability yet. Public C++ API may still change; no c_api exists. |

**Check before tagging:** Run `scripts/end-of-milestone.sh <tag>`; it will prompt for CHANGELOG and ROADMAP.

---

## c_api minimal (tag: e.g. `c-api-minimal` or `v0.2.0`)

**Scope:** A stable C ABI surface over the current C++ core so that R/Python bindings can depend on it. See [Design Document](../architecture/design_document.md) § FFI & Interfaces.

| Gate | Criteria |
|------|----------|
| **Features** | c_api exists: extern "C" entry points for the operations needed by a first binding (e.g. create/destroy handles, run distance/loss/PRNG operations via POD structs). No STL across the boundary; error codes + message buffer instead of exceptions. |
| **Tests** | CI green. New tests (C or C++) that call the c_api and verify behaviour. No regressions in existing C++ unit tests. |
| **Docs** | Design doc § c_api updated to reflect actual surface. Public c_api functions and structs documented (Doxygen or equivalent). ROADMAP near-term updated if c_api scope changed. |
| **API stability** | c_api surface is declared stable for this release: no breaking changes to function signatures or struct layout without a new milestone/tag. C++ internals may still change behind the c_api. |

---

## Geometry + voting (mid-term) (tag: e.g. `v0.3.0`)

**Scope:** Exact 2D geometry (CGAL), voting rules, outcome concepts. See [ROADMAP](roadmap.md) mid-term and [Implementation Priority](../references/implementation_priority.md).

| Gate | Criteria |
|------|----------|
| **Features** | CGAL-based 2D geometry where specified (e.g. Yolk, uncovered set, convex hull); voting rules (plurality, Borda, Condorcet, approval) and aggregation properties; outcome concepts (e.g. Copeland, Yolk, Heart) as per design and implementation priority. c_api extended to expose what bindings need. |
| **Tests** | CI green. Unit tests for new geometry and voting behaviour; regression tests; optional geometric validation against references. |
| **Docs** | Design doc and implementation priority reflected. Public API (C++ and c_api) documented. ROADMAP mid-term updated. |
| **API stability** | Public C++ and c_api surfaces used by bindings are stable; breaking changes require a new minor/major version and callout in CHANGELOG. |

---

## First binding / 1.0 (tag: e.g. `v1.0.0`)

**Scope:** First released R or Python package (or both) that calls the c_api. See [ROADMAP](roadmap.md) long-term.

| Gate | Criteria |
|------|----------|
| **Features** | At least one of: R package (cpp11 → c_api) or Python package (pybind11 → c_api) that builds, installs, and supports the core operations (distance, loss, RNG) and is usable by downstream code. |
| **Tests** | CI green for core and for the binding(s). Binding tests (R or Python) run in CI. |
| **Docs** | User-facing docs for the binding (README, vignette or equivalent, API reference). Design doc and ROADMAP updated. |
| **API stability** | 1.0 API stability promise: semantic versioning in effect; breaking changes require major version bump and CHANGELOG. |

---

## Summary

| Milestone | Features | Tests | Docs | API stability |
|-----------|----------|-------|------|----------------|
| Phase 3 complete | None new | CI green, format, lint | Plan + ROADMAP + gates | N/A |
| c_api minimal | C ABI over core | CI + c_api tests | Design + c_api docs | c_api frozen for tag |
| Geometry + voting | CGAL 2D, voting, outcomes | CI + new tests | Updated design/priority | Public API stable |
| First binding / 1.0 | R or Python package | CI + binding tests | User docs | Semver from 1.0 |

When in doubt, tighten the gate rather than ship: "done" means the criteria above are satisfied, not "we moved on".

---

## Revisit before release / before opening to others

Single list of items to revisit before we tag a release or open the project to other contributors. Add items here when we defer or skip something that we want to decide before launch.

| Item | Source | Note |
|------|--------|------|
| **C++20 vs C++17** | Consensus plan 4, Batch 3 #11 | We keep `CMAKE_CXX_STANDARD 20` and expect to use C++20 features as we build out. Before release: either adopt C++20 features (e.g. concepts, `[[nodiscard("reason")]]`) so the requirement is justified, or lower to C++17 if we do not need them. |
