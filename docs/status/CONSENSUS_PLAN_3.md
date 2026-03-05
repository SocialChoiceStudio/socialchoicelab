# Consensus Plan — SocialChoiceLab Third Review

**Date:** 2026-03-05  
**Reviewers:** Claude Opus 4.6 (independent), ChatGPT Codex (independent)  
**Rounds to consensus:** 3  
**Status:** CONSENSUS REACHED — both agents agree on all items below.

The second review (35 items, all completed) is archived in `docs/status/project_log.md` (session 2026-03-05). Source review files: `agent_conversations/claude_r02.md`, `codex_r02.md`, `codex_r03.md`.

---

## How to use this plan

Work through batches in order. Batch 1 (Critical) and Batch 2 (High) must be done before c_api design begins. Batch 3 (Medium) can run in parallel with c_api planning. Batch 4 (Low) whenever convenient.

For each item: the **Origin** column shows which agent identified it (Claude = C, Codex = X, Both = B).

---

## Findings summary

| Finding | Detail |
|---------|--------|
| Correctness issues | 1 Critical (Minkowski Doxygen convention mismatch) |
| Missing input validation | 3 (discrete_choice, beta isfinite, get_stream contract) |
| Type-safety gaps | 4 (minkowski integral T, PRNG static_asserts, gaussian_loss, uniform_int bool) |
| Documentation gaps | 8 (threshold default, get_stream double-lookup, tolerance, copy/move, normalize_utility, etc.) |
| Low/hygiene items | 10 (Doxygen wording, lint, changelog, CMake, CI, etc.) |

---

## Batch 1 — Critical (fix before c_api)

**Severity: Critical.** These create incorrect behavior or silent data corruption; must be resolved before c_api design work begins.

| # | Item | File | Origin | Severity | Status |
|---|------|------|--------|----------|--------|
| 1 | **Minkowski Doxygen formula vs code mismatch** — The Doxygen formula documents Convention A (weight applied inside the `p`-th root) but the implementation uses Convention B (weight applied to each dimension before the norm, i.e., dimension pre-scaling). Any caller reading the docs and implementing their own inverse computation will get wrong results. **Fix:** Update all four distance function Doxygen blocks to show Convention B explicitly: `d = (Σ wᵢ |xᵢ - yᵢ|^p)^(1/p)`; add a note in `design_document.md` documenting the rationale for choosing Convention B. | `include/socialchoicelab/preference/distance/distance_functions.h` | B | Critical | ✅ Done |
| 2 | **`discrete_choice` all-zero / negative / NaN weights** — Passing all-zero weights, any negative weight, or NaN/Inf weight to `std::discrete_distribution` is undefined behavior (or implementation-defined silent corruption). **Fix:** Before constructing the distribution, validate: all weights ≥ 0; finite when floating-point; at least one weight > 0. Throw `std::invalid_argument` otherwise. Add tests: all-zero throws; single negative throws; NaN/Inf weight throws; single positive among zeros returns that index. | `include/socialchoicelab/core/rng/prng.h` | B | Critical | ✅ Done |
| 3 | **`get_stream()` reference invalidation undocumented** — `get_stream()` returns `PRNG&`. Any call to `reset_all()`, `clear()`, `remove_stream()`, or destruction of the `StreamManager` invalidates all references previously returned. This contract is completely undocumented; callers who store the reference will have a use-after-invalidation bug (and it is not obvious this is a risk). **Fix:** Add a class-level Doxygen warning and per-method notes on `get_stream()`, `reset_all()`, `clear()`, and `remove_stream()`. Optionally add a reference-stability note or test. | `include/socialchoicelab/core/rng/stream_manager.h` | B | Critical | ✅ Done |

---

## Batch 2 — High (fix before c_api)

**Severity: High.** Type-safety or validation gaps that will silently produce wrong results at runtime without these guards.

| # | Item | File | Origin | Severity | Status |
|---|------|------|--------|----------|--------|
| 4 | **`minkowski_distance` accepts integral `T`** — When `T` is an integer type, fractional exponents in Minkowski computation silently truncate, producing wrong results with no compile-time error. **Fix:** Add `static_assert(std::is_floating_point_v<T>, "minkowski_distance requires a floating-point type")` (propagate to wrappers). | `include/socialchoicelab/preference/distance/distance_functions.h` | B | High | |
| 5 | **PRNG distribution templates lack type-safety** — `uniform_int<T>` can be instantiated with floating-point `T`; `uniform_real`, `normal`, `exponential`, `gamma`, `beta` can be instantiated with integer `T`. Both silently compile and produce garbage results. **Fix:** Add `static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>)` on `uniform_int`; add `static_assert(std::is_floating_point_v<T>)` on `uniform_real`, `normal`, `exponential`, `gamma`, `beta`. | `include/socialchoicelab/core/rng/prng.h` | B | High | |
| 6 | **`gaussian_loss` integer `T` truncation** — The existing `static_assert` on `gaussian_loss` does not restrict to floating-point types, so integer `T` compiles and silently truncates intermediate results. **Fix:** Tighten to `static_assert(std::is_floating_point_v<T>)`. | `include/socialchoicelab/preference/loss/loss_functions.h` | B | High | |
| 7 | **`uniform_int` accepts `bool`** — `std::uniform_int_distribution<bool>` is not well-defined. **Fix:** Add `!std::is_same_v<T, bool>` to the `uniform_int` static_assert (fold into Item 5). | `include/socialchoicelab/core/rng/prng.h` | C | High | |
| 8 | **`beta()` missing `std::isfinite` checks** — All other PRNG wrappers with floating-point parameters validate finiteness before forwarding to the STL distribution; `beta()` does not. NaN/Inf `alpha` or `beta_param` can silently produce NaN output. **Fix:** Add `isfinite` guards matching the other wrappers. Add NaN/Inf parameter tests. | `include/socialchoicelab/core/rng/prng.h` | B | High | |
| 9 | **Signed/unsigned narrowing in distance functions** — Loop indices in distance functions mix `int` and Eigen's signed index type (`Eigen::Index` = `ptrdiff_t`). On 64-bit platforms with large vectors this causes signed/unsigned comparison warnings and potential narrowing. **Fix:** Use `Eigen::Index` consistently for all loop variables that index into Eigen vectors. | `include/socialchoicelab/preference/distance/distance_functions.h` | B | High | |

---

## Batch 3 — Medium

**Severity: Medium.** Documentation gaps, minor inefficiencies, and missing test coverage. Safe to do in parallel with c_api planning.

| # | Item | File | Origin | Severity | Status |
|---|------|------|--------|----------|--------|
| 10 | **`docs/README.md` precedence list numbering + duplicate rule** — The precedence list is missing step 3 (skips from 2 to 4) and contains a duplicate "update" rule. **Fix:** Insert the missing step 3; remove the duplicate rule. | `docs/README.md` | X | Medium | |
| 11 | **`sample_simulation_description.md` missing per-run seeding** — Step 7 ("Re-run simulation") does not mention `reset_for_run(master_seed, run_index)`. A reader following the guide would not know they must re-seed per run for reproducibility. **Fix:** Add one sentence at Step 7 explaining the call. | `docs/development/sample_simulation_description.md` | B | Medium | |
| 12 | **`distance_to_utility` THRESHOLD default trap** — The default `threshold = 0.5` in `distance_to_utility` (and `normalize_utility`) silently applies even when callers pass `LossFunctionType::THRESHOLD` without reading the signature. A call like `distance_to_utility(d, THRESHOLD, sensitivity)` uses 0.5 with no warning. **Fix:** Add a Doxygen `@note` warning: for `THRESHOLD` loss, always pass `threshold` explicitly; the default 0.5 may not match the model. | `include/socialchoicelab/preference/loss/loss_functions.h` | B | Medium | |
| 13 | **`get_stream()` double hash-table lookup** — After inserting a new stream, `get_stream()` calls `streams_.find(name)` a second time to get the iterator. **Fix:** Use `try_emplace` or capture the return value of `emplace`/insert directly to avoid the redundant lookup. | `include/socialchoicelab/core/rng/stream_manager.h` | C | Medium | |
| 14 | **Test tolerance 1e-15 too tight** — `test_distance_comparison.cpp` asserts absolute differences < 1e-15. This is tighter than `double` machine epsilon (≈2.2e-16) for non-trivial sums and can produce spurious failures across compilers or FP modes. **Fix:** Relax to 1e-12 or use a relative-tolerance comparison. | `tests/unit/test_distance_comparison.cpp` | B | Medium | |
| 15 | **PRNG copy/move semantics untested and undocumented** — `PRNG` contains an `mt19937_64` engine; copy semantics are implicitly defined but never tested. Callers may accidentally copy a PRNG (duplicating its state) or assume it is non-copyable. **Fix:** Either add tests that verify copy produces an independent identically-seeded stream, or explicitly `= delete` copy operations and document that PRNG is move-only. | `include/socialchoicelab/core/rng/prng.h` | C | Medium | |
| 16 | **Unused namespace alias in `test_performance_comparison.cpp`** — `namespace original_distance = socialchoicelab::preference::distance;` is declared but never used. **Fix:** Remove. | `tests/unit/test_performance_comparison.cpp` | X | Medium | |
| 17 | **`normalize_utility` integer `T` behavior undocumented** — For integer `T`, the result is 0 or 1 (binary clipping), not a continuous [0,1] value. This is surprising and undocumented. **Fix:** Add Doxygen note: "For integer `T`, result is 0 or 1; [0,1] normalization is meaningful only for floating-point types." Also document that callers must provide consistent inputs (utility ≤ max possible utility); behavior is algebraically correct but output may exceed [0,1] if they don't. | `include/socialchoicelab/preference/loss/loss_functions.h` | B | Medium | |

---

## Batch 4 — Low / doc hygiene

**Severity: Low.** Minor wording, style, or optional improvements. No urgency.

| # | Item | File | Origin | Severity | Status |
|---|------|------|--------|----------|--------|
| 18 | **Gamma `@param beta` says "Rate parameter"** — `std::gamma_distribution` uses (shape, scale), not (shape, rate). The Doxygen says "Rate parameter (β)" which is incorrect. **Fix:** Change to "Scale parameter (scale = 1/rate)". | `include/socialchoicelab/core/rng/prng.h` | B | Low | |
| 19 | **`lint.sh` unquoted variable** — One or more variables in `lint.sh` are unquoted, risking word-splitting on paths with spaces. **Fix:** Quote or use an array. | `lint.sh` | B | Low | |
| 20 | **CHANGELOG placeholder text** — The `[Unreleased]` section still has "(Add entries here as relevant.)" placeholder lines mixed with real entries. **Fix:** Remove the placeholder lines; keep only real entries. | `CHANGELOG.md` | B | Low | |
| 21 | **`stream_manager_design.md` stale source file references** — Part VII references source file names (e.g. line numbers in `.h`/`.cpp`) that have changed since the document was written. **Fix:** Remove or update the stale references to match current file structure. | `docs/architecture/stream_manager_design.md` | B | Low | |
| 22 | **CMake: add `CMAKE_EXPORT_COMPILE_COMMANDS`** — `compile_commands.json` is not generated, so clangd/LSP tools and `clang-tidy` cannot resolve headers without manual setup. **Fix:** Add `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)` to `CMakeLists.txt`. | `CMakeLists.txt` | X | Low | |
| 23 | **CI: no caching, no Windows** — CI downloads GTest and Eigen on every run. Windows is not tested. **Fix:** Add `actions/cache` for CMake and FetchContent downloads. Add Windows runner when needed. | `.github/workflows/ci.yml` | B | Low | |
| 24 | **`reset_stream` Doxygen missing behavior note** — The doc does not say what happens when the named stream does not yet exist. **Fix:** Add one-line note: "If the stream does not exist, it is created with the given seed." | `include/socialchoicelab/core/rng/stream_manager.h` | X | Low | |
| 25 | **`state_string()` Doxygen missing throw note** — `state_string()` builds a `std::string` via concatenation and can throw on allocation failure, but is not documented as potentially throwing. **Fix:** Add `@throws std::bad_alloc` or a note that string allocation may throw. | `include/socialchoicelab/core/rng/prng.h` | C | Low | |
| 26 | **`stream_names()` Doxygen missing order note** — The doc does not say the order of returned names is unspecified (it comes from `unordered_map`). **Fix:** Add: "Order of names is unspecified." | `include/socialchoicelab/core/rng/stream_manager.h` | C | Low | |

---

## Test coverage additions

Do these alongside the items they correspond to.

| Test | Alongside item | File |
|------|---------------|------|
| `discrete_choice`: all-zero weights throw | Item 2 | `tests/unit/test_prng.cpp` |
| `discrete_choice`: single negative weight throws | Item 2 | `tests/unit/test_prng.cpp` |
| `discrete_choice`: NaN/Inf weight throws | Item 2 | `tests/unit/test_prng.cpp` |
| `discrete_choice`: single positive among zeros returns that index | Item 2 | `tests/unit/test_prng.cpp` |
| `beta()`: NaN alpha/beta throw | Item 8 | `tests/unit/test_prng.cpp` |
| `register_streams` → `reset_stream` rejection (unregistered name) | Existing | `tests/unit/test_prng.cpp` |
| PRNG copy produces independent identically-seeded stream (or confirm = delete) | Item 15 | `tests/unit/test_prng.cpp` |
| 1D Minkowski distance (N=1 vector) | Item 4 | `tests/unit/test_distance_functions.cpp` |
| `calculate_distance` with MINKOWSKI + order_p=1.0 and order_p=2.0 | Item 4 | `tests/unit/test_distance_functions.cpp` |
| `Construction` test: widen uniform range to avoid 1/101 false collision | Item 5 | `tests/unit/test_prng.cpp` |
| StreamManager reference-stability (get_stream ref survives non-mutating ops) | Item 3 | `tests/unit/test_prng.cpp` |

---

## Documentation / general follow-ups

| Topic | File | Note |
|-------|------|------|
| Layers not yet implemented | `docs/architecture/design_document.md` | One-line forward-looking note (geometry, voting rules, etc. are planned, not built) |
| Running tests from build dir | `docs/development/development.md` | Mention `ctest -LE benchmark` alongside existing build instructions |
| README test suite description | `README.md` | Update unit test count / description if significantly changed |

---

## Notes on prior items

| Prior item | Status | Note |
|------------|--------|------|
| Item 8 (claude_r02): `consensus_plan.md` file path | **Intentionally deleted** | User deleted the completed second review plan; this item is moot. |
| Item 20 (claude_r02): `where_we_are.md` authority wording | **Already done** | Updated this session to reflect consensus plan deletion. |

---

## Severity breakdown

| Severity | Count |
|----------|-------|
| Critical | 3 (Items 1–3) |
| High | 6 (Items 4–9) |
| Medium | 8 (Items 10–17) |
| Low | 9 (Items 18–26) |
| Test additions | 11 |
| Doc follow-ups | 3 |
| **Total actionable** | **26** |

---

## Recommended implementation order

1. **Items 1–3** (Critical) — before any c_api work. One focused session.
2. **Items 4–9** (High) — same session or immediately after.
3. **Items 10–17** (Medium) — in parallel with c_api planning.
4. **Items 18–26** (Low) + doc follow-ups — whenever convenient.
5. **Test additions** — alongside the item they correspond to; do not defer.
