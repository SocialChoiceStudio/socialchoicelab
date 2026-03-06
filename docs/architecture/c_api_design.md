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

---

## Error handling contract

Every function returns `int`:

| Return value | Meaning |
|---|---|
| `SCS_OK` (0) | Success |
| `SCS_ERROR_INVALID_ARGUMENT` (1) | Bad argument (null pointer, wrong dimension, out-of-range value, unregistered stream name) |
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

### Legacy and preferred polygon sampling APIs

The original minimal surface includes:

```c
int scs_level_set_to_polygon(const SCS_LevelSet2d* level_set, int num_samples,
    double* out_xy, int* out_n, char* err_buf, int err_buf_len);
```

This remains supported for backward compatibility, but new binding code should
prefer the additive size-query helpers:

```c
int scs_level_set_polygon_size(
    const SCS_LevelSet2d* level_set, int num_samples,
    int* out_xy_pairs,
    char* err_buf, int err_buf_len);

int scs_level_set_sample_polygon(
    const SCS_LevelSet2d* level_set, int num_samples,
    double* out_xy, int out_capacity,   // counted in (x,y) pairs
    int* out_n,
    char* err_buf, int err_buf_len);
```

Both paths must produce identical sampled coordinates.

### Planned opaque handles

The expanded stable surface uses opaque handles for C++ types that cannot be
represented safely as flat POD:

```c
typedef struct SCS_StreamManagerImpl SCS_StreamManager;
typedef struct SCS_WinsetImpl SCS_Winset;
typedef struct SCS_ProfileImpl SCS_Profile;
```

All such handles follow explicit ownership via create/destroy pairs.

### Planned geometry export rules

For sampled multi-path geometry such as winset boundaries:

- Paths are represented as interleaved `(x,y)` pairs.
- `out_path_starts[i]` gives the starting pair index for path `i`.
- Sampled paths do not repeat the first point at the end.
- Hole vs outer-boundary status is explicit via `out_path_is_hole[i]`; bindings
  must not infer topology from winding orientation.

For example:

```c
int scs_winset_sample_boundary_2d(
    const SCS_Winset* ws,
    int num_samples_per_path,
    double* out_xy, int out_xy_capacity,
    int* out_xy_n,
    int* out_path_starts, int out_path_capacity,
    int* out_path_is_hole,
    int* out_n_paths,
    char* err_buf, int err_buf_len);
```

Bounding-box queries over possibly empty geometry should expose that possibility
explicitly:

```c
int scs_winset_bbox_2d(
    const SCS_Winset* ws,
    int* out_found,
    double* out_min_x, double* out_min_y,
    double* out_max_x, double* out_max_y,
    char* err_buf, int err_buf_len);
```

### Planned ranking rule consistency

Generic ranking helpers should accept `double` score vectors so they remain
compatible with fractional scoring rules:

```c
int scs_rank_by_scores(
    const double* scores, int n_alts,
    SCS_TieBreak tie_break,
    SCS_StreamManager* mgr, const char* stream_name,
    int* out_ranking, int out_len,
    char* err_buf, int err_buf_len);
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

int scs_level_set_2d(double ideal_x, double ideal_y, double utility_level,
    const SCS_LossConfig* loss_cfg, const SCS_DistanceConfig* dist_cfg,
    SCS_LevelSet2d* out, char* err_buf, int err_buf_len);

int scs_level_set_to_polygon(const SCS_LevelSet2d* level_set, int num_samples,
    double* out_xy,   // interleaved [x0,y0,x1,y1,...]; caller-provided
    int* out_n,       // number of (x,y) pairs written
    char* err_buf, int err_buf_len);
```

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

## Mapping summary: C type ↔ C++ type

| C type | C++ type |
|---|---|
| `SCS_LossConfig` | `preference::indifference::LossConfig` (via `to_cpp_loss_config`) |
| `SCS_DistanceConfig` + pointer/len | `preference::indifference::DistanceConfig` + `std::vector<double>` |
| `SCS_LevelSet2d` | `preference::indifference::LevelSet2d` (`std::variant`) |
| `SCS_Yolk2d` | trivial POD result struct |
| `SCS_PairwiseResult` | fixed-width result domain (`int32_t`) |
| `SCS_TieBreak` | fixed-width tie-break domain (`int32_t`) |
| `SCS_StreamManager*` | `SCS_StreamManagerImpl { core::rng::StreamManager mgr; }` |
| `SCS_Winset*` | `SCS_WinsetImpl { ...WinsetRegion / CGAL polygon-set state... }` |
| `SCS_Profile*` | `SCS_ProfileImpl { ...Profile ranking state... }` |
| `double*, int n_dims` | `Eigen::Map<const Eigen::VectorXd>` |

---

## Usage example (C pseudocode)

```c
// Create a manager and draw voter positions.
char err[512] = {0};
SCS_StreamManager* mgr = scs_stream_manager_create(42, err, 512);
assert(mgr);

const char* names[] = {"voters", "candidates"};
scs_register_streams(mgr, names, 2, err, 512);

double x = 0.0;
scs_uniform_real(mgr, "voters", -10.0, 10.0, &x, err, 512);

// Compute distance from voter ideal to candidate.
double ideal[2] = {x, 0.0};
double cand[2]  = {3.0, 4.0};
double w[2]     = {1.0, 1.0};
SCS_DistanceConfig dc = {SCS_DIST_EUCLIDEAN, 2.0, w, 2};
double dist = 0.0;
scs_calculate_distance(ideal, cand, 2, &dc, &dist, err, 512);

// Convert to utility.
SCS_LossConfig lc = {SCS_LOSS_QUADRATIC, 1.0, 1.0, 1.0, 0.5};
double u = 0.0;
scs_distance_to_utility(dist, &lc, &u, err, 512);

scs_stream_manager_destroy(mgr);
```

---

## Thread-safety contract  (C0.5)

| Context | Rule |
|---------|------|
| **Stateless functions** | All pure-computation functions (`scs_calculate_distance`, `scs_distance_to_utility`, `scs_normalize_utility`, `scs_level_set_1d`, `scs_level_set_2d`, `scs_level_set_to_polygon`, `scs_convex_hull_2d`, and all geometry functions added in Phase C1–C4) are **thread-safe** — they take no global mutable state. Callers may invoke them concurrently from multiple threads without synchronisation. |
| **SCS_StreamManager** | A `SCS_StreamManager` handle is **not thread-safe**. Concurrent calls on the same handle produce undefined behaviour. The caller is responsible for serialising access (e.g. one manager per thread, or an external mutex). |
| **scs_api_version** | Thread-safe — reads compile-time constants only. |
| **Error buffers** | `err_buf` is caller-owned per-call storage. Concurrent calls with distinct buffers are safe. Never share an `err_buf` between concurrent calls. |

This contract is intentionally minimal: the API imposes no internal locking, so callers pay no synchronisation cost for the common single-threaded or per-thread-manager patterns.

---

## Stability note

This is the **c_api minimal** milestone surface plus the stable-surface rules
for the upcoming geometry / aggregation expansion. Functions already marked in
the header are stable for R/Python binding use. Additional functions from the
extension milestone are added additively rather than by signature-breaking changes.

`scs_level_set_to_polygon` gained an `out_capacity` parameter in Phase C0.
Variable-size output functions follow the explicit size-query / `SCS_ERROR_BUFFER_TOO_SMALL`
contract: pass `out_xy = NULL` to discover the required capacity, then allocate
and call again with the buffer.
