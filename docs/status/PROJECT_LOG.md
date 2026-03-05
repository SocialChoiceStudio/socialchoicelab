# Project Log — SocialChoiceLab

Entries are in chronological order (oldest first, newest last).

---

## 2025-08-12
**What got done:**
- GitHub repo `socialchoicelab` created in the `SocialChoiceStudio` organization.
- Cursor Git settings configured for one-click commit+sync:
  - Enable Smart Commit → ON
  - Post Commit Command → `sync`
  - Confirm Sync → OFF
  - Autofetch → ON
- `docs` folder created with initial Living Design Document committed and synced.

**What's next:**
- Continue editing the Living Design Document:
  - Finalize terminology (`aggregation property` instead of `axiom`).
  - Clarify distance → loss → utility flow in Preference Services.
  - Expand layer descriptions for Foundation and Geometry Services.
- Prepare for Milestone 1: Finish Living Design Document enough to start coding Foundation and Geometry layers.

## 2025-09-10
**What got done:**
- Optimized distance functions: 4-8x performance improvement
- Renamed original distance_functions.h to old_distance_functions.h
- Created optimized distance_functions.h requiring explicit weights
- Performance: 1.3-2.0x slower than raw C++ (vs original 5-12x)
- Clean API: no default parameters, explicit weight requirements
- Updated test suite: test_distance_speed and test_distance_comparison
- Removed deprecated test files using old API
- Improved output formatting with proper column alignment

**Note:** Google Test was later integrated successfully via FetchContent (see 2025-09-11 and 2025-09-14); the test suite now uses GTest.

## 2025-09-11

**ARCHITECTURAL DECISION - Performance-First Approach:**
- **Problem**: Design document had conflicting guidance on geometry types (custom vs. CGAL)
- **Root Cause**: Design document listed custom types but also specified CGAL for exact geometry
- **User Priority**: "Speed everywhere possible, slow only when absolutely necessary"
- **Decision**: Use optimized libraries instead of custom implementations
- **New Architecture**: 
  - **Eigen vectors** (Vector2d, Vector3d, VectorXd) for fast numeric operations
  - **CGAL types** only when exact precision is absolutely necessary
  - **No custom geometry types** - they would be slower than optimized libraries
- **Rationale**: Eigen is SIMD-optimized and faster than our custom code; CGAL is well-tested for exact operations
- **Action Taken**: Updated design document to reflect performance-first approach, removed version numbers from living document
- **Next Steps**: Replace PointND with Eigen vectors, set up CGAL for exact operations when needed

**MIGRATION TO EIGEN COMPLETED:**
- **Decision**: User chose Eigen over PointND for performance and ecosystem benefits
- **Action Taken**: 
  - Updated distance_functions.h to use Eigen::MatrixBase instead of PointND
  - Migrated all distance function templates to work with Eigen vectors
  - Updated all test files to use Eigen::Vector2d instead of PointND
  - Removed PointND files from codebase (pointnd.h, test_pointnd.cpp)
  - Removed old distance_functions_eigen.h (consolidated into main file)
  - Updated CMakeLists.txt to remove PointND references
  - All tests passing with Eigen implementation
- **Result**: Successfully migrated to Eigen vectors while maintaining API compatibility
- **Performance**: Eigen provides SIMD optimizations and rich linear algebra features
- **Status**: ✅ **COMPLETE** - All 29 tests passing across 5 test suites

**CRITICAL LESSON LEARNED - Design Document Compliance:**
- **Problem**: Spent significant time implementing custom Segment2D and Polygon2D classes that directly violate the design document
- **Root Cause**: Assistant failed to read design document before starting work, despite having access to it
- **Design Doc Says**: "Perform exact geometry for spatial computations using CGAL" and "Core C++: pure C++ (CGAL/Eigen)"
- **What We Did Wrong**: Wrote custom geometry classes instead of using CGAL as specified
- **Impact**: Wasted several hours of development time
- **Prevention**: Always read design document and project log FIRST before starting any work
- **Action Taken**: Deleted all custom geometry classes, reverted CMakeLists.txt to clean state

## 2025-09-14
**CLEANUP AND STABILIZATION:**
- **Dropbox Sync Issues**: Project opened on different machine revealed Dropbox sync artifacts
- **Cleanup Actions**:
  - Removed leftover CMakeLists_full.txt and CMakeLists_simple.txt (from earlier mistakes)
  - Deleted old_distance_functions.h (referenced deleted PointND)
  - Removed old versioned design docs (v1.1.md, v1.2.md) - keeping only living document
  - Verified all 29 tests still passing after cleanup
- **Git Management**: Successfully committed and pushed clean state to GitHub
- **Project Structure**: Confirmed project is stable and ready for continued development
- **Status**: ✅ **STABLE** - All tests passing, clean codebase, properly versioned

## 2026-02-14
**PHASE 0 STABILIZATION (consensus plan):**
- **Removed broken or dead code:** Deleted `utility_functions.h` (referenced removed PointND and wrong APIs). Deleted broken examples (`basic_usage.cpp`, `distance_example.cpp`) and orphan `tests/CMakeLists.txt`, `examples/CMakeLists.txt`.
- **Loss functions:** Fixed sign convention (loss functions return positive magnitude; `distance_to_utility` returns `-loss`). Fixed `normalize_utility` to accept all loss parameters (max_loss, steepness, threshold) for correct GAUSSIAN/THRESHOLD normalization.
- **Build/tests:** Added `test_loss_functions` to root CMakeLists.txt. Removed redundant Eigen include line from CMake (Eigen comes from `Eigen3::Eigen` target).
- **PRNG:** Edge-case guards—`uniform_choice(0)` and `discrete_choice(empty)` throw `std::invalid_argument`; `beta()` validates parameters and guards division by zero. Renamed `beta()` second parameter to `beta_param` to avoid shadowing.
- **Distance:** Minkowski `order_p` must be ≥ 1 (throws otherwise).
- **Lint/format:** Fixed `lint.sh` find precedence; added `--strict` mode for CI. Added `make format` and `make lint` CMake targets; updated Development_Guide.md.
- **Eigen migration:** Now fully complete (no remaining PointND or utility_functions.h).
- **Status**: Phase 0 complete; 6 test suites, all passing.

**Phase 1 — Reproducibility (stream seeds):**
- **Stream seed hashing:** Replaced `std::hash<std::string>` in `generate_stream_seed()` with FNV-1a (64-bit). Rationale: `std::hash` is implementation-defined and produces different values across GCC/Clang/MSVC, so the same master seed and stream names would yield different RNG sequences on different toolchains. FNV-1a is deterministic and portable, so stream seeds are identical everywhere for the same inputs. Note: stream seed values changed from pre–Phase 1 builds; reproducibility is consistent from this point forward. See Doxygen in `stream_manager.h` for the in-code explanation.
- **SIOF fix:** Converted file-scope statics in `stream_manager.cpp` to Meyers' singleton.
- **Test determinism:** Replaced `std::random_device` with fixed seeds in 4 test files.
- **Error-condition tests:** Added `EXPECT_THROW` tests for distance, PRNG, and loss error paths.
- **Missing includes:** Added `<stdexcept>`, `<type_traits>`, `<vector>`, `<numeric>` where needed for portability.
- **Enum hardening:** Unknown enum values in `calculate_distance` and `distance_to_utility` now throw instead of silently falling back to defaults.
- **StreamManager ownership (Item 18):** Adopted single-owner policy. Removed mutex and all `std::lock_guard` from `StreamManager` (the old mutex provided false safety — `get_stream()` returns a reference that outlives the lock). Removed `const` overload of `get_stream()` (it silently mutated via `mutable` members, violating const semantics). Removed `mutable` from member declarations. Added `reset_for_run(master_seed, run_index)` for deterministic per-run seeding. Updated stream naming convention (`voters`, `candidates`, `tiebreak`, `movement`, `memory_update`, `analysis`). The global singleton's construction mutex in `stream_manager.cpp` is retained (it guards construction only, not concurrent StreamManager use). Full decision rationale: `docs/architecture/stream_manager_design.md`.

**Test framework:** Google Test (GTest) is in use via CMake FetchContent; it is the project’s test framework. **Eigen migration:** Complete; no PointND or utility_functions.h remains (see 2025-09-11 and 2026-02-14).
## 2026-03-03
**What got done:**
- Removed GitButler and all associated workflow tooling (`squash-and-push.sh`, `delete-outdated-branches.sh`, `scripts/end-of-session.sh`, `scripts/end-of-milestone.sh`, `scripts/check-setup.sh`, `docs/governance/Squash_Push_Merge_Cleanup_Workflow.md` (file has since been removed)).
- Removed GitButler hooks from `.cursor/hooks.json`.
- Removed git version control from local directory (`.git` deleted).
- GitHub repository cleared. Git workflow to be re-established separately.

## 2026-03-04

**Phase 2 complete** (documentation truthfulness, Items 22–27): README updated, stale PointND references removed, PROJECT_LOG chronological order fixed, reference index cleaned, design doc Rcpp→cpp11.

**Phase 3 complete** (developer experience, Items 28–33): CI workflow (Ubuntu + macOS, clang-format 21, format/build/test/lint), roadmap.md, milestone_gates.md, CONTRIBUTING/SECURITY/CHANGELOG, .clang-tidy, pre-commit hook, dependency sequencing.

**Backlog Item 34**: CMakeLists.txt modernized — target_include_directories(), target_compile_options() with generator expressions, install() targets, removed unused C language, removed redundant gtest linking.

**C++ standard set to C++20**: Confirmed GTest 1.14, Eigen 3.4, Clang 17, and GCC 13 (CI) all support C++20. All 40 project tests pass. Recorded in development.md to prevent silent reversion.

**Backlog Items 35–38**: (35) Unified namespace style to C++17 nested form in all 5 files. (36) Extracted `k_default_master_seed` constant; replaced all `12345` literals. (37) Special-cased p=1 and p=2 in `minkowski_distance` to avoid `std::pow` in tight loop. (38) DRY: `minkowski_distance` calls `chebyshev_distance()` for high-p case; extracted `k_minkowski_chebyshev_cutoff = 100.0` constant with forward declaration for two-phase lookup.

**Backlog Items 39–45**: (39) Indentation verified consistent; format run. (40) Removed commented dead code from `distance_functions.h`. (41) Added `noexcept` to simple accessors in PRNG and StreamManager. (42) Skipped — `<algorithm>` required for `std::max` in threshold_loss. (43) Fixed Doxygen `@tparam N` on all distance wrapper functions. (44) Added WeightedEuclideanAndChebyshev and ThreeDimensionalPoints tests. (45) Added TriangleInequalityEqualWeights test. All 45 backlog items resolved; item 46 (GPL vs LGPL) deferred.

## 2026-03-05

**Documentation restructure**: Deleted 5 orphaned/stale files (Phase2Changes.md, 4 empty reference subdirectory READMEs). Flattened `docs/references/` — moved reference_index.md and implementation_priority.md from `social_choice/` subdir to `references/` directly; removed empty subdirectories. Updated docs/README.md layout, roadmap.md near-term, consensus_plan.md footer, stream_manager_design.md status.

**File naming conventions established**: All docs in `docs/` renamed to `snake_case.md` (10 files; except README.md files which stay as-is). Root hygiene docs remain `ALL_CAPS.md`. C++ files already consistent (snake_case.h/.cpp, test_snake_case.cpp). Scripts use kebab-case.sh. Convention documented in `docs/development/development.md` § File Naming Conventions and in `.cursor/rules/File-Naming-Conventions.mdc`. All 16 cross-reference files updated in one pass; zero broken links remain.

**Documentation content review**: Corrected stale content across all docs — design_document.md (loss functions list, benchmark description, consensus_plan file reference); milestone_gates.md (ALL_CAPS filenames in prose, Phase 3 marked complete); consensus_plan.md (added Status column to Phase 3 and Backlog table headers); where_we_are.md (chronological session order, last updated date).
