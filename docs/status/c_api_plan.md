# c_api Run Plan

Short-term plan to deliver a **minimal stable C API** over the current C++ core so R/Python bindings can depend on it. Definition of done: [milestone_gates.md](milestone_gates.md) § c_api minimal.

**Authority:** This plan is the source for "what's next" for c_api work. When a step is done, mark it in the Status column and update [where_we_are.md](where_we_are.md) if needed.

---

## Scope

- **In scope:** extern "C" layer over existing core only: distance, loss (including distance_to_utility, normalize_utility), indifference (level_set_1d, level_set_2d, to_polygon), and StreamManager/PRNG (create/destroy, register streams, reset, draw methods). No STL across the boundary; POD structs and opaque handles; error codes + message buffer instead of exceptions.
- **Out of scope for this run:** R or Python bindings, geometry, profiles, or any C++ API not already in the core.

Design constraints: [Design Document](../architecture/design_document.md) § Integration Pattern and § c_api design inputs (Items 29–31: SCS_LossConfig, stream registration at boundary, do not expose PRNG::engine()).

---

## Step-by-step plan

| Step | Task | Deliverable | Status |
|------|------|-------------|--------|
| **1** | **Specify** | Write the c_api surface: public header (function names, POD structs, opaque handle types, error codes and message buffer contract). Can be a new doc (e.g. `docs/architecture/c_api_design.md`) or an expanded section in the design doc. Must cover: distance (config + in/out arrays), loss (SCS_LossConfig per Item 29), indifference (configs + level-set result representation in C), StreamManager (handle lifecycle, register_streams, reset, get stream, draws). | ✅ Done |
| **2** | **Implement** | Add c_api implementation: C or C++ with extern "C", calling existing C++ core; catch exceptions → return code + message buffer. Build as library (e.g. separate target or part of core build). No new C++ core features; only the wrapper layer. | ✅ Done |
| **3** | **Test** | Tests that call the c_api (from C or C++) and verify behaviour (distance, loss, indifference, PRNG). CI green; no regressions in existing C++ unit tests. | ✅ Done |
| **4** | **Document** | Design doc § c_api (or linked c_api_design.md) updated to match actual surface. Public c_api functions and structs documented (Doxygen or equivalent). Roadmap near-term updated if scope changed. Declare c_api stable for this milestone per [milestone_gates.md](milestone_gates.md). | ✅ Done |

---

## Definition of done (c_api minimal)

- All steps 1–4 complete.
- c_api exists: create/destroy handles, distance/loss/indifference/PRNG operations via POD structs; error codes + message buffer; no STL across boundary.
- CI green; c_api tests pass; existing unit tests unchanged.
- Docs and API stability criteria in milestone_gates § c_api minimal are satisfied.

---

## References

- [Design Document](../architecture/design_document.md) — § FFI & Interfaces, § Integration Pattern, § c_api design inputs (Items 29–31)
- [Milestone gates](milestone_gates.md) — c_api minimal
- [Roadmap](roadmap.md) — near-term: c_api design
