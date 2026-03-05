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
| `include/scs_api.h` | Public C header — all enums, POD structs, and function declarations |
| `src/c_api/scs_api.cpp` | `extern "C"` implementation — maps C types ↔ C++ types, catches exceptions → error codes |
| `tests/unit/test_c_api.cpp` | GTest suite calling the C API using only C types |

The library is built as a **shared library** (`libscs_api.dylib` / `libscs_api.so`).
R and Python bindings link only against this artifact; they do not see Eigen or STL.

---

## Error handling contract

Every function returns `int`:

| Return value | Meaning |
|---|---|
| `SCS_OK` (0) | Success |
| `SCS_ERROR_INVALID_ARGUMENT` (1) | Bad argument (null pointer, wrong dimension, out-of-range value, unregistered stream name) |
| `SCS_ERROR_INTERNAL` (2) | Unexpected internal error |

If `err_buf != NULL` and `err_buf_len > 0`, a null-terminated message is written
into `err_buf` on any non-zero return. Bindings should always provide a buffer
(e.g. 512 bytes) and propagate the message to their native error type.

`scs_stream_manager_create` returns `NULL` on failure (rare; only on allocation
error) and writes the message into `err_buf`.

---

## POD structs

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

---

## Functions

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
| `SCS_StreamManager*` | `SCS_StreamManagerImpl { core::rng::StreamManager mgr; }` |
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

## Stability note

This is the **c_api minimal** milestone surface. Functions marked in the header
are stable for R/Python binding use. Additional functions (e.g. for geometry
layers 3+) will be added in a future milestone without breaking existing
callers.
