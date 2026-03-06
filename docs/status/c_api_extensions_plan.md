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
   outputs are plain C types: scalars, flat arrays, POD structs, or opaque
   handles. Caller owns all output buffers unless a function allocates an
   opaque handle (in which case a matching `*_destroy` function exists).
2. **Opaque handles for complex C++ objects.** `WinsetRegion` (CGAL
   `General_polygon_set_2`) and `Profile` (vector-of-vectors) cannot be
   represented as flat C POD. Both are exposed as opaque `SCS_Winset*` and
   `SCS_Profile*` handles with explicit lifecycle (create / destroy).
3. **Flat arrays for polygon output.** Functions returning a `Polygon2E`
   (uncovered set boundary, heart boundary, winset serialization) write
   interleaved `[x0,y0, x1,y1, …]` into a caller-provided `double* out_xy`
   buffer with an `int* out_n` count of (x,y) pairs.
4. **Caller-provided output buffers for fixed-size arrays.** Score vectors,
   winner lists, pairwise matrices, and ranking arrays are written into
   caller-provided buffers whose required size is always `n_alts` or
   `n_alts * n_alts` — predictable before the call.
5. **Tie-breaking via `SCS_TieBreak` enum + nullable `SCS_StreamManager*`.**
   `SCS_TIEBREAK_RANDOM` requires a non-null manager; `SCS_TIEBREAK_SMALLEST_INDEX`
   accepts a null manager. Missing or wrong manager → `SCS_ERROR_INVALID_ARGUMENT`.
6. **Optional outputs use a found-flag.** Where C++ returns `std::optional`
   (e.g. `condorcet_winner`), the C API uses an `int* out_found` flag (1 = found,
   0 = absent) alongside the value output parameter.
7. **All indices are 0-based.** The R binding layer translates to 1-based at
   the binding boundary, not here.
8. **Error handling unchanged.** All functions return `SCS_OK` / error code +
   optional `char* err_buf` / `int err_buf_len`, matching the existing pattern.

---

## Scope

**In scope:**

- C API wrappers for all geometry services: majority preference, winset (opaque
  handle + boolean ops), Yolk, uncovered set (discrete + boundary), Copeland,
  core/Condorcet detection, Heart (discrete + boundary).
- C API wrappers for all aggregation services: profile construction, synthetic
  generators, positional voting rules (plurality, Borda, anti-plurality,
  generic scoring rule), approval voting, social rankings, Pareto efficiency,
  Condorcet/majority consistency.
- New POD structs: `SCS_Yolk2d`, `SCS_TieBreak` enum.
- New opaque types: `SCS_Winset*`, `SCS_Profile*`.
- Tests covering all new functions (normal paths + key error paths).
- Updated `c_api_design.md` with all new types and functions.

**Out of scope:**

- R or Python binding code (next milestone).
- Any new C++ algorithms (geometry and aggregation layers are feature-complete).
- GUI or visualization.

---

## Phase C1 — Geometry: majority preference

### C1.1 — `scs_majority_prefers_2d`

Wraps `geometry::majority_prefers(x, y, voter_ideals, cfg, k)`.

```c
// Returns 1 in *out if strict k-majority of voters prefer x to y, else 0.
int scs_majority_prefers_2d(
    double x_x, double x_y,           // point X
    double y_x, double y_y,           // point Y
    const double* voter_ideals_xy,    // flat [x0,y0, x1,y1, …]; length 2*n_voters
    int n_voters,
    const SCS_DistanceConfig* dist_cfg,
    int k,                            // majority threshold (-1 = simple majority)
    int* out,                         // 1 if majority prefers X to Y, else 0
    char* err_buf, int err_buf_len);
```

### C1.2 — `scs_pairwise_matrix_2d`

Wraps `geometry::pairwise_matrix(alternatives, voter_ideals, cfg, k)`.
Returns an `n_alts × n_alts` matrix of `{-1, 0, 1}` as a flat row-major int array.

```c
int scs_pairwise_matrix_2d(
    const double* alt_xy,          // flat [x0,y0, …]; length 2*n_alts
    int n_alts,
    const double* voter_ideals_xy, // flat; length 2*n_voters
    int n_voters,
    const SCS_DistanceConfig* dist_cfg,
    int k,
    int* out_matrix,               // caller buffer, size n_alts*n_alts (row-major)
    char* err_buf, int err_buf_len);
```

### C1.3 — `scs_weighted_majority_prefers_2d`

Wraps `geometry::weighted_majority_prefers(x, y, voter_ideals, weights, cfg, threshold)`.

```c
int scs_weighted_majority_prefers_2d(
    double x_x, double x_y,
    double y_x, double y_y,
    const double* voter_ideals_xy,
    int n_voters,
    const double* weights,          // length n_voters
    const SCS_DistanceConfig* dist_cfg,
    double threshold,               // weight fraction in (0,1]
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

Wraps `geometry::winset_2d(sq, voter_ideals, cfg, k, num_samples)`.

```c
SCS_Winset* scs_winset_2d(
    double sq_x, double sq_y,
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg,
    int k, int num_samples,        // num_samples <= 0 → default (64)
    char* err_buf, int err_buf_len);
```

Returns NULL on error.

### C2.3 — `scs_weighted_winset_2d`

Wraps `geometry::weighted_winset_2d(sq, voter_ideals, weights, cfg, threshold, num_samples)`.

```c
SCS_Winset* scs_weighted_winset_2d(
    double sq_x, double sq_y,
    const double* voter_ideals_xy, int n_voters,
    const double* weights,
    const SCS_DistanceConfig* dist_cfg,
    double threshold, int num_samples,
    char* err_buf, int err_buf_len);
```

### C2.4 — `scs_winset_with_veto_2d`

Wraps `geometry::winset_with_veto_2d(sq, voter_ideals, cfg, veto_ideals, k, num_samples)`.

```c
SCS_Winset* scs_winset_with_veto_2d(
    double sq_x, double sq_y,
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg,
    const double* veto_ideals_xy, int n_veto,   // 0 veto players → same as winset_2d
    int k, int num_samples,
    char* err_buf, int err_buf_len);
```

### C2.5 — Lifecycle and query

```c
void scs_winset_destroy(SCS_Winset* ws);  // NULL is no-op

int scs_winset_is_empty(
    const SCS_Winset* ws, int* out_is_empty,
    char* err_buf, int err_buf_len);

// Serialize to flat polygon: writes up to *out_max_n (x,y) pairs.
// Sets *out_n to actual count written (may be 0 if empty).
// Caller must provide out_xy buffer of size 2 * num_samples.
int scs_winset_to_polygon(
    const SCS_Winset* ws, int num_samples,
    double* out_xy, int* out_n,
    char* err_buf, int err_buf_len);
```

### C2.6 — Boolean set operations

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

Wraps `geometry::yolk_2d(voter_ideals, k, n_sample)`.

```c
int scs_yolk_2d(
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg,
    int k,                         // -1 = simple majority
    int n_sample,                  // directional samples (0 → default 720)
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
    int* out_scores,               // caller buffer, size n_alts
    char* err_buf, int err_buf_len);
```

### C4.2 — `scs_copeland_winner_2d`

Wraps `geometry::copeland_winner(alternatives, voter_ideals, cfg, k)`.
Returns the xy coordinates of the alternative with the highest Copeland score
(ties broken by first in vector).

```c
int scs_copeland_winner_2d(
    const double* alt_xy, int n_alts,
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg, int k,
    double* out_x, double* out_y,
    char* err_buf, int err_buf_len);
```

### C4.3 — `scs_has_condorcet_winner_2d` and `scs_condorcet_winner_2d`

Wraps `geometry::has_condorcet_winner` and `geometry::condorcet_winner` (finite alternative set).

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
    int* out_found, double* out_x, double* out_y,
    char* err_buf, int err_buf_len);
```

### C4.4 — `scs_core_2d`

Wraps `geometry::core_2d(voter_ideals, cfg, k)` (continuous space: checks Yolk
centre and voter ideal points).

```c
int scs_core_2d(
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg, int k,
    int* out_found, double* out_x, double* out_y,
    char* err_buf, int err_buf_len);
```

### C4.5 — `scs_uncovered_set_2d` (discrete)

Wraps `geometry::uncovered_set(alternatives, voter_ideals, cfg, k)`.
Returns the 0-based indices of uncovered alternatives.

```c
int scs_uncovered_set_2d(
    const double* alt_xy, int n_alts,
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg, int k,
    int* out_indices,              // caller buffer, size n_alts (worst case)
    int* out_n,                    // actual count written
    char* err_buf, int err_buf_len);
```

### C4.6 — `scs_uncovered_set_boundary_2d` (continuous polygon)

Wraps `geometry::uncovered_set_boundary_2d(voter_ideals, cfg, grid_resolution, k)`.

```c
int scs_uncovered_set_boundary_2d(
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg,
    int grid_resolution,           // 0 → default (15)
    int k,
    double* out_xy,                // caller buffer, size 2*grid_resolution^2 is safe
    int* out_n,
    char* err_buf, int err_buf_len);
```

Note on buffer sizing: The convex hull of a `grid_resolution²` grid has at most
`grid_resolution²` vertices; a buffer of `2 * grid_resolution² * sizeof(double)`
is always sufficient.

### C4.7 — `scs_heart_2d` (discrete)

Wraps `geometry::heart(alternatives, voter_ideals, cfg, k)`.
Returns 0-based indices of Heart alternatives.

```c
int scs_heart_2d(
    const double* alt_xy, int n_alts,
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg, int k,
    int* out_indices,              // caller buffer, size n_alts
    int* out_n,
    char* err_buf, int err_buf_len);
```

### C4.8 — `scs_heart_boundary_2d` (continuous polygon)

Wraps `geometry::heart_boundary_2d(voter_ideals, cfg, grid_resolution, k)`.

```c
int scs_heart_boundary_2d(
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg,
    int grid_resolution,           // 0 → default (15)
    int k,
    double* out_xy,
    int* out_n,
    char* err_buf, int err_buf_len);
```

---

## Phase C5 — Aggregation: Profile (opaque handle)

The `Profile` struct holds `vector<vector<int>>` rankings and cannot be
flattened without losing structure. It is exposed as an opaque `SCS_Profile*`.

### C5.1 — Opaque type and `SCS_TieBreak`

```c
typedef struct SCS_ProfileImpl SCS_Profile;

typedef enum {
    SCS_TIEBREAK_RANDOM         = 0,
    SCS_TIEBREAK_SMALLEST_INDEX = 1
} SCS_TieBreak;
```

`SCS_TIEBREAK_RANDOM` requires a non-null `SCS_StreamManager*` at call sites
that use tie-breaking. `SCS_TIEBREAK_SMALLEST_INDEX` accepts null.

### C5.2 — `scs_profile_build_spatial`

Wraps `aggregation::build_spatial_profile(alternatives, voter_ideals, cfg)`.

```c
SCS_Profile* scs_profile_build_spatial(
    const double* alt_xy, int n_alts,
    const double* voter_ideals_xy, int n_voters,
    const SCS_DistanceConfig* dist_cfg,
    char* err_buf, int err_buf_len);
```

### C5.3 — `scs_profile_from_utility_matrix`

Wraps `aggregation::profile_from_utility_matrix(utilities)`.
Input: `n_voters × n_alts` utility matrix, row-major (voter i's utility for
alternative j is `utilities[i * n_alts + j]`).

```c
SCS_Profile* scs_profile_from_utility_matrix(
    const double* utilities,       // flat row-major, size n_voters*n_alts
    int n_voters, int n_alts,
    char* err_buf, int err_buf_len);
```

### C5.4 — Synthetic profile generators

All require a `SCS_StreamManager*` (for reproducible randomness).

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

// Copy voter i's ranking into out_ranking (size out_len must be >= n_alts).
// out_ranking[rank] = alternative index (0-based, most preferred first).
int scs_profile_get_ranking(
    const SCS_Profile* p, int voter,
    int* out_ranking, int out_len,
    char* err_buf, int err_buf_len);
```

---

## Phase C6 — Aggregation: voting rules

All positional voting rule functions (Category 3) follow the same pattern.
`*_scores` writes `n_alts` ints. `*_all_winners` writes up to `n_alts` ints.
`*_one_winner` returns a single int and requires a `SCS_TieBreak` + nullable
`SCS_StreamManager*`.

### C6.1 — Plurality

```c
int scs_plurality_scores(
    const SCS_Profile* p,
    int* out_scores, int out_len,    // out_len must be >= n_alts
    char* err_buf, int err_buf_len);

int scs_plurality_all_winners(
    const SCS_Profile* p,
    int* out_winners, int* out_n,    // out_winners buffer size >= n_alts
    char* err_buf, int err_buf_len);

int scs_plurality_one_winner(
    const SCS_Profile* p,
    SCS_TieBreak tie_break,
    SCS_StreamManager* mgr,          // required if tie_break == SCS_TIEBREAK_RANDOM
    const char* stream_name,
    int* out_winner,
    char* err_buf, int err_buf_len);
```

### C6.2 — Borda Count

Same pattern as plurality, plus `scs_borda_ranking` (full social order):

```c
int scs_borda_scores(const SCS_Profile*, int* out, int out_len, char*, int);
int scs_borda_all_winners(const SCS_Profile*, int* out, int* out_n, char*, int);
int scs_borda_one_winner(const SCS_Profile*, SCS_TieBreak, SCS_StreamManager*,
                         const char*, int* out_winner, char*, int);
// out_ranking[i] = alternative index ranked i-th (0-based, best first).
int scs_borda_ranking(const SCS_Profile*, SCS_TieBreak, SCS_StreamManager*,
                      const char*, int* out_ranking, int out_len, char*, int);
```

### C6.3 — Anti-plurality

```c
int scs_antiplurality_scores(const SCS_Profile*, int* out, int out_len, char*, int);
int scs_antiplurality_all_winners(const SCS_Profile*, int* out, int* out_n, char*, int);
int scs_antiplurality_one_winner(const SCS_Profile*, SCS_TieBreak, SCS_StreamManager*,
                                 const char*, int* out_winner, char*, int);
```

### C6.4 — Generic positional scoring rule

```c
int scs_scoring_rule_scores(
    const SCS_Profile* p,
    const double* score_weights,  // length n_alts, non-increasing
    int n_weights,
    int* out_scores, int out_len,
    char* err_buf, int err_buf_len);

int scs_scoring_rule_all_winners(const SCS_Profile*, const double*, int,
                                 int* out, int* out_n, char*, int);
int scs_scoring_rule_one_winner(const SCS_Profile*, const double*, int,
                                SCS_TieBreak, SCS_StreamManager*, const char*,
                                int* out_winner, char*, int);
```

### C6.5 — Approval voting (Category 1 — no `_one_winner`)

Spatial variant (threshold on distance from voter ideal to alternative):

```c
int scs_approval_scores_spatial(
    const SCS_Profile* p,
    const double* alt_xy, int n_alts,
    const double* voter_ideals_xy, int n_voters,
    double threshold,
    const SCS_DistanceConfig* dist_cfg,
    int* out_scores, int out_len,
    char* err_buf, int err_buf_len);

int scs_approval_all_winners_spatial(
    const SCS_Profile* p,
    const double* alt_xy, int n_alts,
    const double* voter_ideals_xy, int n_voters,
    double threshold,
    const SCS_DistanceConfig* dist_cfg,
    int* out_winners, int* out_n,
    char* err_buf, int err_buf_len);
```

Ordinal top-k variant:

```c
int scs_approval_scores_topk(
    const SCS_Profile* p, int k,
    int* out_scores, int out_len,
    char* err_buf, int err_buf_len);

int scs_approval_all_winners_topk(
    const SCS_Profile* p, int k,
    int* out_winners, int* out_n,
    char* err_buf, int err_buf_len);
```

---

## Phase C7 — Aggregation: social rankings and properties

### C7.1 — `scs_rank_by_scores`

Wraps `aggregation::rank_by_scores(scores, tie_break, prng)`.
Input: flat int score array (n_alts). Output: ranking as alt indices, best first.

```c
int scs_rank_by_scores(
    const int* scores, int n_alts,
    SCS_TieBreak tie_break, SCS_StreamManager* mgr, const char* stream_name,
    int* out_ranking, int out_len,
    char* err_buf, int err_buf_len);
```

### C7.2 — `scs_pairwise_ranking_from_matrix`

Wraps `aggregation::pairwise_ranking(pairwise_matrix, tie_break, prng)`.
Input: flat int pairwise matrix (n_alts × n_alts, row-major, values −1/0/1).

```c
int scs_pairwise_ranking_from_matrix(
    const int* pairwise_matrix, int n_alts,
    SCS_TieBreak tie_break, SCS_StreamManager* mgr, const char* stream_name,
    int* out_ranking, int out_len,
    char* err_buf, int err_buf_len);
```

### C7.3 — Pareto efficiency

```c
int scs_pareto_set(
    const SCS_Profile* p,
    int* out_indices, int* out_n,  // out_indices buffer size >= n_alts
    char* err_buf, int err_buf_len);

int scs_is_pareto_efficient(
    const SCS_Profile* p, int alt_idx,
    int* out,                      // 1 if in Pareto set, else 0
    char* err_buf, int err_buf_len);
```

### C7.4 — Condorcet and majority consistency

```c
int scs_has_condorcet_winner_profile(
    const SCS_Profile* p, int* out_found,
    char* err_buf, int err_buf_len);

// out_found=1 and out_winner=0-based index if a Condorcet winner exists.
int scs_condorcet_winner_profile(
    const SCS_Profile* p,
    int* out_found, int* out_winner,
    char* err_buf, int err_buf_len);

// Is alt_idx selected by every reasonable winner rule in every tie-break scenario?
int scs_is_condorcet_consistent(
    const SCS_Profile* p, int alt_idx, int* out,
    char* err_buf, int err_buf_len);

int scs_is_majority_consistent(
    const SCS_Profile* p, int alt_idx, int* out,
    char* err_buf, int err_buf_len);
```

---

## Phase C8 — Tests

Extend `tests/unit/test_c_api.cpp` with tests for all new functions.

**Coverage targets:**

- **C1:** majority prefers (true/false cases), pairwise matrix (3-alt Condorcet
  cycle + Condorcet winner), weighted majority prefers.
- **C2:** winset lifecycle (create, is_empty, to_polygon, destroy), winset
  is_empty for SQ at majority centre vs cycle, all four boolean ops (union ⊇
  each operand; intersection ⊆ each; X∩X = X; X∪X = X; X\X = ∅).
- **C3:** Yolk for equilateral triangle (centre = centroid, radius = √3/6 ±
  tolerance); radius > 0 for cycle configuration.
- **C4:** Copeland scores and winner for Condorcet winner configuration;
  condorcet_winner_2d found/not-found; core_2d found for median voter;
  uncovered_set_2d for Condorcet winner (singleton) and cycle (all three);
  boundary functions return non-empty polygon.
- **C5:** build_spatial_profile dims; get_ranking for known configuration;
  impartial_culture produces well-formed profile; round-trip utility matrix.
- **C6:** scores/all_winners/one_winner for all four Category-3 rules;
  approval spatial and top-k; one_winner with kSmallestIndex is deterministic;
  one_winner with kRandom requires non-null manager.
- **C7:** rank_by_scores correct order; pareto_set non-empty; condorcet winner
  profile found/not-found; is_condorcet_consistent true for Condorcet winner;
  is_majority_consistent true for unanimous preference.
- **Error paths:** null pointer → `SCS_ERROR_INVALID_ARGUMENT`; wrong buffer
  size; `SCS_TIEBREAK_RANDOM` with null manager.

---

## Phase C9 — Documentation

Update `docs/architecture/c_api_design.md`:

- Add sections for each new phase (C1–C7).
- Document `SCS_Winset*` lifecycle contract (who owns, when to destroy, NULL
  return on error).
- Document `SCS_Profile*` lifecycle contract.
- Document `SCS_TieBreak` with note on null manager requirement.
- Add `SCS_Yolk2d` to the POD struct table.
- Extend the mapping table (C type ↔ C++ type) for all new types.
- Add a usage example showing: create voters, build spatial profile, run Borda,
  get Yolk, check Condorcet winner.
- Update stability note: geometry + aggregation extensions are now part of the
  stable surface.

---

## Step checklist

| Step | Description | Status |
|------|-------------|--------|
| C1.1 | `scs_majority_prefers_2d` | 🔲 |
| C1.2 | `scs_pairwise_matrix_2d` | 🔲 |
| C1.3 | `scs_weighted_majority_prefers_2d` | 🔲 |
| C2.1 | `SCS_Winset` opaque type declaration | 🔲 |
| C2.2 | `scs_winset_2d` | 🔲 |
| C2.3 | `scs_weighted_winset_2d` | 🔲 |
| C2.4 | `scs_winset_with_veto_2d` | 🔲 |
| C2.5 | Winset lifecycle + query (`destroy`, `is_empty`, `to_polygon`) | 🔲 |
| C2.6 | Winset boolean ops (`union`, `intersection`, `difference`, `symmetric_difference`) | 🔲 |
| C3.1 | `SCS_Yolk2d` POD struct | 🔲 |
| C3.2 | `scs_yolk_2d` | 🔲 |
| C4.1 | `scs_copeland_scores_2d` | 🔲 |
| C4.2 | `scs_copeland_winner_2d` | 🔲 |
| C4.3 | `scs_has_condorcet_winner_2d` + `scs_condorcet_winner_2d` | 🔲 |
| C4.4 | `scs_core_2d` | 🔲 |
| C4.5 | `scs_uncovered_set_2d` (discrete) | 🔲 |
| C4.6 | `scs_uncovered_set_boundary_2d` (polygon) | 🔲 |
| C4.7 | `scs_heart_2d` (discrete) | 🔲 |
| C4.8 | `scs_heart_boundary_2d` (polygon) | 🔲 |
| C5.1 | `SCS_Profile` opaque type + `SCS_TieBreak` enum | 🔲 |
| C5.2 | `scs_profile_build_spatial` | 🔲 |
| C5.3 | `scs_profile_from_utility_matrix` | 🔲 |
| C5.4 | Synthetic generators (impartial culture, uniform spatial, Gaussian spatial) | 🔲 |
| C5.5 | Profile lifecycle + inspection (`destroy`, `dims`, `get_ranking`) | 🔲 |
| C6.1 | Plurality (`scores`, `all_winners`, `one_winner`) | 🔲 |
| C6.2 | Borda (`scores`, `all_winners`, `one_winner`, `ranking`) | 🔲 |
| C6.3 | Anti-plurality (`scores`, `all_winners`, `one_winner`) | 🔲 |
| C6.4 | Generic scoring rule (`scores`, `all_winners`, `one_winner`) | 🔲 |
| C6.5 | Approval voting (`scores_spatial`, `all_winners_spatial`, `scores_topk`, `all_winners_topk`) | 🔲 |
| C7.1 | `scs_rank_by_scores` | 🔲 |
| C7.2 | `scs_pairwise_ranking_from_matrix` | 🔲 |
| C7.3 | Pareto (`scs_pareto_set`, `scs_is_pareto_efficient`) | 🔲 |
| C7.4 | Condorcet/majority consistency (4 functions) | 🔲 |
| C8   | Tests for C1–C7 | 🔲 |
| C9   | Update `c_api_design.md` | 🔲 |

---

## Implementation order

Recommended order within a session:

1. **C1 + C3** first — simple wrappers, no new types, fastest to verify.
2. **C2** — winset opaque handle (most complex; establishes the pattern for
   opaque types).
3. **C4** — remaining geometry; all return flat arrays or scalars.
4. **C5** — profile opaque handle + `SCS_TieBreak`.
5. **C6** — voting rules (many functions, same pattern; batch them).
6. **C7** — social properties (short functions).
7. **C8** — tests (write as you go per phase, or batch at end).
8. **C9** — documentation last.

Avoid implementing C5–C7 before C2 is done: the winset pattern establishes
how opaque handles are declared, implemented (`struct SCS_*Impl` in
`scs_api.cpp` only), and destroyed, and that pattern is reused for
`SCS_Profile`.
