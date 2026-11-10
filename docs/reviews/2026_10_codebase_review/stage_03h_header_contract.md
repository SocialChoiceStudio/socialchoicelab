# Stage 03h — C API header contract
Model / effort: Opus / high   Commit: e6772eb   Files reviewed: 2 (`include/scs_api.h` 2,020 lines; `docs/architecture/c_api_design.md` 641 lines)
Not read (if any): nothing in the slice. `src/c_api/scs_api.cpp` was read only to check specific claims, as the prompt says.

Scratch programs: `/private/tmp/claude-501/-Users-robiragan-Dropbox-SocialChoiceStudioDev/2402f97e-221e-494b-88cb-fc63c41efc50/scratchpad/stage3h/` (`contract.c`, `enums.c`, `enums2.c`, `plott.c`, `layout.c`, `ls.c`, `cdef_cmp.py`), linked against the shared Stage 3 builds.

## Summary
- **Counts:** 0 P0, 1 P1, 7 P2, 7 P3. All confirmed, except the Windows half of S03h-013 (plausible).
- **Most important:** the header tells callers to use `threshold = 0.5` for "simple weighted majority", and both bindings default to it. With two equal-weight voters at (0,0) and (1,0), A beats B **and** B beats A. This is a Q6 instance the Implies list missed (S03h-001).
- **The ABI itself is sound today.** The header compiles cleanly as C99/C11/C17 and C++20 with `-Wall -Wextra -Wpedantic -Werror`. All 124 declared functions are exported and defined. Struct sizes and offsets match Python's hand-copied cdef.
- **The error contract is only partly met.** All 66 null-handle checks pass and every error path writes `err_buf`. But 13 functions report a too-small buffer as `INVALID_ARGUMENT` (S03h-003), and `scs_winset_2d_export_boundary` reports bad user input as `INTERNAL` (S03h-004).
- **C enums in the ABI** make an out-of-range value undefined behaviour when the C++ side reads it, which UBSan confirms, and they make struct layout depend on a compiler flag (S03h-002).
- **The competition API is almost undocumented:** 25 of its 26 functions have no comment, so ownership and buffer shapes appear nowhere (S03h-005).

## Contracts this layer provides
For Stage 5. "Verified" means run against the Stage 3 build (`contract.c`). Items marked (Q*n*) are expected to change under that owner decision.

**ABI types**
- Fixed-width: `SCS_PairwiseResult` and `SCS_TieBreak` are `int32_t`; seeds are `uint64_t`; `scs_uniform_int` and `scs_uniform_choice` use `int64_t`.
- Ten public C enums are all 4 bytes on Apple clang by default, but the width is implementation-defined (S03h-002).
- Struct sizes on macOS arm64: `SCS_LossConfig` 40, `SCS_DistanceConfig` 32, `SCS_LevelSet2d` 120, `SCS_Yolk2d` 24, `SCS_VoronoiCellsHeap` 32, `SCS_CompetitionStepConfig` 48, `SCS_CompetitionTerminationConfig` 48, `SCS_CompetitionEngineConfig` 120, `SCS_VoterSamplerConfig` 24. Python's cdef gives the same sizes.
- Python's cdef differs from the header only in `int32_t`/`int64_t`/`uint64_t` spellings, the missing `SCS_COMPETITION_STRATEGY_HUNTER_STICKER` (Q8 removes it) and the four unbound functions (Yolk, Heart ×3; Q14, Q1).
- All indices are 0-based. All coordinate arrays are interleaved `[x0,y0,x1,y1,...]`.

**Return codes and `err_buf`**
- Codes: `SCS_OK` 0, `INVALID_ARGUMENT` 1, `INTERNAL` 2, `BUFFER_TOO_SMALL` 3, `OUT_OF_MEMORY` 4.
- Factory functions return `NULL` on failure instead of a code, so the caller gets only the message. `void` functions report nothing.
- `err_buf` is optional: `NULL`, length 0 or a negative length are safe. Length 1 writes only the NUL. Messages are truncated and always NUL-terminated. On success the buffer is **left untouched**, so bindings must not read it after `SCS_OK` (verified).
- Every non-zero return path sets a message (verified by scanning the code and by running the failing calls).

**Ownership**

| Output | Owner | Release with |
|---|---|---|
| `SCS_StreamManager*` | caller | `scs_stream_manager_destroy` |
| `SCS_Winset*` (factories, boolean ops, clone) | caller | `scs_winset_destroy` |
| `SCS_Profile*` (constructors, clone) | caller | `scs_profile_destroy` |
| `SCS_CompetitionTrace*` | caller | `scs_competition_trace_destroy` (not stated in header) |
| `SCS_CompetitionExperiment*` | caller | `scs_competition_experiment_destroy` (not stated in header) |
| `SCS_VoronoiCellsHeap` fields (malloc) | caller | `scs_voronoi_cells_heap_destroy`, which frees the fields and zeroes the struct, but not the struct itself |
| `*out_xy` from `scs_uncovered_set_boundary_2d_heap` (malloc) | caller | `scs_heap_free` |

- Every destroy function, and `scs_heap_free`, accepts `NULL` (verified).
- `SCS_DistanceConfig.salience_weights` is borrowed for the call only.

**Null handles.** Every non-destroy function rejects a null handle: `INVALID_ARGUMENT` plus a message, or `NULL` plus a message for factories (66 calls, 0 failures).

**Size-query and buffer behaviour**
- Capacity is in **(x,y) pairs** for polygon, winset and Voronoi exports, but in **doubles** for `out_len` (`scs_draw_voters`, trace and experiment positions).
- Variable-size functions set `*out_n` to the required size, and also on `BUFFER_TOO_SMALL` (verified for level-set polygon, winset, Voronoi, uncovered set).
- `NULL` buffer **or** capacity 0 is a size query for: `*_all_winners`, `scs_pareto_set`, `scs_uncovered_set_2d`, the uncovered-set boundary, winset boundary export, Voronoi.
- **Only `NULL`** is a size query for: `scs_level_set_to_polygon`, `scs_level_set_polygon_2d`, `scs_ic_polygon_2d`, `scs_competition_trace_dist_config`. A non-null buffer with capacity 0 returns `BUFFER_TOO_SMALL` and still sets the size (S03h-008).
- Fixed-length `out_len` functions do not support size queries: a `NULL` output is `INVALID_ARGUMENT`. A short buffer is `BUFFER_TOO_SMALL` or `INVALID_ARGUMENT` depending on the function (S03h-003).
- Winset rings and Voronoi cells do **not** repeat the first vertex. Winset outer rings came out CCW (verified). `*out_path_is_hole` is written only when `out_path_starts` is supplied.

**Not-found outputs** (differ per function)
- `scs_condorcet_winner_2d` writes −1. `scs_core_2d` writes (0, 0). `scs_condorcet_winner_profile` leaves the output untouched. `scs_winset_bbox_2d` leaves outputs untouched (verified).
- `scs_winset_interval_1d` writes NaN, NaN when empty, so bindings must expect NaN output.

**Thread safety (Q9).** No global or static mutable state in `scs_api.cpp` (grep: only `static` helper functions). The owner's rule applies: separate handles on separate threads are fine, and one handle must never be used from two threads, even for reads. The header does not say this (S03h-007).

**Argument domains worth knowing**
- `k` is `SCS_MAJORITY_SIMPLE` or 1..n, but **any negative `k` behaves as simple majority** (`k = -5` returned `SCS_OK`); `k = 0` is an error.
- `tie_break = SCS_TIEBREAK_RANDOM` needs `mgr` and `stream_name` even when no tie occurs.
- An unlisted stream name is rejected only after an allowlist has been set. With no allowlist, any name works. `n_names = 0` clears the allowlist.
- `scs_competition_run` accepts `mgr = NULL` today (Q7 changes this).
- 2D shape functions reject a salience weight ≤ 0 (Q18).
- `scs_yolk_2d` validates `dist_cfg` and then ignores it.
- `n_dims` must be 2 for `scs_draw_voters` and the spatial profile generators.

**Version.** `scs_api_version` returns 0.2.0 today; Q11 sets it to 0.3.0.

## Findings

### S03h-001 [P1, confirmed] Header calls 0.5 "simple weighted majority", which lets each side beat the other
- **Where:** include/scs_api.h:656–657, :666–667 (`scs_weighted_majority_prefers_2d`). Decision: Q6
- **What:** The function returns 1 when the weight preferring A "meets or exceeds" threshold × total, and the header says to use 0.5 for simple majority. At exactly half the weight both directions win. Q6's Implies list names `scs_weighted_winset_2d` but not this function.
- **Failure scenario:** Voters at (0,0) and (1,0) with weights 1 and 1, A = (0,0), B = (1,0), threshold 0.5. `contract.c` output: `weighted 0.5: A over B=1, B over A=1`. `scs_majority_prefers_2d` with `SCS_MAJORITY_SIMPLE` on the same input gives `A>B=0 B>A=0`. Python and R both default to 0.5, so a default call hits this.
- **Suggested fix:** Apply Q6's rule here too: a "more than half" sentinel as the default, with an explicit threshold documented as "at least".
- **Also affects:** include/socialchoicelab/geometry/majority.h:255 (C++ default); python/src/socialchoicelab/_functions.py:571; r/R/functions.R:359, :372. Website not affected as far as checked (it does not call this function).

### S03h-002 [P2, confirmed] Public C enums make bad values undefined behaviour and layout flag-dependent
- **Where:** include/scs_api.h:107, :135, :162, :1229–1264, :1396; used as struct fields at :123, :151, :181, :1267, :1288–1291, :1410, and as output pointers at :1908, :1927, :1999. Rule at docs/architecture/c_api_design.md:118–120
- **What:** Ten mode and result types are C enums, although the design doc asks for fixed-width integers. A C or FFI caller can legally store any `int` there. The C++ implementation then reads it as an enum with no fixed underlying type, and an out-of-range value is undefined behaviour **before** the `default:` branch that is meant to reject it.
- **Failure scenario:** With `distance_type = 99` the ASan+UBSan build stops with `scs_api.cpp:630:23: runtime error: load of value 99, which is not a valid value for type 'SCS_DistanceType'`. The same happens at :183 (`to_dist_config`), :679 (`loss_type = 42`) and :908 (`SCS_LevelSet2d.type = 7`). Separately, `-fshort-enums` shrinks `SCS_LossType` from 4 bytes to 1 and `SCS_CompetitionEngineConfig` from 120 to 112, so a caller built with that flag silently disagrees with the library (`layout.c`).
- **Suggested fix:** Use `typedef int32_t` plus named constants, as `SCS_TieBreak` already does. Q12 allows this directly. Validate the integer before mapping it.
- **Also affects:** Python cdef, R glue. Missing validation of `loss_type` is slice 3a's lead.

### S03h-003 [P2, confirmed] Too-small buffers return two different error codes
- **Where:** include/scs_api.h:74–77; docs/architecture/c_api_design.md:62, :81–82; src/c_api/scs_api.cpp:2148, 2961, 2989, 3073, 3143, 3213, 3250, 3330, 3426, 3511, 3809, 3858, 4425
- **What:** The contract says a valid but too-small buffer gives `SCS_ERROR_BUFFER_TOO_SMALL`. Thirteen functions return `SCS_ERROR_INVALID_ARGUMENT` instead: `scs_plurality_scores`, `scs_borda_scores`, `scs_antiplurality_scores`, `scs_scoring_rule_scores`, `scs_approval_scores_spatial`, `scs_approval_scores_topk`, `scs_borda_ranking`, `scs_rank_by_scores`, `scs_profile_get_ranking`, `scs_profile_export_rankings`, `scs_copeland_scores_2d`, `scs_pairwise_ranking_from_matrix`, `scs_competition_trace_strategy_kinds`. Peers such as `scs_pairwise_matrix_2d`, `scs_draw_voters` and the other trace getters return `BUFFER_TOO_SMALL`.
- **Failure scenario:** `scs_plurality_scores(p, buf, 1, ...)` on a 3-alternative profile returns 1, while `scs_pairwise_matrix_2d` with a short buffer returns 3 (`contract.c`). Python raises `SCSInvalidArgumentError` for one and `SCSBufferTooSmallError` for the other, for the same kind of mistake.
- **Suggested fix:** Return `BUFFER_TOO_SMALL` from all thirteen, and add a test that loops over the fixed-length functions.
- **Also affects:** Python error classes.

### S03h-004 [P2, confirmed] `scs_winset_2d_export_boundary` reports bad input as an internal error
- **Where:** src/c_api/scs_api.cpp:1756 (`scs_winset_2d_export_boundary`: `if (!ws) return SCS_ERROR_INTERNAL;`); contract at docs/architecture/c_api_design.md:60–61
- **What:** Any failure to build the winset is returned as `SCS_ERROR_INTERNAL`, including ordinary input errors.
- **Failure scenario:** `k = 99` with 5 voters gives `rc=2` with the message `majority: k must be in [1, n] (got k=99, n=5).` (`contract.c`). `scs_winset_2d` returns only `NULL` and this message, so the wrapper has no code to pass on. In Python the overlays in `plots.py:1074` and `:1095` would raise `SCSInternalError`, which tells the user the library is at fault.
- **Suggested fix:** Split construction from the handle factory so the real code can be returned, or classify the failure before building.
- **Also affects:** Python overlays (`plots.py`).

### S03h-005 [P2, confirmed] The competition API has almost no contract documentation
- **Where:** include/scs_api.h:1226–1294, :1892–2016; docs/architecture/c_api_design.md (no competition section)
- **What:** Of the 26 competition functions only `scs_competition_trace_dist_config` has a comment. The enums and config structs have none. Not stated anywhere: ownership of the trace and experiment handles, which pairs with which destroy, buffer lengths, flat layout, whether `mgr` may be `NULL`, field meanings (`jitter`, `proportionality_constant`, `cycle_memory`), and the meaning of `retain_traces`. The design doc's handle list and mapping table omit both handle types.
- **Failure scenario:** `scs_competition_experiment_run_final_positions` needs `n_runs * n_competitors * n_dims` **doubles**, laid out as `(run * n_competitors + c) * n_dims + d`. Elsewhere in the same header, polygon capacities count (x,y) **pairs**. A caller who sizes the buffer in pairs, or reads the rows in the wrong order, gets `BUFFER_TOO_SMALL` or scrambled positions, and nothing in the header says otherwise.
- **Suggested fix:** Give each competition function a doc block with ownership, shape and units. Q10's bounds rule goes in the same place, as already decided.
- **Also affects:** Stage 5 has only the implementation to rely on for these functions.

### S03h-006 [P2, confirmed] The header overclaims winset emptiness and core results
- **Where:** include/scs_api.h:744 ("no policy beats the SQ"), :1050–1055 (`out_found` "1 if the core is non-empty"). Decision: Q5
- **What:** Both statements are unconditional, but `is_empty` is judged on sampled polygons and `core_2d` uses that test. Q5's Implies list covers the code and `c_api_design.md`, not these header comments. This is a documentation instance, so it is P2 rather than P1.
- **Failure scenario:** Five Plott-symmetric voters with one rotated by δ = 0.03 and status quo at the origin (`plott.c`): `is_empty=1` at 64 samples and `is_empty=0` at 256 and 1,024. `scs_core_2d` reports `found=1` at (0,0) for the same voters, so the header's "core is non-empty" is false for this input.
- **Suggested fix:** Reword both comments when the Q5 exact test lands, and label the boolean-operation case approximate.
- **Also affects:** R and Python docs for `winset_is_empty` and `core_2d` (Stage 5).

### S03h-007 [P2, confirmed] The header says nothing about thread safety
- **Where:** include/scs_api.h (the word "thread" does not appear). Decision: Q9
- **What:** Q9 makes one-handle-one-thread a requirement, and the rubric requires thread safety to be stated at the boundary. Q9's Implies list covers `c_api_design.md` and the R/Python docs, not the header, which is the file a C caller reads.
- **Failure scenario:** A C or FFI caller reads the `const SCS_Winset*` in `scs_winset_contains_point_2d` and shares one handle across threads for read-only queries. Q9 measured 46 data races for this pattern.
- **Suggested fix:** Add one top-of-file paragraph stating the Q9 rule.
- **Also affects:** none.

### S03h-008 [P2, confirmed] Size-query rule differs between functions and from the design doc
- **Where:** docs/architecture/c_api_design.md:79–80, :240, :246–248; include/scs_api.h:334, :1919–1924; src/c_api/scs_api.cpp:1006, 1096, 4266
- **What:** The design doc says `NULL` or zero capacity is a size query. `scs_level_set_to_polygon`, `scs_level_set_polygon_2d`, `scs_ic_polygon_2d` and `scs_competition_trace_dist_config` honour only `NULL`. The header describes these four correctly; the design doc does not, and the rule is not uniform across the API.
- **Failure scenario:** A non-null buffer with capacity 0 returns `rc=3` from these four and `rc=0` from `scs_plurality_all_winners`, `scs_uncovered_set_2d`, `scs_winset_sample_boundary_2d` and `scs_voronoi_cells_2d` (`contract.c`). A shared binding helper that passes an empty array as a size query would fail on these four only.
- **Suggested fix:** Make capacity 0 a size query in all four, or state the exceptions.
- **Also affects:** bindings, if a shared helper is written.

### S03h-009 [P3, confirmed] Heart API still declared (to be removed)
- **Where:** include/scs_api.h:1131–1183; docs/architecture/c_api_design.md:185, :525. Decision: Q1
- **What:** `scs_heart_2d`, `scs_heart_boundary_size_2d` and `scs_heart_boundary_2d` remain declared, and the design doc still lists them. Not reviewed for correctness, per Q1.
- **Failure scenario:** none beyond the withdrawn behaviour itself; listed once as "to be removed".
- **Suggested fix:** Remove with the rest of the Heart code.
- **Also affects:** see Q1 Implies.

### S03h-010 [P3, confirmed] `scs_api_version` comment treats the C API version as independent
- **Where:** include/scs_api.h:205–207. Decision: Q11
- **What:** The comment defines major as "breaking changes", minor as "additive changes" and patch as "behavioural / doc fixes". Under Q11 the number is the project version, and it goes up when only R or Python changes. Q11's Implies list covers the values at :29–31 and the design doc, not this comment.
- **Failure scenario:** A reader takes a minor bump to mean new C functions were added; after an R-only release it does not.
- **Suggested fix:** Say it returns the project version.
- **Also affects:** none.

### S03h-011 [P3, confirmed] More stability language presented as already in force
- **Where:** docs/architecture/c_api_design.md:3, :37, :76, :218–222; include/scs_api.h:91, :1297. Decision: Q12
- **What:** "Stable C API", "the stable surface", "the stable contract", "Stable-Surface Function Rules" and "ABI-stable" are additional instances beyond the lines Q12's Implies names (:44–49, :629–642).
- **Failure scenario:** A future change is held back by, or argued against with, wording Q12 says is not yet in force.
- **Suggested fix:** Adjust with the Q12 edits, or describe the types as "fixed-width" instead of "ABI-stable".
- **Also affects:** none.

### S03h-012 [P3, confirmed] Stale or inaccurate header comments
- **Where:** include/scs_api.h, as listed
- **What and failure scenario, per item:**
  - :60 calls `SCS_DEFAULT_BOUNDARY_GRID_RESOLUTION` the default for "yolk/boundary", but the Yolk's sample count is 720 and this constant is 15. The comment at :1203 builds 720 as "resolution * 48", which is arbitrary.
  - :95–99 describe the pairwise values as "under the given majority rule" and "neither achieves the required majority". `scs_pairwise_matrix_2d` ignores `k` for the values (Stage 2b contracts), as :637–638 correctly says. A caller passing k = 4 of 5 and reading a WIN would misread it.
  - :261 says the normalized output is in [0, 1], but it is not clamped: `scs_normalize_utility(-50, 1.0, linear)` returned −49 (`contract.c`).
  - :466 refers to "scs_*_from_stream functions", which do not exist.
  - :505 says `scs_skip` skips "values". It discards raw engine outputs, not draws: after a `scs_normal` draw and after `scs_skip(1)` the next uniform differs (0.4298 vs 0.0897, `contract.c`).
  - :1198 says Euclidean is "recommended" for `scs_yolk_2d`, but the metric is ignored: Manhattan with weights (1, 5) returned the same centre and radius as Euclidean (`contract.c`).
  - `SCS_DistanceConfig` (:142–155) and the winset, core and uncovered-boundary comments do not mention that 2D shape functions need weights > 0 (Q18); only the design doc does.
- **Suggested fix:** Correct each comment.
- **Also affects:** none.

### S03h-013 [P3, confirmed for exports; plausible for Windows] `SCS_API` does not do what the docs say
- **Where:** include/scs_api.h:37–45; docs/architecture/c_api_design.md:32–35, :621–623; CMakeLists.txt:326–336
- **What:** The library is built without hidden visibility, so the macro has no effect on what is exported: 124 `scs_*` symbols plus 4,481 others (CGAL, GMP wrapper and libc++ template instantiations). `SCS_API_BUILD` is never defined by CMake or any build file, so on Windows `scs_api.cpp` would compile against `dllimport` declarations.
- **Failure scenario:** On Windows the library would not build or export correctly (not run; no Windows CI, though `r/src/Makevars.win` and a `win32` branch in `_loader.py` exist). On other platforms, exported template instantiations can interpose with another copy of CGAL or GMP loaded in the same process.
- **Suggested fix:** Set `CXX_VISIBILITY_PRESET hidden` and define `SCS_API_BUILD` for the target, or state that Windows is unsupported.
- **Also affects:** R on Windows, if supported.

### S03h-014 [P3, confirmed] Design doc does not match the header
- **Where:** docs/architecture/c_api_design.md, as listed
- **What and failure scenario, per item:**
  - :55 says "Every function returns `int`". Factories return handles and destroys are `void`.
  - :14 (constraint 30) and :380 say unregistered streams are rejected. They are rejected only once an allowlist is set; without one a draw from any name returned `SCS_OK` (`contract.c`). Line :504 has it right.
  - :87–92 says all numeric doubles are validated for finiteness, but `scs_competition_run` accepted `fixed_step_size = NaN` and returned a trace (slice 3c lead).
  - :139 lists `SCS_DEFAULT_YOLK_SAMPLES`, which is not in the header.
  - :220–222 and :398 describe a "current minimal milestone"; the surface has long been complete.
  - :222 and :530 link `docs/status/c_api_extensions_plan.md`, which moved to `docs/status/archive/`.
  - :440–443 shows `scs_level_set_to_polygon` without `out_capacity` (Stage 1 invariant 26).
  - :527 says approval has `one_winner`; the header has none.
  - :592 comments the Yolk as "smallest circle intersecting all median lines", which the header warns against (:580–585).
- **Suggested fix:** Update with the Q9/Q11/Q12 edits.
- **Also affects:** none.

### S03h-015 [P3, confirmed] Heap-export failure state undocumented and uneven
- **Where:** include/scs_api.h:913–928; src/c_api/scs_api.cpp:1907–1916 (`scs_voronoi_cells_2d_heap`)
- **What:** The header does not say what `out` holds on failure. The code zeroes it for most errors but returns before zeroing when `sites_xy` is `NULL`. Neither `_heap` function has a full doc block.
- **Failure scenario:** A caller with an uninitialised `SCS_VoronoiCellsHeap` passes `NULL` sites (`xy` stayed `0xabab…` in `contract.c`). If it then cleans up with `scs_voronoi_cells_heap_destroy` on the error path, as it safely could for other errors, `free` receives a garbage pointer.
- **Suggested fix:** Zero `out` before the null check on `sites_xy`, and document the failure state.
- **Also affects:** none in the bindings as far as checked.

## Tool leads checked
- Stage 0, 7 never-called functions → all documented except the two competition accessors, and all should stay, since both bindings use them. They are `scs_centroid_2d`, `scs_marginal_median_2d`, `scs_geometric_mean_2d`, `scs_winset_symmetric_difference`, `scs_competition_trace_final_positions`, `scs_competition_trace_round_seat_shares`, `scs_competition_experiment_mean_final_seat_shares`. Correct but untested in C++; S03h-005 for the docs.
- Stage 1 invariant 14 (winset "empty iff") → S03h-006.
- Stage 1 invariant 15 (rings vs canvas schema "closed") → rings do not repeat the first vertex (verified); the schema allows JS to close them. Not a defect at this layer; hole winding is for Stage 7.
- Stage 1 invariant 16 (Yolk wording) → S03h-012, S03h-014.
- Stage 1 invariant 17 (grid default 15 vs 30) → header 15 matches R and Python; only the C++ default is 30 (`uncovered_set.h:194`). Not a C API defect.
- Stage 1 invariant 23 (exceptions) → slices 3a–3c own this.
- Stage 1 invariant 24 (null handles) → confirmed holds, 66 calls.
- Stage 1 invariant 25 (non-finite) → S03h-014; per-function detail in 3a–3c.
- Stage 1 invariant 26 → S03h-014.
- Stage 1 invariant 27 (enums) → S03h-002.
- Stage 1 invariant 28 (version, missing Yolk constant) → Q11 known; S03h-014.
- Stage 1 invariant 29 (stability) → Q12 known; S03h-011.
- Stage 1 invariant 30 (thread safety) → Q9 known; S03h-007.
- Stage 1 hotspot, hand-maintained FFI surfaces → Python cdef matches (`cdef_cmp.py`); R glue compiles against the header, so left to Stage 5b.
- SCP-001–004, clang-tidy entries → implementation issues, slices 3a–3c; none concern the header.

## Leak runs
Not applicable to slice 3h. The prompt assigns leak runs to slices 3a–3c.

## Open questions
1. Q17 (unanswered): the header cites `design_document.md`, `where_we_are.md` and `milestone_gates.md` (:13, :583, :1140, :1194), but the files are `Design_Document.md`, `WHERE_WE_ARE.md`, `MILESTONE_GATES.md`.
2. Q20 (unanswered): header :516 promises `[min, max)` for `scs_uniform_real`; Stage 2a found max can be returned.
3. Q27 (unanswered): the header defines "Copeland score" two ways, wins minus losses (:984–985) and wins only (:1720–1721).
4. Q28 (unanswered): `scs_is_selected_by_majority_consistent_rules` documents the current meaning (:1816–1822); the name is in question.
5. Q14 (unanswered): Yolk stays declared but unbound.
6. New: factory functions return only `NULL`, so a binding cannot tell invalid input from out-of-memory except by parsing text. Should factories return an `int` and take an output handle, given Q12 allows the change?
7. New: is Windows a supported target? This sets the priority of S03h-013.

## Out-of-slice notes
- 3a: `k = -5` returns `SCS_OK` from `scs_majority_prefers_2d`, although the header allows only −1 or 1..n.
- 3a: `loss_type = 42` silently computes quadratic loss (`rc=0`, u = −50).
- 3b: `scs_plurality_one_winner` with `tie_break = 99` returned `SCS_OK` (tested on a profile with no tie).
- 3c: `scs_competition_run` accepts `fixed_step_size = NaN`.
