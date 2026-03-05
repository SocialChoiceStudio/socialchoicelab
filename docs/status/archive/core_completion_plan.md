# Core Completion Plan

Step-by-step plan to complete the **C++ core** (Layer 1 Foundation + Layer 2 Preference Services) as defined in the [Design Document](../../architecture/design_document.md). This document does not cover the c_api, geometry (Layer 3), or bindings.

**Authority:** This plan is the source of truth for "what's left to complete the core." When an item is done, mark it in the Status column and update [where_we_are.md](../where_we_are.md) if it affects "what's next."

---

## Scope: What the core consists of

| Layer | Component | Design doc description | Status |
|-------|-----------|------------------------|--------|
| **1. Foundation** | core::rng | Seeded PRNG, multiple streams | ✅ Done |
| | core::types | Eigen vectors (Vector2d, Vector3d, VectorXd), CGAL when needed | ✅ Done (core/types.h) |
| | core::kernels | CGAL kernel policy, numeric kernels | Deferred (Layer 3) |
| | core::linalg | Eigen linear algebra | ✅ Done (core/linalg.h) |
| | core::serialization | Deferred (see Step 4) | Deferred |
| **2. Preference** | distance | Minkowski, Euclidean, Manhattan, Chebyshev, Convention B | ✅ Done |
| | loss | Linear, quadratic, Gaussian, threshold; distance_to_utility, normalize_utility | ✅ Done |
| | utility | Applies loss to distance | ✅ Done (via loss) |
| | indifference | Level-set construction; stateless service | ✅ Done |

**Deferred (not part of core completion):** core::kernels (needed for CGAL in Layer 3). Geometry, profiles, voting, simulation, c_api, and bindings are outside this plan.

---

## Current status summary

- **Done:** distance, loss, utility (via loss), PRNG, StreamManager, numeric_constants. All have tests, validation, and docs.
- **Done:** Steps 1–3. Step 4 (serialization) deferred; core complete without it.

---

## Step-by-step plan

Work in order. Each step ends with tests and doc updates; mark the step ✅ in the table when complete.

| Step | Task | Rationale | Status |
|------|------|-----------|--------|
| **1** | **Types and linalg** | Design doc lists core::types and core::linalg. Today we use Eigen directly in distance/loss with no separate module. **Option A:** Add a short `core/types.h` (or `core/point_types.h`) that re-exports or documents the canonical types (e.g. "use Eigen::VectorXd for points; Vector2d/3d for fixed-dim"). **Option B:** Document in the design doc that "Eigen types used in distance/loss constitute core::types; no separate header." Same for linalg: either a thin wrapper or "satisfied by Eigen use in existing code." Decide and implement or document. | ✅ |
| **2** | **Indifference service — specify** | Layer 2 lists "indifference – Level-set construction; stateless service." No API is specified yet. **Deliverable:** Design the API (in design_document.md or a short design note). Typical level-set: given ideal point, utility level, loss config, and distance config, return a representation of { x : u(x) = level } (e.g. in 1D two points; in 2D a curve or sampled points). Decide inputs (ideal, utility value, loss type + params, distance type + weights, dimension), outputs (e.g. points on the level set, or a curve type), and whether this is Eigen-only for now or CGAL-ready. | ✅ |
| **3** | **Indifference service — implement** | Implement the specified indifference (level-set) service in C++, with unit tests. Prefer stateless functions; use existing distance and loss APIs. | ✅ |
| **4** | **Serialization (optional)** | Design doc: "core::serialization – Protobuf for data exchange (planned; not yet implemented)." **Option A:** Add a minimal Protobuf-based serialization path for core data (e.g. stream seed state, or a small config struct) and document it. **Option B:** Explicitly defer: add a line in the design doc that serialization is post-core or tied to ModelConfig, and leave implementation for a later milestone. | Deferred |

---

## Definition of done for "core complete"

- All Step 1–3 items are done (and Step 4 either done or explicitly deferred in the design doc).
- Layer 1 (except deferred kernels and optionally serialization) and Layer 2 are implemented and documented.
- CI green; new code has unit tests; design doc and this plan are updated.
- [milestone_gates.md](../milestone_gates.md) and [roadmap.md](../roadmap.md) can then treat "C++ core complete" as reached before c_api minimal.

---

## Dependencies and order

1. **Step 1** can be done anytime; it does not block the rest.
2. **Step 2** must be done before Step 3.
3. **Step 3** depends on Step 2 and on existing distance/loss (done).
4. **Step 4** is independent; can be done or deferred in parallel with 1–3.

---

## References

- [Design Document](../../architecture/design_document.md) — Layered Architecture § 1–2, Design Principles
- [Implementation Priority](../../references/implementation_priority.md) — Phase 1 (distance implemented; geometry is Layer 3)
- [Roadmap](../roadmap.md) — Near-term: c_api design; this plan completes the core first
- [Milestone gates](../milestone_gates.md) — c_api minimal assumes a "current C++ core" to wrap
