# Consensus Plan — SocialChoiceLab Fourth Review

**Date:** 2026-03-05  
**Reviewers:** Claude Opus 4.6 (independent), ChatGPT Codex (independent)  
**Rounds to consensus:** 5 (r01_claude → r02_GPT → r03_claude → r04_GPT → r05_claude)  
**Status:** CONSENSUS REACHED — both agents agree on all items below.

The third review (26 items, all completed) is archived in `docs/status/consensus_plan_3.md`. Source review files: `agent_conversations/r01_claude.md`, `r02_GPT.md`, `r03_claude.md`, `r04_GPT.md`, `r05_claude.md`.

---

## How to use this plan

Work through batches in order. Batch 1 (High) must be done before c_api design begins. Batch 2 (Medium) can run in parallel with c_api planning. Batch 3 (Low) whenever convenient.

For each item: the **Origin** column shows which agent identified it (Claude = C, Codex = X, Both = B).

---

## Findings summary

| Finding | Detail |
|---------|--------|
| False dependency / include hygiene | 1 (dead Eigen/vector includes in loss_functions.h) |
| Tolerance convention violation | 1 (local epsilon instead of numeric_constants.h) |
| Compiler warning | 1 (signed/unsigned comparison in distance validation) |
| Type-safety gaps | 1 (unsigned + INT_MIN UB in loss templates; merged H-4 + NEW-H-1) |
| Documentation gaps / stale | 3 (where_we_are.md stale, distance_to_utility param table, design_document.md unbuilt layers) |
| Boundary / correctness | 1 (Minkowski cutoff `>` vs `>=` overflow at p=100) |
| API usability | 2 (missing [[nodiscard]], duplicate validation helper) |
| CI portability | 1 (Ubuntu pip PEP 668) |
| Low / hygiene items | 7 (C++20 policy, normalize recompute, CMake features, empty stream name, state_string, changelog, archived formula) |
| Test coverage gaps | 8 |
| Doc follow-ups | 5 |

---

## Batch 1 — High (fix before c_api)

**Severity: High.** Must be resolved before c_api design work begins.

| # | Item | File | Origin | Severity | Status |
|---|------|------|--------|----------|--------|
| 1 | **Dead includes in `loss_functions.h`** — `<Eigen/Dense>` and `<vector>` are included but neither is used anywhere in the file. All loss functions operate on scalar `T`. This creates unnecessary compile-time overhead and a false dependency on Eigen for any consumer of loss functions, complicating the future c_api module boundary. **Fix:** Remove both `#include <Eigen/Dense>` and `#include <vector>`. Verify the file still builds standalone (only `<algorithm>`, `<cmath>`, `<stdexcept>`, `<type_traits>`, and `"socialchoicelab/core/numeric_constants.h"` are needed). | `include/socialchoicelab/preference/loss/loss_functions.h` | C | High | ✅ Done |
| 2 | **Minkowski p=1/p=2 fast-path uses local epsilon** — `const T eps = static_cast<T>(1e-9)` is a magic constant that duplicates the value of `k_near_zero_rel` from `numeric_constants.h`, violating the single-source-of-truth convention established in the third review (Item 28). If the shared tolerance ever changes, this path is silently left behind. **Fix:** Replace the local constant with `static_cast<T>(socialchoicelab::core::near_zero::k_near_zero_rel)` and add `#include "socialchoicelab/core/numeric_constants.h"` to `distance_functions.h`. | `include/socialchoicelab/preference/distance/distance_functions.h` | C | High | ✅ Done |
| 3 | **Signed/unsigned comparison in distance validation** — `salience_weights.size()` (`std::vector::size_type`, unsigned) is compared to `N` (`Eigen::Index` = `ptrdiff_t`, signed) in the size-mismatch checks. This produces `-Wsign-compare` warnings under the project's own `-Wall -Wextra` flags, present in both `minkowski_distance` and `chebyshev_distance`. **Fix:** Cast before comparison: `static_cast<Eigen::Index>(salience_weights.size()) != N`. Or store as `const Eigen::Index n_weights = static_cast<Eigen::Index>(salience_weights.size())` and use `n_weights` throughout. | `include/socialchoicelab/preference/distance/distance_functions.h` | B | High | ✅ Done |
| 4 | **Loss function type constraints too permissive — unsigned types and signed INT_MIN UB** — `linear_loss`, `quadratic_loss`, and `threshold_loss` use `static_assert(std::is_arithmetic_v<T>)`, which accepts (a) unsigned types where `std::abs` is not defined, and (b) signed types where `std::abs(std::numeric_limits<T>::min())` is undefined behavior ([C++20 §24.2.1]: result not representable). Instantiating `linear_loss<unsigned int>(d, s)` or `linear_loss(INT_MIN, 1)` compiles silently and is UB. **Fix:** Restrict to floating-point types: `static_assert(std::is_floating_point_v<T>, "loss functions require a floating-point type")`. If integer support is intentionally retained, restrict to `std::is_signed_v<T>` and add an explicit guard: `if (distance == std::numeric_limits<T>::min()) throw std::invalid_argument(...)`. | `include/socialchoicelab/preference/loss/loss_functions.h` | B | High | ✅ Done |

---

## Batch 2 — Medium

**Severity: Medium.** Safe to do in parallel with c_api planning.

| # | Item | File | Origin | Severity | Status |
|---|------|------|--------|----------|--------|
| 5 | **`where_we_are.md` is stale** — The file reports "Current phase: Batch 2 / Next item: 4" but all 26 items in `consensus_plan_3.md` are marked ✅ Done. This violates the file's stated purpose as the single source of truth for "what's next." **Fix:** Update to reflect plan completion and point to `docs/status/roadmap.md` for next steps. The correct content is: "Current phase: All phases complete. Next item: All consensus items complete. See `docs/status/roadmap.md` for next steps." | `docs/status/where_we_are.md` | C | Medium | ✅ Done |
| 6 | **Minkowski Chebyshev cutoff uses `>` instead of `>=`** — The guard `order_p > k_minkowski_chebyshev_cutoff` means p=100.0 exactly still enters the general `std::pow` path. For coordinates with magnitudes above ~1580, `pow(x, 100)` overflows `double` (1580^100 ≈ 10^320; `DBL_MAX` ≈ 1.8×10^308), producing `inf`. The existing test `MinkowskiAtChebyshevCutoffBoundary` uses a point pair that avoids this range (max coord 1000), masking the hazard. The constant is named "cutoff," implying the cutoff value itself should route to Chebyshev. **Fix:** Change to `order_p >= static_cast<T>(k_minkowski_chebyshev_cutoff)`. Update or add a test that exercises p=100 with large coordinates (e.g., coords > 2000) to confirm no overflow. | `include/socialchoicelab/preference/distance/distance_functions.h` | B | Medium | ✅ Done |
| 7 | **No `[[nodiscard]]` on pure computation functions** — All distance functions, loss functions, `distance_to_utility`, `normalize_utility`, and all PRNG draw methods (`uniform_int`, `uniform_real`, `normal`, `exponential`, `gamma`, `beta`, `bernoulli`, `discrete_choice`, `uniform_choice`) return values that must be used. Silently discarding the return value is almost certainly a bug. C++17 (and C++20, which this project targets) `[[nodiscard]]` would catch this at compile time. **Fix:** Add `[[nodiscard]]` to all pure computation functions and PRNG draw methods. For templates, the attribute goes before the `template` keyword. | `distance_functions.h`, `loss_functions.h`, `prng.h` | C | Medium | ✅ Done |
| 8 | **CI `pip3 install` on Ubuntu may fail on PEP 668 runners** — The Ubuntu CI step uses `pip3 install --user cpplint || pip3 install cpplint`. On Ubuntu 24.04+ (which `ubuntu-latest` may map to), PEP 668 marks the system Python as externally managed and rejects both forms without `--break-system-packages`. The macOS step (line 57 in `ci.yml`) already uses `--break-system-packages`, but the Ubuntu step does not, creating an asymmetry that will cause CI failures when the runner image updates. **Fix:** Add `--break-system-packages` to the Ubuntu pip command, or replace with `pipx install cpplint` (the recommended approach for PEP 668 systems). | `.github/workflows/ci.yml` | C | Medium | ✅ Done |
| 9 | **`distance_to_utility` (and `normalize_utility`) Doxygen doesn't clarify which parameters are used per loss type** — The function takes 6 parameters after `distance`, but `sensitivity` is silently ignored for GAUSSIAN, and `max_loss`/`steepness` are silently ignored for LINEAR/QUADRATIC/THRESHOLD. A caller writing `distance_to_utility(d, LossFunctionType::GAUSSIAN, 5.0)` passes `5.0` as `sensitivity` — which is ignored — with no warning. **Fix:** Add a parameter-usage table to the Doxygen for both `distance_to_utility` and `normalize_utility`: which parameters are active for each `LossFunctionType`. | `include/socialchoicelab/preference/loss/loss_functions.h` | C | Medium | ✅ Done |
| 10 | **Duplicate validation logic in `minkowski_distance` and `chebyshev_distance`** — Both functions contain ~40 identical lines of input validation (dimension matching, empty/wrong-size salience weights, finite coordinates, finite salience weights, non-negative weights). When `minkowski_distance` routes to `chebyshev_distance` for high-p, validation runs twice. Any future change to validation must be applied in both places. **Fix:** Extract a private `validate_distance_inputs` helper template called from both functions. When minkowski routes to chebyshev internally, either skip re-validation or accept the minor overhead for simpler code. | `include/socialchoicelab/preference/distance/distance_functions.h` | B | Medium | ✅ Done |

---

## Batch 3 — Low / doc hygiene

**Severity: Low.** Minor wording, style, or optional improvements. No urgency.

| # | Item | File | Origin | Severity | Status |
|---|------|------|--------|----------|--------|
| 11 | **C++20 required but no C++20 features used** — `set(CMAKE_CXX_STANDARD 20)` is set but the codebase uses only C++17 features (`if constexpr`, nested namespaces, `std::is_floating_point_v`, etc.). No concepts, ranges, `std::format`, or other C++20 features are present. This unnecessarily restricts compatible compiler versions. **Fix:** Either begin using C++20 features to justify the requirement (e.g., concepts for template constraints, `[[nodiscard("reason")]]`), or lower to `CMAKE_CXX_STANDARD 17` until C++20 features are needed. | `CMakeLists.txt` | C | Low | Skipped (premature; C++20 will be used as we build out) |
| 12 | **`normalize_utility` recomputes bounds on every call** — Each call to `normalize_utility` recomputes `max_utility` (distance 0) and `min_utility` (max_distance) via two `distance_to_utility` calls. For batch normalization of N voters × M candidates, this does 2NM redundant calls. **Fix:** Consider a batch normalization API or a `UtilityNormalizer` struct that precomputes bounds once and provides a `normalize(T utility)` method. This also aligns with the planned c_api `SCS_LossConfig` struct (Item 29 from third review). | `include/socialchoicelab/preference/loss/loss_functions.h` | C | Low | ✅ Done |
| 13 | **CMake uses global `CMAKE_CXX_STANDARD` instead of target compile feature** — `CMAKE_CXX_STANDARD` is a default variable; it doesn't propagate to downstream consumers via `target_link_libraries`. **Fix:** Add (or replace with) `target_compile_features(socialchoicelab_core PUBLIC cxx_std_20)` so the C++ standard requirement propagates correctly to any target that links the library. | `CMakeLists.txt` | C | Low | ✅ Done |
| 14 | **`register_streams` accepts empty string as a valid stream name** — `register_streams({"", "voters"})` allows empty-string keys; `get_stream("")` would succeed. An empty stream name is almost certainly a caller bug. **Fix:** Optionally validate that no registered name is empty and throw `std::invalid_argument` with a descriptive message if one is. Or document that empty strings are permitted (if intentional). | `include/socialchoicelab/core/rng/stream_manager.h` | C | Low | ✅ Done |
| 15 | **`PRNG::state_string()` doesn't reflect engine position** — Returns `"PRNG(master_seed=N)"` regardless of how many draws have been made. Two PRNGs with the same seed but at different sequence positions return identical strings, limiting debug utility. **Fix:** Consider including a draw count, a sequence index, or a truncated hash of the engine state. Low priority since this is a debug aid only. | `include/socialchoicelab/core/rng/prng.h` | C | Low | ✅ Done |
| 16 | **`CHANGELOG.md` has empty Fixed section** — The `### Fixed` section contains `(None this release.)` per Keep a Changelog convention; an empty section should be omitted rather than filled with a placeholder. **Fix:** Remove `### Fixed` and its placeholder line. Re-add the section when there are actual fixes to record. | `CHANGELOG.md` | C | Low | ✅ Done (none present) |
| 17 | **Convention B formula in archived consensus plan is ambiguous** — `consensus_plan_3.md` Item 1 fix description shows `d = (Σ wᵢ |xᵢ - yᵢ|^p)^(1/p)` while claiming Convention B. This is the Convention A form (weight outside exponent). Convention B is `d = (Σ (wᵢ |xᵢ - yᵢ|)^p)^(1/p)`. The governing docs (design_document.md, code Doxygen) are correct; this is a cosmetic error in a completed archived plan. **Fix:** Correct the formula to `d = (Σ (wᵢ \|xᵢ - yᵢ\|)^p)^(1/p)` for archival accuracy. | `docs/status/consensus_plan_3.md` | X | Low | ✅ Done |

---

## Test coverage additions

Do these alongside the items they correspond to.

| Test | Alongside item | File |
|------|----------------|------|
| Minkowski distance at p=100 with coords > 2000: confirm no overflow (uses Chebyshev path after M-2 fix) | Item 6 | `tests/unit/test_distance_functions.cpp` |
| Loss functions reject unsigned `T` at compile time (verify `static_assert` fires) | Item 4 | `tests/unit/test_loss_functions.cpp` |
| Loss functions reject signed `T` with `INT_MIN` distance — throw `std::invalid_argument` (if guard retained) | Item 4 | `tests/unit/test_loss_functions.cpp` |
| `PRNG::skip(N)` wrapper: call `skip(10)`, compare next raw draw to engine after `discard(10)` | Item gap | `tests/unit/test_prng.cpp` |
| `reset_stream("typo", seed)` throws when allowlist is set | Item gap | `tests/unit/test_prng.cpp` |
| 1D Minkowski distance (N=1 Eigen vector): all four metrics | Item gap | `tests/unit/test_distance_functions.cpp` |
| `normalize_utility<int>()`: verify binary clipping behavior (integer T returns 0 or 1) | Item gap | `tests/unit/test_loss_functions.cpp` |
| `distance_to_utility(d, GAUSSIAN, 5.0)`: verify `sensitivity=5.0` is silently ignored, output matches default `sensitivity=1.0` | Item gap | `tests/unit/test_loss_functions.cpp` |

---

## Documentation follow-ups

| Topic | File | Note |
|-------|------|------|
| Mark Layers 3–8 as "planned, not yet built" | `docs/architecture/design_document.md` | One-line note per unimplemented layer so readers don't assume they exist |
| Remove or defer Protobuf mention | `docs/architecture/design_document.md` | `core::serialization – Protobuf` is aspirational with no current implementation or near-term plan; either note it as future or remove |
| Add `core/numeric_constants.h` to project structure tree | `docs/development/development.md` | Currently omitted from the "Public headers" layout |
| Consider splitting jargon tutorial from simulation walkthrough | `docs/development/sample_simulation_description.md` | Lines 70–277 are a process/thread/memory tutorial that may be clearer as a separate document |
| Update `docs/README.md` active backlog pointer | `docs/README.md` | Change "Active backlog (third review): `consensus_plan_3.md`" to fourth review once this plan is active |

---

## Items removed by consensus

| Proposed item | Reason removed | Round |
|--------------|----------------|-------|
| NEW-H-2: "Probabilistic/flaky single-draw `EXPECT_NE` tests" | Tests use fixed seeds; `std::mt19937_64` with fixed seed is fully deterministic. Not flaky. | R03 Claude argued, R04 GPT agreed |
| NEW-M-1: "Broken path casing in `where_we_are.md`" | File on disk is `consensus_plan_3.md` (lowercase), matching the reference exactly. No mismatch. | R03 Claude argued, R04 GPT agreed |

---

## Severity breakdown

| Severity | Count |
|----------|-------|
| High | 4 (Items 1–4) |
| Medium | 6 (Items 5–10) |
| Low | 7 (Items 11–17) |
| Test additions | 8 |
| Doc follow-ups | 5 |
| **Total actionable** | **30** |

---

## Recommended implementation order

1. **Items 1–4** (High) — one focused session before c_api design begins.
2. **Items 5–10** (Medium) — in parallel with c_api planning; start with Items 5 and 6 (stale doc and overflow fix).
3. **Items 11–17** (Low) + documentation follow-ups — whenever convenient.
4. **Test additions** — alongside the item they correspond to; do not defer.
