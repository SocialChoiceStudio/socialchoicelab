# Stage 03b — C API: geometry II and aggregation
Model / effort: Opus / high   Commit: e6772eb   Files reviewed: `src/c_api/scs_api.cpp` lines 1–285 (context), 2035–3575, 3787–4131 (about 1,890 lines in slice); the matching declarations in `include/scs_api.h`.
Not read: nothing in the slice was skipped. Heart functions (`scs_api.cpp:2513–2627`) were read but not reviewed, per Q1.

Scratch programs and outputs are in
`/private/tmp/claude-501/-Users-robiragan-Dropbox-SocialChoiceStudioDev/2402f97e-221e-494b-88cb-fc63c41efc50/scratchpad/stage3b/`
(`$S3B` below). Line numbers are at `e6772eb`.

## Summary
- **Counts:** 1 P0, 5 P1, 6 P2, 5 P3 (17 findings). All are demonstrated by running code, except S03b-005 and two sub-cases of S03b-002 (unambiguous logic).
- **Most important (S03b-001, P0):** `resolve_prng` runs outside the `try` block in all seven tie-breaking entry points. A mistyped stream name with `tie_break="random"` throws through the C boundary and aborts the process. Reproduced from C, Python and R.
- **Wrong results at the boundary:** `scs_uncovered_set_2d` returns the same index twice for duplicate alternatives (S03b-003). `scs_yolk_2d` returns NaN with `SCS_OK` for coordinates around 3e7 and above, and `scs_core_2d` (bound in R and Python) then fails on those valid inputs with a misleading message (S03b-004).
- **Integer validation is the weak spot:** `n * 2` is computed before `n` is checked at 11 entry points, plus 3 Heart ones (S03b-002, the SCP-003 pattern). Negative counts surface as a raw `"vector"` message with `SCS_ERROR_INTERNAL` (S03b-009).
- **What holds:** PR #1's checks work. In a sweep across 8 magnitudes up to 1.7e308, non-finite values never reached EPEC through `scs_core_2d` or the uncovered-set boundary. All 87 gtest cases are leak-free; the handle leak S03b-007 lies on error paths they do not exercise.

## Contracts this layer provides
For Stage 5 (bindings). "Checked" means rejected at the C boundary, not by the core.

**All functions in this slice**
- Return `SCS_OK` (0) or an error code; constructors return `SCS_Profile*` or `NULL` with `err_buf` set. Null `err_buf` and `err_buf_len <= 0` are safe.
- All indices are 0-based.
- Exceptions: `std::invalid_argument` → `SCS_ERROR_INVALID_ARGUMENT`; other `std::exception` → `SCS_ERROR_INTERNAL`. `bad_alloc` is `SCS_ERROR_OUT_OF_MEMORY` only in `scs_draw_voters` and the heap variant's `malloc`; elsewhere it is INTERNAL, or NULL for constructors.
- **Do not rely on "no exception escapes" until S03b-001 is fixed.** Error-message construction also runs outside `try` in validators, so an allocation failure while building a message could escape; no scenario beyond out-of-memory.
- No global or static mutable state in the slice (Q9). Handles (`SCS_Profile*`, `SCS_StreamManager*`) must not be used from two threads at once, even for reads. Tie-breaking and generators advance the stream they are given.

**Geometry (Yolk, Copeland, Condorcet, core, uncovered set, approval spatial)**
- Null pointers are checked. Coordinates are checked for finiteness, but only when the count is positive and `n * 2` does not overflow (S03b-002).
- Counts: `n_alts >= 1` only for Copeland; `n_voters >= 2` only for Yolk. Elsewhere negative values give INTERNAL `"vector"`, and zero is passed to the core (S03b-009).
- `dist_cfg`: `to_dist_config` (exactly 2 weights, finite, known type) everywhere except `scs_yolk_2d`, which checks only for NULL and ignores the config. Winset-based paths (`scs_core_2d`, `scs_uncovered_set_boundary_2d`) also reject zero, negative and non-finite weights (Q18); `order_p` is rejected by the core.
- `k`: any negative value means simple majority; 0 or above n is an error (S03b-014).
- Copeland: `out_len < n_alts` is INVALID_ARGUMENT. Winner ties go to the smallest index.
- `scs_condorcet_winner_2d`: `*out_winner_idx = -1` when not found. `scs_core_2d`: `*out_x = *out_y = 0` when not found, and `*out_found` is heuristic (S03b-015).
- `scs_uncovered_set_2d`: size query with `out_indices == NULL` or `out_capacity == 0`; `*out_n` is always written on success and on `BUFFER_TOO_SMALL`. Indices are wrong for duplicate alternatives (S03b-003).
- Boundary: `scs_uncovered_set_boundary_2d` size query likewise. `_heap`: `*out_xy = NULL`, `*out_n_pairs = 0` are set first; on success the caller frees with `scs_heap_free`; zero vertices gives NULL. A result may have 1 or 2 vertices (Q25). `_size_` delegates to the fill function and repeats the whole computation.

**Profiles and generators**
- `scs_profile_build_spatial`, `_uniform_spatial` and `_gaussian_spatial` accept `n_alts == 0`. `_from_utility_matrix` and `_impartial_culture` require >= 1. All four generators require `n_dims == 2` where they take it; `lo`, `hi`, `mean`, `stddev` are checked.
- Caller owns the returned handle (`scs_profile_destroy`, NULL no-op). `scs_profile_clone` is a deep copy.
- `scs_profile_get_ranking`: voter index checked; `out_len < n_alts` is INVALID_ARGUMENT. `scs_profile_export_rankings`: same, against `n_voters * n_alts`.
- `scs_draw_voters`: `out_len` too small is `BUFFER_TOO_SMALL` (unlike the others); unknown kind is rejected; `param1`/`param2` validation comes from `PRNG`.

**Voting rules and properties**
- Scores: `out_len < n_alts` is INVALID_ARGUMENT, not `BUFFER_TOO_SMALL` (S03b-013). `*_all_winners`, `scs_pareto_set`: size-query pattern via `fill_int_winners`.
- `tie_break` is not validated (S03b-008). With `SCS_TIEBREAK_RANDOM`, `mgr` and `stream_name` must be non-null or the call returns INVALID_ARGUMENT; the stream name must be registered or the process aborts (S03b-001).
- `scs_is_pareto_efficient` checks `alt_idx`. `scs_is_selected_by_*_consistent_rules` do not (S03b-006). `scs_condorcet_winner_profile`: `out_winner` may be NULL; it is left untouched when not found.
- Centrality: `n_voters >= 1`, coordinates finite; `scs_geometric_mean_2d` rejects non-positive coordinates through the core.

## Findings

### S03b-001 [P0, confirmed] Unregistered stream name throws through the C boundary and aborts the process
- **Where:** src/c_api/scs_api.cpp:3034 (`resolve_prng`, called outside `try`); call sites 3118, 3190, 3222, 3297, 3395, 3821, 3867
- **What:** `resolve_prng` calls `mgr->mgr.get_stream(stream_name)`, which throws `std::invalid_argument` when the manager has an allowlist and the name is not in it (`stream_manager.h:96–101`). All seven callers call it before their `try`, so nothing catches it. This breaks the design rule at `scs_api.cpp:7` and `c_api_design.md:60`, which lists an unregistered stream name under `SCS_ERROR_INVALID_ARGUMENT`. The call is eager, so it aborts even when no tie exists. The generators and `scs_draw_voters` call `get_stream` inside `try` and fail correctly.
- **Failure scenario:** `scl.plurality_one_winner(p, tie_break="random", stream_manager=sm, stream_name="typo")` with `sm = StreamManager(1, ["registered"])`:
  `libc++abi: terminating due to uncaught exception of type std::invalid_argument: StreamManager: stream name not in registered set (typo?): "typo"`, exit 134. Python's `try/except` cannot catch it and the interpreter dies. R behaves the same (`$S3B/t07_r_escape.R`, exit 134). All seven functions abort from C: `$S3B/t01_boundary.c`, output `$S3B/t01_boundary.out`, cases `esc_*`; Python: `$S3B/t04_py_escape.py`, output `$S3B/t04_py.out`.
- **Suggested fix:** Move each `resolve_prng` call inside the `try`, or catch inside `resolve_prng` and return an error code. Add a test per entry point (no test covers this path).
- **Also affects:** R and Python, for `scs_plurality_one_winner`, `scs_borda_one_winner`, `scs_borda_ranking`, `scs_antiplurality_one_winner`, `scs_scoring_rule_one_winner`, `scs_rank_by_scores`, `scs_pairwise_ranking_from_matrix`.

### S03b-002 [P1, confirmed] Array lengths are computed by multiplication before the counts are range-checked
- **Where:** src/c_api/scs_api.cpp — no check before `n * 2`: 2157, 2202 (`n_voters` in Copeland), 2244/2246, 2279/2281, 2319, 2365/2367, 2429, 2465, 2642/2644, 3433/3435, 3473/3475. Other products: 2685 (`n_voters * n_alts`), 2883 (`needed`), 2988 and 3000 (`required`, the write index), 3873 (`i * n_alts`). Positive overflow only, because a lower bound is checked first: 2054, 2155, 2200, 4046, 4079, 4113. Core: `uncovered_set.h:239` (`res * res`).
- **What:** The SCP-003 pattern, at other sites. `validate_finite_doubles` trusts `count`. With `n = -1073741825`, `n * 2` is signed overflow and in practice wraps to +2147483646, so the check scans about 2 billion doubles past a small buffer. Large positive products wrap negative and skip validation, or wrap to a small value and pass a buffer check.
- **Failure scenario:**
  - Plain build: `scs_has_condorcet_winner_2d(a, -1073741825, v, 3, ...)` dies with SIGBUS (exit 138) in the validation meant to protect the call (`$S3B/t02_overflow.c`).
  - UBSan build: `runtime error: signed integer overflow: -1073741825 * 2` at lines 2157, 2244, 2319, 2365, 2429, 2465, 2642 and 3435; `65536 * 65536` at 2685; `1073741824 * 2` at 2883; `46341 * 46341` at `uncovered_set.h:239` (`$S3B/t02_asan.out`).
  - `scs_draw_voters(1073741824, 2, ..., out, 2, ...)`: `needed` wraps negative, so the buffer check passes. Logic: the call then draws 2³¹ values and returns `SCS_OK` having written nothing.
  - `scs_profile_export_rankings` on a 65536 × 65536 profile (about 17 GB, not run): `required` wraps to 0, `out_len < 0` is false, and the loop writes through `out_rankings` out of bounds. This is the one write-side instance.
- **Suggested fix:** A shared helper that checks counts against `[1, INT_MAX / elements_per_item]` before multiplying, used by every entry point. `out_len` and `required` arithmetic in `size_t` or `int64_t`.
- **Also affects:** R and Python pass real lengths, so direct C callers are the main reach. An R or Python profile over 2³¹ cells would reach line 2988. Heart functions have the same pattern and are being removed.

### S03b-003 [P1, confirmed] Duplicate alternatives are reported as the same index
- **Where:** src/c_api/scs_api.cpp:2093–2098 (`find_alt_index`), 2390
- **What:** `uncovered_set` returns points, and `find_alt_index` maps each to the *first* alternative with equal coordinates. Equal-coordinate alternatives therefore all map to the lowest index: one index repeats and the others are never reported. The comment at 2091 ("Safe because…") covers rounding but not duplicates. Branches `(idx >= 0) ? idx : 0` (2296) and `: -1` (2391) are unreachable, since the point always comes from `alts`.
- **Failure scenario:** alternatives `(1,1), (1,1), (3,3)`, voters near `(1,1)`: `out_indices = [0, 0]`, expected `[0, 1]`. Python `uncovered_set_2d([1,1, 1,1, 3,3], [1,1, 1,1.1, 1.1,1])` returns `[0 0]` (`$S3B/t05_py_dup.py`). Indices `[1, 1]` for duplicates at 1 and 3 (`dup_unc_cycle` in `$S3B/t01_boundary.out`). `-0.0` and `0.0` also compare equal. Co-located alternatives are ordinary in spatial models, for example converged competitors.
- **Suggested fix:** Add an index-returning core function (the covering relation can run on indices) and drop `find_alt_index`. Alternatively, reject duplicates with an end-user error, if that is the owner's preference.
- **Also affects:** R and Python `uncovered_set_2d`.

### S03b-004 [P1, confirmed] Yolk returns NaN with SCS_OK at large coordinates; `scs_core_2d` then rejects valid input with a misleading error
- **Where:** src/c_api/scs_api.cpp:2067–2071, 2330; root cause include/socialchoicelab/geometry/yolk.h (`alpha = spread * 0.6` and `step = alpha * gap / (iter + 1)`, so the step scales with the square of the coordinates)
- **What:** The solver is not scale-invariant. Past about 1e7 it diverges to NaN, and the C API copies that to `SCS_Yolk2d` and returns `SCS_OK`. `core_2d` starts from that centre, so `calculate_distance` throws "alternative_point[0] is not finite", naming an input the caller never supplied. Stage 2b's S02b-002 describes the same step rule stalling at ordinary scales; this divergence is a different consequence.
- **Failure scenario:** five fixed voters scaled by m (`$S3B/t09_yolk_threshold.c`, output `$S3B/t09.out`):

  | m | Yolk r/m | Yolk rc | `scs_core_2d` |
  |---|---|---|---|
  | 1e4 | 0.2352 | 0 | found=0 |
  | 1e6 | 0.2314 | 0 | found=0 |
  | 1e7 | 0.1968 | 0 | found=0 |
  | 3e7 and above | NaN (centre NaN, r = 0) | 0 | rc=1, "alternative_point[0] is not finite" |

  The answer also changes with units well before NaN appears. Data in dollars or population counts reach this range. R and Python users hit it through `core_2d`.
- **Suggested fix:** Normalise the coordinates in `yolk_2d` (solve on `(x − centroid)/spread`, scale back). In `scs_yolk_2d`, check outputs for finiteness and return `SCS_ERROR_INTERNAL` with an end-user message. No EPEC abort occurs on this path: the sweep to 1.7e308 ended in errors, never a signal (`$S3B/t06_misc.out`).
- **Also affects:** `scs_yolk_2d` (C only, Q14 unanswered), `core_2d` in R and Python, and anything built on the Yolk.

### S03b-005 [P1, confirmed (logic)] Alternative coordinates drawn in compiler-dependent order
- **Where:** src/c_api/scs_api.cpp:2779, 2836. Decision: Q4
- **What:** `alts.emplace_back(prng.uniform_real(lo, hi), prng.uniform_real(lo, hi))`, and the same with `prng.normal`, makes both draws arguments of one call. C++ leaves their evaluation order unspecified. These sites are in the C API and are not in SCP-004 or the Q4 Implies list. `scs_draw_voters` is **not** affected: `voter_sampler.h` draws in a loop, one `push_back` per coordinate.
- **Failure scenario:** the same seed gives alternatives with x and y swapped between compilers that evaluate arguments in different orders. Q4 requires identical uniform draws across platforms. Only Apple clang is available here (`/usr/bin/gcc` is clang), so the actual difference was not observed.
- **Suggested fix:** Draw into named locals first, as part of the Q4 RNG work.
- **Also affects:** seeded `profile_uniform_spatial` and `profile_gaussian_spatial` in R and Python.

### S03b-006 [P1, confirmed] Condorcet and majority predicates accept out-of-range indices (S02c-004 passes through the C API)
- **Where:** src/c_api/scs_api.cpp:3978–4024; include/socialchoicelab/aggregation/condorcet_consistency.h
- **What:** Neither `scs_is_selected_by_*` function checks `alt_idx`, and the core does not either. `scs_is_pareto_efficient` shows the expected behaviour.
- **Failure scenario:** on a 3-alternative profile with a Condorcet winner, `scs_is_selected_by_majority_consistent_rules(p, 999, &out)` and `(p, -1, &out)` return `SCS_OK` with `out = 1`. `scs_is_pareto_efficient(p, 999, ...)` correctly returns an error (`idx_range` in `$S3B/t01_boundary.out`). Stage 2c records that R rejects these and Python does not.
- **Suggested fix:** Validate in the C++ functions (S02c-004's fix); the C API then needs no separate check.
- **Also affects:** Python `is_majority_consistent` and `is_condorcet_consistent`.

### S03b-007 [P2, confirmed] Profile handle leaks whenever a constructor's core call throws
- **Where:** src/c_api/scs_api.cpp:2653, 2695, 2724, 2781, 2839
- **What:** Each constructor does `auto* p = new SCS_ProfileImpl{}; p->profile = core_call(...);`. If `core_call` throws, the catch returns `nullptr` and `p` is lost.
- **Failure scenario:** `n_voters = 0` on any of `scs_profile_build_spatial`, `_impartial_culture`, `_uniform_spatial`, `_gaussian_spatial`, 10 calls each: `40 leaks for 1920 total leaked bytes`, stacks at lines 2653, 2724, 2781 and 2839 (`$S3B/t03_leak.c`, output `$S3B/t03_leak.out`). The Python suite hits it too: `test_profile_impartial_culture_zero_voters_raises` and `_zero_alts_raises`. Line 2695 needs `bad_alloc`, so it was not run.
- **Suggested fix:** Compute into a local `Profile` first, then `new SCS_ProfileImpl{std::move(profile)}`, or hold `p` in a `std::unique_ptr` until success. Same pattern in the winset factories (slice 3a).
- **Also affects:** long-running R and Python sessions that repeatedly hit invalid input; the amount per failure is small.

### S03b-008 [P2, confirmed] `tie_break` values are not validated
- **Where:** src/c_api/scs_api.cpp:3016–3019 (`to_cpp_tie_break`), 3027 (`resolve_prng`)
- **What:** Any value other than `SCS_TIEBREAK_SMALLEST_INDEX` maps to `kRandom`, but `resolve_prng` treats only `SCS_TIEBREAK_RANDOM` as needing a PRNG. An unknown value therefore gets `kRandom` with a null PRNG.
- **Failure scenario** (`$S3B/t06_misc.c`, output `$S3B/t06_misc.out`): `scs_plurality_one_winner(p, 99, NULL, NULL, ...)` with a unique winner returns `SCS_OK`, while on a tie it returns an error saying "TieBreak::kRandom requires a non-null PRNG", which names neither the bad value nor the C API. Whether an invalid enum errors depends on the data. The checklist requires enums to be validated, not defaulted.
- **Suggested fix:** Reject values other than the two defined, with a message naming the value and the allowed names, in `resolve_prng`.
- **Also affects:** direct C callers. R and Python pass mapped values.

### S03b-009 [P2, confirmed] Negative and zero counts are not rejected at the boundary
- **Where:** src/c_api/scs_api.cpp `build_voters` callers: 2165–2166, 2254–2255, 2289–2290, 2329, 2375–2376, 2440, 2476, 2651–2652, 3448–3449, 3488–3489; `reserve` calls 2777, 2834; 3339, 3367, 3398 (`n_weights`)
- **What:** The header says `n_alts >= 1` and `n_voters >= 1` in many places, but most functions do not check. Negative values reach `reserve(size_t(negative))` or a negative-length `vector` constructor. Zero values reach core paths that accept them.
- **Failure scenario** (`$S3B/t01_boundary.out`):
  - Negative counts give `rc=2` with the message `"vector"` (or `NULL`) for `n_alts` or `n_voters` in `scs_profile_build_spatial`, `scs_approval_scores_spatial`, `scs_has_condorcet_winner_2d`, `scs_uncovered_set_2d`, `scs_core_2d`, the uncovered-set boundary and `scs_profile_uniform_spatial`, and for negative `n_weights`. The code is wrong (should be INVALID_ARGUMENT) and the message tells the user nothing.
  - `scs_condorcet_winner_2d(a, 1, v, 0, ...)` reports `found=1, winner=0` with no voters. `scs_uncovered_set_2d` likewise reports `n=1` with no voters.
  - `scs_approval_scores_spatial` with `n_voters=0` returns all zeros.
  - `scs_profile_build_spatial` and `_uniform_spatial` with `n_alts=0` return a live handle (S02c-007 at the C API). `scs_pareto_set` then gives `n=0`, contradicting the header ("always non-empty").
- **Suggested fix:** Use the S03b-002 helper for every count and return INVALID_ARGUMENT naming the argument, the value and the valid range.
- **Also affects:** Python `profile_build_spatial([], voters)`.

### S03b-010 [P2, confirmed] `scs_yolk_2d` accepts any distance config, ignores it, and does not validate `num_samples`
- **Where:** src/c_api/scs_api.cpp:2058–2064, 2067; include/scs_api.h (`scs_yolk_2d` `@param dist_cfg`)
- **What:** The header says "n_weights == 2; Euclidean recommended", and the code comment says "validated by to_dist_config". Neither is true: only the pointer is checked, and the config is never used. `num_samples` has no range check.
- **Failure scenario** (`yolk_metric`, `yolk_neg_samples*` in `$S3B/t01_boundary.out`): Manhattan, weights (5, 0.2), and a garbage config (type 77, `order_p` NaN, NULL weights, `n_weights` 0) all return `SCS_OK` and the same Euclidean centre (2, 1.8), r = 0.8. A caller passing Manhattan gets a Euclidean answer and no signal. `num_samples = 0` or `-1` return `SCS_OK`. `-1000` returns INTERNAL `"vector"`.
- **Suggested fix:** Either remove the parameter or reject non-Euclidean and non-unit configurations with an end-user error; correct the comment and header. Require `num_samples >= 1`. Q14 (bindings) does not settle this.
- **Also affects:** C API callers only today.

### S03b-011 [P2, confirmed] `grid_resolution` is silently clamped and has no upper limit
- **Where:** src/c_api/scs_api.cpp:2441, 2477; include/socialchoicelab/geometry/uncovered_set.h:237 (`res = std::max(2, grid_resolution)`), :239
- **What:** Confirms the Stage 2 lead. Values below 2, including negative, are silently treated as 2. There is no maximum; cost is O(res⁴·n), and `res * res` overflows at 46341.
- **Failure scenario:** `grid_resolution` = -5, 0, 1 each return `SCS_OK` and the grid-2 result (`grid_vals` in `$S3B/t01_boundary.out`). 46341 is signed overflow (UBSan, `$S3B/t02_asan.out`).
- **Suggested fix:** Reject values below 2 and above a documented limit with an end-user error.
- **Also affects:** R and Python uncovered-set boundary functions.

### S03b-012 [P2, confirmed] Finite ranges whose width overflows produce non-finite voters with SCS_OK
- **Where:** src/c_api/scs_api.cpp:2906–2914, 2758–2769, 2815–2826
- **What:** S02a-008's pattern at more entry points. `lo` and `hi` are finite, but `hi − lo` overflows, so draws are Inf.
- **Failure scenario:** `scs_draw_voters` with uniform(-1.7e308, 1.7e308) returns `SCS_OK` and `o = inf inf inf inf`. `scs_profile_uniform_spatial` with those bounds returns NULL with "ideal_point[0] is not finite", which does not explain the cause (`dv_overflow`, `uni_overflow` in `$S3B/t01_boundary.out`). The profile failure path also leaks (S03b-007).
- **Suggested fix:** Reject ranges whose width is not finite; share the check with `scs_uniform_real` (slice 3c).
- **Also affects:** R and Python voter samplers, for extreme inputs only.

### S03b-013 [P3, confirmed] "Buffer too small" uses two different error codes
- **Where:** `SCS_ERROR_BUFFER_TOO_SMALL` at scs_api.cpp:2890; `SCS_ERROR_INVALID_ARGUMENT` at 2153, 2967, 2995, 3079, 3149, 3219, 3256, 3336, 3431, 3517, 3814, 3864
- **What:** `scs_draw_voters` returns `BUFFER_TOO_SMALL`, and every other fixed-size output returns `INVALID_ARGUMENT` for the same condition. Header `@return` lines say only "or error code". `c_api_design.md:82` promises `BUFFER_TOO_SMALL` for a too-small non-null buffer.
- **Failure scenario:** `$S3B/t01_boundary.out`, `buf_codes`: `out_len` too small gives 1 for scores, rankings, export and Copeland, and 3 for `draw_voters`. Python raises `SCSInvalidArgumentError` or `SCSBufferTooSmallError` for the same mistake.
- **Suggested fix:** Use `BUFFER_TOO_SMALL` for all of them.
- **Also affects:** Python exception types.

### S03b-014 [P3, confirmed] Any negative `k` means simple majority, unlike the header
- **Where:** src/c_api/scs_api.cpp (forwarded unchanged at 2168, 2257, 2292, 2330, 2377, 2442); include/scs_api.h:992 and other `@param k`
- **What:** The header allows `SCS_MAJORITY_SIMPLE` (-1) or `1..n_voters`. Other negative values are accepted silently, because the core treats any negative `k` as simple majority.
- **Failure scenario:** `scs_copeland_scores_2d(..., k = -5, ...)` returns `SCS_OK` and the same scores as `k = -1` (`k_neg`). R rejects `k < 1` other than `"simple"`.
- **Suggested fix:** Reject `k < 1` except -1 at the C boundary, or document the behaviour. Same treatment in slice 3a's majority functions.
- **Also affects:** Stage 6 parity check.

### S03b-015 [P3, confirmed] `scs_core_2d` documents an exact result that it does not compute
- **Where:** include/scs_api.h:1048–1056 (`scs_core_2d` doc, `@param out_found`). Decision: Q5
- **What:** The header says `out_found` is "1 if the core is non-empty, 0 otherwise" with no caveat. `core_2d` tests candidates against sampled-polygon emptiness. Q5's Implies list covers `core.h:165` and other docs, but not this header text.
- **Failure scenario:** four Plott-symmetric voters plus a centre voter, one voter rotated by δ: `found=1` at δ = 0.003, 0.01 and 0.03, where Q5 says the winset is non-empty and so no core exists at the centre (`$S3B/t10_core_plott.c`, output `$S3B/t10.out`).
- **Suggested fix:** Update the header with the Q5 change: state the cases that are exact and those labelled approximate.
- **Also affects:** R and Python `core_2d` docs.

### S03b-016 [P3, confirmed] Error messages often omit the argument, the value or the fix
- **Where:** src/c_api/scs_api.cpp — for example 2044, 2137 and the other "null pointer argument" sites; 2680–2682; 2766–2767; 2903; 3030
- **What:** The end-user rule asks for what failed, what was entered, how to fix it. "null pointer argument" does not say which pointer. "n_voters and n_alts must be >= 1" omits the values. "lo and hi must be finite" omits them too. "unknown SCS_VoterSamplerKind" omits the value. Raw core or library messages also come through: `"vector"` (S03b-009).
- **Failure scenario:** `scs_profile_from_utility_matrix(u, 0, 3, ...)` reports "n_voters and n_alts must be >= 1" with neither value shown; a caller with two possibly-wrong counts cannot tell which failed.
- **Suggested fix:** Name each argument and value, as `scs_copeland_scores_2d`'s messages do at 2143.
- **Also affects:** messages shown by R and Python.

### S03b-017 [P3, confirmed] Heart C API to be removed
- **Where:** src/c_api/scs_api.cpp:2509–2627; include/scs_api.h:1132–1184; tests/unit/test_c_api.cpp `CApi_Heart.*` (from :2184). Decision: Q1
- **What:** Withdrawn functions `scs_heart_2d`, `scs_heart_boundary_size_2d` and `scs_heart_boundary_2d` are still present. Not reviewed, per Q1.
- **Failure scenario:** none; listed for removal.
- **Suggested fix:** Remove them with the Q1 work.
- **Also affects:** none (not bound in R or Python).

## Tool leads checked
- Stage 2b: `scs_uncovered_set_boundary_2d` does not validate `grid_resolution` or `n_voters` → S03b-011, S03b-009.
- Stage 2c: profile `n_alts` negative values → S03b-009 (`"vector"`, no crash). `n_alts = 0` (S02c-007) → S03b-009.
- Stage 2b: `scs_yolk_2d` takes `dist_cfg` but ignores it → S03b-010.
- S02c-001 (Hare remainders) → not in this slice's calls; competition only.
- S02c-002 (`pairwise_ranking` "Copeland") → C API neither adds nor hides it; `scs_pairwise_ranking_from_matrix` documents the wins-only count but calls it Copeland, while `scs_copeland_scores_2d` documents wins minus losses. Open under Q27.
- S02c-003 (`is_majority_consistent` naming) → forwarded unchanged; the C name `scs_is_selected_by_majority_consistent_rules` carries it. Open under Q28.
- S02c-004 → S03b-006.
- SCP-003 pattern → other instances: S03b-002.
- SCP-004 → `scs_draw_voters` not affected (false positive for that function); additional instances at the C API: S03b-005; the generators' core lines remain SCP-004.
- SCP-001 / SCP-002 → checked on `scs_core_2d` and the boundary: sweep of 8 magnitudes (1e100 to 1.7e308, `$S3B/t06_misc.out`) gave only errors, never SIGFPE. The Yolk's NaN stops `core_2d` before any winset is built. Evidence, not proof.
- Stage 0 never-called centrality functions (`scs_marginal_median_2d`, `scs_centroid_2d`, `scs_geometric_mean_2d`) → correct by reading; not called by any C API test in `test_c_api.cpp`, but exercised through Python (`python/tests/test_b5.py`) and R (`r/tests/testthat/test_geometry.R`).
- Stage 0 coverage (scs_api.cpp 60%) → the paths that matter here are untested in `test_c_api.cpp`: unregistered-stream tie-break, negative counts, duplicates.
- Stage 0 clang-tidy `implicit-widening` in this slice (the `2 * i` index in `export_polygon2e`, reported at an older line number) → harmless, since `i < n` and the buffer is checked; the real multiplication risks are S03b-002.
- Stage 0 swappable-parameters notes → style only, no finding.
- Q1 → S03b-017. Q3 → the C API has no default and requires an explicit value; correct. Q4 → S03b-005 (Q4 Implies covers the rest). Q9 → no static or global mutable state in this slice; correct. Q14 unanswered → see Open questions.

## Leak runs
Filter from the prompt, Stage 3 build (non-ASan):

```
cd .../scratchpad/stage3/build
MallocStackLogging=1 leaks --atExit -- ./test_c_api --gtest_brief=1 --gtest_filter='CApi_Yolk*:CApi_Copeland*:CApi_Condorcet*:CApi_Core*:CApi_UncoveredSet*:CApi_Heart*:CApi_Profile*:CApi_VoterSampler*:CApi_Plurality*:CApi_Borda*:CApi_Antiplurality*:CApi_ScoringRule*:CApi_Approval*:CApi_RankByScores*:CApi_PairwiseRanking*:CApi_Pareto*'
```
Result: 87 tests pass. `Process 58770: 0 leaks for 0 total leaked bytes.` Log: `$S3B/leak_ctest.txt`.

Python (the `python3` launcher re-executes, so `leaks` reported nothing on the first try; run again on the real interpreter binary):

```
PYTHONDONTWRITEBYTECODE=1 SCS_LIB_PATH=.../stage3/build PYTHONPATH=<repo>/python/src \
MallocStackLogging=1 leaks --atExit --list -- \
  /Library/Frameworks/Python.framework/Versions/3.11/Resources/Python.app/Contents/MacOS/Python \
  -m pytest <repo>/python/tests -q -q -p no:cacheprovider --rootdir=$S3B
```
Result: all 307 tests pass. `Process 59284: 64 leaks for 18016 total leaked bytes.` Two leaks have `scs_`/`libscs_api` frames: 48-byte allocations in `scs_profile_impartial_culture` (`scs_api.cpp:2724`), from the two zero-count tests → S03b-007. The other 62 are interpreter allocations. Log: `$S3B/leak_pytest.txt`.

Targeted: `$S3B/t03_leak.c` → 40 leaks, 1,920 bytes (S03b-007).

## Open questions
1. **Q14 (unanswered):** the Yolk is not bound in R or Python, so S03b-004's NaN result is direct-C only for `scs_yolk_2d`, but reaches R and Python through `core_2d`.
2. **Q25 (unanswered):** the uncovered-set boundary returned 1 or 2 vertices on the small test (`grid_vals`). Left as open behaviour.
3. **Q27 / Q28 (unanswered):** see the S02c-002 and S02c-003 lines above.
4. For the owner: should duplicate alternatives be accepted with correct indices, or rejected (S03b-003)? Both are valid; the current behaviour is neither.

## Out-of-slice notes
- `include/socialchoicelab/geometry/yolk.h`: `lines.reserve(n_sample + n * (n - 1) / 2)` overflows `int` at about 46,342 voters (logic, not run).
- `core_2d` at tiny scales: coordinates scaled by 1e-100 report `found=1` where the same configuration scaled to 1 reports `found=0` (`$S3B/t08.out`). Related to the absolute tolerances in S02b's scale findings. Stage 4.
- R `.Call` and Python cffi both abort on an escaped C++ exception (S03b-001); Stage 5 may want a binding-level regression test.
- Website: S03b-001 cannot be triggered by `socialchoicestudiowebsite/backend/app/simulation.py`, which calls only `competition_run`. No website change expected from this slice's fixes.
