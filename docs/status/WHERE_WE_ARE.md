# Where We Are

**Single source for "what's next" so any agent on any machine can answer correctly.**

- **Current phase:** Batch 1
- **Next item:** 1 — **Minkowski Doxygen formula vs code mismatch** — The Doxygen…. Severity: Critical.
- **Last updated:** 2026-03-05

**Authority:** This file is a cached pointer. The **Status** column in `docs/status/consensus_plan_3.md` is the source of truth. When you complete an item: (1) mark it ✅ Done in the plan, (2) update this file. If this file and the plan disagree, the plan wins — fix this file.

**Rule for agents:** When the user asks "where are we" or "what's next", read this file and `docs/status/consensus_plan_3.md`. Use the plan's Status column to confirm; if they disagree, update this file.
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
- Doc updates: README (Implemented/Planned), PROJECT_LOG (chronological + Feb 14), development.md (style, structure, install), foundation README (no PointND).

**Phase 1 (all complete)**
- Item 12: FNV-1a in `generate_stream_seed()` (deterministic cross-platform).
- Item 13: SIOF fix in `stream_manager.cpp` (Meyers' singleton).
- Item 14: Fixed seeds in tests (no `std::random_device`); relaxed ratio for high-p in `test_distance_speed`.
- Item 15: Error-condition tests (`EXPECT_THROW`) for invalid inputs across distance, loss, and PRNG functions.
- Item 16: Missing includes (`loss_functions.h`, `stream_manager.h`, `test_distance_speed.cpp`).
- Item 17: Unknown enum throws in `calculate_distance` and `distance_to_utility`; tests updated.
- Item 18: Single-owner StreamManager policy adopted and implemented. See `docs/architecture/stream_manager_design.md`.
- Item 19: Tautological/empty tests fixed.
- Item 20: `beta_param` rename confirmed already done.
- Item 21: Timing assertions removed from benchmarks; CTest `benchmark` label added.

**Phase 2 (all complete)**
- Item 22: README updated (Implemented / Planned separation).
- Item 23: Stale PointND references removed from docs.
- Item 24: PROJECT_LOG chronological order fixed.
- Item 25: Reference index structure repaired.
- Item 26: development.md style guide verified correct.
- Item 27: Design doc Rcpp → cpp11 fix.

### Session: 2026-03-03 — Git/GitButler recovery; docs reorganization

- Removed GitButler. Git history was corrupted; files recovered from working tree.
- Reorganized docs into `docs/status/`, `docs/architecture/`, `docs/development/` subdirectories.
- Deleted duplicates (`agent_discussions/`), corrupted pre-recovery file, and empty stubs.

### Session: 2026-03-04 — CI workflow (Item 28)

- Added `.github/workflows/ci.yml`: runs on push and pull_request to `main`.
- Matrix: `ubuntu-latest`, `macos-latest`. Steps: install deps (cmake, clang-format, cpplint), format check (./lint.sh format then git diff --exit-code), configure & build, ctest -LE benchmark, ./lint.sh --strict lint.
- Marked Item 28 ✅ Done in CONSENSUS_PLAN; next item 29 (roadmap.md).

### Session: 2026-03-04 — ROADMAP (Item 29)

- Created `docs/status/roadmap.md`: near-term (1–2 months), mid-term (3–6 months), long-term (6+ months). Links to Design Document and Implementation Priority Guide; does not duplicate. Dependency order stated (c_api before bindings, geometry before voting rules).
- Added ROADMAP to docs index (`docs/README.md`). ROADMAP is reviewed at end-of-milestone (see `scripts/end-of-milestone.sh`).
- Marked Item 29 ✅ Done in CONSENSUS_PLAN; next item 30 (milestone gates).

### Session: 2026-03-04 — Milestone gates (Item 30)

- Created `docs/status/milestone_gates.md`: definition-of-done for Phase 3 complete, c_api minimal, Geometry + voting (mid-term), First binding / 1.0. Each milestone has gates for features, tests, docs, API stability.
- Linked from ROADMAP and docs index. Marked Item 30 ✅ Done in CONSENSUS_PLAN; next item 31 (CONTRIBUTING/SECURITY/CHANGELOG).

### Session: 2026-03-04 — CONTRIBUTING, SECURITY, CHANGELOG + docs consolidation (Item 31)

- Added `CONTRIBUTING.md`, `SECURITY.md`, `CHANGELOG.md` at project root. Linked from docs index.
- Docs consolidation: slimmed `docs/references/social_choice/README.md` to a short pointer; reference_index.md is the single source for the reference library. Added note in `docs/references/README.md` about subdir scope. Updated `docs/README.md` with root docs in table and layout, Phase2Changes note, and update rule to prefer single source of truth.
- Marked Item 31 ✅ Done in CONSENSUS_PLAN; next item 32 (.clang-tidy and pre-commit).

### Session: 2026-03-04 — .clang-tidy and pre-commit hook (Item 32)

- Added `.clang-tidy`: FormatStyle file, focused checks (bugprone-*, readability-*, modernize-use-nullptr/override, performance-*). Used by end-of-milestone.sh.
- Added `scripts/pre-commit.sh`: runs `./lint.sh format`, fails if include/src/tests/unit changed. Documented install in development.md (cp to .git/hooks/pre-commit).
- Marked Item 32 ✅ Done in CONSENSUS_PLAN; next item 33.

### Session: 2026-03-04 — Dependency sequencing in ROADMAP (Item 33)

- Added "Dependency sequencing" subsection to ROADMAP: c_api before bindings, geometry primitives before advanced electoral methods, foundation before new layers.
- Marked Item 33 ✅ Done. Phase 3 (Items 28–33) complete.

### Session: 2026-03-04 — CMake modernization (Item 34)

- CMakeLists.txt: removed C language; replaced global include_directories() with target_include_directories(socialchoicelab_core PUBLIC include); replaced global CMAKE_CXX_FLAGS_* with target_compile_options() and generator expressions ($<CONFIG:Debug/Release>); added install(TARGETS) and install(DIRECTORY) for library and headers; test targets link only gtest_main (redundant gtest removed). Kept CMAKE_CXX_STANDARD 17 at project level. Build and project tests verified.
- Marked Item 34 ✅ Done; next Backlog item 35.

### Session: 2026-03-04 — Backlog 35 & 36

- **Item 35:** Unified namespace style to C++17 nested form in all 5 files: `prng.h`, `stream_manager.h`, `stream_manager.cpp`, `loss_functions.h`, `distance_functions.h`.
- **Item 36:** Added `k_default_master_seed` in `prng.h`; used in `prng.h`, `stream_manager.h`, `stream_manager.cpp`, and `test_prng.cpp`.
- Marked 35 and 36 ✅ Done in CONSENSUS_PLAN; next item 37.

### Session: 2026-03-04 — Backlog 37, 39, 40, 41, 43, 44, 45 (not 38, 46)

- **37:** Special-cased p=1 and p=2 in minkowski_distance (no std::pow in loop); epsilon-based comparison.
- **39:** Indentation verified consistent; format run.
- **40:** Removed commented dead code (type aliases) from distance_functions.h.
- **41:** Added noexcept to PRNG::master_seed(), StreamManager::master_seed(), StreamManager::size(). Not state_string() (can throw).
- **42:** Skipped — \<algorithm\> is required for std::max in threshold_loss.
- **43:** Fixed Doxygen @tparam N → Derived1, Derived2, T on minkowski, euclidean, manhattan, chebyshev, calculate_distance.
- **44:** Added WeightedEuclideanAndChebyshev and ThreeDimensionalPoints tests.
- **45:** Added TriangleInequalityEqualWeights test (Euclidean, Manhattan, Chebyshev, Minkowski p=1.5).
- Marked 37, 39, 40, 41, 43, 44, 45 Done; 38 and 46 skipped per user request.

### Session: 2026-03-04 — Backlog 38 (DRY minkowski/Chebyshev)

- **38:** For order_p > k_minkowski_chebyshev_cutoff, minkowski_distance now calls chebyshev_distance() instead of duplicating the max loop. Extracted threshold to k_minkowski_chebyshev_cutoff = 100.0 with comment that it is a heuristic; revisit if a formal bound is needed. Added forward declaration for chebyshev_distance. Marked 38 ✅ Done; backlog complete (46 deferred).

### Session: 2026-03-05 — Documentation restructure + file naming conventions

- Deleted 5 orphaned/stale files: `docs/status/Phase2Changes.md`, and 4 empty subdirectory READMEs under `docs/references/` (social_choice, foundation, geometry, development).
- Moved `reference_index.md` and `implementation_priority.md` from `docs/references/social_choice/` to `docs/references/` (flat structure; subdirs removed).
- Rewrote `docs/references/README.md` to reflect new flat layout.
- Updated `docs/README.md`: removed Phase2Changes entry and stale subdirectory tree; updated layout block; added "Where to Put a New Doc" section.
- Updated `docs/status/roadmap.md`: near-term now reflects backlog done → c_api + CGAL start; all links updated to new paths; table formatting fixed.
- Updated `docs/status/consensus_plan.md`: removed stale "Next step: work through Phase 0" footer.
- Updated `docs/architecture/stream_manager_design.md`: status changed from "Proposed — for adoption" to "Adopted".
- Renamed 10 doc files to `snake_case.md` (all docs in `docs/` except `README.md` files); updated all cross-references in 16 files.
- Added file naming convention to `docs/development/development.md` § File Naming Conventions.
- Added `.cursor/rules/File-Naming-Conventions.mdc` Cursor rule for agents.

### Session: 2026-03-05 — Item 20 (distance input validation)

- **Item 20:** Distance functions now validate inputs: `order_p` must be finite (when T is floating point); each coordinate and salience weight must be finite; salience weights must be ≥ 0 (zero allowed for dimension masking). `std::invalid_argument` thrown with precise messages. Validation in `minkowski_distance` and `chebyshev_distance`; euclidean/manhattan inherit via minkowski.
- Doxygen updated for all four distance functions and `calculate_distance`: salience weights "finite and ≥ 0; zero means that dimension is ignored (dimension masking)."
- Design doc: distance bullet now states salience weights (finite, ≥0; zero = dimension masking) and invalid inputs throw.
- Tests: `NonFiniteAndInvalidInputsThrow` (NaN/Inf order_p, NaN/Inf coordinates, NaN/Inf/negative weights), `ZeroSalienceWeightAllowed` (dimension masking). Marked Item 20 ✅ Done in consensus_plan.md.

### Session: 2026-03-05 — Items 21 & 22 (loss and PRNG input validation)

- **Item 21:** Loss functions now validate: sensitivity > 0 (linear, quadratic, threshold); max_loss > 0, steepness > 0 (gaussian); threshold >= 0 (threshold_loss); finite distance/params when T is floating point; in normalize_utility, finite utility and max_distance, max_distance >= 0. All throw std::invalid_argument with clear messages.
- **Item 22:** PRNG wrappers now validate: uniform_int min <= max; uniform_real min < max and finite; normal stddev > 0 and finite; exponential lambda > 0 and finite; gamma alpha, beta > 0 and finite; bernoulli 0 <= probability <= 1 and finite. beta() already had guards. All throw std::invalid_argument.
- Tests: LossFunctionsTest::InvalidLossParametersThrow; PRNGTest::InvalidDistributionParametersThrow. Marked Items 21 and 22 ✅ Done. Next item: 23.

### Session: 2026-03-05 — Items 23–27 (test coverage gaps)

- **Item 23:** StreamManager tests: HasStream, ResetStream, RemoveStream, Clear, DebugInfo, ResetForRunReproducibility (same master+run_index → same sequence), ResetForRunDifferentIndicesDifferentSequences.
- **Item 24:** PRNG statistical tests: ExponentialMean (mean = 1/λ), BetaHappyPath (values in (0,1), mean α/(α+β)), GammaHappyPath (mean = α×β for std::gamma_distribution).
- **Item 25:** Skip test now uses engine() directly: 10 draws from rng1.engine(), rng2.engine().discard(10), then EXPECT_EQ(rng1.engine()(), rng2.engine()()).
- **Item 26:** MinkowskiAtChebyshevCutoffBoundary test (p = 100 and p = 100.1 match Chebyshev). Invalid-input EXPECT_THROW coverage was already in place from Items 20–21.
- **Item 27:** SetGlobalStreamManagerSeedSecondCallProducesDifferentSequences — call set_global_stream_manager_seed twice with different seeds, verify second call yields different sequence.
- Marked 23–27 ✅ Done. Next item: 28.

### Session: 2026-03-05 — Item 28 (normalize_utility epsilon + numeric tolerance)

- **Item 28:** Added `include/socialchoicelab/core/numeric_constants.h` with `k_near_zero_rel` (1e-9) and `k_near_zero_abs` (1e-12) as single source of truth for "negligible" in float comparisons. Scale is computed locally at each use (no global space size). `normalize_utility` now uses relative+absolute tolerance for degenerate range and returns 1.0 when range ≤ tol; Doxygen documents degenerate case. Integer T unchanged (exact equality).
- **Docs:** `docs/development/development.md` § Numeric tolerance describes convention (PEP 485 style; scale local). `numeric_constants.h` documents usage.
- **Test:** NormalizeUtilityDegenerateRangeReturnsOne (tiny max_distance triggers degenerate path → 1.0). Marked Item 28 ✅ Done.

### Session: 2026-03-05 — Items 29–31 (c_api design inputs)

- Items 29–31 are design inputs for when the c_api is built, not current implementation tasks. **Documented** in `docs/architecture/design_document.md` § c_api design inputs (table with 29: SCS_LossConfig; 30: register_streams consideration; 31: do not expose engine()).
- **No C++ code changes.** Consensus plan updated to point to the design doc. **Decisions to make later:** (30) Add `register_streams` to C++ StreamManager now, or only enforce at c_api boundary? (31) Remove `PRNG::engine()` from public C++ API (would require changing the skip test) or keep it and only exclude from c_api?

### Session: 2026-03-05 — Items 30 & 31 implemented and documented

- **Item 30:** Implemented in C++. `StreamManager::register_streams(names)` sets an optional allowlist; when set, `get_stream(name)` and `reset_stream(name, seed)` throw `std::invalid_argument` for names not in the set. `register_streams({})` clears the allowlist. Documented in `stream_manager.h`, `stream_manager_design.md`, and design_document.md table; c_api will enforce same at boundary.
- **Item 31:** No code change. `PRNG::engine()` remains in C++ API; Doxygen note added: internal/test use only, do not expose via c_api. Design doc table updated with decision.
- **Tests:** `RegisterStreamsThenGetUnknownThrows`, `RegisterStreamsThenGetAllowedWorks`, `RegisterStreamsEmptyClearsAllowlist` in `test_prng.cpp`.

### Session: 2026-03-05 — Items 32–35 (best-practice check, implemented)

- **32 & 33:** SplitMix64-style finalizer applied in `stream_manager.h`: new `finalize_splitmix64()` used in `combine_seed()` and `generate_stream_seed()`. Improves avalanche; deterministic and portable. (Stream seed values change vs pre-32/33 code; same code version remains reproducible.)
- **34:** CMake: compiler-ID guard added. `if(MSVC)` uses `/Zi /Od /W4` (Debug) and `/O2 /DNDEBUG` (Release); else GCC/Clang use `-g -O0 -Wall -Wextra` and `-O3 -DNDEBUG`. Single `SocialChoiceLab_compile_options` used for core library and all test targets.
- **35:** Empty `SetUp()` overrides removed from `PRNGTest` and `StreamManagerTest` in `test_prng.cpp` (GTest best practice: omit when no per-test setup needed).
