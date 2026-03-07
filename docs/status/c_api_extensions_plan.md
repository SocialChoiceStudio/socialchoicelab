# c_api Extensions Plan

Short-to-medium-term plan to extend the stable C API (`scs_api`) with all
geometry (Layer 3) and aggregation (Layers 4–5) services. After this milestone,
R/Python bindings will be able to call every implemented feature without ever
touching C++, Eigen, or CGAL directly.

**Authority:** This plan is the source for "what's next" for c_api work.
When a step is done, mark it ✅ Done and update [where_we_are.md](where_we_are.md).

**References:**
- [Design Document](../architecture/design_document.md) — FFI & Interfaces, c_api design inputs
- [c_api_design.md](../architecture/c_api_design.md) — existing minimal surface (Items 29–31)
- [Archived geometry plan](archive/geometry_plan.md) — C++ services being exposed
- [Archived aggregation plan](archive/profiles_and_aggregation_plan.md) — C++ services being exposed
- [ROADMAP.md](ROADMAP.md) — near/mid-term context

---

## Design principles (apply throughout)

1. **No C++ types, STL, or exceptions cross the boundary.** All inputs and
   outputs are plain C types: scalars, flat arrays, POD structs, enums, or
   opaque handles.
2. **Opaque handles for complex C++ objects.** `WinsetRegion` (CGAL
   `General_polygon_set_2`) and `Profile` (vector-of-vectors) cannot be
   represented as flat C POD. Both are exposed as opaque `SCS_Winset*` and
   `SCS_Profile*` handles with explicit lifecycle (create / destroy).
3. **Array-writing APIs always carry explicit output capacity.** If a function
   writes into `int*` or `double*` output storage, it also takes an `out_len`
   or `out_capacity` parameter. For fixed-size outputs, capacity must be at
   least the required size. For variable-size outputs, the function writes at
   most `out_capacity` elements and sets `*out_n` to the number required or
   written.
4. **Variable-size outputs support safe discovery of required size.** For APIs
   such as sampled boundaries, winner lists, and discrete sets, passing a null
   output buffer or zero capacity is permitted when the intent is to query
   `*out_n` first. If a non-null buffer is too small, return
   `SCS_ERROR_BUFFER_TOO_SMALL`.
5. **Complex geometry export is approximate unless stated otherwise.** The C
   API does not pretend to serialize CGAL polygon sets losslessly. Any boundary
   export function is explicitly documented as sampled / approximate, with
   enough metadata to represent multiple disconnected paths.
6. **Tie-breaking is explicit and deterministic.** `SCS_TieBreak` + nullable
   `SCS_StreamManager*` is used wherever a rule can return more than one winner
   but the API must expose a single-winner variant.
   `SCS_TIEBREAK_RANDOM` requires a non-null manager and valid stream name.
   `SCS_TIEBREAK_SMALLEST_INDEX` accepts a null manager.
7. **Optional outputs use a found-flag.** Where C++ returns `std::optional`
   (e.g. `condorcet_winner`, `core_2d`), the C API returns `SCS_OK` and writes
   `*out_found = 1` or `0`.
8. **All indices are 0-based.** The R binding layer translates to 1-based at
   the binding boundary, not here.
9. **Borrowed inputs are valid only for the duration of the call.** In
   particular, `SCS_DistanceConfig.salience_weights` is caller-owned storage and
   must not be cached by long-lived handles unless copied internally.
10. **NULL-handle behavior is explicit.** `*_destroy(NULL)` is a no-op. All
   other opaque-handle APIs return `SCS_ERROR_INVALID_ARGUMENT` if passed a null
   handle.
11. **Error handling remains uniform, but is slightly extended.** Functions
   return `SCS_OK` / error code + optional `char* err_buf` /
   `int err_buf_len`, matching the existing pattern, with two additions:
   `SCS_ERROR_BUFFER_TOO_SMALL` and `SCS_ERROR_OUT_OF_MEMORY`.
12. **FFI ergonomics take priority over C++-native type choices.** Public sizes
   and counts remain `int` for binding simplicity, but every derived size
   computation must validate non-negativity and guard integer overflow.
13. **New ABI-visible scalar domains use fixed-width integer representations.**
   For new stable values that cross the boundary (for example tie-break modes
   and pairwise outcomes), prefer `int32_t` typedefs plus named constants over
   public enums whose size is implementation-defined in C.
14. **Thread-safety and reentrancy are documented explicitly.** Functions that
   do not mutate shared state should be callable concurrently on distinct
   handles. `SCS_StreamManager*` is not thread-safe for concurrent mutation or
   draws without external synchronization. Shared handle access rules are
   documented in `c_api_design.md` so Python bindings can decide when it is
   safe to release the GIL and R bindings can attach the correct caveats to
   external-pointer wrappers.

---

## Scope

**In scope:**

- C API wrappers for all geometry services: majority preference, pairwise
  matrix, winset (opaque handle + boolean ops + sampled boundary export), Yolk,
  uncovered set (discrete + boundary), Copeland, core/Condorcet detection,
  Heart (discrete + boundary).
- C API wrappers for all aggregation services: profile construction, synthetic
  generators, positional voting rules (plurality, Borda, anti-plurality,
  generic scoring rule), approval voting, social rankings, Pareto efficiency,
  and profile-level Condorcet / majority-selection predicates.
- Header-level API hygiene improvements needed by the expanded stable surface:
  version query, named majority sentinel, fixed-width ABI-safe scalar domains
  for new result/tie-break values, new error codes, explicit sampled-boundary
  contracts, thread-safety notes, and additive helpers that bring older
  variable-size APIs into the same size-query pattern.
- New POD structs / scalar domains: `SCS_Yolk2d`, `SCS_TieBreak`,
  `SCS_PairwiseResult`.
- New opaque types: `SCS_Winset*`, `SCS_Profile*`.
- Tests covering all new functions (normal paths + key error paths).
- Updated `c_api_design.md` with all new types, functions, contracts, and
  stability notes.

**Out of scope:**

- R or Python binding code (next milestone).
- Any new C++ algorithms (geometry and aggregation layers are feature-complete).
- GUI or visualization.
- A fully lossless serialized polygon-set format for `WinsetRegion`. This plan
  exposes sampled boundary paths plus useful query helpers instead.

---

## Phase C0 — Shared C API hygiene and contracts

These are cross-cutting items that should land before the large batch of new
wrappers, because they affect signatures, tests, and documentation throughout.

### C0.1 — API version and visibility hooks

Add the following to `scs_api.h`:

```c
#define SCS_API_VERSION_MAJOR 0
#define SCS_API_VERSION_MINOR 1
#define SCS_API_VERSION_PATCH 0

#define SCS_MAJORITY_SIMPLE (-1)

#define SCS_DEFAULT_WINSET_SAMPLES 64
#define SCS_DEFAULT_YOLK_SAMPLES 720
#define SCS_DEFAULT_BOUNDARY_GRID_RESOLUTION 15

#if defined(_WIN32)
#  if defined(SCS_API_BUILD)
#    define SCS_API __declspec(dllexport)
#  else
#    define SCS_API __declspec(dllimport)
#  endif
#else
#  define SCS_API __attribute__((visibility("default")))
#endif

int scs_api_version(
    int* out_major, int* out_minor, int* out_patch,
    char* err_buf, int err_buf_len);
```

Notes:

- `scs_api_version` follows the standard C API return-code pattern rather than
  introducing a one-off `void` function.
- `SCS_API` is included now so the stable surface can grow behind one macro
  rather than retrofitting symbol visibility later.
- Version policy is explicit: matching major versions are intended to be
  ABI-compatible; minor versions are additive; patch versions are behavioral /
  documentation fixes that do not change signatures or struct layout.

### C0.2 — Extend error codes minimally

Keep the current compact scheme, but add:

```c
#define SCS_ERROR_BUFFER_TOO_SMALL 3
#define SCS_ERROR_OUT_OF_MEMORY    4
```

Behavior:

- `SCS_ERROR_INVALID_ARGUMENT` remains the result for null pointers, bad
  dimensions, invalid indices, invalid tie-break combinations, non-finite
  inputs, and other caller-side contract violations.
- `SCS_ERROR_BUFFER_TOO_SMALL` is used when a caller-provided output buffer is
  valid but not large enough.
- `SCS_ERROR_OUT_OF_MEMORY` is used when a handle construction or internal
  allocation fails.

### C0.3 — Name new scalar result domains without enum-size ambiguity

Replace anonymous `{-1, 0, 1}` documentation with an ABI-stable named scalar
domain:

```c
typedef int32_t SCS_PairwiseResult;

#define SCS_PAIRWISE_LOSS ((int32_t)-1)
#define SCS_PAIRWISE_TIE  ((int32_t)0)
#define SCS_PAIRWISE_WIN  ((int32_t)1)
```

This keeps the ABI simple and predictable for `ctypes`, `cffi`, and `.Call`
bindings without relying on implementation-defined enum size.

### C0.4 — Add capacity parameter to `scs_level_set_to_polygon`

The existing `scs_level_set_to_polygon` takes a caller-supplied `out_xy`
buffer but never checks whether it is large enough — the caller must know the
right size themselves and there is no protection against overflow.

Replace the existing signature directly with one that adds an explicit
`out_capacity` parameter (counted in `(x,y)` pairs) and returns
`SCS_ERROR_BUFFER_TOO_SMALL` if the buffer is too small:

```c
int scs_level_set_to_polygon(
    const SCS_LevelSet2d* level_set,
    int num_samples,
    double* out_xy, int out_capacity,   // counted in (x,y) pairs
    int* out_n,
    char* err_buf, int err_buf_len);
```

Required capacity is `num_samples` pairs for smooth shapes
(CIRCLE/ELLIPSE/SUPERELLIPSE) and 4 pairs for POLYGON type. If `out_xy` is
non-null and `out_capacity` is smaller than the required size, return
`SCS_ERROR_BUFFER_TOO_SMALL` and write the required size into `*out_n`. If
`out_xy` is null, write the required size into `*out_n` and return `SCS_OK`
(size-query mode).

Update `test_c_api.cpp` accordingly.

### C0.5 — Thread-safety contract

Document the concurrency contract before adding many more entry points:

- Public functions are reentrant unless they operate on shared mutable state.
- Read-only queries on distinct `SCS_Profile*` / `SCS_Winset*` handles are safe
  to call concurrently.
- Concurrent access to the same `SCS_Profile*` or `SCS_Winset*` is only
  supported for read-only operations if the implementation stores no lazy
  caches; otherwise document that callers must synchronize.
- `SCS_StreamManager*` is not thread-safe for concurrent draws, resets, or
  registration changes without external locking.
- `c_api_design.md` must include a short "Thread safety / reentrancy" section so
  Python bindings know when `Py_BEGIN_ALLOW_THREADS`-style release is safe.

---

## Phase C1 — Geometry: majority preference

### C1.1 — `scs_majority_prefers_2d`

Wraps `geometry::majority_prefers(x, y, voter_ideals, cfg, k)`.

Contract updates:

- Use clearer point names in the public signature.
- `k == SCS_MAJORITY_SIMPLE` means simple majority.
- Otherwise require `1 <= k <= n_voters`.
- Returns `1` in `*out` if **at least** `k` voters prefer point A to point B.

```c
int scs_majority_prefers_2d(
    double point_a_x, double point_a_y,
    double point_b_x, double point_b_y,
    const double* voter_ideals_xy,      // length 2*n_voters
    int n_voters,
    const SCS_DistanceConfig* dist_cfg,
    int k,                              // SCS_MAJORITY_SIMPLE or 1..n_voters
    int* out,                           // 1 if A defeats B, else 0
    char* err_buf, int err_buf_len);
```

### C1.2 — `scs_pairwise_matrix_2d`

Wraps `geometry::pairwise_matrix(alternatives, voter_ideals, cfg, k)`.
Returns an `n_alts × n_alts` row-major matrix of `SCS_PairwiseResult`.

```c
int scs_pairwise_matrix_2d(
    const double* alt_xy,               // length 2*n_alts
    int n_alts,
    const double* voter_ideals_xy,      // length 2*n_voters
    int n_voters,
    const SCS_DistanceConfig* dist_cfg,
    int k,
    SCS_PairwiseResult* out_matrix,     // length n_alts*n_alts
    int out_len,                        // must be >= n_alts*n_alts
    char* err_buf, int err_buf_len);
```

### C1.3 — `scs_weighted_majority_prefers_2d`

Wraps `geometry::weighted_majority_prefers(x, y, voter_ideals, weights, cfg, threshold)`.

Contract updates:

- `weights` must be non-null when `n_voters > 0`.
- Every weight must be finite and strictly positive.
- `threshold` is a fraction of total weight in `(0, 1]`.
- A point wins iff winning weight is **greater than or equal to**
  `threshold * total_weight`.

```c
int scs_weighted_majority_prefers_2d(
    double point_a_x, double point_a_y,
    double point_b_x, double point_b_y,
    const double* voter_ideals_xy,      // length 2*n_voters
    int n_voters,
    const double* weights,              // length n_voters
    const SCS_DistanceConfig* dist_cfg,
    double threshold,                   // fraction of total weight in (0,1]
    int* out,
    char* err_buf, int err_buf_len);
```

---

## Phase C2 — Geometry: winset (opaque handle)

The `WinsetRegion` (CGAL `General_polygon_set_2`) cannot be serialized without
information loss, so it is exposed as an opaque handle. All boolean operations
produce a new owned handle. The caller must destroy every handle it receives.

### C2.1 — Opaque type declaration

```c
typedef struct SCS_WinsetImpl SCS_Winset;
```

Added to `scs_api.h`.

### C2.2 — `scs_winset_2d`

Wraps `geometry::winset_2d(status_quo, voter_ideals, cfg, k, num_samples)`.

```c
SCS_Winset* scs_winset_2d(
    double status_quo_x, double status_quo_y,
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg,
    int k, int num_samples,             // must be > 0; use SCS_DEFAULT_WINSET_SAMPLES if unsure
    char* err_buf, int err_buf_len);
```

Returns `NULL` on error.

### C2.3 — `scs_weighted_winset_2d`

Wraps `geometry::weighted_winset_2d(status_quo, voter_ideals, weights, cfg, threshold, num_samples)`.

```c
SCS_Winset* scs_weighted_winset_2d(
    double status_quo_x, double status_quo_y,
    const double* voter_ideals_xy, int n_voters,
    const double* weights,              // length n_voters
    const SCS_DistanceConfig* dist_cfg,
    double threshold, int num_samples,
    char* err_buf, int err_buf_len);
```

### C2.4 — `scs_winset_with_veto_2d`

Wraps `geometry::winset_with_veto_2d(status_quo, voter_ideals, cfg, veto_ideals, k, num_samples)`.

```c
SCS_Winset* scs_winset_with_veto_2d(
    double status_quo_x, double status_quo_y,
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg,
    const double* veto_ideals_xy, int n_veto,   // if n_veto == 0, veto_ideals_xy may be NULL
    int k, int num_samples,
    char* err_buf, int err_buf_len);
```

### C2.5 — Lifecycle and query helpers

```c
void scs_winset_destroy(SCS_Winset* ws);   // NULL is no-op

int scs_winset_is_empty(
    const SCS_Winset* ws,
    int* out_is_empty,
    char* err_buf, int err_buf_len);

int scs_winset_contains_point_2d(
    const SCS_Winset* ws,
    double x, double y,
    int* out_contains,
    char* err_buf, int err_buf_len);

int scs_winset_bbox_2d(
    const SCS_Winset* ws,
    int* out_found,                     // 0 if empty, 1 if bbox written
    double* out_min_x, double* out_min_y,
    double* out_max_x, double* out_max_y,
    char* err_buf, int err_buf_len);
```

### C2.6 — Approximate boundary export (multi-path)

Do **not** expose winset export as a single flat polygon. The C API contract is
sampled boundary paths for plotting / inspection, not faithful serialization.

Semantics:

- Each boundary path is a closed sampled ring represented as interleaved
  `(x,y)` pairs in `out_xy`.
- Sampled paths do **not** repeat the first point at the end; consumers close
  the ring themselves if needed.
- `out_path_starts[i]` is the starting pair index of path `i`.
- The final path ends at `out_xy_n`.
- `out_path_is_hole[i]` is `0` for an outer boundary and `1` for a hole.
- Winding order is not part of the public contract; consumers must use
  `out_path_is_hole` rather than inferring topology from orientation.

```c
int scs_winset_boundary_size_2d(
    const SCS_Winset* ws,
    int num_samples_per_path,
    int* out_xy_pairs,                 // total number of (x,y) pairs required
    int* out_n_paths,                  // number of sampled boundary paths
    char* err_buf, int err_buf_len);

int scs_winset_sample_boundary_2d(
    const SCS_Winset* ws,
    int num_samples_per_path,
    double* out_xy, int out_xy_capacity,        // capacity counted in (x,y) pairs
    int* out_xy_n,                               // actual / required number of pairs
    int* out_path_starts, int out_path_capacity, // capacity counted in paths
    int* out_path_is_hole,                       // optional, length out_path_capacity
    int* out_n_paths,                            // actual / required path count
    char* err_buf, int err_buf_len);
```

Query pattern:

- If `out_xy == NULL` or `out_xy_capacity == 0`, the function may be used to
  query `*out_xy_n` without writing coordinates.
- If `out_path_starts == NULL` or `out_path_capacity == 0`, the function may be
  used to query `*out_n_paths` without writing path offsets.
- If `out_path_is_hole == NULL`, the function may omit hole flags; plotting-only
  consumers can ignore them, but geometry-aware bindings should request them.

### C2.7 — Boolean set operations

Each operation returns a new owned `SCS_Winset*` (caller must destroy).

```c
SCS_Winset* scs_winset_union(
    const SCS_Winset* a, const SCS_Winset* b,
    char* err_buf, int err_buf_len);

SCS_Winset* scs_winset_intersection(
    const SCS_Winset* a, const SCS_Winset* b,
    char* err_buf, int err_buf_len);

SCS_Winset* scs_winset_difference(
    const SCS_Winset* a, const SCS_Winset* b,
    char* err_buf, int err_buf_len);

SCS_Winset* scs_winset_symmetric_difference(
    const SCS_Winset* a, const SCS_Winset* b,
    char* err_buf, int err_buf_len);
```

### C2.8 — Optional clone helper

Not required for the first implementation pass, but explicitly worth adding if
time permits or if bindings need it:

```c
SCS_Winset* scs_winset_clone(
    const SCS_Winset* ws,
    char* err_buf, int err_buf_len);
```

---

## Phase C3 — Geometry: Yolk

### C3.1 — `SCS_Yolk2d` POD struct

```c
typedef struct {
    double center_x;
    double center_y;
    double radius;
} SCS_Yolk2d;
```

Added to `scs_api.h`.

### C3.2 — `scs_yolk_2d`

Wraps `geometry::yolk_2d(voter_ideals, k, num_samples)`.

Contract updates:

- Standardize on `num_samples`.
- Document clearly that this is a sampled approximation, with accuracy/runtime
  controlled by `num_samples`.

```c
int scs_yolk_2d(
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg,
    int k,                              // SCS_MAJORITY_SIMPLE or 1..n_voters
    int num_samples,                    // directional samples; use SCS_DEFAULT_YOLK_SAMPLES if unsure
    SCS_Yolk2d* out,
    char* err_buf, int err_buf_len);
```

---

## Phase C4 — Geometry: uncovered set, Copeland, core, Heart

### C4.1 — `scs_copeland_scores_2d`

Wraps `geometry::copeland_scores(alternatives, voter_ideals, cfg, k)`.

```c
int scs_copeland_scores_2d(
    const double* alt_xy, int n_alts,
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg, int k,
    int* out_scores, int out_len,        // must be >= n_alts
    char* err_buf, int err_buf_len);
```

### C4.2 — `scs_copeland_winner_2d`

Wraps `geometry::copeland_winner(alternatives, voter_ideals, cfg, k)`.

Contract change:

- Return the **index** of the winning alternative, not its coordinates.
- Ties are broken deterministically by smallest alternative index.

```c
int scs_copeland_winner_2d(
    const double* alt_xy, int n_alts,
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg, int k,
    int* out_winner_idx,
    char* err_buf, int err_buf_len);
```

### C4.3 — `scs_has_condorcet_winner_2d` and `scs_condorcet_winner_2d`

Wraps `geometry::has_condorcet_winner` and `geometry::condorcet_winner` over a
finite alternative set.

Contract change:

- Return the **index** of the Condorcet winner rather than copying coordinates.

```c
int scs_has_condorcet_winner_2d(
    const double* alt_xy, int n_alts,
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg, int k,
    int* out_found,
    char* err_buf, int err_buf_len);

int scs_condorcet_winner_2d(
    const double* alt_xy, int n_alts,
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg, int k,
    int* out_found,
    int* out_winner_idx,
    char* err_buf, int err_buf_len);
```

### C4.4 — `scs_core_2d`

Wraps `geometry::core_2d(voter_ideals, cfg, k)` (continuous space: checks Yolk
centre and voter ideal points).

```c
int scs_core_2d(
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg, int k,
    int* out_found,
    double* out_x, double* out_y,
    char* err_buf, int err_buf_len);
```

### C4.5 — `scs_uncovered_set_2d` (discrete)

Wraps `geometry::uncovered_set(alternatives, voter_ideals, cfg, k)`.
Returns 0-based indices of uncovered alternatives.

```c
int scs_uncovered_set_2d(
    const double* alt_xy, int n_alts,
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg, int k,
    int* out_indices,
    int out_capacity,                    // counted in indices
    int* out_n,
    char* err_buf, int err_buf_len);
```

### C4.6 — `scs_uncovered_set_boundary_2d` (continuous polygon)

Wraps `geometry::uncovered_set_boundary_2d(voter_ideals, cfg, grid_resolution, k)`.

The current C++ service returns one boundary polygon, so a single flat path is
still appropriate here. Unlike winset export, no multi-path contract is needed.
The boundary is still approximate and should follow the same size-query pattern
as other variable-size outputs.

```c
int scs_uncovered_set_boundary_size_2d(
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg,
    int grid_resolution,
    int k,
    int* out_xy_pairs,
    char* err_buf, int err_buf_len);

int scs_uncovered_set_boundary_2d(
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg,
    int grid_resolution,                 // must be > 0; use SCS_DEFAULT_BOUNDARY_GRID_RESOLUTION if unsure
    int k,
    double* out_xy, int out_capacity,    // counted in (x,y) pairs
    int* out_n,
    char* err_buf, int err_buf_len);
```

### C4.7 — `scs_heart_2d` (discrete)

Wraps `geometry::heart(alternatives, voter_ideals, cfg, k)`.
Returns 0-based indices of Heart alternatives.

```c
int scs_heart_2d(
    const double* alt_xy, int n_alts,
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg, int k,
    int* out_indices,
    int out_capacity,                    // counted in indices
    int* out_n,
    char* err_buf, int err_buf_len);
```

### C4.8 — `scs_heart_boundary_2d` (continuous polygon)

Wraps `geometry::heart_boundary_2d(voter_ideals, cfg, grid_resolution, k)`.

```c
int scs_heart_boundary_size_2d(
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg,
    int grid_resolution,
    int k,
    int* out_xy_pairs,
    char* err_buf, int err_buf_len);

int scs_heart_boundary_2d(
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg,
    int grid_resolution,                 // must be > 0; use SCS_DEFAULT_BOUNDARY_GRID_RESOLUTION if unsure
    int k,
    double* out_xy, int out_capacity,    // counted in (x,y) pairs
    int* out_n,
    char* err_buf, int err_buf_len);
```

---

## Phase C5 — Aggregation: Profile (opaque handle)

The `Profile` struct holds `vector<vector<int>>` rankings and cannot be
flattened without losing structure. It is exposed as an opaque `SCS_Profile*`.

### C5.1 — Opaque type and ABI-stable `SCS_TieBreak`

```c
typedef struct SCS_ProfileImpl SCS_Profile;

typedef int32_t SCS_TieBreak;

#define SCS_TIEBREAK_RANDOM         ((int32_t)0)
#define SCS_TIEBREAK_SMALLEST_INDEX ((int32_t)1)
```

`SCS_TIEBREAK_RANDOM` requires a non-null `SCS_StreamManager*` and valid
registered stream name at call sites that use tie-breaking.
`SCS_TIEBREAK_SMALLEST_INDEX` accepts null.

### C5.2 — `scs_profile_build_spatial`

Wraps `aggregation::build_spatial_profile(alternatives, voter_ideals, cfg)`.

Contract updates:

- Reject non-finite coordinates.
- Ties in distance are broken deterministically by smallest alternative index.

```c
SCS_Profile* scs_profile_build_spatial(
    const double* alt_xy, int n_alts,
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg,
    char* err_buf, int err_buf_len);
```

### C5.3 — `scs_profile_from_utility_matrix`

Wraps `aggregation::profile_from_utility_matrix(utilities)`.

Contract updates:

- Reject non-finite utilities.
- Ties in utility are broken deterministically by smallest alternative index.

```c
SCS_Profile* scs_profile_from_utility_matrix(
    const double* utilities,       // flat row-major, size n_voters*n_alts
    int n_voters, int n_alts,
    char* err_buf, int err_buf_len);
```

### C5.4 — Synthetic profile generators

All require a `SCS_StreamManager*` and valid stream name for reproducible
randomness.

```c
SCS_Profile* scs_profile_impartial_culture(
    int n_voters, int n_alts,
    SCS_StreamManager* mgr, const char* stream_name,
    char* err_buf, int err_buf_len);

SCS_Profile* scs_profile_uniform_spatial(
    int n_voters, int n_alts, int n_dims,
    double lo, double hi,          // bounding box for both voters and alts
    const SCS_DistanceConfig* dist_cfg,
    SCS_StreamManager* mgr, const char* stream_name,
    char* err_buf, int err_buf_len);

SCS_Profile* scs_profile_gaussian_spatial(
    int n_voters, int n_alts, int n_dims,
    double mean, double stddev,
    const SCS_DistanceConfig* dist_cfg,
    SCS_StreamManager* mgr, const char* stream_name,
    char* err_buf, int err_buf_len);
```

### C5.5 — Lifecycle and inspection

```c
void scs_profile_destroy(SCS_Profile* p);   // NULL is no-op

int scs_profile_dims(
    const SCS_Profile* p,
    int* out_n_voters, int* out_n_alts,
    char* err_buf, int err_buf_len);

int scs_profile_get_ranking(
    const SCS_Profile* p, int voter,
    int* out_ranking, int out_len,   // must be >= n_alts
    char* err_buf, int err_buf_len);
```

### C5.6 — Optional bulk export helper

Not required to finish the milestone, but valuable for bindings that would
otherwise loop over `scs_profile_get_ranking` for every voter:

```c
int scs_profile_export_rankings(
    const SCS_Profile* p,
    int* out_rankings, int out_len,   // must be >= n_voters*n_alts
    char* err_buf, int err_buf_len);
```

Layout:

- row-major by voter
- `out_rankings[voter * n_alts + rank] = alternative index`

### C5.7 — Optional clone helper

```c
SCS_Profile* scs_profile_clone(
    const SCS_Profile* p,
    char* err_buf, int err_buf_len);
```

---

## Phase C6 — Aggregation: voting rules

All positional voting rule functions follow the same pattern:

- `*_scores` writes one score per alternative.
- `*_all_winners` writes up to `n_alts` alternative indices.
- `*_one_winner` returns one index and accepts `SCS_TieBreak` plus nullable
  `SCS_StreamManager*`.

### C6.1 — Plurality

```c
int scs_plurality_scores(
    const SCS_Profile* p,
    int* out_scores, int out_len,    // must be >= n_alts
    char* err_buf, int err_buf_len);

int scs_plurality_all_winners(
    const SCS_Profile* p,
    int* out_winners, int out_capacity,
    int* out_n,
    char* err_buf, int err_buf_len);

int scs_plurality_one_winner(
    const SCS_Profile* p,
    SCS_TieBreak tie_break,
    SCS_StreamManager* mgr,
    const char* stream_name,
    int* out_winner,
    char* err_buf, int err_buf_len);
```

### C6.2 — Borda Count

```c
int scs_borda_scores(
    const SCS_Profile* p,
    int* out_scores, int out_len,
    char* err_buf, int err_buf_len);

int scs_borda_all_winners(
    const SCS_Profile* p,
    int* out_winners, int out_capacity,
    int* out_n,
    char* err_buf, int err_buf_len);

int scs_borda_one_winner(
    const SCS_Profile* p,
    SCS_TieBreak tie_break,
    SCS_StreamManager* mgr,
    const char* stream_name,
    int* out_winner,
    char* err_buf, int err_buf_len);

int scs_borda_ranking(
    const SCS_Profile* p,
    SCS_TieBreak tie_break,
    SCS_StreamManager* mgr,
    const char* stream_name,
    int* out_ranking, int out_len,    // must be >= n_alts
    char* err_buf, int err_buf_len);
```

### C6.3 — Anti-plurality

```c
int scs_antiplurality_scores(
    const SCS_Profile* p,
    int* out_scores, int out_len,
    char* err_buf, int err_buf_len);

int scs_antiplurality_all_winners(
    const SCS_Profile* p,
    int* out_winners, int out_capacity,
    int* out_n,
    char* err_buf, int err_buf_len);

int scs_antiplurality_one_winner(
    const SCS_Profile* p,
    SCS_TieBreak tie_break,
    SCS_StreamManager* mgr,
    const char* stream_name,
    int* out_winner,
    char* err_buf, int err_buf_len);
```

### C6.4 — Generic positional scoring rule

Contract change:

- `score_weights` remain `double`.
- Output scores become `double*` to avoid silent truncation or ambiguous rounding.

```c
int scs_scoring_rule_scores(
    const SCS_Profile* p,
    const double* score_weights,  // length n_alts, non-increasing
    int n_weights,
    double* out_scores, int out_len,  // must be >= n_alts
    char* err_buf, int err_buf_len);

int scs_scoring_rule_all_winners(
    const SCS_Profile* p,
    const double* score_weights, int n_weights,
    int* out_winners, int out_capacity,
    int* out_n,
    char* err_buf, int err_buf_len);

int scs_scoring_rule_one_winner(
    const SCS_Profile* p,
    const double* score_weights, int n_weights,
    SCS_TieBreak tie_break,
    SCS_StreamManager* mgr, const char* stream_name,
    int* out_winner,
    char* err_buf, int err_buf_len);
```

### C6.5 — Approval voting

#### Spatial variant

These functions should not require a `SCS_Profile*`; the spatial data is
sufficient.

```c
int scs_approval_scores_spatial(
    const double* alt_xy, int n_alts,
    const double* voter_ideals_xy, int n_voters,
    double threshold,
    const SCS_DistanceConfig* dist_cfg,
    int* out_scores, int out_len,      // must be >= n_alts
    char* err_buf, int err_buf_len);

int scs_approval_all_winners_spatial(
    const double* alt_xy, int n_alts,
    const double* voter_ideals_xy, int n_voters,
    double threshold,
    const SCS_DistanceConfig* dist_cfg,
    int* out_winners, int out_capacity,
    int* out_n,
    char* err_buf, int err_buf_len);
```

#### Ordinal top-k variant

```c
int scs_approval_scores_topk(
    const SCS_Profile* p, int k,
    int* out_scores, int out_len,      // must be >= n_alts
    char* err_buf, int err_buf_len);

int scs_approval_all_winners_topk(
    const SCS_Profile* p, int k,
    int* out_winners, int out_capacity,
    int* out_n,
    char* err_buf, int err_buf_len);
```

---

## Phase C7 — Aggregation: social rankings and properties

### C7.1 — `scs_rank_by_scores`

Wraps `aggregation::rank_by_scores(scores, tie_break, prng)`.
Input: flat score array of length `n_alts`. Output: ranking as alt indices,
best first.

```c
int scs_rank_by_scores(
    const double* scores, int n_alts,
    SCS_TieBreak tie_break,
    SCS_StreamManager* mgr, const char* stream_name,
    int* out_ranking, int out_len,
    char* err_buf, int err_buf_len);
```

This keeps the helper compatible with generic scoring rules and any future API
that produces non-integer scores.

### C7.2 — `scs_pairwise_ranking_from_matrix`

Wraps `aggregation::pairwise_ranking(pairwise_matrix, tie_break, prng)`.
Input: row-major pairwise matrix (n_alts × n_alts) of `SCS_PairwiseResult`.

```c
int scs_pairwise_ranking_from_matrix(
    const SCS_PairwiseResult* pairwise_matrix, int n_alts,
    SCS_TieBreak tie_break,
    SCS_StreamManager* mgr, const char* stream_name,
    int* out_ranking, int out_len,
    char* err_buf, int err_buf_len);
```

### C7.3 — Pareto efficiency

```c
int scs_pareto_set(
    const SCS_Profile* p,
    int* out_indices, int out_capacity,
    int* out_n,
    char* err_buf, int err_buf_len);

int scs_is_pareto_efficient(
    const SCS_Profile* p, int alt_idx,
    int* out,
    char* err_buf, int err_buf_len);
```

### C7.4 — Condorcet and majority-selection predicates

Keep the existing profile-level winner discovery helpers:

```c
int scs_has_condorcet_winner_profile(
    const SCS_Profile* p,
    int* out_found,
    char* err_buf, int err_buf_len);

int scs_condorcet_winner_profile(
    const SCS_Profile* p,
    int* out_found, int* out_winner,
    char* err_buf, int err_buf_len);
```

Rename the ambiguous alt-level predicates so their semantics are explicit:

```c
int scs_is_selected_by_condorcet_consistent_rules(
    const SCS_Profile* p, int alt_idx,
    int* out,
    char* err_buf, int err_buf_len);

int scs_is_selected_by_majority_consistent_rules(
    const SCS_Profile* p, int alt_idx,
    int* out,
    char* err_buf, int err_buf_len);
```

Documentation requirement:

- These names must be defined precisely in `c_api_design.md`.
- If the underlying C++ semantics differ from the names above, adjust the names
  to match the actual mathematical property before implementation.
- For `out_found`-style APIs, document whether the remaining outputs are left
  untouched when `*out_found == 0`; prefer "leave untouched" to avoid forcing
  bindings to inspect placeholder values.

---

## Phase C8 — Tests

Extend `tests/unit/test_c_api.cpp` with tests for all new functions.

**Coverage targets:**

- **C0:** version query succeeds; new error codes exposed; `SCS_MAJORITY_SIMPLE`
  is documented/used consistently; `SCS_PairwiseResult` /
  `SCS_TieBreak` values are exposed as fixed-width scalar domains;
  `scs_level_set_to_polygon(...)` with null buffer returns required size and
  with a sufficient buffer returns sampled coordinates.
- **C1:** majority prefers (true/false cases); `k = SCS_MAJORITY_SIMPLE`;
  invalid `k`; weighted majority prefers with threshold edge cases and invalid
  weights; pairwise matrix values match repeated majority comparisons.
- **C2:** winset lifecycle (create, destroy, null-destroy), `is_empty`,
  `contains_point_2d`, `bbox_2d` with empty/non-empty cases, sampled boundary
  query/fill pattern, insufficient buffer → `SCS_ERROR_BUFFER_TOO_SMALL`, and
  all four boolean ops (union ⊇ each operand; intersection ⊆ each operand;
  X∩X = X; X∪X = X; X\X = ∅).
- **C2 boundary metadata:** `out_path_starts` splits rings correctly;
  `out_path_is_hole` flags holes correctly; sampled rings do not duplicate the
  closing point.
- **C3:** Yolk for equilateral triangle (centre = centroid, radius = √3/6 ±
  tolerance); radius > 0 for cycle configuration; effect of sample count is
  stable within tolerance.
- **C4:** Copeland scores; Copeland winner returns index and uses deterministic
  smallest-index tie break; finite-alt Condorcet winner found/not-found;
  `core_2d` found for median voter; uncovered set and Heart return expected
  index sets; boundary size-query functions report exact required size; boundary
  fill functions return non-empty output for non-empty cases and respect output
  capacity.
- **C5:** build_spatial_profile dims; deterministic tie handling in ranking
  construction; `profile_from_utility_matrix` round-trip; reject non-finite
  utilities/coordinates; `profile_export_rankings` (if added) matches repeated
  `get_ranking`.
- **C6:** scores / all_winners / one_winner for all Category-3 rules; every
  `*_all_winners` respects `out_capacity`; one_winner with
  `SCS_TIEBREAK_SMALLEST_INDEX` is deterministic; one_winner with
  `SCS_TIEBREAK_RANDOM` requires non-null manager + stream; generic scoring rule
  handles fractional weights correctly.
- **C7:** `rank_by_scores` correct order; `pairwise_ranking_from_matrix`
  consistent with pairwise inputs; `pareto_set` non-empty where expected;
  condorcet winner profile found/not-found; renamed selection predicates return
  documented results.
- **Error paths across all phases:** null pointer → `SCS_ERROR_INVALID_ARGUMENT`;
  null handle to non-destroy opaque-handle API; insufficient output buffer →
  `SCS_ERROR_BUFFER_TOO_SMALL`; failed allocation paths where feasible;
  invalid tie-break / stream combinations; `err_buf = NULL`, `err_buf_len = 0`,
  `err_buf_len = 1`.
- **Robustness / randomized checks:** non-finite values rejected; degenerate
  geometry cases; sampled boundary export does not overflow or misreport
  required sizes.

---

## Phase C9 — Documentation

Update `docs/architecture/c_api_design.md`:

- Add sections for each new phase (C0–C7).
- Document the new error codes and the size-query / buffer-too-small contract.
- Document `SCS_MAJORITY_SIMPLE`, `SCS_PairwiseResult`, and the `scs_api_version`
  function.
- Add a short "Thread safety / reentrancy" section covering distinct-handle
  concurrency, same-handle caveats, and `SCS_StreamManager*` synchronization
  expectations.
- Document `SCS_Winset*` lifecycle contract (ownership, NULL behavior,
  approximate sampled-boundary export, hole metadata, bbox empty behavior, and
  query helpers).
- Document `SCS_Profile*` lifecycle contract.
- Document `SCS_TieBreak` with note on null manager / required stream name.
- Document that `SCS_DistanceConfig.salience_weights` is borrowed input storage.
- Add `SCS_Yolk2d` to the POD struct table.
- Extend the mapping table (C type ↔ C++ type) for all new types and scalar
  domains.
- Document the updated `scs_level_set_to_polygon(...)` signature: null-buffer
  size-query mode, `out_capacity` parameter, `SCS_ERROR_BUFFER_TOO_SMALL` on
  insufficient buffer.
- Add a usage example showing: create voters, build spatial profile, run Borda,
  query winset sampled boundary, get Yolk, check Condorcet winner.
- Add an ABI note: exported declarations use `SCS_API`; `scs_api` should move
  to explicit `VERSION` / `SOVERSION` settings in CMake as part of stabilizing
  the shared library.
- Add a stability / deprecation note: once these functions ship, future changes
  to the stable surface are additive rather than signature-breaking.

---

## Step checklist

| Step | Description | Status |
|------|-------------|--------|
| C0.1 | Version / visibility hooks + `SCS_MAJORITY_SIMPLE` | ✅ Done |
| C0.2 | New error codes (`BUFFER_TOO_SMALL`, `OUT_OF_MEMORY`) | ✅ Done |
| C0.3 | ABI-stable `SCS_PairwiseResult` scalar domain | ✅ Done |
| C0.4 | Add `out_capacity` to `scs_level_set_to_polygon`; null-buffer = size query | ✅ Done |
| C0.5 | Document thread-safety / reentrancy contract | ✅ Done |
| C1.1 | `scs_majority_prefers_2d` | ✅ Done |
| C1.2 | `scs_pairwise_matrix_2d` | ✅ Done |
| C1.3 | `scs_weighted_majority_prefers_2d` | ✅ Done |
| C2.1 | `SCS_Winset` opaque type declaration | ✅ Done |
| C2.2 | `scs_winset_2d` | ✅ Done |
| C2.3 | `scs_weighted_winset_2d` | ✅ Done |
| C2.4 | `scs_winset_with_veto_2d` | ✅ Done |
| C2.5 | Winset lifecycle + queries (`destroy`, `is_empty`, `contains_point_2d`, `bbox_2d`) | ✅ Done |
| C2.6 | Winset boundary export (`boundary_size_2d`, `sample_boundary_2d`, hole flags) | ✅ Done |
| C2.7 | Winset boolean ops (`union`, `intersection`, `difference`, `symmetric_difference`) | ✅ Done |
| C2.8 | Optional `scs_winset_clone` | ✅ Done |
| C3.1 | `SCS_Yolk2d` POD struct | ✅ Done |
| C3.2 | `scs_yolk_2d` | ✅ Done |
| C4.1 | `scs_copeland_scores_2d` | ✅ Done |
| C4.2 | `scs_copeland_winner_2d` (index-returning) | ✅ Done |
| C4.3 | `scs_has_condorcet_winner_2d` + `scs_condorcet_winner_2d` | ✅ Done |
| C4.4 | `scs_core_2d` | ✅ Done |
| C4.5 | `scs_uncovered_set_2d` (discrete) | ✅ Done |
| C4.6 | `scs_uncovered_set_boundary_size_2d` + `scs_uncovered_set_boundary_2d` | ✅ Done |
| C4.7 | `scs_heart_2d` (discrete) | ✅ Done |
| C4.8 | `scs_heart_boundary_size_2d` + `scs_heart_boundary_2d` | ✅ Done |
| C5.1 | `SCS_Profile` opaque type + ABI-stable `SCS_TieBreak` | ✅ Done |
| C5.2 | `scs_profile_build_spatial` | ✅ Done |
| C5.3 | `scs_profile_from_utility_matrix` | ✅ Done |
| C5.4 | Synthetic generators (impartial culture, uniform spatial, Gaussian spatial) | ✅ Done |
| C5.5 | Profile lifecycle + inspection (`destroy`, `dims`, `get_ranking`) | ✅ Done |
| C5.6 | Optional `scs_profile_export_rankings` | ✅ Done |
| C5.7 | Optional `scs_profile_clone` | ✅ Done |
| C6.1 | Plurality (`scores`, `all_winners`, `one_winner`) | 🔲 |
| C6.2 | Borda (`scores`, `all_winners`, `one_winner`, `ranking`) | 🔲 |
| C6.3 | Anti-plurality (`scores`, `all_winners`, `one_winner`) | 🔲 |
| C6.4 | Generic scoring rule (`scores`, `all_winners`, `one_winner`) | 🔲 |
| C6.5 | Approval voting (`scores_spatial`, `all_winners_spatial`, `scores_topk`, `all_winners_topk`) | 🔲 |
| C7.1 | `scs_rank_by_scores` | 🔲 |
| C7.2 | `scs_pairwise_ranking_from_matrix` | 🔲 |
| C7.3 | Pareto (`scs_pareto_set`, `scs_is_pareto_efficient`) | 🔲 |
| C7.4 | Condorcet / majority-selection predicates | 🔲 |
| C8 | Tests for C0–C7 | 🔲 |
| C9 | Update `c_api_design.md` | 🔲 |

---

## Implementation order

Recommended order within a session:

1. **C0 first** — these choices affect the rest of the header and test style.
2. **C1 + C3** — simple wrappers, no new opaque types, fastest to verify.
3. **C2** — winset opaque handle + sampled boundary export (most complex;
   establishes the pattern for opaque types and variable-size output handling).
4. **C4** — remaining geometry; mostly scalars, indices, and flat arrays.
5. **C5** — profile opaque handle + `SCS_TieBreak`.
6. **C6** — voting rules (many functions, same pattern; batch them).
7. **C7** — social properties / rankings (short functions, but naming needs care).
8. **C8** — tests (write as you go per phase, or batch at end).
9. **C9** — documentation and ABI notes last.

Avoid implementing C5–C7 before C2 is done: the winset pattern establishes how
opaque handles are declared, implemented (`struct SCS_*Impl` in `scs_api.cpp`
only), destroyed, queried, and exported safely, and that pattern is reused for
`SCS_Profile`.
