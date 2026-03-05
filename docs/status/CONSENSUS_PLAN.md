# Consensus Plan — SocialChoiceLab Second Review

**Date:** 2026-03-05  
**Reviewers:** Claude Opus 4.6 (independent), ChatGPT Codex (independent)  
**Rounds to consensus:** 3  
**Status:** CONSENSUS REACHED — both agents agree on all items below.

The first review (46 items, all completed) is archived in `docs/status/project_log.md` (session 2026-02-14 and 2026-03-04).

---

## How to use this plan

This is the actionable backlog produced by the second independent review of SocialChoiceLab. Work through batches in order. Batch 1 contains live bugs that will break the workflow if any script is run. Batches 2–3 can be parallelized. Batch 4 items should be done before or during c_api design work.

For each item: the **Origin** column shows which agent(s) identified it (Claude = C, Codex = X, Both = B).

---

## Findings summary

| Finding | Detail |
|---------|--------|
| Correctness bugs in library source | **None.** Previous consensus plan was well-executed. |
| Broken scripts | 2 (lint.sh, pre-commit.sh) — live bugs |
| Script that corrupts docs if run | 1 (update-where-we-are.sh) — live bug |
| CI/docs policy divergence | 1 (benchmark test exclusion) |
| Stale documentation references | 7 specific inaccuracies |
| Missing input validation | All 3 modules (distance, loss, PRNG) — High for c_api |
| Test coverage gaps | 16 specific untested behaviours |

---

## Batch 1 — Script fixes (do first; live bugs)

**Severity: High.** These are broken right now. Do not run `end-of-session.sh` or `lint.sh <file>` until fixed.

| # | Item | File | Origin | Severity | Status |
|---|------|------|--------|----------|--------|
| 1 | `lint.sh` TARGET argument silently ignored — `TARGET` initialized to `"."` before the parse loop; `[ -z "$TARGET" ]` is always false so the file/directory argument can never override TARGET. `./lint.sh format include/foo.sh` formats ALL files. **Fix:** Initialize `TARGET=""` before the loop; set default `"."` after (the fallback at lines 32–34 already exists). | `lint.sh` line 14 | B | High | ✅ Done |
| 2 | `pre-commit.sh` ROOT wrong when installed as git hook — `ROOT="$(cd "$(dirname "$0")/.." && pwd)"` evaluates to `.git/` when the script is at `.git/hooks/pre-commit` (per documented install instructions), making `./lint.sh` unreachable. **Fix:** `ROOT="$(git rev-parse --show-toplevel)"` — always correct regardless of where the script lives. | `scripts/pre-commit.sh` line 10 | B | High | ✅ Done |
| 3 | `update-where-we-are.sh` corrupts `where_we_are.md` when backlog is complete — Python parser falls through to `print("Phase 0 (Immediate Stabilization)|0|No items found|Unknown")` and exits non-zero; bash then overwrites `where_we_are.md` header with "Next item: 0 — No items found." Since the backlog IS complete, this is a live bug. **Fix:** When all items are ✅ Done, emit `"All phases complete|—|See docs/status/roadmap.md|—"` and exit 0. Handle the sentinel in bash to write a proper "all done" pointer instead of overwriting the header. | `scripts/update-where-we-are.sh` lines 72–73; `scripts/end-of-session.sh` | B | High | ✅ Done |

---

## Batch 2 — Documentation truthfulness

**Severity: Medium** (unless noted). Inaccuracies or stale references in docs and code comments. Fast to fix in one pass.

| # | Item | File | Origin | Severity | Status |
|---|------|------|--------|----------|--------|
| 4 | `stream_manager.h` stale doc path — line 31 comment says `docs/governance/StreamManager_Design.md`; file was moved and renamed to `docs/architecture/stream_manager_design.md`. **Fix:** Update to `docs/architecture/stream_manager_design.md`. | `include/socialchoicelab/core/rng/stream_manager.h` line 31 | B | High | ✅ Done |
| 5 | `stream_manager.h` orphaned Doxygen block — Doxygen comment for `generate_stream_seed()` (lines 165–177) sits above `combine_seed()` (lines 178–186), not above `generate_stream_seed()` (line 197). Doxygen associates it with the wrong function. **Fix:** Move the block to immediately above `generate_stream_seed()`. | `include/socialchoicelab/core/rng/stream_manager.h` lines 165–177 | C | Medium | ✅ Done |
| 6 | `distance_functions.h` Doxygen claims salience weights are optional — four `@param salience_weights` blocks say "optional" or "default: equal weights" but the parameter is mandatory with no default argument. **Fix:** Remove "optional/default" wording from all four blocks (lines 52, 134, 153, 173). Replace with "Salience weights for each dimension (mandatory, one per dimension)." | `include/socialchoicelab/preference/distance/distance_functions.h` lines 52, 134, 153, 173 | B | High | ✅ Done |
| 7 | `stream_manager_design.md` stale reference — section D3 says "Update `SESSION_PROGRESS.md`"; file does not exist (renamed to `project_log.md`). **Fix:** Change to `project_log.md`. | `docs/architecture/stream_manager_design.md` | C | Medium | ✅ Done |
| 8 | `git_reference.md` "not a git repo" — line 9 says "This directory is not currently a git repo." Repo is active on `main` tracking `origin/main`. **Fix:** Replace with conditional wording: "If this directory is not yet a git repo, run the following setup commands. If it is already initialized, skip to [Workflow]." | `docs/development/git_reference.md` line 9 | B | Medium | ✅ Done |
| 9 | `git_reference.md` branching policy stale — says "Revisit at Phase 3 completion." Phase 3 is complete; no update was made. **Fix:** "Phase 3 complete. Branching deferred until the project has collaborators or a stable API to protect. Revisit at c_api milestone." | `docs/development/git_reference.md` line 163 | C | Low | ✅ Done |
| 10 | `git_reference.md` legacy undo command — `git checkout -- .` is outdated. **Fix:** Replace with `git restore .`; keep legacy form in a note for older Git versions. | `docs/development/git_reference.md` lines 175–178 | X | Low | ✅ Done |
| 11 | `project_log.md` not chronological — 2026-03-03 entry precedes older 2025 entries. **Fix:** Reorder all entries ascending by date. Add an explicit ordering rule in the file header. | `docs/status/project_log.md` | X | Medium | ✅ Done |
| 12 | `development.md` include-guard wording mismatches practice — style guide says "include guards" but project uses `#pragma once` throughout. **Fix:** Update to "self-contained, using `#pragma once` (project standard)." | `docs/development/development.md` | X | Medium | ✅ Done |
| 13 | `sample_simulation_description.md` presents Option A/B as active choices — single-owner policy (Option B) was adopted in the first plan Item 18, but the doc still frames both as open. **Fix:** Convert Option A/B section into a historical note and state the adopted policy near first mention. | `docs/development/sample_simulation_description.md` lines 216–225 | X | Low | ✅ Done |
| 14 | `test_distance_comparison.cpp` "Raw C++ C++" typo — "Raw C++ C++ Minkowski distance" doubled at lines 53, 85, 86, 115. **Fix:** Change all to "Raw C++ Minkowski distance." | `tests/unit/test_distance_comparison.cpp` lines 53, 85, 86, 115 | C | Medium | ✅ Done |
| 15 | `development.md` lint.sh usage examples document behaviour that doesn't work — targeted formatting examples have never worked due to the Item 1 TARGET bug. **Fix:** After fixing Item 1, verify all documented examples in lines 26–31 work as written. | `docs/development/development.md` lines 26–31 | C | Medium | ✅ Done |
| 16 | `stream_manager.cpp` locking Doxygen unclear — `get_global_stream_manager()` acquires a construction mutex and returns a reference after releasing it; can be misread as implying the returned manager is thread-safe. **Fix:** Add explicit Doxygen note: "Lock guards lazy construction only. The returned `StreamManager&` is not thread-safe; single-owner contract applies." | `src/core/rng/stream_manager.cpp` lines 30–39 | X | Medium | ✅ Done |

---

## Batch 3 — Build and CI alignment

**Severity: Mixed.** Item 17 is High (CI is running tests it should exclude). Items 18–19 are Medium/Low.

| # | Item | File | Origin | Severity | Status |
|---|------|------|--------|----------|--------|
| 17 | CI runs benchmark tests via direct executables — runs `./test_distance_speed` and `./test_performance_comparison` directly, contrary to documented policy (`ctest -LE benchmark`). Also means new tests added to CMakeLists.txt are never picked up by CI automatically. **Fix:** Replace test step with `cd build && ctest --output-on-failure -LE benchmark`. | `.github/workflows/ci.yml` line 63 | B | High | ✅ Done |
| 18 | `CMakeLists.txt` VERSION 1.0.0 is premature — `milestone_gates.md` defines 1.0 as "first released R or Python package" with semver stability promise. **Fix:** Change to `VERSION 0.1.0`. Increment to 1.0.0 when first-binding milestone gates are met. | `CMakeLists.txt` line 2 | B | Medium | ✅ Done |
| 19 | CMakeLists.txt duplicate C++ standard setting — set globally (lines 5–7) and again on the target via `set_target_properties` (lines 47–49). **Fix:** Remove lines 47–49. | `CMakeLists.txt` lines 47–49 | C | Low | ✅ Done |

---

## Batch 4 — Input validation and test coverage

**Severity: High** (validation items), **Medium** (test items). Implement during or before c_api design — the c_api must map C++ errors to error codes, so C++ must throw on invalid input rather than silently producing NaN.

### Input validation

| # | Item | File | Origin | Severity | Status |
|---|------|------|--------|----------|--------|
| 20 | Distance functions accept non-finite/invalid inputs — `order_p`, coordinates, and salience weights are not checked for NaN, infinity, or negative values. Non-integer `p` with negative weighted differences passed to `std::pow` can silently produce NaN. **Fix:** Before computation in `minkowski_distance` (propagate to wrappers): `std::isfinite(order_p)` and `order_p >= 1`; each coordinate finite; each salience weight finite and `>= 0`. Throw `std::invalid_argument` with precise messages. (Whether zero weights are permitted — dimension masking — is a design decision; document it explicitly.) | `include/socialchoicelab/preference/distance/distance_functions.h` | B | High | ✅ Done |
| 21 | Loss functions do not enforce documented parameter domains — Doxygen specifies positive domains but none are enforced. Negative `sensitivity` inverts utility semantics. **Fix:** Add `std::invalid_argument` guards: `sensitivity > 0`; `max_loss > 0`; `steepness > 0`; `threshold >= 0`; finite numeric inputs; in `normalize_utility`, require finite `utility` and `max_distance >= 0`. | `include/socialchoicelab/preference/loss/loss_functions.h` | B | High | ✅ Done |
| 22 | PRNG wrappers forward invalid distribution parameters to STL without checking — STL behaviour with invalid params is implementation-defined (assert, exception, or silent nonsense). **Fix:** Add explicit guards for each wrapper: `min <= max` (integer); `min < max` (real); `stddev > 0`; `lambda > 0`; gamma shape/scale > 0; `0 <= probability <= 1`. Throw `std::invalid_argument`. | `include/socialchoicelab/core/rng/prng.h` | X | High | ✅ Done |

### Test coverage gaps

| # | Item | File | Origin | Severity | Status |
|---|------|------|--------|----------|--------|
| 23 | `StreamManager` methods have zero test coverage — `reset_stream()`, `has_stream()`, `remove_stream()`, `clear()`, `debug_info()`, `reset_for_run()` untested. `reset_for_run()` is especially important: verify same `(master_seed, run_index)` always produces same sequences; different indices produce different sequences. | `tests/unit/test_prng.cpp` | C | Medium | ✅ Done |
| 24 | PRNG distribution happy paths untested — `gamma()` has zero tests. `beta()` only tests error conditions. `exponential()` only checks `>= 0`. **Fix:** Add statistical tests: sample N values, check mean/range within tolerance (same pattern as existing Bernoulli/Normal tests). | `tests/unit/test_prng.cpp` | C | Medium | ✅ Done |
| 25 | `PRNG::skip()` test only verifies no crash — checks output is in range, which passes even if `skip()` did nothing. **Fix:** Test engine position directly: draw 10 values from `rng1.engine()`, discard 10 on `rng2.engine()`, compare `rng1.engine()() == rng2.engine()()`. | `tests/unit/test_prng.cpp` lines 168–197 | C | Medium | ✅ Done |
| 26 | No tests for invalid-input edge cases — NaN/Inf coordinates, negative/zero salience weights, invalid loss params, `minkowski_distance` at exactly `p = k_minkowski_chebyshev_cutoff` (100.0) boundary, `normalize_utility` degenerate case. **Fix:** Once Items 20–22 are done, add corresponding `EXPECT_THROW` tests for each. | `tests/unit/test_distance_functions.cpp`, `tests/unit/test_loss_functions.cpp` | B | Medium | ✅ Done |
| 27 | `set_global_stream_manager_seed` second-call branch untested — function has two paths (create new / reset existing); only the create path is exercised. **Fix:** Call twice with different seeds; verify second call produces different sequences. | `tests/unit/test_prng.cpp` | C | Low | ✅ Done |
| 28 | `normalize_utility` uses exact floating-point equality — `if (max_utility == min_utility)` is fragile; returning `T{1.0}` on zero range is undocumented. **Fix:** Use epsilon comparison; document the degenerate-case convention; add a test. | `include/socialchoicelab/preference/loss/loss_functions.h` line 184 | C | Medium | ✅ Done |

---

## c_api design inputs (not implementation tasks — reference when designing c_api)

**Captured in** `docs/architecture/design_document.md` § c_api design inputs. No C++ code changes; apply when implementing the c_api. Items 30 and 31 have optional decisions (see design doc).

| # | Topic | Detail | Origin |
|---|-------|--------|--------|
| 29 | `distance_to_utility` parameter explosion | Six parameters, most conditionally used. C has no default arguments. **Recommended:** `SCS_LossConfig` POD struct with type discriminant + parameter union. | C |
| 30 | Stream auto-creation vs pre-registration | `get_stream()` silently creates a stream for any name — typos create decoupled streams with no error. **Consider:** `register_streams(names)` to lock the allowed set; throw on unknown names at c_api boundary. | C |
| 31 | Do not expose `PRNG::engine()` through c_api | Returns non-const ref to mt19937_64, bypassing reproducibility guarantees. Keep internal; consider removing if no internal code needs it. | C |

---

## Deferred — Style/preference (Items 32–35 implemented per best-practice check)

| # | Item | Origin | Note | Status |
|---|------|--------|------|--------|
| 32 | `generate_stream_seed` hash-mixing strength — XOR combining of master_seed and name_hash weaker than SplitMix64. Not a correctness issue. | C | Optional optimization | ✅ Done |
| 33 | `combine_seed` mixing — custom XOR-multiply sequence; SplitMix64 finalizer gives better avalanche. | C | Optional optimization | ✅ Done |
| 34 | CMake compiler flags not guarded by compiler-ID — `-g -O0 -Wall -O3` are GCC/Clang-only. Not a live issue. | C | Add if Windows CI planned | ✅ Done |
| 35 | Empty `SetUp()` overrides in test fixtures. | C | Minor cleanup | ✅ Done |

---

## Severity breakdown

| Severity | Count |
|----------|-------|
| High | 8 (Items 1–4, 6, 17, 20–22) |
| Medium | 14 (Items 5, 7–8, 11–12, 14–16, 18, 23–26, 28) |
| Low | 6 (Items 9–10, 13, 19, 27) |
| c_api design inputs | 3 (Items 29–31) |
| Deferred | 4 (Items 32–35) |
| **Total actionable** | **28** |

---

## Recommended implementation order

1. **Items 1–3** — broken scripts. Fix before running any automation.
2. **Items 4–16** — documentation sweep. One session.
3. **Items 17–19** — CI and build alignment. One session.
4. **Items 20–22** — input validation. Before or during c_api design.
5. **Items 23–28** — test coverage. In parallel with or after validation.
6. **Items 29–31** — c_api design inputs. Reference at start of c_api design work, not implementation tasks now.
7. **Items 32–35** — deferred. Whenever convenient.
