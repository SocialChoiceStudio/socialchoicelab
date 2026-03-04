# Where We Are

**Single source for "what's next" so any agent on any machine can answer correctly.**

- **Current phase:** Phase 3 (Developer Experience and Process)
- **Next item:** 32 — Add `.clang-tidy` config and pre-commit hooks for automated quality enforcement. Severity: Medium.
- **Last updated:** 2026-03-04

**Authority:** This file is a cached pointer. The **Status** column in `docs/status/CONSENSUS_PLAN.md` is the source of truth. When you complete an item: (1) mark it ✅ Done in CONSENSUS_PLAN.md, (2) update this file (next item = first row in CONSENSUS_PLAN not marked Done; update Last updated date). If this file and the plan disagree, the plan wins — fix this file.

**Rule for agents:** When the user asks "where are we" or "what's next", read this file and CONSENSUS_PLAN.md. Use the plan's Status column to confirm; if they disagree, update this file.

---

## Recent Work

### Session: 2026-02-14 — Phase 0 & Phase 1 complete

**Phase 0 (all complete)**
- Cleanup: removed dead/broken files (examples, orphan CMake, `utility_functions.h`); updated consensus plan.
- Item 1–2: Loss sign convention fixed; `normalize_utility` accepts all loss params.
- Item 5: `test_loss_functions` added to build.
- Item 6: Dead `tests/CMakeLists.txt` and `examples/CMakeLists.txt` removed.
- Item 7: Eigen include line removed from CMake.
- Item 8: PRNG edge-case guards (`uniform_choice(0)`, `discrete_choice(empty)`, `beta` division-by-zero).
- Item 9: Minkowski `order_p >= 1` validation.
- Item 10: `lint.sh` find fix + `--strict` mode.
- Item 11: `make format` / `make lint` CMake targets; docs updated.
- Doc updates: README (Implemented/Planned), PROJECT_LOG (chronological + Feb 14), DEVELOPMENT.md (style, structure, install), foundation README (no PointND).

**Phase 1 (all complete)**
- Item 12: FNV-1a in `generate_stream_seed()` (deterministic cross-platform).
- Item 13: SIOF fix in `stream_manager.cpp` (Meyers' singleton).
- Item 14: Fixed seeds in tests (no `std::random_device`); relaxed ratio for high-p in `test_distance_speed`.
- Item 15: Error-condition tests (`EXPECT_THROW`) for invalid inputs across distance, loss, and PRNG functions.
- Item 16: Missing includes (`loss_functions.h`, `stream_manager.h`, `test_distance_speed.cpp`).
- Item 17: Unknown enum throws in `calculate_distance` and `distance_to_utility`; tests updated.
- Item 18: Single-owner StreamManager policy adopted and implemented. See `docs/architecture/StreamManager_Design.md`.
- Item 19: Tautological/empty tests fixed.
- Item 20: `beta_param` rename confirmed already done.
- Item 21: Timing assertions removed from benchmarks; CTest `benchmark` label added.

**Phase 2 (all complete)**
- Item 22: README updated (Implemented / Planned separation).
- Item 23: Stale PointND references removed from docs.
- Item 24: PROJECT_LOG chronological order fixed.
- Item 25: Reference index structure repaired.
- Item 26: DEVELOPMENT.md style guide verified correct.
- Item 27: Design doc Rcpp → cpp11 fix.

### Session: 2026-03-03 — Git/GitButler recovery; docs reorganization

- Removed GitButler. Git history was corrupted; files recovered from working tree.
- Reorganized docs into `docs/status/`, `docs/architecture/`, `docs/development/` subdirectories.
- Deleted duplicates (`agent_discussions/`), corrupted pre-recovery file, and empty stubs.

### Session: 2026-03-04 — CI workflow (Item 28)

- Added `.github/workflows/ci.yml`: runs on push and pull_request to `main`.
- Matrix: `ubuntu-latest`, `macos-latest`. Steps: install deps (cmake, clang-format, cpplint), format check (./lint.sh format then git diff --exit-code), configure & build, ctest -LE benchmark, ./lint.sh --strict lint.
- Marked Item 28 ✅ Done in CONSENSUS_PLAN; next item 29 (ROADMAP.md).

### Session: 2026-03-04 — ROADMAP (Item 29)

- Created `docs/status/ROADMAP.md`: near-term (1–2 months), mid-term (3–6 months), long-term (6+ months). Links to Design Document and Implementation Priority Guide; does not duplicate. Dependency order stated (c_api before bindings, geometry before voting rules).
- Added ROADMAP to docs index (`docs/README.md`). ROADMAP is reviewed at end-of-milestone (see `scripts/end-of-milestone.sh`).
- Marked Item 29 ✅ Done in CONSENSUS_PLAN; next item 30 (milestone gates).

### Session: 2026-03-04 — Milestone gates (Item 30)

- Created `docs/status/MILESTONE_GATES.md`: definition-of-done for Phase 3 complete, c_api minimal, Geometry + voting (mid-term), First binding / 1.0. Each milestone has gates for features, tests, docs, API stability.
- Linked from ROADMAP and docs index. Marked Item 30 ✅ Done in CONSENSUS_PLAN; next item 31 (CONTRIBUTING/SECURITY/CHANGELOG).

### Session: 2026-03-04 — CONTRIBUTING, SECURITY, CHANGELOG + docs consolidation (Item 31)

- Added `CONTRIBUTING.md`, `SECURITY.md`, `CHANGELOG.md` at project root. Linked from docs index.
- Docs consolidation: slimmed `docs/references/social_choice/README.md` to a short pointer; REFERENCE_INDEX.md is the single source for the reference library. Added note in `docs/references/README.md` about subdir scope. Updated `docs/README.md` with root docs in table and layout, Phase2Changes note, and update rule to prefer single source of truth.
- Marked Item 31 ✅ Done in CONSENSUS_PLAN; next item 32 (.clang-tidy and pre-commit).
