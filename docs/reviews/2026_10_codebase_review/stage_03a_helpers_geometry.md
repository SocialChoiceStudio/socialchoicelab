# Stage 03a — C API helpers, preference, geometry I
Model / effort: Opus / high   Commit: e6772eb   Files reviewed: `src/c_api/scs_api.cpp` lines 1–282 and 582–2034 (1,735 lines), read in full; the matching parts of `include/scs_api.h`; core headers only to settle specific points (level_set.h, winset.h, voronoi_2d.h, winset_1d.h, majority.h).
Not read (if any): none of the slice was skipped.

Probe programs and their output are in
`/private/tmp/claude-501/-Users-robiragan-Dropbox-SocialChoiceStudioDev/2402f97e-221e-494b-88cb-fc63c41efc50/scratchpad/stage3a/`
(called `SCR3A` below): `probe.c`, `probe2.c`, `probe3.c`, `leak_throw.c`, `plain.out`,
`asan.out`, `rel.out`, `rel2.out`, `probe3.out`, `leak_throw.out`, `gtest_leaks.out`.
`SCR3A/rel/libscs_api.dylib` is a private `-O1 -DNDEBUG` build of the same `scs_api.cpp`,
made because the two shared builds are Debug and CGAL behaves differently under `NDEBUG`
(see S03a-007).

## Summary
- 0 P0, 6 P1, 5 P2, 4 P3.
- Input validation for ordinary use is sound after PR #1: finiteness on voters, status quo,
  weights and Voronoi sites/bbox (all three variants), and Q18 zero weights on every shape entry point, all hold when run.
- The most important P1s: all three winset factories leak the handle whenever the core
  throws (S03a-001, 880 bytes per failed call, confirmed by `leaks`); unknown loss and
  distance enum values are silently replaced by quadratic and Euclidean (S03a-002);
  `scs_level_set_to_polygon` returns uninitialised memory for a polygon with more than 4
  vertices (S03a-003); `scs_pairwise_matrix_2d` passes its buffer check when `n_alts*n_alts`
  overflows (S03a-004).
- SCP-001 reproduces through the C API and is wider than Copilot reported: ordinary unit
  weights with a Manhattan or Chebyshev metric are enough, and Voronoi has the same fault
  (S03a-007). On a Release-style build this is SIGFPE (exit 136) or a winset with an Inf
  vertex and `SCS_OK`; on Debug it is a clean CGAL error.
- Leak run: the 109 slice tests pass with 0 leaks. They do not exercise the leaking error path; `leak_throw.c` does.

## Contracts this layer provides
For Stage 5. Applies to `scs_api.cpp` at e6772eb.

**General**
- Error codes: `SCS_OK` 0, `INVALID_ARGUMENT` 1, `INTERNAL` 2, `BUFFER_TOO_SMALL` 3,
  `OUT_OF_MEMORY` 4. Handle-returning functions return `NULL` and only write `err_buf`.
- `err_buf`: written only when non-null and `err_buf_len > 0`; always NUL-terminated,
  truncated to fit. With `err_buf_len == 1` it becomes the empty string.
- Core `invalid_argument` → 1 and other `std::exception` → 2 in most functions. Exceptions
  are not caught as a separate `bad_alloc` anywhere in this slice, so allocation failure is 2
  (S03a-013).
- Thread safety (Q9): this slice has no static or global mutable state; each `static`
  here is a file-local function. Per Q9, no handle may be shared between threads, even for
  reads: `contains_point`, `bbox` and boundary export call `polygons_with_holes`, which
  writes face flags on the shared region.
- Debug and Release differ for extreme finite coordinates (S03a-007); tests on a Debug build
  do not show the Release behaviour.

**Distance and loss**
- `scs_calculate_distance`: rejects null pointers, `n_dims <= 0`, `n_weights != n_dims`,
  non-finite points or weights, and unknown distance type (1). Weight 0 is allowed; negative
  weight and bad Minkowski `order_p` are rejected by the core (1).
- `scs_distance_to_utility`, `scs_normalize_utility`: reject null and non-finite scalar
  inputs; parameter domains are checked by the core (1). `normalize_utility` does not clamp:
  the header's "[0, 1]" is not enforced (utility −30, max distance 5 gives −0.2).
  **Unknown `loss_type` is treated as quadratic** (S03a-002).

**Level sets and indifference curves**
- `out_points` for the 1D functions must hold 2 doubles; `*out_n` is 0, 1 or 2.
- `scs_level_set_1d` / `scs_ic_interval_1d`: an unusable loss config returns `SCS_OK` with
  `*out_n = 0` (S02a-005, still present). A weight of 0 or less is an error.
- `scs_level_set_2d`: requires `n_weights == 2`, finite values, both weights > 0
  (Q18, core message). `out` is written only on success. Unknown `distance_type` falls back
  to Euclidean (S03a-002). A POLYGON result always has `center_x = center_y = 0`
  (S03a-011).
- `scs_level_set_to_polygon`, `scs_level_set_polygon_2d`, `scs_ic_polygon_2d`: size query
  with `out_xy == NULL`. Non-null with `out_capacity` below the required size returns 3 with
  `*out_n` set, including `out_capacity == 0`. Capacity is in (x, y) pairs. `num_samples >= 3`
  for smooth shapes. `scs_ic_polygon_2d` requires this for polygon shapes too; the level-set functions do not.

**Convex hull and majority**
- `scs_convex_hull_2d`: `out_xy` has no capacity argument and must hold `2*n_points` doubles.
- `k`: the header allows `SCS_MAJORITY_SIMPLE` or `1..n`; in practice any negative value
  means simple majority (S03a-012). `k = 0` or `k > n` is rejected by the core.
- `scs_pairwise_matrix_2d`: no size query; `out_len < n_alts*n_alts` returns 3 (subject to
  S03a-004). `k` is validated but not used in the matrix.
- `scs_weighted_majority_prefers_2d`: `threshold` in (0, 1] and weights > 0, core-checked. Exactly
  half the weight wins in both directions (S02b-006; Q6).

**Winsets**
- Factories: voters non-null, `n_voters >= 1`, `num_samples >= 4`, finite voters/status
  quo/weights/threshold, two finite salience weights > 0 (Q18), valid distance type.
  `NULL` on any failure. Caller frees with `scs_winset_destroy` (NULL is a no-op). A failure
  raised inside the core leaks one handle (S03a-001).
- `is_empty`: polygon-level today (Q5). `contains_point`: strict interior, point must be
  finite. `bbox`: on empty, `*out_found = 0` and the other outputs are untouched.
- `sample_boundary`: `NULL` or capacity 0 means size query for that buffer; 3 if a supplied
  buffer is too small, with required sizes set and nothing written. `out_path_is_hole` is
  optional and is written only if `out_path_starts` is supplied.
- `scs_winset_2d_export_boundary`: every input error returns 2, not 1 (S03a-008).
- `scs_winset_interval_1d`: does not reject NaN or Inf (S03a-006). NaN output means empty.
- Boolean operations and clone return new owned handles and do not modify their inputs.

**Voronoi**
- `_size`, fill and `_heap` all reject non-finite sites and bbox before the core; `n_sites < 1`
  or an inverted box returns 1. `n_sites` itself is unchecked until the core (SCP-003).
- Fill: as `sample_boundary` above; `out_cell_start` needs `n_sites + 1` entries.
- `_heap`: `out` is zeroed first. Free with `scs_voronoi_cells_heap_destroy`, not
  `scs_heap_free`. `xy` is NULL when there are no vertices. `cell_start` is always allocated on success.
- There is one cell per site. Empty cells have equal consecutive `cell_start` values.
  Coincident sites return two overlapping cells with no shared marker (Q7, known).

## Findings

### S03a-001 [P1, confirmed] Winset factories leak the handle when the core throws
- **Where:** src/c_api/scs_api.cpp:1409, :1449, :1496 (`new SCS_WinsetImpl{}`, then assignment inside `try`); same pattern at :2000 (`WINSET_BINOP`)
- **What:** The handle is allocated before calling the core, and the `catch` blocks only set the error and return `nullptr`. Any exception from `winset_2d`, `weighted_winset_2d` or `winset_with_veto_2d` loses the handle.
- **Failure scenario:** `SCR3A/leak_throw.c` makes 200 failing calls to each factory (k = 9 with 3 voters; threshold 2.0). `leaks --atExit`: "6000 leaks for 528000 total leaked bytes", with stacks at `scs_winset_2d` (:1409), `scs_weighted_winset_2d` (:1449) and `scs_winset_with_veto_2d` (:1496). That is 880 bytes and 10 blocks per failed call. Reachable from R and Python with any `k` or threshold the core rejects. The boolean operations follow the same pattern but I found no input that makes them throw.
- **Suggested fix:** Build the region first and allocate last (`auto region = ...; return new SCS_WinsetImpl{std::move(region)};`), or hold the handle in `std::unique_ptr` until success. Add a leak test for the failing path.
- **Also affects:** R and Python `winset_2d` family.

### S03a-002 [P1, confirmed] Unknown loss and distance enum values are silently replaced
- **Where:** src/c_api/scs_api.cpp:235–236 (`to_cpp_loss` default → QUADRATIC), :260–261 (`to_cpp_dist` default → EUCLIDEAN)
- **What:** Out-of-range enum values from C are mapped to a default instead of being rejected. `to_dist_config` (:196) and `scs_calculate_distance` (:643) do reject them, so behaviour differs by function. Stage 2a's contract relies on the C API to pass only valid types.
- **Failure scenario:** `SCR3A/probe enum_loss`, `enum_loss_ic`, `enum_dist`: `loss_type = 99`, distance 2 → `SCS_OK`, utility −4 (quadratic). `distance_type = 77` in `scs_level_set_2d` → `SCS_OK`, a radius-1 circle; `scs_ic_polygon_2d` also succeeds; `scs_winset_2d` correctly returns "unknown SCS_DistanceType value". `scs_level_set_1d`, `scs_normalize_utility`, `scs_distance_to_utility` and `scs_ic_polygon_2d` all take the same path.
- **Suggested fix:** Return an error from both mappings (unknown value named in the message) and call them before any computation.
- **Also affects:** Only direct C callers and any binding that forwards an unchecked integer. The R and Python layers map from strings.

### S03a-003 [P1, confirmed] `scs_level_set_to_polygon` does not validate a caller-supplied polygon
- **Where:** src/c_api/scs_api.cpp:932–933, :980–984, :995–997; `SCS_LevelSet2d` doc at include/scs_api.h:177 ("n_vertices (always 4)")
- **What:** `n_vertices` is used as given. For values above 4, the vector is resized and only the first 4 vertices are filled, so the remaining `Eigen::Vector2d` values are uninitialised and copied to the caller. A negative value becomes a huge `size_t`. `n_vertices * 2` also overflows. `exponent_p` is not checked either.
- **Failure scenario:** `SCR3A/probe lvl_uninit` on the ASan build with `malloc_fill_byte=190`: `n_vertices = 6`, `n=6: 1 2 3 4 5 6 7 8 -1.83255e-06 -1.83255e-06 -1.83255e-06 -1.83255e-06`, which is the allocator's fill pattern returned with `SCS_OK`. `lvl_neg`: `n_vertices = -1` → rc 2, message "vector". `lvl_huge`: `n_vertices = 1<<30` → UBSan "signed integer overflow" at :932. `lvl_exp`: `exponent_p` 0 gives all-zero vertices; −2 or 5000 give `inf`/`nan` vertices, all with `SCS_OK`. An R user can reach the first case with a hand-built list: `r/src/functions.c` copies `nrows(vmat)` into `n_vertices`. Python's cffi bounds check blocks more than 4.
- **Suggested fix:** For POLYGON require `n_vertices == 4` (the header's contract), and for SUPERELLIPSE require a finite `exponent_p >= 1`, with end-user errors. Return 1.
- **Also affects:** R `level_set_to_polygon`; `exponent_p` in Python's dict path as well. Relates to S02a-009.

### S03a-004 [P1, confirmed] `scs_pairwise_matrix_2d` buffer check is bypassed when `n_alts * n_alts` overflows
- **Where:** src/c_api/scs_api.cpp:1246–1254, also :1287 (`i * n_alts + j`)
- **What:** `required = n_alts * n_alts` is computed in `int` with no upper limit on `n_alts`. For `n_alts >= 46341` the product overflows, so a tiny `out_len` passes the check. The fill then writes `n_alts²` cells into the caller's buffer. This is the SCP-003 pattern with a far lower threshold than `n*2`.
- **Failure scenario:** `SCR3A/probe pair_wrap` (`n_alts = 46341`, `out_len = 1`) and `pair_wrap0` (`65536`, `out_len = 0`, product wraps to 0). The plain build returns rc 1 from the later NaN check on `alt_xy[0]` instead of the expected `BUFFER_TOO_SMALL`, proving the size check passed. The ASan/UBSan build stops at :1246 with "signed integer overflow: 46341 * 46341". With finite input the function would go on to write out of bounds; I stopped short of that because it needs an 8.6 GB matrix.
- **Suggested fix:** Compute `required` as `int64_t` and reject `n_alts` whose square exceeds `INT_MAX` with an end-user error.
- **Also affects:** Other slices use `n_alts * n_alts`-style lengths; each reviewer checks its own.

### S03a-005 [P1, confirmed] `scs_convex_hull_2d` computes `n_points * 2` before checking `n_points` (another SCP-003 instance)
- **Where:** src/c_api/scs_api.cpp:1129–1133
- **What:** The finiteness scan uses `n_points * 2` before the `n_points < 1` check. A large negative count wraps to a large positive scan length that reads past the buffer.
- **Failure scenario:** `SCR3A/probe hull_neg` with a 4-element array and `n_points = -1073741825`. Plain build: rc 1, "points: non-finite value at index 49", so it read at least 46 doubles past the end. ASan/UBSan build: "signed integer overflow: -1073741825 * 2" at :1129. Only direct C callers can do this.
- **Suggested fix:** Move the range check first and bound `n_points` by `INT_MAX / 2`; reuse the helper proposed for SCP-003.
- **Also affects:** See S03a-009 for the positive-only sites.

### S03a-006 [P1, confirmed] `scs_winset_interval_1d` does not reject NaN or Inf
- **Where:** src/c_api/scs_api.cpp:1716–1740; core include/socialchoicelab/geometry/winset_1d.h (`std::sort`, NaN return meaning "empty")
- **What:** It is the only function in the slice that accepts `double` positions without a finiteness check. NaN is also the documented "empty" marker, so a bad input looks like a valid "no winset" answer. A NaN in `xs` also violates `std::sort`'s strict weak ordering, which is undefined behaviour.
- **Failure scenario:** `SCR3A/probe w1d_nan`, all `SCS_OK`: voters (NaN, 2, 3) with seat 1 give the same answer as voters (0, 2, 3), `[1, 3]`, so the NaN voter is silently ignored; a NaN or Inf seat returns NaN/NaN ("empty winset"); an Inf voter gives `[1, 3]`. `w1d_nan_sort` (2,000 elements, one third NaN) completes under ASan in this libc++, but that is not a guarantee.
- **Suggested fix:** Validate `voter_x` and `seat_x` with `validate_finite_doubles` / `isfinite`, returning 1 with an end-user message.
- **Also affects:** `python/.../plots.py` (direct call) and `r/src/winset_1d.c` both reach it; neither adds a check visible in the code I inspected (Stage 5 to confirm).

### S03a-007 [P2, confirmed] Finite but extreme coordinates still reach CGAL as Inf (SCP-001, wider than reported)
- **Where:** include/socialchoicelab/preference/indifference/level_set.h:228 and :359–388 (vertex = centre + semi-axis); geometry/winset.h:98 and :111 (`to_exact_polygon`, `pts_polygon`); geometry/voronoi_2d.h:59–62 (`perp_bisector_line`). Unchecked at src/c_api/scs_api.cpp:1389 (`scs_winset_2d`) and :1818 (`scs_voronoi_cells_2d_size`).
- **What:** PR #1 checks the semi-axes but not the sums that become vertices, and the C API checks only that inputs are finite. Copilot's case needed tiny weights; none are needed for Manhattan or Chebyshev. The Voronoi bisector overflows the same way.
- **Failure scenario:** On the private NDEBUG build (`rel.out`, `rel2.out`):
  - Manhattan, weights (1, 1), one voter (1.5e308, 0), status quo (0, 0): `scs_winset_2d` returns `SCS_OK` with a boundary containing `inf`.
  - The same voter with 3 voters and k = 2: **exit 136 (SIGFPE)**, killing the process.
  - Voronoi sites (1e308, 1e308) and (1.5e308, −1.5e308), box ±1: **exit 136**.
  - `scs_ic_polygon_2d` returns `(inf, 0)` as a vertex with `SCS_OK`.
  - On the shared Debug build the same winset and Voronoi inputs give `CGAL ERROR: assertion violation! Expr: is_finite(d)` and a clean error, so a Debug test would not show the crash.
  - Rated P2 as for SCP-001 because the inputs are extreme.
- **Suggested fix:** Check every sampled vertex and every Voronoi bisector point for finiteness before constructing exact values, and return an end-user error. Consider scaling the bisector construction.
- **Also affects:** All winset-based C API calls and R/Python. Stage 10 should test on a Release build.

### S03a-008 [P2, confirmed] `scs_winset_2d_export_boundary` reports every input error as internal
- **Where:** src/c_api/scs_api.cpp:1753–1756 (`if (!ws) return SCS_ERROR_INTERNAL;`)
- **What:** `scs_winset_2d` signals failure only by NULL, so the compound function can't tell invalid input from internal failure. It returns 2 for everything, although `err_buf` has the right text and the header's error-code contract defines 1 for invalid arguments.
- **Failure scenario:** `SCR3A/probe export_code`: NaN status quo, `k = 9`, and null voters all give rc 2. Zero weights (`zero_w_shapes`) also give rc 2 with the correct Q18 message. Python would raise `SCSInternalError` for a user's bad `k`.
- **Suggested fix:** Have the compound call a shared internal builder that returns a code, or validate first and map to 1.
- **Also affects:** `python/.../plots.py` winset boundary export (error type shown to users).

### S03a-009 [P2, confirmed] Remaining `n * 2` length computations can overflow (SCP-003 pattern)
- **Where:** src/c_api/scs_api.cpp:1186, :1255, :1257, :1317, :1382, :1479 (after a lower-bound check only); also `2 * i` at :279, :1143, :1204 and elsewhere
- **What:** These sites check `n >= 1` but not an upper limit. For `n > INT_MAX / 2`, `n * 2` overflows. A wrapped negative length makes `validate_finite_doubles` return true (`count <= 0`), so the NaN check is skipped.
- **Failure scenario:** `SCR3A/probe maj_big`, `n_voters = 1073741825`: UBSan reports the overflow at :1186. In a non-sanitised build the finiteness scan would be skipped. Reachable only with a buffer of 16 GB or a dishonest count. Not reported again for the three Voronoi entry points (:1787), which is SCP-003 itself.
- **Suggested fix:** One helper checks `n` against `[1, INT_MAX/2]` and returns an end-user error before any arithmetic. Use it in every slice.
- **Also affects:** The same pattern appears at lines 2054–4113, in slices 3b and 3c.

### S03a-010 [P2, confirmed (logic)] Allocations before `try` can let `std::bad_alloc` cross the C boundary
- **Where:** src/c_api/scs_api.cpp:123–136 (`validate_finite_doubles`), :160–204 (`to_dist_config`, vector assignment at :201), :1179–1184 and every other pre-`try` message built with `std::string`
- **What:** The file's design rule says no exception escapes. Validation runs before each `try` and builds `std::string`s or assigns a vector, so an allocation failure there unwinds through `extern "C"`. In the whole slice, one function has `catch (...)` (:1157).
- **Failure scenario:** Under memory pressure, a NaN voter's error-message concatenation throws `bad_alloc`, unwinding into R or Python C frames (undefined behaviour). Requires low memory, so reach is narrow; not run.
- **Suggested fix:** Run validation inside each `try`, or wrap each entry point in one macro with `catch (...)` as a last resort.
- **Also affects:** All slices.

### S03a-011 [P2, confirmed] A POLYGON level set reports centre (0, 0), contrary to the header
- **Where:** src/c_api/scs_api.cpp:875–878; include/scs_api.h:177; `python/.../_functions.py:272` (docstring "centre of the level set")
- **What:** The header lists `center_x/y` as valid for POLYGON, and the Python docstring promises them. The code sets them to 0.
- **Failure scenario:** `SCR3A/probe poly_center`: Manhattan with ideal (3, 4) → `center=(0,0)`, first vertex (4, 4). R and Python both pass this on.
- **Suggested fix:** Set the centre to the ideal point in `scs_level_set_2d`, or change the header and the docstrings to say the centre is unset.
- **Also affects:** R and Python `level_set_2d` return values.

### S03a-012 [P3, confirmed] Any negative `k` is accepted as simple majority
- **Where:** src/c_api/scs_api.cpp:1172, :1392 (forwarded unchecked); include/scs_api.h:609, :690, and similar
- **What:** The header allows only `SCS_MAJORITY_SIMPLE` or `1..n_voters`. The core treats any negative value as simple majority (Stage 2b Contracts).
- **Failure scenario:** `SCR3A/probe k_neg`: `k = -7` → `SCS_OK` in `scs_majority_prefers_2d` and a valid winset, where a typo could otherwise be caught.
- **Suggested fix:** Reject `k < 1` unless `k == SCS_MAJORITY_SIMPLE`, or change the header to say "any negative value".
- **Also affects:** Bindings that forward `k`.

### S03a-013 [P3, confirmed (logic)] `SCS_ERROR_OUT_OF_MEMORY` is never returned for a C++ allocation failure in this slice
- **Where:** src/c_api/scs_api.cpp, slice-wide (no `bad_alloc` catch; only the two `malloc` checks in `scs_voronoi_cells_2d_heap`, :1940, :1950)
- **What:** The header defines code 4 for "a handle construction or internal heap allocation failed", but `bad_alloc` and `length_error` fall into `catch (std::exception)` and return 2. `scs_voronoi_cells_2d_heap` also leaves non-zero `n_xy_pairs` / `cell_start_len` with null pointers on `malloc` failure.
- **Failure scenario:** A very large `num_samples` that makes `reserve` fail returns 2 with a libc++ message rather than 4 with an end-user message. Not run, because of memory use on this machine.
- **Suggested fix:** Add `catch (const std::bad_alloc&)` to the shared error handling, and reset the heap struct on failure.
- **Also affects:** All slices.

### S03a-014 [P3, confirmed] Many error messages do not meet the end-user rule
- **Where:** src/c_api/scs_api.cpp, e.g. :604, :609, :732, :782, :822, :1061, :1370, :1516; about 115 `set_error` uses in the slice against 12 lines using `std::to_string`
- **What:** Most messages say what failed but not what was entered or how to fix it, e.g. "null pointer or non-positive n_dims" and "n_weights must equal n_dims". Some omit the function name ("voter_ideals_xy must not be null"). Exception text passes straight through: `lvl_neg` shows the message "vector", and on Debug users see a raw `CGAL ERROR: assertion violation!`.
- **Failure scenario:** `scs_calculate_distance` with `n_weights = 3`, `n_dims = 2`: the message names neither value. `SCR3A/plain.out`.
- **Suggested fix:** Standardise on messages containing what failed, what was entered and the fix; convert `length_error` and CGAL text to end-user messages at the boundary.
- **Also affects:** R and Python, which pass the C text through.

### S03a-015 [P3, confirmed] Header still states exact emptiness for `scs_winset_is_empty`
- **Where:** include/scs_api.h:744 ("empty (i.e. no policy beats the SQ)"). Decision: Q5
- **What:** An additional instance beyond the Q5 Implies list: the header claims the true meaning of emptiness, but the value comes from sampled polygons. The same value is also returned via `*out_is_empty` from `scs_winset_2d_export_boundary` (:1763).
- **Failure scenario:** Near-Plott voters: the header says no policy beats the status quo when the polygon set is empty, but Q5 records this as false at finite sampling.
- **Suggested fix:** Reword when Q5 is implemented, and have the export use the same exact test. Slice 3h may report the same header line.
- **Also affects:** R and Python documentation.

## Tool leads checked
- Stage 2: `scs_level_set_to_polygon` uninitialised vertices and negative `n_vertices` → S03a-003.
- Stage 2: `to_cpp_dist` maps unknown type to Euclidean → S03a-002.
- Stage 2: `to_cpp_loss_config` does not validate → enum half is S03a-002; parameter domains are checked by the core (2D returned rc 1 for NaN sensitivity); the 1D swallow is S02a-005, still present.
- Stage 2: Voronoi finiteness → fixed in PR #1; `validate_voronoi_inputs` covers `_size`, fill and `_heap`; verified by `CApi_NonFiniteInput` tests.
- Stage 2: `to_dist_config` zero weights → fixed in PR #1 for all three winset factories via `check_shape_weights`; `level_set_2d` and `ic_polygon_2d` reject in the core; `zero_w_shapes` confirms (Q18). Zero remains allowed for distance and majority, as decided.
- SCP-001 → confirmed through the C API, S03a-007.
- SCP-002 → reachable: weights (1e-13, 1e-12) through `scs_level_set_2d` return a circle (`SCR3A/probe3.out`); no C API check; no new finding.
- SCP-003 → not re-reported; other instances are S03a-004, S03a-005, S03a-009, and `n_vertices * 2` in S03a-003.
- Stage 2 S02a-004 (Gaussian IC round trip through `scs_ic_interval_1d`) → code unchanged; not re-reported.
- Stage 2 S02b-006 (weighted majority half-weight) → unchanged in `scs_weighted_majority_prefers_2d`; not re-reported.
- Stage 0 clang-tidy `security.ArrayBound` at old :1894 (`cs_buf[n_cells]`, now :1964) → false positive: `voronoi_cells_2d` returns one cell per site, and the buffer holds `n_sites + 1`.
- Stage 0 `implicit-widening` at old :263, :865, :967, :998, :1087, :1127, :1188, :1254, :1256, :1324, :1660, :1821, :1889 → part of the overflow class in S03a-009; no separate finding.
- Stage 0 coverage/never-called functions → in this slice only `scs_winset_symmetric_difference` has no call in `test_c_api.cpp`. Direct probe (`probe3.c`) gives correct results for a point in one region and a point in both: correct but untested.
- Stage 0 uncovered error paths → untested paths in this slice include the throw path in S03a-001, unknown enums (S03a-002) and malformed polygons (S03a-003).
- Q2 → `level_set_at_distance_2d` p ≥ 100 branch is the Implies item, moved by PR #1; not re-reported.
- Q5 → `scs_winset_is_empty` polygon-level (Implies); extra header claim is S03a-015.
- Q6 → `scs_weighted_winset_2d` copying and threshold (Implies); no new instance.
- Q7 → coincident Voronoi sites return two overlapping cells with no shared marker (`vor_dup`), the known Implies item; `SCS_VoronoiCellsHeap` has no field for it yet.
- Q9 → no global or static mutable state in this slice; no new instance.
- Q18 → holds at every shape entry point in the slice.

## Leak runs
- Group run, from the scratch directory, on the Debug build:
  `MallocStackLogging=1 leaks --atExit -- $SCR/stage3/build/test_c_api --gtest_filter='CApi_Version*:CApi_ErrorCodes*:CApi_ErrorPaths*:CApi_Distance*:CApi_SalienceWeights*:CApi_ScalarDomains*:CApi_Loss*:CApi_LevelSet*:CApi_IcPolygon2d*:CApi_IcInterval1d*:CApi_ToPolygon*:CApi_ConvexHull*:CApi_Majority*:CApi_WeightedMajority*:CApi_Pairwise.*:CApi_PairwiseMatrix*:CApi_Winset*:CApi_Voronoi2d*:CApi_NonFinite*'`
  → 109 tests passed; `0 leaks for 0 total leaked bytes` (`SCR3A/gtest_leaks.out`).
- Failing-factory run: `MallocStackLogging=1 leaks --atExit -- ./leak_throw` → 600 null returns; `6000 leaks for 528000 total leaked bytes`, stacks through `scs_winset_2d`, `scs_weighted_winset_2d` and `scs_winset_with_veto_2d` (`SCR3A/leak_throw.out`). See S03a-001.

## Open questions
1. S03a-007: should the finite-vertex check be in the core (every shape and bisector) or at the C API? The core has callers besides the C API, which argues for the core; the choice affects the Stage 11 fix plan.
2. S03a-006: when a 1D winset gets non-finite input, an error is the natural fix, but NaN is also the "empty" return value. Please confirm that NaN stays as the "empty" sentinel for outputs only.

## Out-of-slice notes
- The `n * 2` and `n_alts * n_alts` overflow pattern appears in `src/c_api/scs_api.cpp` at lines 2054–2644, 3433–3475 and 4046–4113 (slices 3b, 3c).
- R `level_set_to_polygon` (`r/src/functions.c`) and Python's dict path pass caller-built level sets without validating them. Stage 5.
- `python/.../plots.py` and `r/src/winset_1d.c` do not appear to check finiteness before `scs_winset_interval_1d`. Stage 5.
- `scs_ic_polygon_2d` needs `num_samples >= 3` even for polygon shapes, while `scs_level_set_polygon_2d` ignores it for them. A small inconsistency for the 3h header review.
