# Stage 02a — C++ core: core types, RNG, preference
Model / effort: Opus / high   Commit: 3f19579 (source unchanged at HEAD 3a4e908)   Files reviewed: 10, 1,895 lines
Not read: none. All slice files read in full: `core/{types,linalg,kernels,numeric_constants}.h`,
`core/rng/{prng,stream_manager}.h`, `src/core/rng/stream_manager.cpp`,
`preference/distance/distance_functions.h`, `preference/loss/loss_functions.h`,
`preference/indifference/level_set.h`.

Scratch programs: `SCR=/private/tmp/claude-501/-Users-robiragan-Dropbox-SocialChoiceStudioDev/2402f97e-221e-494b-88cb-fc63c41efc50/scratchpad/stage2a/`
(`t1_level_set.cpp`, `t2_rng.cpp`, `t3_loss_distance.cpp`, `t4_ic1d.cpp`, `t5_global.cpp`,
`t6_halfopen.cpp`; output in the matching `t*.out`). Built with Apple clang 21, `-std=c++20 -O3
-DNDEBUG`, `-I include`, Eigen 3.4.0 from `build/_deps/eigen-src` (the version `CMakeLists.txt`
pins). `t5` used `-g -O0 -fsanitize=address`.

## Summary
- **1 P0, 4 P1, 5 P2, 9 P3.** The P0 is in the indifference service:
  `level_set_2d` returns a circle of radius r instead of r/w when the two salience weights
  are equal but not 1. With weights (2, 2), every circular indifference curve
  (`level_set_2d`, `ic_polygon_2d`, the C API and both bindings) is twice the true size
  (S02a-001). The same code is copied into `winset.h` (see Out-of-slice notes).
- **RNG seeding (P1, S02a-002).** `reset_for_run` XORs the master seed with the run index
  before any mixing. Experiments with master seeds 1 and 2 (or 42 and 43) and 100 runs
  produce the **same 100 run seeds**, just in a different order. "Independent" experiments
  with nearby seeds are mostly duplicates.
- **Q2 additional instance (P1, S02a-003).** `level_set_2d` has its own p ≥ 100 →
  Chebyshev-square branch, which Q2's Implies list does not mention. I also did the
  superellipse check that Q2 asked for: sampling gives `inf` vertices at p ≳ 2150, and
  `to_polygon` validates none of its inputs (S02a-009).
- **Loss-inversion round trip (P1, S02a-004; and S02a-007).** `ic_polygon_2d` and
  `scs_ic_interval_1d` convert distance to utility and back. With Gaussian loss, utility
  rounds to its minimum once βd² > ~37. So with default parameters the 2D call **throws**
  for a status quo more than about 6.1 units away, and the 1D call **silently returns an
  empty interval**. Near d = 0, `1 − exp(·)` without `expm1` loses all precision for
  d < 1e-8.
- **Error swallowing (P1, S02a-005).** `level_set_1d` catches every `invalid_argument`
  and returns an empty set. A negative sensitivity, a NaN level or an unknown loss type
  all give "no points" instead of an error.
- Q9: nothing new beyond the unused, mutable global `StreamManager` singleton (S02a-006,
  removal candidate). The distance, loss and level-set functions are stateless and
  reentrant.

## Contracts this layer provides
Stage 3 can rely on these without reading the headers. "Today" means at 3f19579, before
the Q2/Q4 changes are made.

**Core basics**
- `core::types::Point2d/Point3d/PointNd` are `Eigen::Vector2d/Vector3d/VectorXd`. Eigen's
  default constructor leaves them **uninitialised**.
- `EpecKernel` / `EpicKernel` are the CGAL EPEC / EPIC kernels.
- Tolerance constants: `near_zero::k_near_zero_rel = 1e-9`, `k_near_zero_abs = 1e-12`.
  They are applied PEP 485-style, with a scale computed at the call site.

**PRNG (`core::rng::PRNG`)**
- Engine: `std::mt19937_64`, seeded directly with the given 64-bit seed. The default seed
  is `k_default_master_seed = 12345`.
- Engine output is fixed by the standard. **Draws are not** today, because they go through
  `std::*_distribution`, which differs between libc++ and libstdc++ (Q4 will change this).
- Each method throws `std::invalid_argument` on bad parameters:
  - `uniform_int(min, max)` when min > max (the result is inclusive).
  - `uniform_real(min, max)` when either bound is non-finite or min ≥ max. The nominal
    range is [min, max), but the result **can equal max** (S02a-011) and is **inf** if
    max − min overflows (S02a-008).
  - `normal(mean, sd)` when either is non-finite or sd ≤ 0.
  - `bernoulli(p)` unless p is finite and in [0, 1].
  - `uniform_choice(n)` when n = 0; otherwise it returns a value in [0, n).
  - `exponential`, `gamma`, `beta` and `discrete_choice` validate their inputs too. They
    have no library callers, and `beta` can also throw `std::domain_error` (S02a-012).
- `skip(n)` discards **n raw engine outputs**, not n draws.
- `PRNG` is copyable (a copy duplicates the stream). It is not thread-safe.
- `engine()` is public (S02a-013).

**StreamManager**
- Stream seed = SplitMix64 finalizer(master ^ (h<<1) ^ (h>>1)), where h is FNV-1a-64 over
  the bytes of the name. This is portable, and the map from h is injective.
- Streams are created lazily on `get_stream`. A stream's seed depends only on (current
  master, name), not on the order streams are created.
- `reset_stream(name, seed)` seeds that stream with the raw `seed`, with no hashing.
- `reset_all(m)` clears all streams and keeps the allowlist.
- `reset_for_run(m, r)` is `reset_all(combine_seed(m, r))`, after which `master_seed()`
  returns the run seed. Its weakness is S02a-002.
- With a non-empty allowlist, `get_stream` and `reset_stream` throw `invalid_argument`
  for unknown names. `has_stream`, `remove_stream` and `stream_names` ignore the allowlist.
- `stream_names()` order is unspecified (it comes from an `unordered_map`).
- The `PRNG&` returned by `get_stream` is invalidated by `reset_all`, `reset_for_run`,
  `clear`, `remove_stream` and destruction.
- Single owner (Q9): no concurrent use of one manager, including `get_stream`, which
  mutates.
- Moving a manager keeps references valid (the PRNGs live on the heap), but this is not
  documented.
- A process-wide mutable singleton exists (`get_global_stream_manager`, `*_rng()`). It is
  unused (S02a-006).

**Distance (`preference::distance`)**
- Convention B: d = (Σ (wᵢ|xᵢ−yᵢ|)^p)^(1/p).
- Every call throws `std::invalid_argument` if:
  - the two points have different sizes;
  - the weights vector is empty or its size ≠ N (so N = 0 always throws);
  - any coordinate or weight is non-finite;
  - any weight is negative.
- A weight of 0 is allowed and masks that dimension.
- `order_p` is validated (finite, ≥ 1) **only for MINKOWSKI**. `calculate_distance` with
  EUCLIDEAN, MANHATTAN or CHEBYSHEV ignores it, even if it is NaN.
- Fast paths: |p − 1| < 1e-9 is computed as exact L1, and |p − 2| < 1e-9 as exact L2.
- Today, p ≥ 100 returns Chebyshev (Q2 removes this).
- Today's code is unscaled: for p < 100, the result overflows to `inf` or underflows to 0
  for moderate magnitudes (at p = 99, differences of 1e4 give inf and 1e-4 give 0; covered
  by Q2). The L2 path overflows above ~1e154.
- Stateless, deterministic, thread-safe.

**Loss (`preference::loss`)**
- Each loss returns L ≥ 0; `distance_to_utility` returns −L.
- Inputs: the distance must be finite; parameter domains are checked per type (positive
  sensitivity, max_loss and steepness; threshold ≥ 0), each violation throwing
  `invalid_argument`.
- Parameters a given loss type does not use are ignored and **not validated**.
- A negative distance is accepted (|d| for linear and threshold, d² for quadratic and
  Gaussian).
- The output can be −inf for finite inputs (quadratic with d > ~1e154).
- Gaussian utility rounds to exactly −max_loss once βd² > ~37.4, and to −0 for d < ~1e-8
  (S02a-007).
- `normalize_utility` and `UtilityNormalizer` return 1.0 when the utility range is
  negligible. They do not clamp, so inconsistent inputs give results outside [0, 1].
- Only floating-point T compiles (S02a-017).

**Level sets (`preference::indifference`)**
- Utility → distance inversion: a level above 0 is unreachable for every loss. Gaussian
  levels ≤ −max_loss are unreachable too.
- `level_set_2d` throws `invalid_argument` on an unreachable level or a bad config.
- `level_set_1d` returns `{}` on an unreachable level **and on any bad loss config**
  (S02a-005). It returns 1 point when r = 0, otherwise 2 points in ascending order.
- 2D needs exactly 2 weights, **both > 0**. This is stricter than distance, which allows
  w = 0.
- Shapes returned by `level_set_2d`:

  | Distance | Shape |
  |---|---|
  | Euclidean, weights equal within 1e-9 relative | `Circle2d` — radius **wrong when w ≠ 1** (S02a-001) |
  | Euclidean, otherwise | `Ellipse2d(r/w0, r/w1)` |
  | Manhattan | 4-vertex polygon, CCW, vertices E, N, W, S |
  | Chebyshev | 4-vertex polygon, CCW, vertices NE, NW, SW, SE |
  | Minkowski, p ≤ 1 + 1e-9 | Manhattan diamond |
  | Minkowski, p ≥ 100 | Chebyshev square (S02a-003) |
  | Minkowski, otherwise | `Superellipse2d(r/w0, r/w1, p)` |

- r = 0 gives a degenerate shape: a radius-0 circle, or 4 coincident vertices.
- `to_polygon(ls, n = 128)`:
  - Circle, ellipse and superellipse are sampled at n points at parameter angles
    2πi/n, CCW from angle 0, with all vertices on the curve (an inscribed polygon) and the
    first vertex not repeated.
  - A `Polygon2d` is returned unchanged and n is ignored.
  - No validation: n = 0 gives an empty polygon, and any exponent or semi-axis is accepted
    (S02a-009).
- `ic_polygon_2d(ideal, sq, …, n = 64)` throws if n < 3 or the weights size ≠ 2. It goes
  distance → utility → level set (S02a-004).
- Everything here is stateless and thread-safe. Errors are `std::invalid_argument` only;
  `std::bad_alloc` is possible for huge n.

## Findings

### S02a-001 [P0, confirmed] Equal non-unit salience weights give a circle of the wrong radius
- **Where:** include/socialchoicelab/preference/indifference/level_set.h:231–239 (line 237 `return Circle2d{ideal_point, r};`)
- **What:** r is the *weighted* distance. For weights w0 = w1 = w the Euclidean IC is the
  circle of radius r/w, and `a0 = r/w0` is already computed on line 231. The circle
  branch returns radius r instead. The ellipse branch is correct. `equal_weights_2d` is
  true for any equal pair, not only (1, 1).
- **Failure scenario:** Euclidean distance, weights (2, 2), quadratic loss, ideal (0,0),
  sq (1,0). The weighted distance is 2. `level_set_2d` returns a `Circle2d` of radius
  2.0, but the true IC has radius 1.0. `ic_polygon_2d` puts vertex[0] at (2,0) instead of
  through the sq at (1,0); its vertices sit 2.0 distance units off the IC. With weights
  (2, 2+1e-6), the ellipse branch gives vertex (1,0) and error 0. (`SCR/t1_level_set.cpp`,
  section A; `t1.out`.) Weights (0.5, 0.5) shrink the curve by half instead.
- **Suggested fix:** Use `Circle2d{ideal_point, a0}`. Better: add one internal
  "shape at weighted distance r" builder, used by `level_set_2d`, `ic_polygon_2d` and
  `geometry::detail::pts_polygon`, so the logic exists once. Add tests with weights (2, 2)
  and (0.5, 0.5) that check vertex distances equal r.
- **Also affects:** C API `scs_level_set_2d`, `scs_level_set_polygon_2d`,
  `scs_ic_polygon_2d`. R/Python `level_set_2d`, `level_set_polygon_2d`, `ic_polygon_2d`,
  IC layers in `plots.py` and `competition_canvas.R:606`. The duplicated copy in
  `geometry/winset.h:130–131` (see Out-of-slice notes).

### S02a-002 [P1, confirmed] `reset_for_run` gives the same run seeds for nearby master seeds
- **Where:** include/socialchoicelab/core/rng/stream_manager.h:261–269 (`combine_seed`); doc comment :254–255
- **What:** `h = master ^ run_index` comes first, before any mixing. So
  combine_seed(m1, r1) = combine_seed(m2, r2) whenever m1 ^ r1 = m2 ^ r2 and the two
  indices agree above bit 17. For masters that differ only in low bits, the sets
  {m ^ r : r < N} largely or entirely coincide. The comment calls this "FNV-1a mixing",
  but it is a word-level XOR-multiply, not FNV-1a.
- **Failure scenario:** `run_seed(0,1) == run_seed(1,0)`. Run seeds shared between two
  experiments:

  | Master seeds | Runs | Shared |
  |---|---|---|
  | 1 vs 2 | 100 | 100/100 |
  | 42 vs 43 | 100 | 100/100 |
  | 2026 vs 2027 | 1000 | 1000/1000 |
  | 100 vs 200 | 1000 | 976/1000 |
  | 1 vs 2 | 10 | 8/10 |
  | 12345 vs 54321 | 100 | 0/100 |

  Master 1, run 0 and master 2, run 3 give the same first draw (0.074642463838656761).
  A user who checks robustness by re-running `run_competition_experiment` with seeds 1
  and 2 gets the same set of runs, so identical summary statistics, and may conclude the
  result is stable. (`SCR/t2_rng.cpp` section A; `t2.out`.)
- **Suggested fix:** Mix the master before combining it with the run index, e.g.
  `F(F(master) + (run_index + 1) * 0x9E3779B97F4A7C15)` with F the SplitMix64 finalizer,
  or FNV-1a over the 16 bytes of (master, run) followed by F. Fix the comment. Add a test
  that masters m, m+1 and m^d with N runs give disjoint run seeds. This changes seeded
  outputs, so do it together with the Q4 regeneration and the RNG algorithm-version bump.
- **Also affects:** `competition/experiment_runner.h` (`run_seed_for_index`, reported
  `run_seed`); `scs_competition_run_experiment`; R/Python `CompetitionExperiment`; the
  website if it runs experiments.

### S02a-003 [P1, confirmed] `level_set_2d` has its own p ≥ 100 → Chebyshev-square branch
- **Where:** include/socialchoicelab/preference/indifference/level_set.h:276–284. Decision: Q2
- **What:** Q2 requires exact Minkowski at every finite p, with ICs, winsets and distances
  using the same metric. The Implies list covers `distance_functions.h` and
  `winset.h:163–169`, but not this branch, which returns the L∞ square for any p ≥ 100.
  Once `distance_functions.h` drops its cutoff, ICs from `level_set_2d`, `ic_polygon_2d`
  and the C API will disagree with the distances that define them.
- **Failure scenario:** Minkowski p = 150, weights (1,1), linear loss, level −1. The
  result is a 4-vertex `Polygon2d` with a corner at (1,1). The exact p = 150 distance of
  (1,1) is 2^(1/150) = 1.004632, so the curve is 0.46% off the true IC at its corners.
  (`t1.out` section B.)
- **Suggested fix:** Remove the branch and return `Superellipse2d` for every finite
  p > 1 + 1e-9, with the sampling fixed as in S02a-009. Keep a single source for the p ≈ 1
  tolerance, which today is `<` 1e-9 in distance and `<=` 1 + 1e-9 here.
- **Also affects:** `scs_level_set_2d` (output type changes from POLYGON to SUPERELLIPSE
  for p ≥ 100), R/Python level-set and IC functions, tests that assume a square.

### S02a-004 [P1, confirmed] IC through a point round-trips through utility: Gaussian throws (2D) or returns nothing (1D)
- **Where:** include/socialchoicelab/preference/indifference/level_set.h:403–407 (`ic_polygon_2d`), :117–138 (Gaussian inversion); src/c_api/scs_api.cpp:753–791 (`scs_ic_interval_1d`, same sequence through `level_set_1d`)
- **What:** The IC through sq is the iso-distance curve at d = dist(ideal, sq). The loss is
  monotone, so no loss inversion is needed. Instead, the code computes u = −L(d) and
  inverts it. For Gaussian loss, L = α(1 − e^(−βd²)) rounds to exactly α once βd² > ~37.4.
  The level then equals −max_loss and is rejected as unreachable. Before that point, the
  recovered radius loses precision (cancellation in `1 + level/α` and `log`).
- **Failure scenario:** Gaussian, defaults α = β = 1, ideal (0,0):
  - sq (6.2, 0) or (7, 0): `ic_polygon_2d` throws "utility level at or below minimum for
    Gaussian".
  - sq (6, 0): radius 6.00364 (relative error 6e-4).
  - sq (5, 0): relative error 7.6e-8.
  - sq (1e-9, 0): radius 0.
  - 1D, the `scs_ic_interval_1d` sequence: reference 7 → 0 points and no error.
  (`t1.out` section D; `SCR/t4_ic1d.cpp`, `t4.out`.) This is reachable from R/Python
  `ic_polygon_2d` and from plot IC layers given a Gaussian `ic_loss_config`.
- **Suggested fix:** Build the IC straight from d with the shared shape builder from
  S02a-001, skipping the loss. Keep utility inversion only for the "level" entry points
  (`level_set_1d/2d`), using `log1p` (S02a-007). Add tests for Gaussian with
  βd² ∈ {1, 30, 40}.
- **Also affects:** `scs_ic_polygon_2d`, `scs_ic_interval_1d`, R/Python wrappers. For
  Threshold loss inside the zone, see Open question 1.

### S02a-005 [P1, confirmed] `level_set_1d` swallows invalid-argument errors and returns an empty set
- **Where:** include/socialchoicelab/preference/indifference/level_set.h:189–194
- **What:** `catch (const std::invalid_argument&) { return {}; }` wraps
  `distance_from_utility_level`. That function throws both for "level unreachable", which
  should give an empty set, and for invalid configuration, which should throw (the
  indifference_design.md "Errors" row says invalid config throws). `level_set_2d` lets the
  same errors through, so 1D and 2D behave differently.
- **Failure scenario:** Inputs that return `{}` with no exception: LINEAR with
  sensitivity −1 (`level_set_2d` on the same input throws "LossConfig sensitivity must be
  positive and finite"); level = NaN; `loss_type = 42` (`t1.out` section E). Via
  `scs_level_set_1d`, which calls `to_cpp_loss_config` without validating it, a negative
  sensitivity returns `SCS_OK` with `out_n = 0`.
- **Suggested fix:** Make unreachability a return value of the inversion (e.g.
  `std::optional<double>`) rather than an exception. Validate the config separately and
  let config errors propagate. Test both cases.
- **Also affects:** `scs_level_set_1d`, `scs_ic_interval_1d`, R/Python `level_set_1d` /
  `ic_interval_1d`.

### S02a-006 [P2, confirmed] Unused mutable global `StreamManager` singleton
- **Where:** include/socialchoicelab/core/rng/stream_manager.h:307–331; src/core/rng/stream_manager.cpp:11–51
- **What:** `get_global_stream_manager()`, `set_global_stream_manager_seed()` and
  `voters_rng()`…`analysis_rng()` give the process one shared, mutable RNG state. They
  have no callers outside their own files (Stage 1) and are not in the C API. The mutex
  guards construction only, so the API looks safe for threads but is not, which is the
  "false safety" `stream_manager_design.md` C2 rejected. Q9 asks for no shared mutable
  state between users. `set_global_stream_manager_seed` also invalidates any `PRNG&` that
  `*_rng()` returned, and its doc does not say so.
- **Failure scenario:** `PRNG& v = voters_rng(); set_global_stream_manager_seed(99);
  v.uniform_real(0.0, 1.0);` gives an ASan heap-use-after-free in `PRNG::uniform_real`
  (`SCR/t5_global.cpp`, `t5.out`). Two threads that use `voters_rng()` (the header
  describes it as "safe for single-threaded use", but nothing enforces that) race on one
  `unordered_map`.
- **Suggested fix:** Delete the singleton, the `*_rng()` helpers, `stream_manager.cpp`,
  and R2 / C2 in the design doc. The project is pre-open, so no deprecation is needed.
- **Also affects:** `stream_manager_design.md` R2, C2, P-sections; CMake source list.

### S02a-007 [P2, confirmed] Gaussian loss and its inverse lose precision near d = 0
- **Where:** include/socialchoicelab/preference/loss/loss_functions.h:121; include/socialchoicelab/preference/indifference/level_set.h:133–137
- **What:** `1 − exp(−βd²)` cancels catastrophically for small βd², and the inverse
  `−log(1 + level/α)` cancels the same way. `-expm1(-x)` and `-log1p(level/α)` are exact
  to rounding.
- **Failure scenario:** α = β = 1:
  - The loss is 0 for d = 1e-9 and d = 2e-9; the true values are 1e-18 and 4e-18.
  - `utility(1e-9) == utility(5e-9) == -0`, so two alternatives at different distances
    tie.
  - At d = 1e-8 the loss is 1.11e-16 against a true 1.0e-16 (11% error).
  - At d = 1e-5 the relative error is 8e-8.
  (`SCR/t3_loss_distance.cpp` section A, `t3.out`.) A 2D IC through a point 1e-9 away
  has radius 0 (`t1.out` D).
- **Suggested fix:** Use `std::expm1` in `gaussian_loss` and `std::log1p` in the
  inversion. Add tests at d ∈ {1e-9, 1e-5}. Document that Gaussian utility saturates in
  double precision at βd² ≈ 37.
- **Also affects:** `scs_distance_to_utility`, `scs_level_set_*` with Gaussian loss.

### S02a-008 [P2, confirmed] `uniform_real` returns inf when max − min overflows
- **Where:** include/socialchoicelab/core/rng/prng.h:80–95. Decision: Q4
- **What:** It checks that both bounds are finite and min < max, but not that max − min is
  finite. The standard distribution computes min + u·(max − min), which is inf.
- **Failure scenario:** `uniform_real(-DBL_MAX, DBL_MAX)` returned `inf` in 1000/1000
  draws (`t2.out` B). This is reachable through `scs_uniform_real`, R/Python
  `StreamManager.uniform_real`, and uniform voter sampling (`voter_sampler.h:90`) with
  wide bounds.
- **Suggested fix:** When writing the project-owned uniform-real algorithm (Q4), either
  reject ranges whose width is not finite, with an end-user message, or compute
  `min*(1-u) + max*u`. Add a test.
- **Also affects:** C API, R, Python draw functions.

### S02a-009 [P2, confirmed] Superellipse sampling gives inf at large p; `to_polygon` validates nothing
- **Where:** include/socialchoicelab/preference/indifference/level_set.h:328–343 (337–338), :296–348. Decision: Q2 (the check requested in Implies)
- **What:** r = (|c|^p + |s|^p)^(−1/p). At t = π/4 both terms equal 2^(−p/2), which falls
  to 0 once p > ~2148, so r = pow(0, −1/p) = inf. Today `level_set_2d` caps p at 100, so
  this only shows up after the Q2 fix, but `to_polygon` is reachable directly:
  `scs_level_set_to_polygon` rebuilds a `Superellipse2d` from a caller-supplied
  `SCS_LevelSet2d` and checks only that its fields are finite. `to_polygon` also accepts
  exponent < 1 (or 0), negative semi-axes, negative radius, and `num_samples` 0–2.
- **Failure scenario:**
  - p = 2000: all vertices finite.
  - p = 2200: 4 of 64 vertices are (inf, inf).
  - p = 5000: 20 of 64 are inf.
  - p = 0: vertex[1] = (0,0).
  - Semi-axis −1: accepted.
  (`t1.out` C.) Infinite vertices then go into `to_exact` and CGAL in winset code after
  Q2.
- **Suggested fix:** Scale by the larger term: m = max(|c|, |s|);
  r = (1/m)·(1 + (min/m)^p)^(−1/p). This never underflows, and its limit is the square.
  Validate p ≥ 1 and finite, semi-axes and radius ≥ 0, and num_samples ≥ 3 in
  `to_polygon` itself, not only in the C API. Add tests at p ∈ {50, 99, 100, 1000, 1e4}.
- **Also affects:** `scs_level_set_to_polygon`, winset sampling after Q2.

### S02a-010 [P2, confirmed] Most `level_set.h` branches have no C++ tests (65% line coverage)
- **Where:** tests/unit/test_level_set.cpp (10 tests, 189 lines) vs include/socialchoicelab/preference/indifference/level_set.h
- **What:** No C++ test covers:
  - LINEAR, GAUSSIAN or THRESHOLD inversion in 2D;
  - the Gaussian lower-bound path;
  - Minkowski p ≤ 1 + 1e-9 and p ≥ 100;
  - superellipse sampling (only the variant type is checked);
  - Chebyshev and Manhattan vertex values beyond the vertex count;
  - `ic_polygon_2d` (only through test_c_api);
  - equal non-unit weights;
  - r = 0 in 2D;
  - `to_polygon` argument edge cases.
  S02a-001, -003, -004, -005 and -009 all sit in these gaps.
- **Failure scenario:** The radius bug in S02a-001 passes the suite, because
  `EuclideanEqualWeightsReturnsCircle` uses weights (1, 1).
- **Suggested fix:** For each shape and loss type, add tests that check every sampled
  vertex lies at weighted distance r (relative tolerance), plus the edge cases above
  (Stage 4).
- **Also affects:** none.

### S02a-011 [P3, confirmed] `uniform_real` documents [min, max) but can return max
- **Where:** include/socialchoicelab/core/rng/prng.h:73–79; include/scs_api.h:516. Decision: Q4
- **What:** min + u·(max − min) rounds up to max for narrow ranges. libc++'s
  `generate_canonical` also returns exactly 1.0 when the 64-bit engine output rounds up
  to 2^64 (probability 2^-54 per double draw, much higher for `float`).
- **Failure scenario:**
  - `uniform_real(1, nextafter(1,2))` returned max in 4988 of 10,000 draws.
  - `uniform_real(1e6, 1e6+1e-9)` returned max in 5700 of 100,000.
  - A stub engine at its maximum makes `std::uniform_real_distribution<double>(0,1)`
    return 1.0.
  (`SCR/t6_halfopen.cpp`, `t6.out`; `t2.out` C.)
- **Suggested fix:** In the Q4 algorithm, build u from 53 bits, `(x >> 11) * 0x1p-53`,
  and either clamp the result below max or document "[min, max] for ranges narrower than
  a few ulps".
- **Also affects:** C API, R, Python docs for `uniform_real`.

### S02a-012 [P3, confirmed] Unused distributions (`exponential`, `gamma`, `beta`, `discrete_choice`) have edge-case defects; remove rather than port
- **Where:** include/socialchoicelab/core/rng/prng.h:121–262. Decision: Q4
- **What:** No library or C API callers; they are used only in test_prng. Q4 asks for every
  draw to be replaced with a cited project-owned algorithm, and these four would add work
  for no user.
- **Failure scenario:**
  - `beta(1e-3, 1e-3)` throws `std::domain_error` (both gamma draws underflow to 0) in
    219 of 1000 draws for valid input, and returns exactly 0 or 1 in 639.
  - `discrete_choice({DBL_MAX, DBL_MAX, 1})` passes validation; the weight sum overflows
    and index 2 is chosen 1000 of 1000 times.
  (`t2.out` D, E.)
  - `uniform_int<signed char>` compiles, which is undefined per [rand.req.genl].
- **Suggested fix:** Delete the four methods and their tests. Restrict `uniform_int` to
  the standard integer types.
- **Also affects:** test_prng.cpp; Q4's beta/gamma guarantee wording.

### S02a-013 [P3, confirmed] `engine()` is public, `draw_count_` is not observable, and `PRNG` is copyable
- **Where:** include/socialchoicelab/core/rng/prng.h:293–309, :26–36. Decision: Q4
- **What:**
  - The Stage 1 hotspot "direct `engine()` calls bypass `draw_count_`" has no
    functional effect. `draw_count_` is read only by `state_string()` / `debug_info()`,
    which have no library callers, and `skip()` and `beta()` also keep it wrong.
  - The real problem is that the Q4 rule ("no direct `engine()` use outside `PRNG`") is
    enforced only by a comment, because `engine()` is public.
  - `PRNG` has an implicit copy constructor. `auto p = sm.get_stream("x");`, with the `&`
    left out, silently forks the stream, so later users of "x" repeat its numbers. I
    found no such copies today.
- **Failure scenario:** 5 draws through `engine()` leave `state_string()` reporting
  "draws=0" (`t2.out` F).
- **Suggested fix:** After moving callers to `PRNG` methods (Q4), make `engine()` private
  or delete it. Delete the copy constructor and copy assignment. Either expose
  `draw_count()` or drop `draw_count_`.
- **Also affects:** tie_break, borda, social_ranking, profile_generators, step_policy,
  initialization (see Out-of-slice notes).

### S02a-014 [P3, confirmed] `skip(n)` discards raw engine outputs, not draws; the docs don't say so
- **Where:** include/socialchoicelab/core/rng/prng.h:277–281; include/scs_api.h:504–509; r/R/stream_manager.R:74–76; python/src/socialchoicelab/_stream_manager.py:134
- **What:** "Skip (discard) n values" reads as "skip n draws". In fact one `normal` uses
  at least 2 engine outputs, and `uniform_int` uses a variable number (rejection).
- **Failure scenario:** A user calls `skip(stream, 10)` to jump past 10 earlier `normal()`
  draws. The stream ends up somewhere other than where 10 normal draws would leave it.
- **Suggested fix:** Document it as "n raw 64-bit engine outputs". After Q4 fixes the
  outputs consumed per draw, either document that count or drop `skip`.
- **Also affects:** R, Python docs.

### S02a-015 [P3, confirmed] The p = 2 fast path is the one unscaled Minkowski path left after Q2
- **Where:** include/socialchoicelab/preference/distance/distance_functions.h:154–164. Decision: Q2
- **What:** sqrt(Σ(wd)²) overflows for |wd| ≳ 1.3e154 and underflows to 0 for
  |wd| ≲ 1e-162. Q2's scaled formula is planned for the general path (lines 172–178); if
  it is applied only there, Euclidean (the default metric) stays unscaled.
- **Failure scenario:** Difference (1e154, 1e154) gives inf (expected 1.41e154);
  difference (1e-170, 1e-170) gives 0 (`t3.out` D). These are extreme magnitudes, hence
  P3.
- **Suggested fix:** Apply the same max-term scaling, or `std::hypot` for N = 2, in the
  p = 2 path, so that every p is overflow-safe.
- **Also affects:** none.

### S02a-016 [P3, confirmed] Core error messages omit the value entered
- **Where:** include/socialchoicelab/preference/distance/distance_functions.h:49–80, :132, :137; include/socialchoicelab/preference/loss/loss_functions.h:47–155; include/socialchoicelab/preference/indifference/level_set.h:94–157, :183–229
- **What:** The C API passes `e.what()` unchanged to R and Python users, but the messages
  rarely state the value entered or how to fix it, contrary to the end-user error-message
  rule.
- **Failure scenario:** order_p = 0.5 gives "Minkowski order_p must be >= 1". Sizes 2 vs 3
  give "Ideal point and alternative point must have same dimension". Sensitivity −3 gives
  "linear_loss: sensitivity must be positive". (`t3.out` F.)
- **Suggested fix:** Include the received value or sizes and the valid range, e.g.
  "order_p must be finite and >= 1 (got 0.5); use 1 for Manhattan or CHEBYSHEV for L∞."
- **Also affects:** every C API function that forwards these messages.

### S02a-017 [P3, confirmed] Inaccurate comments in `loss_functions.h`
- **Where:** include/socialchoicelab/preference/loss/loss_functions.h:21, :225–226, :246, :282–285, :330–331, :348–349
- **What:** The enum comment gives the Gaussian *utility* as "u = α(1 − e^(−βd²))"; that
  is the loss, and u is its negative. `normalize_utility` and `UtilityNormalizer`
  document and implement an "integer T → 0 or 1" path, but integer T cannot compile,
  because every loss function `static_assert`s floating point.
- **Failure scenario:** `normalize_utility<int>(…)` fails to compile despite the
  documented behaviour. A reader who takes the enum comment's sign gets utilities with the
  wrong sign.
- **Suggested fix:** Correct the sign, and delete the integer branches and their docs.
- **Also affects:** none.

### S02a-018 [P3, plausible] `distance_to_utility` / `normalize_utility` take four adjacent defaulted `T` parameters
- **Where:** include/socialchoicelab/preference/loss/loss_functions.h:183–186, :249–252, :316–318 (clang-tidy `easily-swappable-parameters` :184, :185, :249)
- **What:** The parameters are `(sensitivity, max_loss, steepness, threshold)`, all `T`
  and all defaulted, so a swapped argument compiles silently. `indifference::LossConfig`
  already bundles them, and `ic_polygon_2d` unpacks it field by field (level_set.h:403–405).
- **Failure scenario:** `distance_to_utility(d, THRESHOLD, 0.5)`, intending threshold 0.5,
  sets sensitivity = 0.5 and threshold = the default 0.5. The result is plausible-looking
  but wrong.
- **Suggested fix:** Move `LossConfig` into `preference/loss` and take it as the single
  parameter.
- **Also affects:** C API (already struct-based), callers in level_set.h.

### S02a-019 [P3, confirmed] `level_set.h` docs and validation order
- **Where:** include/socialchoicelab/preference/indifference/level_set.h:122–132, :174–177, :212–215
- **What:**
  - `level_set_2d`'s doc does not say that it throws on an unreachable level, that it
    returns a degenerate shape when r = 0, or that it needs weights > 0 (distance allows
    0). indifference_design.md says this should be documented.
  - The Gaussian branch compares the level with `-cfg.max_loss` before checking that
    max_loss is positive, so a bad max_loss gets the wrong message.
- **Failure scenario:** Gaussian with max_loss = −1, level −0.5 gives "utility level at or
  below minimum for Gaussian" instead of "max_loss must be positive" (`t1.out` F).
- **Suggested fix:** Validate parameters first, then check reachability. Document the
  throws, the degenerate shape and the weight rule in the header.
- **Also affects:** none.

## Tool leads checked
- clang-tidy `bugprone-signed-bitwise` stream_manager.h:244, :246, :248, :266, :298 (×2) →
  false positive: unsigned `uint64_t` shifted by `int` literals, well defined.
- clang-tidy `easily-swappable-parameters` stream_manager.h:261 (`combine_seed`) → false
  positive as a swap hazard (private, one caller). The function is still defective; see
  S02a-002.
- clang-tidy `easily-swappable-parameters` loss_functions.h:184, :185, :249 → S02a-018.
- `-Wsign-conversion` distance_functions.h (6 sites: :72, :77, :149, :160, :175, :250) →
  false positive: a non-negative `Eigen::Index` indexes a `std::vector`.
- `-Wsign-conversion` stream_manager.h:292 → false positive: the `char` → `unsigned char`
  conversion is intended for FNV-1a over bytes and portable.
- `-Wunused-result` test_prng.cpp (29), test_distance_functions.cpp (22),
  test_loss_functions.cpp (14) → false positive: `[[nodiscard]]` calls inside
  `EXPECT_THROW`.
- clang-analyzer `EnumCastOutOfRange` test_distance_functions.cpp:155,
  test_loss_functions.cpp:259 → false positive: intentional invalid-enum tests.
- clang-analyzer `DeadStores` / `-Wunused-variable` test_loss_functions.cpp:344–345 →
  test hygiene, not a library defect; Stage 4.
- Coverage `level_set.h` 65.4% → S02a-010 (and the defects in S02a-001/-003/-004/-005/-009
  sit in the gaps).
- Coverage `stream_manager.h` 90% lines / 81.8% functions → the uncovered functions are
  the global helpers → S02a-006. The rest is correct and tested.
- Coverage `prng.h` 95.6% / 87.5% functions → the uncovered pieces (`const engine()`,
  rarely hit throw paths) are correct but untested; S02a-012 and -013 propose removals.
- Coverage `loss_functions.h` 90.5%, `distance_functions.h` 95.0% → correct but untested
  (mostly non-finite-parameter throws and unknown-enum defaults); Stage 4.
- S00-001 (p = 100 accuracy) → covered by Q2. Today p = 100 returns Chebyshev
  (`t3.out` E: 1 vs exact 1.0069555500567189), so the test is the thing to fix. No new
  finding.
- Stage 1 hotspot "direct `engine()` bypasses `draw_count_`" → S02a-013 (no functional
  effect; portability is Q4).
- Lead "superellipse at very large p (`level_set.h:335–337`)" → S02a-009.
- Lead "loss near zero distance / extreme parameters" → S02a-007 (Gaussian near 0),
  S02a-004 (Gaussian saturation). Quadratic overflow to −inf above d ≈ 1e154 is noted in
  Contracts; no finding (extreme input, and `normalize_utility` happens to return the
  limit value 1).

## Open questions
1. **Threshold loss, IC through a point inside the indifference zone.**
   - What happens: with threshold τ = 0.5 and sq 0.2 from the ideal, u(sq) = 0 and the
     returned IC is the zone boundary (radius 0.5, or ±0.5 in 1D; `t4.out`), not a curve
     through sq.
   - Why it is unclear: the true level set {u = 0} is the whole disc.
   - Options: keep the zone boundary (it is the boundary of the "at least as good" set),
     return the iso-distance curve through sq, or flag the case. Whichever is chosen
     should be documented. Winset and preferred-to-set code build from distances, so they
     are not affected.
2. **Zero salience weights.** Distance allows w = 0 (dimension masking), but
   `level_set_1d/2d` reject it, because the IC would be an unbounded strip or a pair of
   lines. Is masking meant to work with ICs and winsets? If not, should distance configs
   used for 2D geometry reject zero weights at the C boundary with a clear message?
3. **`uniform_real` range contract after Q4.** Should the guarantee be strictly
   [min, max) (clamping), or [min, max] with a documented narrow-range exception
   (S02a-011)?
4. **Gaussian loss sources.** I could not check the exact parameterisation of the
   Gaussian loss against Conevska & Mutlu (2025, arXiv 2501.03196, in
   `references/social_choice/loss_functions/`): the extracted text names the "reverse
   S-shaped … Gaussian" form and the NOMINATE Gaussian, but gives no formula I could read.
   Singh (2013, same folder) supports the linear-vs-quadratic claim. Neither paper is
   listed in `REFERENCE_INDEX.md`. Threshold loss has no citation.

## Out-of-slice notes
- `geometry/winset.h:108–181` (`detail::pts_polygon`) duplicates `level_set_2d`. It has the
  same equal-weights circle radius bug (`Circle2d{voter_ideal, r}` at :130–131). If that
  path is used, Euclidean winsets with equal non-unit weights are scaled by w; not run.
  It uses a different equal-weight tolerance (1e-10 vs 1e-9). Stage 2b.
- `competition/initialization.h:76` and `:91` call `prng.engine()` directly. Q4's Implies
  list names only `initialization.h:55–67`. Stage 2d.
- `src/c_api/scs_api.cpp:962–969` (`scs_level_set_to_polygon`, POLYGON case): the vector
  is resized to `n_vertices`, but only 4 vertices are filled. For n_vertices > 4 the extra
  vertices are uninitialised Eigen vectors and are copied to the output. A negative
  `n_vertices` becomes a huge `size_t` resize. Stage 3.
- `src/c_api/scs_api.cpp` `to_cpp_dist` maps an unknown `SCS_DistanceType` to EUCLIDEAN
  without an error, and `to_cpp_loss_config` does not validate. Stage 3.
- `src/c_api/scs_api.cpp:3603–3620` `scs_uniform_real` forwards ranges whose width
  overflows (S02a-008). Stage 3.
