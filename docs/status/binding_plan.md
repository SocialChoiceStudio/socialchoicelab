# Binding Plan — R and Python

Short-to-medium-term plan to build `socialchoicelab` R and Python packages that
expose the full stable C API (`scs_api`) via the C ABI. After this milestone,
users can run spatial voting analyses entirely from R or Python without any C++,
Eigen, or CGAL on their machines.

**Authority:** This plan is the source for "what's next" for binding work.
When a step is done, mark it ✅ Done and update [where_we_are.md](where_we_are.md).

**References:**
- [Design Document](../architecture/design_document.md) — FFI & Interfaces
- [c_api_design.md](../architecture/c_api_design.md) — C API contract
- [binding_decisions.md](binding_decisions.md) — resolved design decisions
- [ROADMAP.md](ROADMAP.md) — near/mid-term context

---

## Resolved decisions (do not re-open without user approval)

| Decision | Resolution |
|----------|------------|
| Languages | R and Python simultaneously |
| Package names | `socialchoicelab` for both |
| Scope | All c_api functions **except** `scs_yolk_2d`, `scs_heart_2d`, `scs_heart_boundary_size_2d`, `scs_heart_boundary_2d` (known approximation issues) |
| R binding mechanism | `.Call()` via thin C registration layer; no C++ compilation in R package |
| Python binding mechanism | cffi ABI mode; no compilation step at `pip install` |
| R opaque handles | External pointer (`EXTPTR`) holding C handle + R6 class as user-facing API |
| Python opaque handles | Python class with `__del__` (safety net) + `__enter__`/`__exit__` (deterministic) |
| CI | Single workflow file; core job builds `libscs_api` first, then R and Python jobs run in parallel |
| R docs | roxygen2 + pkgdown site |
| Python docs | mkdocs + mkdocstrings site |
| Examples | Paired R vignette + Python Jupyter notebook showing the same spatial voting analysis |

---

## Design principles (apply throughout)

1. **Bindings call only `scs_api.h`.** Never call C++ headers, Eigen, or CGAL from R or Python code.
2. **No compilation in the binding packages.** `libscs_api` is a pre-built shared library. R and Python packages link against it; they do not compile it.
3. **Index translation at the binding boundary only.** The C API is 0-indexed. R wrappers translate to 1-indexed on the way in and out. Python keeps 0-indexed.
4. **Error propagation is uniform.** Every C API call checks the return code. Non-zero codes become R `stop()` / Python exceptions, always including the `err_buf` message.
5. **Opaque handles are never exposed raw.** `SCS_Winset*`, `SCS_Profile*`, and `SCS_StreamManager*` are always wrapped (R: R6 + external pointer; Python: class). Users never see raw pointers.
6. **Destroy is always called.** R6 `$finalize()` and Python `__del__`/`__exit__` must call the corresponding `scs_*_destroy` function. Resource leaks are not acceptable.
7. **Null-buffer size-query pattern is hidden from users.** Where the C API requires a two-call pattern (query size then fill), the binding wraps this into a single call that allocates the right-sized vector automatically.
8. **Parallel API across R and Python.** Function names, argument names, and return shapes should be as close as possible between R and Python, modulo language conventions (snake_case in both; R uses named list returns, Python uses dataclasses or dicts where needed).
9. **Reproducibility is first-class.** `StreamManager` is exposed in both bindings with the same seed/stream semantics as the C API. Users can reproduce any simulation by recording the master seed and stream names.
10. **Documentation is mandatory, not optional.** Every exported function must have a roxygen2 doc (R) or docstring (Python) before the phase is marked done.

---

## Scope

**In scope:**

All `scs_api.h` functions **except** the four excluded below, grouped as:
- Version and utility: `scs_api_version`
- Distance / loss / level-set: `scs_calculate_distance`, `scs_distance_to_utility`, `scs_normalize_utility`, `scs_level_set_1d`, `scs_level_set_2d`, `scs_level_set_to_polygon`
- Geometry (stateless): `scs_convex_hull_2d`, `scs_majority_prefers_2d`, `scs_pairwise_matrix_2d`, `scs_weighted_majority_prefers_2d`, `scs_copeland_scores_2d`, `scs_copeland_winner_2d`, `scs_has_condorcet_winner_2d`, `scs_condorcet_winner_2d`, `scs_core_2d`, `scs_uncovered_set_2d`, `scs_uncovered_set_boundary_size_2d`, `scs_uncovered_set_boundary_2d`
- Winset (opaque handle): `scs_winset_2d`, `scs_weighted_winset_2d`, `scs_winset_with_veto_2d`, `scs_winset_destroy`, `scs_winset_is_empty`, `scs_winset_contains_point_2d`, `scs_winset_bbox_2d`, `scs_winset_boundary_size_2d`, `scs_winset_sample_boundary_2d`, `scs_winset_clone`, winset boolean ops
- Profile (opaque handle): `scs_profile_build_spatial`, `scs_profile_from_utility_matrix`, `scs_profile_impartial_culture`, `scs_profile_uniform_spatial`, `scs_profile_gaussian_spatial`, `scs_profile_destroy`, `scs_profile_dims`, `scs_profile_get_ranking`, `scs_profile_export_rankings`, `scs_profile_clone`
- Voting rules: plurality, Borda, anti-plurality, scoring rule, approval (spatial + top-k)
- Social rankings and properties: `scs_rank_by_scores`, `scs_pairwise_ranking_from_matrix`, `scs_pareto_set`, `scs_is_pareto_efficient`, `scs_has_condorcet_winner_profile`, `scs_condorcet_winner_profile`, `scs_is_selected_by_condorcet_consistent_rules`, `scs_is_selected_by_majority_consistent_rules`
- StreamManager (opaque handle): create, destroy, register, reset_all, reset_stream, skip, uniform_real, normal, bernoulli, uniform_int, uniform_choice

**Explicitly excluded from this release:**
- `scs_yolk_2d` — LP yolk approximation, not the true yolk (Stone & Tovey 1992)
- `scs_heart_2d` — deferred with Yolk
- `scs_heart_boundary_size_2d` — grid approximation only
- `scs_heart_boundary_2d` — grid approximation only

**Out of scope:**
- Any new C++ or C API code
- GUI or Shiny app
- 3D / N-D geometry

---

## Phase B0 — Repository structure and build infrastructure

### B0.1 — Directory layout

Create the top-level directories for both packages inside the repo:

```
r/
  DESCRIPTION
  NAMESPACE
  R/                  ← R source files
  src/                ← init.c (C registration) + Makevars
  man/                ← generated by roxygen2; do not edit by hand
  vignettes/
  tests/testthat/

python/
  pyproject.toml
  README.md
  src/
    socialchoicelab/
      __init__.py
      _loader.py      ← cffi ABI-mode loader
      _error.py       ← exception types
      stream_manager.py
      winset.py
      profile.py
      distance.py
      geometry.py
      voting.py
  tests/
  docs/               ← mkdocs source
  notebooks/
```

### B0.2 — R package skeleton

- `DESCRIPTION`: package name `socialchoicelab`, license GPL-3, imports `R6`.
- `NAMESPACE`: generated by roxygen2 via `devtools::document()`.
- `src/Makevars` and `src/Makevars.win`: link against pre-built `libscs_api`. The library path is resolved at install time via an `configure` script or a bundled `.so`/`.dylib`. Document clearly in CONTRIBUTING.md how to set the library path for local development.
- `src/init.c`: `R_registerRoutines` and `R_useDynamicSymbols(dll, FALSE)` for all `.Call()` entry points.

### B0.3 — Python package skeleton

- `pyproject.toml`: package name `socialchoicelab`, build backend `hatchling` (or `flit`); cffi as a runtime dependency; mkdocs + mkdocstrings as optional docs dependencies.
- `src/socialchoicelab/_loader.py`: cffi ABI-mode loader that locates and opens `libscs_api` and parses the C declarations from `scs_api.h`.

### B0.4 — CI workflow update

Extend `.github/workflows/ci.yml`:

```
jobs:
  core:          ← existing; builds libscs_api and runs C++ tests
  r-binding:     ← new; depends on core; installs R, runs R CMD check
  python-binding: ← new; depends on core; runs pytest
```

The `libscs_api` shared library artifact is passed from the `core` job to `r-binding` and `python-binding` via GitHub Actions artifact upload/download.

---

## Phase B1 — Shared conventions (document before writing any binding code)

### B1.1 — Library distribution strategy

For local development: `libscs_api.dylib` / `libscs_api.so` is built in `build/` by CMake. Binding packages find it via an environment variable `SCS_LIB_PATH` or a `configure` script.

For CI: the `core` job uploads `libscs_api` as a GitHub Actions artifact; binding jobs download it before running.

For end-user distribution: document as future work (pre-built binaries / conda-forge / CRAN binary packages). This plan covers the developer/CI path only.

### B1.2 — Error handling conventions

**R:** A helper C function `scs_check(int rc, const char* err_buf)` (in `src/init.c`) calls `Rf_error("%s", err_buf)` if `rc != SCS_OK`. All `.Call()` wrappers call this after every c_api call.

**Python:** `socialchoicelab._error` defines `SCSError(RuntimeError)`, `SCSInvalidArgumentError(SCSError)`, `SCSBufferTooSmallError(SCSError)`, `SCSInternalError(SCSError)`, `SCSOutOfMemoryError(SCSError)`. A helper `_check(rc, err_buf)` maps return codes to exception types and raises.

### B1.3 — Type mapping

| C type | R type | Python type |
|--------|--------|-------------|
| `double` | `numeric` (length 1) | `float` |
| `double*` (array) | `numeric` vector | `numpy.ndarray` (float64) |
| `int` | `integer` (length 1) | `int` |
| `int*` (array) | `integer` vector | `numpy.ndarray` (int32) |
| `SCS_Winset*` | `Winset` R6 object | `Winset` Python object |
| `SCS_Profile*` | `Profile` R6 object | `Profile` Python object |
| `SCS_StreamManager*` | `StreamManager` R6 object | `StreamManager` Python object |
| `SCS_DistanceConfig` | named list (marshalled in C glue) | dataclass / dict (marshalled in cffi) |
| `SCS_LossConfig` | named list | dataclass / dict |
| 0-based index | translated to 1-based on return | kept 0-based |

---

## Phase B2 — R binding: infrastructure

### B2.1 — StreamManager R6 class

```r
StreamManager <- R6::R6Class("StreamManager",
  private = list(ptr = NULL),
  public = list(
    initialize = function(master_seed, stream_names = NULL) { ... },
    finalize   = function() { scs_stream_manager_destroy(private$ptr) },
    reset_all  = function(master_seed) { ... },
    # ... all stream methods
  )
)
```

The external pointer is stored in `private$ptr`. `finalize()` calls `scs_stream_manager_destroy`. All method calls go through `.Call("scs_stream_manager_*", private$ptr, ...)`.

### B2.2 — Winset R6 class

```r
Winset <- R6::R6Class("Winset",
  private = list(ptr = NULL),
  public = list(
    initialize  = function(ptr) { private$ptr <- ptr },
    finalize    = function() { scs_winset_destroy(private$ptr) },
    is_empty    = function() { ... },
    contains    = function(x, y) { ... },
    bbox        = function() { ... },
    boundary    = function(n_samples) { ... },
    clone_winset = function() { ... },
    union       = function(other) { ... },
    intersection = function(other) { ... },
    difference  = function(other) { ... }
  )
)
```

Factory functions (not constructors, to allow error propagation): `winset_2d()`, `weighted_winset_2d()`, `winset_with_veto_2d()` — each calls the c_api, checks the return, and wraps the pointer in a `Winset` R6 object.

### B2.3 — Profile R6 class

```r
Profile <- R6::R6Class("Profile",
  private = list(ptr = NULL),
  public = list(
    initialize   = function(ptr) { private$ptr <- ptr },
    finalize     = function() { scs_profile_destroy(private$ptr) },
    dims         = function() { ... },
    get_ranking  = function(voter) { ... },   # 1-indexed voter → 1-indexed ranking
    export       = function() { ... },         # returns integer matrix
    clone_profile = function() { ... }
  )
)
```

Factory functions: `profile_build_spatial()`, `profile_from_utility_matrix()`, `profile_impartial_culture()`, `profile_uniform_spatial()`, `profile_gaussian_spatial()`.

### B2.4 — Helper utilities

- `make_dist_config(distance_type, weights, order_p = 2)` — builds an `SCS_DistanceConfig`-compatible structure passed to C glue.
- `make_loss_config(loss_type, sensitivity, ...)` — same for `SCS_LossConfig`.
- Internal `err_buf` is always allocated as `raw(512)` in R and passed to `.Call()`; the C glue writes into it and the R wrapper converts to character on error.

---

## Phase B3 — R binding: function groups

### B3.1 — Version and utility

- `scs_version()` → named list `list(major, minor, patch)`

### B3.2 — Distance / loss / level-set

- `calculate_distance(ideal, alt, dist_config)` → scalar double
- `distance_to_utility(distance, loss_config)` → scalar double
- `normalize_utility(utility, max_distance, loss_config)` → scalar double
- `level_set_1d(ideal, weight, utility_level, loss_config)` → numeric vector (0, 1, or 2 points)
- `level_set_2d(ideal_x, ideal_y, utility_level, dist_config, loss_config)` → named list (type + parameters)
- `level_set_to_polygon(level_set, n_samples)` → 2-column matrix of (x, y) pairs

### B3.3 — Geometry: convex hull and majority preference

- `convex_hull_2d(points)` → 2-column matrix (n_vertices × 2)
- `majority_prefers_2d(a, b, voter_ideals, dist_config, k = "simple")` → logical
- `pairwise_matrix_2d(alts, voter_ideals, dist_config, k = "simple")` → integer matrix (WIN/TIE/LOSS as 1/0/-1)
- `weighted_majority_prefers_2d(a, b, voter_ideals, weights, dist_config, threshold)` → logical

### B3.4 — Winset

- `winset_2d(status_quo, voter_ideals, dist_config, k = "simple", n_samples)` → `Winset` R6 object
- `weighted_winset_2d(...)` → `Winset` R6 object
- `winset_with_veto_2d(...)` → `Winset` R6 object

All Winset methods available as R6 methods on the returned object. Winset boolean ops (`$union()`, `$intersection()`, `$difference()`) return new `Winset` R6 objects.

### B3.5 — Geometry: Copeland, Condorcet, core, uncovered set

- `copeland_scores_2d(alts, voter_ideals, dist_config, k)` → integer vector (1-indexed)
- `copeland_winner_2d(alts, voter_ideals, dist_config, k)` → integer (1-indexed)
- `has_condorcet_winner_2d(alts, voter_ideals, dist_config, k)` → logical
- `condorcet_winner_2d(alts, voter_ideals, dist_config, k)` → integer or `NA` (1-indexed)
- `core_2d(voter_ideals, dist_config, k)` → named list `list(found, x, y)`
- `uncovered_set_2d(alts, voter_ideals, dist_config, k)` → integer vector (1-indexed)
- `uncovered_set_boundary_2d(voter_ideals, dist_config, grid_resolution, k)` → 2-column matrix

### B3.6 — Profile construction and inspection

Factory functions return `Profile` R6 objects. Profile methods: `$dims()`, `$get_ranking(voter)`, `$export()`, `$clone_profile()`.

### B3.7 — Voting rules

All voting rule functions take a `Profile` R6 object as first argument.

- `plurality_scores(profile)` → integer vector
- `plurality_all_winners(profile)` → integer vector (1-indexed)
- `plurality_one_winner(profile, tie_break, stream_manager, stream_name)` → integer (1-indexed)
- *(same pattern for borda, antiplurality, scoring_rule)*
- `borda_ranking(profile, tie_break, stream_manager, stream_name)` → integer vector (1-indexed)
- `approval_scores_spatial(alts, voter_ideals, threshold, dist_config)` → integer vector
- `approval_all_winners_spatial(...)` → integer vector (1-indexed)
- `approval_scores_topk(profile, k)` → integer vector
- `approval_all_winners_topk(profile, k)` → integer vector (1-indexed)

### B3.8 — Social rankings and properties

- `rank_by_scores(scores, tie_break, stream_manager, stream_name)` → integer vector (1-indexed)
- `pairwise_ranking_from_matrix(matrix, n_alts, tie_break, ...)` → integer vector (1-indexed)
- `pareto_set(profile)` → integer vector (1-indexed)
- `is_pareto_efficient(profile, alt_idx)` → logical (alt_idx is 1-indexed)
- `has_condorcet_winner_profile(profile)` → logical
- `condorcet_winner_profile(profile)` → integer or `NA` (1-indexed)
- `is_condorcet_consistent(profile)` → logical
- `is_majority_consistent(profile)` → logical

### B3.9 — StreamManager

- `stream_manager(master_seed, stream_names = NULL)` → `StreamManager` R6 object
- Methods: `$register(names)`, `$reset_all(seed)`, `$reset_stream(name, seed)`, `$skip(name, n)`, `$uniform_real(name, min, max)`, `$normal(name, mean, sd)`, `$bernoulli(name, p)`, `$uniform_int(name, min, max)`, `$uniform_choice(name, n, choices)`

---

## Phase B4 — Python binding: infrastructure ✅ Done

### B4.1 — cffi loader ✅ Done

`src/socialchoicelab/_loader.py`:
- Opens `libscs_api` via `cffi.FFI().dlopen(path)`.
- Library path resolved by: (1) `SCS_LIB_PATH` env var, (2) standard system paths, (3) bundled path for packaged distributions.
- C declarations parsed from a trimmed copy of `scs_api.h` (preprocessed, with `SCS_API` stripped).
- A module-level `_lib` and `_ffi` singleton is created on first import.

### B4.2 — Exception hierarchy ✅ Done

```python
class SCSError(RuntimeError): ...
class SCSInvalidArgumentError(SCSError): ...
class SCSInternalError(SCSError): ...
class SCSBufferTooSmallError(SCSError): ...
class SCSOutOfMemoryError(SCSError): ...

def _check(rc: int, err_buf) -> None:
    if rc == SCS_OK: return
    msg = _ffi.string(err_buf).decode()
    # map rc → exception type and raise
```

### B4.3 — StreamManager class ✅ Done

```python
class StreamManager:
    def __init__(self, master_seed, stream_names=None): ...
    def __del__(self): _lib.scs_stream_manager_destroy(self._ptr)
    def __enter__(self): return self
    def __exit__(self, *_): _lib.scs_stream_manager_destroy(self._ptr); self._ptr = None
    def register(self, names): ...
    def reset_all(self, seed): ...
    # ... all stream methods
```

### B4.4 — Winset class ✅ Done

```python
class Winset:
    def __init__(self, ptr): self._ptr = ptr
    def __del__(self): _lib.scs_winset_destroy(self._ptr)
    def __enter__(self): return self
    def __exit__(self, *_): _lib.scs_winset_destroy(self._ptr); self._ptr = None
    def is_empty(self) -> bool: ...
    def contains(self, x, y) -> bool: ...
    def bbox(self) -> dict: ...
    def boundary(self, n_samples) -> np.ndarray: ...
    def clone(self) -> "Winset": ...
    def union(self, other) -> "Winset": ...
    def intersection(self, other) -> "Winset": ...
    def difference(self, other) -> "Winset": ...
```

Factory functions: `winset_2d(...)`, `weighted_winset_2d(...)`, `winset_with_veto_2d(...)`.

### B4.5 — Profile class ✅ Done

```python
class Profile:
    def __init__(self, ptr): self._ptr = ptr
    def __del__(self): _lib.scs_profile_destroy(self._ptr)
    def __enter__(self): return self
    def __exit__(self, *_): _lib.scs_profile_destroy(self._ptr); self._ptr = None
    def dims(self) -> tuple[int, int]: ...
    def get_ranking(self, voter: int) -> np.ndarray: ...
    def export(self) -> np.ndarray: ...   # (n_voters, n_alts) int array
    def clone(self) -> "Profile": ...
```

Factory functions: `profile_build_spatial(...)`, `profile_from_utility_matrix(...)`, etc.

---

## Phase B5 — Python binding: function groups ✅ Done

Mirrors B3 exactly, with Python conventions:
- 0-indexed throughout (no translation).
- Arrays are `numpy.ndarray`; flat `double*` inputs accept any array-like and are converted.
- `DistanceConfig` and `LossConfig` are Python dataclasses.
- Function names identical to R equivalents (snake_case in both).
- All functions in `socialchoicelab` top-level namespace.

### B5.1 — version, distance, loss, level-set, geometry ✅ Done

`python/src/socialchoicelab/_functions.py`:
- `scs_version()` → `dict`
- `calculate_distance`, `distance_to_utility`, `normalize_utility`
- `level_set_1d`, `level_set_2d`, `level_set_to_polygon`
- `convex_hull_2d`, `majority_prefers_2d`, `weighted_majority_prefers_2d`, `pairwise_matrix_2d`

### B5.2 — Copeland, Condorcet, core, uncovered set ✅ Done

`python/src/socialchoicelab/_geometry.py`:
- `copeland_scores_2d`, `copeland_winner_2d`
- `has_condorcet_winner_2d`, `condorcet_winner_2d`
- `core_2d`
- `uncovered_set_2d`, `uncovered_set_boundary_2d`

### B5.3 — Voting rules ✅ Done

`python/src/socialchoicelab/_voting_rules.py`:
- `plurality_scores`, `plurality_all_winners`, `plurality_one_winner`
- `borda_scores`, `borda_all_winners`, `borda_one_winner`, `borda_ranking`
- `antiplurality_scores`, `antiplurality_all_winners`, `antiplurality_one_winner`
- `scoring_rule_scores`, `scoring_rule_all_winners`, `scoring_rule_one_winner`
- `approval_scores_spatial`, `approval_all_winners_spatial`
- `approval_scores_topk`, `approval_all_winners_topk`

### B5.4 — Social rankings and properties ✅ Done

`python/src/socialchoicelab/_social.py`:
- `rank_by_scores`, `pairwise_ranking_from_matrix`
- `pareto_set`, `is_pareto_efficient`
- `has_condorcet_winner_profile`, `condorcet_winner_profile`
- `is_condorcet_consistent`, `is_majority_consistent`

---

## Phase B6 — Documentation

### B6.1 — R roxygen2

Every exported R function must have:
- `@title`, `@description`, `@param` for each argument, `@return`, `@examples` (at least one runnable example).
- Citation to the original social choice paper where relevant (e.g. Copeland 1951, Condorcet 1785).

### B6.2 — R pkgdown site

- `_pkgdown.yml` with reference index grouping functions by category (distance/loss, geometry, winset, profile, voting rules, properties).
- Deployed to GitHub Pages via a CI step.

### B6.3 — Python docstrings

Every public function and class method must have a NumPy-style docstring with `Parameters`, `Returns`, and `Examples`.

### B6.4 — mkdocs + mkdocstrings site

- `docs/mkdocs.yml` with `mkdocstrings` plugin.
- API reference auto-generated from docstrings.
- Deployed to GitHub Pages (separate from R pkgdown).

---

## Phase B7 — Examples

### B7.1 — R vignette

File: `r/vignettes/spatial_voting.Rmd`

Content outline:
1. Create voter ideal points (2D spatial, 5 voters)
2. Create `StreamManager` for reproducible generation
3. Compute majority preference between two alternatives
4. Build and query a winset (is it empty? what's its boundary?)
5. Find Copeland winner
6. Check for Condorcet winner
7. Build a profile and compute plurality and Borda scores
8. Plot results (using base R or ggplot2; Plotly deferred to visualization layer)

### B7.2 — Python Jupyter notebook

File: `python/notebooks/spatial_voting.ipynb`

Same outline as the R vignette, in Python. Side-by-side comparisons in markdown cells where the API differs.

---

## Phase B8 — Tests

### B8.1 — R testthat

File: `r/tests/testthat/test_*.R`

Required coverage:
- One test per exported function covering the normal path.
- Error path tests: null inputs, wrong dimensions, non-finite values (matching C8 tests).
- Lifecycle tests: winset/profile/stream_manager objects are destroyed correctly (test via a custom finalizer that records destruction).
- Index translation: verify that R functions return 1-indexed results where expected.

### B8.2 — Python pytest

File: `python/tests/test_*.py`

Required coverage:
- Same normal-path and error-path coverage as R.
- Context manager tests: `with winset_2d(...) as ws:` correctly destroys on exit.
- NumPy array shape and dtype verification for all array-returning functions.
- Reproducibility tests: same master seed → same results across calls.

---

## Phase B9 — CI

Update `.github/workflows/ci.yml`:

```yaml
jobs:
  core:
    # existing: build libscs_api, run C++ tests, upload libscs_api artifact

  r-binding:
    needs: core
    steps:
      - download libscs_api artifact
      - install R + dependencies (R6, testthat, devtools, roxygen2, pkgdown)
      - R CMD check r/
      - build pkgdown site

  python-binding:
    needs: core
    steps:
      - download libscs_api artifact
      - install Python + dependencies (cffi, numpy, pytest, mkdocs, mkdocstrings)
      - pip install python/
      - pytest python/tests/
      - build mkdocs site
```

---

## Progress tracker

| Phase | Item | Status |
|-------|------|--------|
| B0 | B0.1 Directory layout | ✅ Done |
| B0 | B0.2 R package skeleton | ✅ Done |
| B0 | B0.3 Python package skeleton | ✅ Done |
| B0 | B0.4 CI workflow update | ✅ Done |
| B1 | B1.1 Library distribution strategy | ✅ Done |
| B1 | B1.2 Error handling conventions | ✅ Done |
| B1 | B1.3 Type mapping document | ✅ Done |
| B2 | B2.1 StreamManager R6 class | ✅ Done |
| B2 | B2.2 Winset R6 class | ✅ Done |
| B2 | B2.3 Profile R6 class | ✅ Done |
| B2 | B2.4 R helper utilities | ✅ Done |
| B3 | B3.1 Version and utility | ✅ Done |
| B3 | B3.2 Distance / loss / level-set | ✅ Done |
| B3 | B3.3 Convex hull and majority | ✅ Done |
| B3 | B3.4 Winset functions | ✅ Done (in B2) |
| B3 | B3.5 Copeland, Condorcet, core, uncovered set | ✅ Done |
| B3 | B3.6 Profile construction and inspection | ✅ Done (in B2) |
| B3 | B3.7 Voting rules | ✅ Done |
| B3 | B3.8 Social rankings and properties | ✅ Done |
| B3 | B3.9 StreamManager R methods | ✅ Done (in B2 + stream_manager() factory) |
| B4 | B4.1 cffi loader | ⬜ |
| B4 | B4.2 Exception hierarchy | ⬜ |
| B4 | B4.3 StreamManager Python class | ⬜ |
| B4 | B4.4 Winset Python class | ⬜ |
| B4 | B4.5 Profile Python class | ⬜ |
| B5 | B5 Python function groups (mirrors B3) | ⬜ |
| B6 | B6.1 R roxygen2 docs | ⬜ |
| B6 | B6.2 R pkgdown site | ⬜ |
| B6 | B6.3 Python docstrings | ⬜ |
| B6 | B6.4 mkdocs + mkdocstrings site | ⬜ |
| B7 | B7.1 R vignette | ⬜ |
| B7 | B7.2 Python Jupyter notebook | ⬜ |
| B8 | B8.1 R testthat tests | ⬜ |
| B8 | B8.2 Python pytest tests | ⬜ |
| B9 | B9 CI jobs for R and Python | ⬜ |
