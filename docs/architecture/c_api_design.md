# c_api Design

Stable C API (`scs_api`) for the SocialChoiceLab C++ core. This layer is the
only interface R/Python bindings will depend on. The C++ core is **never**
linked directly by R or Python code.

---

## Design constraints

| # | Constraint | Resolution |
|---|-----------|-----------|
| 29 | `distance_to_utility` has six parameters; C has no defaults | `SCS_LossConfig` POD struct with a discriminant enum and all parameters; callers fill only the fields used by their chosen loss type |
| 30 | Stream auto-creation vs. registration | `scs_register_streams` enforces the allowlist at the C boundary; attempts to draw from an unregistered stream return `SCS_ERROR_INVALID_ARGUMENT` |
| 31 | Do not expose `PRNG::engine()` | Not present in `scs_api.h`; the internal engine is inaccessible from C/R/Python |
| 32 | Strategy kinds stored in `CompetitorState` but not originally exposed via C API | Added `scs_competition_trace_strategy_kinds` to copy per-competitor `CompetitorStrategyKind` enum values into an `int*` buffer. R binding returns character vector; Python binding returns `list[str]`. Used by canvas animation to auto-generate display names from strategies. |

---

## Files

| File | Role |
|------|------|
| `include/scs_api.h` | Public C header — scalar domains, POD structs, opaque handles, and function declarations |
| `src/c_api/scs_api.cpp` | `extern "C"` implementation — maps C types ↔ C++ types, catches exceptions → error codes |
| `tests/unit/test_c_api.cpp` | GTest suite calling the C API using only C types |

The library is built as a **shared library** (`libscs_api.dylib` / `libscs_api.so`).
R and Python bindings link only against this artifact; they do not see Eigen or STL.

Exported declarations use `SCS_API`. On non-Windows builds this expands to
default symbol visibility; on Windows it should map to
`__declspec(dllexport)` / `__declspec(dllimport)` via an `SCS_API_BUILD`
switch.

Versioning policy for the stable surface:

- Matching major versions are intended to be ABI-compatible.
- Minor versions are additive.
- Patch versions change behavior, tests, or documentation without changing
  signatures or public struct layout.

## API stability declarations

| Surface | Declared stable at | Notes |
|---|---|---|
| Core, geometry, aggregation C API | `v0.2.0` (2026-03-08) | All functions in `scs_api.h` covering core, geometry, profiles, voting rules, aggregation properties, and centrality. |
| Competition C API | `v0.3.0` (2026-03-17) | All `scs_competition_*` functions: handle lifecycle, config/voter setup, run execution, trace export (positions, seats, votes, strategy kinds), and experiment runner. |

---

## Error handling contract

Every function returns `int`:

| Return value | Meaning |
|---|---|
| `SCS_OK` (0) | Success |
| `SCS_ERROR_INVALID_ARGUMENT` (1) | Bad argument (null pointer, wrong dimension, out-of-range value, unregistered stream name, non-finite double) |
| `SCS_ERROR_INTERNAL` (2) | Unexpected internal error |
| `SCS_ERROR_BUFFER_TOO_SMALL` (3) | Caller-provided output buffer is valid but not large enough |
| `SCS_ERROR_OUT_OF_MEMORY` (4) | Handle construction or internal allocation failed |

If `err_buf != NULL` and `err_buf_len > 0`, a null-terminated message is written
into `err_buf` on any non-zero return. Bindings should always provide a buffer
(e.g. 512 bytes) and propagate the message to their native error type.

`scs_stream_manager_create` returns `NULL` on failure (rare; only on allocation
error) and writes the message into `err_buf`.

For optional-result APIs, "not found" is not an error: the function returns
`SCS_OK` and writes `*out_found = 0`. Remaining outputs should be documented per
function; prefer leaving them untouched when nothing is found.

For variable-size outputs, the stable contract is:

- Fixed-size outputs require a caller-provided buffer with explicit capacity.
- Variable-size outputs support a safe query-first pattern via `NULL` output
  buffers or zero capacity.
- If a non-null buffer is too small, the function returns
  `SCS_ERROR_BUFFER_TOO_SMALL` rather than truncating silently.

All non-destroy opaque-handle APIs should reject a null handle with
`SCS_ERROR_INVALID_ARGUMENT`. Destroy functions treat `NULL` as a no-op.

**Input validation (non-finite doubles).** At the C API boundary, all double
inputs that represent coordinates, utilities, weights, thresholds, or similar
numeric parameters are validated for finiteness. If any such value is NaN or
infinite, the function returns `SCS_ERROR_INVALID_ARGUMENT` (or `NULL` for
handle-returning APIs) and writes a message into `err_buf`. The C++ core is not
required to tolerate non-finite input; validation is done once in the wrappers.

---

## Thread Safety / Reentrancy

The public contract should be explicit rather than inferred from the C++
implementation:

- Public functions are reentrant unless they operate on shared mutable state.
- Read-only queries on distinct `SCS_Profile*` / `SCS_Winset*` handles are safe
  to call concurrently.
- Concurrent access to the same `SCS_Profile*` or `SCS_Winset*` is only safe for
  read-only operations if the implementation stores no lazy caches; otherwise
  callers must synchronize externally.
- `SCS_StreamManager*` is not thread-safe for concurrent draws, resets, or
  registration changes without external locking.
- Python bindings may release the GIL only around calls that satisfy the rules
  above; R bindings should document the same caveat on external-pointer wrappers.

---

## Scalar Domains And POD Structs

### Scalar domains

New ABI-visible result and mode values should use fixed-width integer
representations rather than public enums whose size is implementation-defined.
This keeps the ABI predictable for `ctypes`, `cffi`, and `.Call`.

```c
typedef int32_t SCS_PairwiseResult;
#define SCS_PAIRWISE_LOSS ((int32_t)-1)
#define SCS_PAIRWISE_TIE  ((int32_t)0)
#define SCS_PAIRWISE_WIN  ((int32_t)1)

typedef int32_t SCS_TieBreak;
#define SCS_TIEBREAK_RANDOM         ((int32_t)0)
#define SCS_TIEBREAK_SMALLEST_INDEX ((int32_t)1)
```

Named stable constants should also be provided for sentinel/default values used
across the expanded surface:

```c
#define SCS_MAJORITY_SIMPLE (-1)
#define SCS_DEFAULT_WINSET_SAMPLES 64
#define SCS_DEFAULT_YOLK_SAMPLES 720
#define SCS_DEFAULT_BOUNDARY_GRID_RESOLUTION 15
```

### POD structs

### `SCS_LossConfig`

```c
typedef struct {
  SCS_LossType loss_type;
  double sensitivity;   // LINEAR, QUADRATIC, THRESHOLD
  double max_loss;      // GAUSSIAN
  double steepness;     // GAUSSIAN
  double threshold;     // THRESHOLD
} SCS_LossConfig;
```

Fields used per loss type:

| `loss_type` | Used fields |
|---|---|
| `SCS_LOSS_LINEAR` | `sensitivity` |
| `SCS_LOSS_QUADRATIC` | `sensitivity` |
| `SCS_LOSS_GAUSSIAN` | `max_loss`, `steepness` |
| `SCS_LOSS_THRESHOLD` | `threshold`, `sensitivity` |

### `SCS_DistanceConfig`

```c
typedef struct {
  SCS_DistanceType distance_type;
  double order_p;                // MINKOWSKI only
  const double* salience_weights;
  int n_weights;                 // must equal n_dims of the point arrays
} SCS_DistanceConfig;
```

`salience_weights` is borrowed caller-owned storage valid only for the duration
of the call unless a function explicitly documents that it deep-copies the data.

### `SCS_LevelSet2d`

Shape-tagged union encoded as a flat POD struct. Fields used per `type`:

| `type` | Fields used |
|---|---|
| `SCS_LEVEL_SET_CIRCLE` | `center_x/y`, `param0` (radius) |
| `SCS_LEVEL_SET_ELLIPSE` | `center_x/y`, `param0` (semi_axis_0), `param1` (semi_axis_1) |
| `SCS_LEVEL_SET_SUPERELLIPSE` | `center_x/y`, `param0`, `param1`, `exponent_p` |
| `SCS_LEVEL_SET_POLYGON` | `n_vertices` (always 4), `vertices[0..7]` as `[x0,y0,…x3,y3]` |

No heap allocation: polygon vertices are stored inline (max 4 for all current
level-set shapes).

### `SCS_Yolk2d`

```c
typedef struct {
  double center_x;
  double center_y;
  double radius;
} SCS_Yolk2d;
```

---

## Stable-Surface Function Rules

The current minimal milestone exposes only a subset of the eventual stable C
surface. The following rules apply both to the existing API and to the planned
geometry / aggregation expansion in `docs/status/c_api_extensions_plan.md`.

### Version / visibility

```c
int scs_api_version(
    int* out_major, int* out_minor, int* out_patch,
    char* err_buf, int err_buf_len);
```

### Level-set to polygon sampling (C0.4)

```c
int scs_level_set_to_polygon(const SCS_LevelSet2d* level_set, int num_samples,
    double* out_xy, int out_capacity, int* out_n,
    char* err_buf, int err_buf_len);
```

- **Size-query mode:** pass `out_xy = NULL` or `out_capacity = 0`. The function
  returns `SCS_OK` and sets `*out_n` to the number of `(x,y)` pairs required.
- **Fill mode:** provide a buffer of length `>= *out_n` (from a prior size query
  or from the known requirement: `num_samples` for circle/ellipse/superellipse,
  or 4 for polygon). The function writes interleaved `[x0,y0,x1,y1,...]` and
  sets `*out_n` to the number of pairs written.
- **Insufficient buffer:** if `out_xy != NULL` and `out_capacity > 0` but
  `out_capacity <` required size, the function returns
  `SCS_ERROR_BUFFER_TOO_SMALL` and sets `*out_n` to the required size.

For circle/ellipse/superellipse, `num_samples` must be ≥ 3. For polygon, the
four vertices are copied and `num_samples` is ignored.

`scs_ic_polygon_2d` uses the same size-query, fill, and buffer contract as
`scs_level_set_to_polygon` (see declaration under **Indifference / level sets**
below).

### Opaque handles (C2, C5)

The stable surface uses opaque handles for C++ types that cannot be represented
safely as flat POD:

```c
typedef struct SCS_StreamManagerImpl SCS_StreamManager;
typedef struct SCS_WinsetImpl SCS_Winset;
typedef struct SCS_ProfileImpl SCS_Profile;
```

All such handles use create/destroy pairs; the **caller owns** the handle and
must call the corresponding `scs_*_destroy` when done. Passing `NULL` to any
destroy function is a no-op and is safe.

**SCS_Winset\*** (C2): Created by `scs_winset_2d`, `scs_weighted_winset_2d`,
`scs_winset_with_veto_2d`, or `scs_winset_clone`. The boundary is exported via
`scs_winset_boundary_size_2d` (size query) and `scs_winset_sample_boundary_2d`
(fill). Bindings that only need a one-shot boundary export may use
`scs_winset_2d_export_boundary` instead: it constructs the winset internally,
returns `*out_is_empty`, and uses the same size-then-fill pattern as
`scs_winset_sample_boundary_2d` without exposing an `SCS_Winset*`. The sampled boundary is an **approximation**; path count and vertex
count depend on the internal representation. Use `out_path_starts` and
`out_path_is_hole` to split the flat vertex array into rings and to distinguish
outer boundaries from holes. For an empty winset, `scs_winset_bbox_2d` sets
`*out_found = 0` and leaves the min/max outputs untouched. Query helpers:
`scs_winset_is_empty`, `scs_winset_contains_point_2d`, `scs_winset_bbox_2d`.
Boolean set operations: `scs_winset_union`, `scs_winset_intersection`,
`scs_winset_difference`, `scs_winset_symmetric_difference`.

**SCS_Profile\*** (C5): Created by `scs_profile_build_spatial`,
`scs_profile_from_utility_matrix`, `scs_profile_impartial_culture`,
`scs_profile_uniform_spatial`, `scs_profile_gaussian_spatial`, or
`scs_profile_clone`. Inspect with `scs_profile_dims`, `scs_profile_get_ranking`,
`scs_profile_export_rankings`. Used as input to all C6 voting-rule and C7
aggregation-property functions. Caller must not use a profile after
`scs_profile_destroy`.

### Winset boundary export (C2.6)

For sampled multi-path geometry:

- Call `scs_winset_boundary_size_2d` first to get `out_xy_pairs` and
  `out_n_paths`. Then allocate buffers and call `scs_winset_sample_boundary_2d`.
- Paths are interleaved `(x,y)` pairs in a single `out_xy` array.
- `out_path_starts[i]` is the starting pair index for path `i`.
- Sampled rings do **not** repeat the closing vertex (first point is not
  duplicated at the end).
- `out_path_is_hole[i]` is 0 for an outer boundary, 1 for a hole; bindings must
  not infer topology from winding.

```c
int scs_winset_boundary_size_2d(const SCS_Winset* ws, int* out_xy_pairs,
    int* out_n_paths, char* err_buf, int err_buf_len);

int scs_winset_sample_boundary_2d(const SCS_Winset* ws,
    double* out_xy, int out_xy_capacity, int* out_xy_n,
    int* out_path_starts, int out_path_capacity, int* out_path_is_hole,
    int* out_n_paths, char* err_buf, int err_buf_len);
```

If either buffer is too small, the function returns `SCS_ERROR_BUFFER_TOO_SMALL`.
Pass `out_xy = NULL` / `out_path_starts = NULL` or capacity 0 for size-query
only.

Bounding box for possibly empty winsets:

```c
int scs_winset_bbox_2d(const SCS_Winset* ws, int* out_found,
    double* out_min_x, double* out_min_y, double* out_max_x, double* out_max_y,
    char* err_buf, int err_buf_len);
```

When the winset is empty, `*out_found = 0` and the min/max outputs are left
untouched.

### Euclidean Voronoi cells 2D (C2.8)

**Scope:** Euclidean (L2) Voronoi only. Generalisation to non-Euclidean
metrics is planned (see ROADMAP.md).

- `scs_voronoi_cells_2d_size`: pass sites (interleaved x,y), `n_sites`, and
  bbox; get `out_total_xy_pairs` and `out_n_cells` (equals `n_sites`).
- `scs_voronoi_cells_2d`: export interleaved `(x,y)` for all cells concatenated;
  `out_cell_start` has length `n_sites + 1`; cell `i` is pairs from
  `out_cell_start[i]` to `out_cell_start[i+1] - 1`. Empty cells have
  `out_cell_start[i] == out_cell_start[i+1]`. Each cell is a single closed
  polygon (no holes). Same size-query pattern as winset: pass NULL or capacity
  0 for buffers to query required sizes only.

```c
int scs_voronoi_cells_2d_size(const double* sites_xy, int n_sites,
    double bbox_min_x, double bbox_min_y, double bbox_max_x, double bbox_max_y,
    int* out_total_xy_pairs, int* out_n_cells, char* err_buf, int err_buf_len);

int scs_voronoi_cells_2d(const double* sites_xy, int n_sites,
    double bbox_min_x, double bbox_min_y, double bbox_max_x, double bbox_max_y,
    double* out_xy, int out_xy_capacity, int* out_xy_n,
    int* out_cell_start, int out_cell_start_capacity,
    char* err_buf, int err_buf_len);
```

**Single-pass heap export:** `scs_voronoi_cells_2d_heap` runs one internal
Voronoi computation and writes into heap-allocated buffers. The caller must
call `scs_voronoi_cells_heap_destroy` when finished. This avoids a separate
`scs_voronoi_cells_2d_size` pass when the goal is to export all cells once.

**Uncovered-set boundary:** `scs_uncovered_set_boundary_2d_heap` similarly
performs one internal geometry pass and sets `*out_xy` to a buffer allocated
with `malloc` (or `NULL` if `*out_n_pairs == 0`). Free with `scs_heap_free`.
`scs_uncovered_set_boundary_size_2d` remains as a thin delegate over
`scs_uncovered_set_boundary_2d` with no fill buffer for callers that still use
the two-call size/fill pattern.

### SCS_TieBreak and randomness (C5, C6, C7)

Voting rules that break ties (`*_one_winner`, `*_ranking`, `scs_rank_by_scores`,
`scs_pairwise_ranking_from_matrix`, `scs_borda_ranking`) take
`SCS_TieBreak tie_break`, `SCS_StreamManager* mgr`, and `const char* stream_name`:

- **SCS_TIEBREAK_SMALLEST_INDEX:** Deterministic; choose the tied alternative
  with the smallest index. `mgr` and `stream_name` may be `NULL`.
- **SCS_TIEBREAK_RANDOM:** Choose uniformly at random among tied alternatives.
  **Requires** `mgr != NULL` and `stream_name != NULL` (a non-null, registered
  stream). Passing `NULL` for either returns `SCS_ERROR_INVALID_ARGUMENT`.

Bindings that want reproducible results without a stream manager should use
`SCS_TIEBREAK_SMALLEST_INDEX`.

### Ranking and aggregation helpers (C7)

Generic ranking accepts `double` score vectors (fractional scoring rules):

```c
int scs_rank_by_scores(const double* scores, int n_alts,
    SCS_TieBreak tie_break, SCS_StreamManager* mgr, const char* stream_name,
    int* out_ranking, int out_len, char* err_buf, int err_buf_len);
```

---

## Current Minimal Functions

### Distance

```c
int scs_calculate_distance(
    const double* ideal_point, const double* alt_point, int n_dims,
    const SCS_DistanceConfig* dist_cfg,
    double* out_distance,
    char* err_buf, int err_buf_len);
```

### Loss / utility

```c
int scs_distance_to_utility(double distance,
    const SCS_LossConfig* loss_cfg,
    double* out_utility, char* err_buf, int err_buf_len);

int scs_normalize_utility(double utility, double max_distance,
    const SCS_LossConfig* loss_cfg,
    double* out_normalized, char* err_buf, int err_buf_len);
```

### Indifference / level sets

```c
int scs_level_set_1d(double ideal, double weight, double utility_level,
    const SCS_LossConfig* loss_cfg,
    double* out_points,   // buffer of at least 2 doubles
    int* out_n,           // 0, 1, or 2
    char* err_buf, int err_buf_len);

int scs_ic_interval_1d(double ideal, double reference_x,
    const SCS_LossConfig* loss_cfg, const SCS_DistanceConfig* dist_cfg,
    double* out_points, int* out_n,
    char* err_buf, int err_buf_len);

int scs_level_set_2d(double ideal_x, double ideal_y, double utility_level,
    const SCS_LossConfig* loss_cfg, const SCS_DistanceConfig* dist_cfg,
    SCS_LevelSet2d* out, char* err_buf, int err_buf_len);

int scs_level_set_to_polygon(const SCS_LevelSet2d* level_set, int num_samples,
    double* out_xy,   // interleaved [x0,y0,x1,y1,...]; caller-provided
    int* out_n,       // number of (x,y) pairs written
    char* err_buf, int err_buf_len);

int scs_ic_polygon_2d(double ideal_x, double ideal_y, double sq_x, double sq_y,
    const SCS_LossConfig* loss_cfg, const SCS_DistanceConfig* dist_cfg,
    int num_samples, double* out_xy, int out_capacity, int* out_n,
    char* err_buf, int err_buf_len);
```

`scs_ic_interval_1d`: compound 1D IC — runs `scs_calculate_distance`,
`scs_distance_to_utility`, and `scs_level_set_1d` internally (same numerics as
the three-call sequence). Requires `dist_cfg->n_weights == 1`.

`scs_ic_polygon_2d`: compound call that runs `scs_calculate_distance`,
`scs_distance_to_utility`, `scs_level_set_2d`, and `scs_level_set_to_polygon`
internally (same numerics as the four-call sequence). Uses the same size-query /
fill / `SCS_ERROR_BUFFER_TOO_SMALL` contract as `scs_level_set_to_polygon`
(`out_xy == NULL` queries `*out_n`). Output vertex count is `num_samples` for
smooth shapes or 4 for polygon level sets (Manhattan / Chebyshev).

`scs_level_set_to_polygon`:
- For `CIRCLE / ELLIPSE / SUPERELLIPSE`: samples `num_samples` points uniformly
  around the curve (must be ≥ 3). Caller buffer must hold `2 * num_samples`
  doubles.
- For `POLYGON`: copies the 4 exact vertices; `num_samples` is ignored. Buffer
  must hold 8 doubles.

### StreamManager lifecycle

```c
SCS_StreamManager* scs_stream_manager_create(uint64_t master_seed,
    char* err_buf, int err_buf_len);   // returns NULL on failure

void scs_stream_manager_destroy(SCS_StreamManager* mgr);  // NULL is a no-op

int scs_register_streams(SCS_StreamManager*, const char** names, int n_names,
    char* err_buf, int err_buf_len);   // n_names=0 clears allowlist

int scs_reset_all(SCS_StreamManager*, uint64_t master_seed,
    char* err_buf, int err_buf_len);

int scs_reset_stream(SCS_StreamManager*, const char* stream_name, uint64_t seed,
    char* err_buf, int err_buf_len);

int scs_skip(SCS_StreamManager*, const char* stream_name, uint64_t n,
    char* err_buf, int err_buf_len);
```

### PRNG draws

Each draw function takes `(mgr, stream_name, …parameters…, out, err_buf, err_buf_len)`.
All require `mgr != NULL`, `stream_name != NULL`, and `out != NULL`.
If a stream allowlist is active, using an unlisted name returns
`SCS_ERROR_INVALID_ARGUMENT`.

| Function | Distribution | Output type |
|---|---|---|
| `scs_uniform_real` | Uniform [min, max) | `double*` |
| `scs_normal` | Normal(mean, stddev) | `double*` |
| `scs_bernoulli` | Bernoulli(p) → 0 or 1 | `int*` |
| `scs_uniform_int` | Uniform integer [min, max] | `int64_t*` |
| `scs_uniform_choice` | Uniform index in [0, n) | `int64_t*` |

---

## Phases C0–C7 at a glance

| Phase | Scope |
|-------|--------|
| **C0** | Version query (`scs_api_version`), error codes (`BUFFER_TOO_SMALL`, `OUT_OF_MEMORY`), `SCS_PairwiseResult` / `SCS_TieBreak` scalar domains, `SCS_MAJORITY_SIMPLE` (-1), `scs_level_set_to_polygon` size-query + `out_capacity`, thread-safety note |
| **C1** | Majority: `scs_majority_prefers_2d`, `scs_pairwise_matrix_2d`, `scs_weighted_majority_prefers_2d` |
| **C2** | Winset: `SCS_Winset*`, create/destroy, `is_empty`, `contains_point_2d`, `bbox_2d`, boundary size + sample, boolean ops, clone |
| **C3** | Yolk: `SCS_Yolk2d`, `scs_yolk_2d` |
| **C4** | Geometry: Copeland scores/winner, Condorcet winner 2d, core, uncovered set (discrete + boundary), Heart (discrete + boundary) |
| **C5** | Profile: `SCS_Profile*`, build_spatial, from_utility_matrix, generators (impartial culture, uniform/Gaussian spatial), lifecycle, export_rankings, clone |
| **C6** | Voting rules: plurality, Borda, anti-plurality, generic scoring rule, approval (spatial + top-k); each with scores / all_winners / one_winner (and Borda ranking) |
| **C7** | Social properties: `scs_rank_by_scores`, `scs_pairwise_ranking_from_matrix`, Pareto set / is_pareto_efficient, Condorcet/majority profile predicates |

See `docs/status/c_api_extensions_plan.md` for the full checklist and contracts.

---

## Mapping summary: C type ↔ C++ type

| C type | C++ type |
|--------|----------|
| `SCS_LossConfig` | `preference::indifference::LossConfig` (via `to_cpp_loss_config`) |
| `SCS_DistanceConfig` + pointer/len | `preference::indifference::DistanceConfig` + `std::vector<double>`; `salience_weights` is **borrowed** (caller-owned, not copied) |
| `SCS_LevelSet2d` | `preference::indifference::LevelSet2d` (`std::variant`) |
| `SCS_Yolk2d` | POD: `center_x`, `center_y`, `radius` |
| `SCS_PairwiseResult` | `int32_t` (-1 / 0 / 1) |
| `SCS_TieBreak` | `int32_t` (0 = kRandom, 1 = kSmallestIndex) |
| `SCS_StreamManager*` | `SCS_StreamManagerImpl { core::rng::StreamManager mgr; }` |
| `SCS_Winset*` | `SCS_WinsetImpl` (internal winset region / CGAL state) |
| `SCS_Profile*` | `SCS_ProfileImpl { aggregation::Profile profile; }` |
| `double*` point arrays, `int n` | `std::vector<Point2d>` or `Eigen::Map` as appropriate |

---

## Usage example (C pseudocode)

End-to-end workflow: fixed voter/alternative positions → spatial profile →
Borda winner → winset boundary sample → Yolk → Condorcet check.

```c
char err[512] = {0};
double w[2] = {1.0, 1.0};
SCS_DistanceConfig dc = {SCS_DIST_EUCLIDEAN, 2.0, w, 2};

// Alternatives and voter ideal points (e.g. 2 alts, 3 voters).
double alts[]    = {0.0, 0.0,  2.0, 0.0};
double voters[]  = {0.0, 0.0,  1.0, 0.0,  2.0, 0.0};

// Build ordinal profile from spatial data (distance → ranking).
SCS_Profile* p = scs_profile_build_spatial(alts, 2, voters, 3, &dc, err, 512);
if (!p) { /* handle err */ }

// Borda scores and single winner (deterministic tie-break).
int scores[2] = {0};
scs_borda_scores(p, scores, 2, err, 512);
int winner = -1;
scs_borda_one_winner(p, SCS_TIEBREAK_SMALLEST_INDEX, NULL, NULL, &winner, err, 512);

// Winset for same geometry; sample boundary.
SCS_Winset* ws = scs_winset_2d(1.0, 1.0, voters, 3, &dc, SCS_MAJORITY_SIMPLE,
                               SCS_DEFAULT_WINSET_SAMPLES, err, 512);
if (ws) {
  int xy_pairs = 0, n_paths = 0;
  scs_winset_boundary_size_2d(ws, &xy_pairs, &n_paths, err, 512);
  double* boundary_xy = (double*)malloc((size_t)(2 * xy_pairs) * sizeof(double));
  int *starts = (int*)malloc((size_t)n_paths * sizeof(int));
  int *holes  = (int*)malloc((size_t)n_paths * sizeof(int));
  int out_xy_n = 0, out_n_paths = 0;
  scs_winset_sample_boundary_2d(ws, boundary_xy, xy_pairs, &out_xy_n,
                                starts, n_paths, holes, &out_n_paths, err, 512);
  // ... use boundary_xy, starts, holes ...
  free(boundary_xy); free(starts); free(holes);
  scs_winset_destroy(ws);
}

// Yolk (smallest circle intersecting all median lines).
SCS_Yolk2d yolk = {0};
scs_yolk_2d(voters, 3, &dc, SCS_MAJORITY_SIMPLE, 720, &yolk, err, 512);

// Condorcet winner from profile (ordinal).
int cw_found = 0, cw_idx = -1;
scs_condorcet_winner_profile(p, &cw_found, &cw_idx, err, 512);
if (cw_found) { /* cw_idx is the Condorcet winner */ }

scs_profile_destroy(p);
```

---

## Thread-safety contract  (C0.5)

| Context | Rule |
|---------|------|
| **Stateless functions** | All pure-computation functions (`scs_calculate_distance`, `scs_distance_to_utility`, `scs_normalize_utility`, `scs_level_set_1d`, `scs_ic_interval_1d`, `scs_level_set_2d`, `scs_level_set_to_polygon`, `scs_ic_polygon_2d`, `scs_convex_hull_2d`, and all geometry functions added in Phase C1–C4) are **thread-safe** — they take no global mutable state. Callers may invoke them concurrently from multiple threads without synchronisation. |
| **SCS_StreamManager** | A `SCS_StreamManager` handle is **not thread-safe**. Concurrent calls on the same handle produce undefined behaviour. The caller is responsible for serialising access (e.g. one manager per thread, or an external mutex). |
| **scs_api_version** | Thread-safe — reads compile-time constants only. |
| **Error buffers** | `err_buf` is caller-owned per-call storage. Concurrent calls with distinct buffers are safe. Never share an `err_buf` between concurrent calls. |

This contract is intentionally minimal: the API imposes no internal locking, so callers pay no synchronisation cost for the common single-threaded or per-thread-manager patterns.

---

## ABI and shared library

All exported declarations use the **SCS_API** macro (default visibility on
Unix; `__declspec(dllexport/dllimport)` on Windows when building/using the
library). The shared library (`libscs_api`) should eventually use explicit
**VERSION** / **SOVERSION** in CMake so that soname compatibility is clear;
until then, matching source and header versions is the contract.

---

## Stability and deprecation

The C API surface documented here (C0–C7) is **stable** for R/Python binding
use. Once these functions ship:

- **Additive only:** New functions and new optional parameters may be added.
- **No signature-breaking changes:** Existing function signatures and the
  layout of public POD structs will not be changed in a backward-incompatible
  way. If a change is ever required, it will be done via a new function or
  struct and a deprecation period for the old one.
- **Size-query contract:** Variable-size output functions support null-buffer
  or zero-capacity size queries and return `SCS_ERROR_BUFFER_TOO_SMALL` when
  the provided buffer is too small; they do not truncate silently.
