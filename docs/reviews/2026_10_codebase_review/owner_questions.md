# Owner questions

Design questions raised by review agents. The owner's answers define intended behaviour:
review agents treat an answered question as the contract and report code that disagrees
with it. "Don't know — flag both ways" is a valid answer; the reviewer then reports the
issue without deciding which side is wrong.

Write the answer under each question. Questions are grouped by the first stage that needs
the answer. Source: [stage_01_architecture_map.md](stage_01_architecture_map.md),
"Open questions".

---

## Cross-cutting principles

**Laver & Sergenti as reference, not target.** The project does not aim to replicate Laver
& Sergenti (2012). Where turning a theoretical model into code requires a choice the
theory leaves open, their solution (book and NetLogo code) is a useful reference but not
binding, especially where their code is inconsistent with itself or with the book. Where
our default differs from theirs, provide an opt-in L&S-compatible setting where feasible,
and document the difference. "Compatible" means the same rule, giving statistically
comparable results. It does not mean number-identical runs, because voter representation
and random-number generation differ. (Recorded during Q8.)

---

## Needed before Stage 2 (C++ core)

### Q1. Heart — which concept?
The code cites Laffond, Laslier & Le Breton (1993), which defines the *bipartisan set*. It
also describes the result as the "smallest" set, but it iterates downward from the
uncovered set, which yields the *largest* fixed point. Is Schofield's Heart intended, or a
refinement of the uncovered set? Which citation is correct?
Where: `include/socialchoicelab/geometry/heart.h`.

**Answer:**
**Decision: withdraw.** The current `heart()` / `heart_boundary_2d()` is not Schofield's
Heart. It cites the bipartisan-set paper, says "smallest" but computes the largest fixed
point inside the uncovered set, and for simple majority with an odd number of voters it
always returns the uncovered set unchanged. It should be removed from the C++ core, the
C API and the tests until a correct Schofield Heart is implemented as a separate task.
Reviewers should not review the current Heart code for correctness. Report any remaining
Heart code, C API entries, tests or doc claims as "to be removed." Any future Heart must
follow Schofield's definition, under which the uncovered set is a subset of the Heart
(Feld, Godfrey & Grofman 2014; Miller, "The Spatial Model"), not "Heart ⊆ Uncovered Set."
Note: double-check this against Schofield's own papers (1995, 1996, 1999; not yet in
`references/`) before implementing; the UC ⊆ Heart relation is taken from secondary
sources.

Implies:
- `include/socialchoicelab/geometry/heart.h` should be deleted, along with
  `tests/unit/test_heart.cpp` and the Heart tests at
  `tests/unit/test_geometry_integration.cpp:367–400`.
- `include/scs_api.h:1132–1180` and `src/c_api/scs_api.cpp:2435–2550` (`scs_heart_2d`,
  `scs_heart_boundary_size_2d`, `scs_heart_boundary_2d`) should be removed, along with
  their tests in `test_c_api.cpp`.
- The Heart sections of `geometry_design.md` §13, `c_api_design.md`,
  `Design_Document.md`, `canvas_overlay_schema.md`, the MVP and service-layer docs,
  `service.py:19` and the WHERE_WE_ARE.md Known Issues entry should be removed or marked
  "withdrawn."
- The website is not affected (it doesn't use the Heart).

### Q2. Minkowski at order p ≥ 100
For p ≥ 100, Minkowski distance is replaced by Chebyshev (L∞) to avoid overflow
(`distance_functions.h:28–31, 166–168`). The relative error can reach n^(1/p) − 1
(about 0.7% for n = 2 dimensions at p = 100). Is this an accepted contract? If so, what error
bound is acceptable, and should it be documented to users? (This decides whether failing
test S00-001 is a test bug or a library bug.)

**Answer:**
**Decision: exact Minkowski at every finite p ≥ 1; remove the Chebyshev substitution.**
The p ≥ 100 → Chebyshev cutoff was only ever an overflow guard (initial commit f9bb18c;
consensus plan 4 Item 6). Scaling by the largest weighted term,
d = m·(Σ(|wᵢΔᵢ|/m)^p)^(1/p), removes overflow and underflow at every p, so no cutoff is
needed. Users who want L∞ select `chebyshev`. Reviewers should treat any p-dependent
switch to Chebyshev, and any unscaled `pow(Σ…, 1/p)` that can overflow or underflow for
normal inputs, as a defect. Indifference curves, winsets and distances must use the same
metric at every p. S00-001 is a test bug: the test should compare against exact Minkowski
with a relative tolerance (about 1e-12).

Implies:
- `distance_functions.h:28–31, 165–168` should drop `k_minkowski_chebyshev_cutoff` and
  use scaled Minkowski.
- `winset.h:163–169` should drop the `p >= 100.0` square branch.
- `level_set.h:335–337` (superellipse sampling) should be checked for underflow at very
  large p.
- `test_distance_functions.cpp:275–300` and `test_distance_speed.cpp:47–80, 162–181`
  should be rewritten. Add tests at p = 50, 99, 100 and 1000 with large (about 1e4) and
  small (about 1e-4) coordinate differences.

### Q3. Default tie-break
The C++ default is `kRandom` with a null PRNG, which throws on any tie. Both the R and
Python bindings default to `"smallest_index"`. `aggregation_design.md` says random is the
user default. Which is intended for end users?

**Answer:**
**Decision: `smallest_index` is the default tie-break at every layer.** C++
`*_one_winner`, `borda_ranking`, `rank_by_scores` and `pairwise_ranking` default to
`TieBreak::kSmallestIndex`. The R and Python defaults stay `"smallest_index"`. The C API
keeps requiring an explicit value. Every user-facing docstring/Rd for a function with
`tie_break` must say that ties go to the lowest-index alternative by default, and that
simulations should use `"random"` with a `StreamManager` and stream name to avoid bias
towards earlier-listed alternatives. Reviewers should report any `kRandom` default, or any
doc that calls random the default, as a defect.

Implies:
- `plurality.h:70`, `borda.h:79, 97`, `antiplurality.h:80`, `scoring_rule.h:109` and
  `social_ranking.h:41, 92` should default to `TieBreak::kSmallestIndex`, with their
  `@param` comments updated.
- `aggregation_design.md` §3 (lines 84–87) should say the default is `kSmallestIndex` and
  explain when to use `kRandom`.
- The R (`voting_rules.R`, `social.R`) and Python (`_voting_rules.py`, `_social.py`) docs
  should add the bias warning.

### Q4. Reproducibility scope
Seeds are portable, but `std::*_distribution` and `std::shuffle` produce different
results on different standard libraries (libc++ on macOS vs. libstdc++ on Linux). Must the
same seed give identical results across platforms, or only on the same toolchain?

**Answer:**
**Decision: portable draws now, with a route to full bit-identity later (option 3).** Same
seed, configuration and RNG-algorithm version must give the same random draws on every
platform, as far as achievable without a bundled maths library.

- **What the project does:**
  - Replace `std::*_distribution` and `std::shuffle` (implementation-defined per C++
    [rand.dist.general] and [alg.random.shuffle]) with project-owned algorithms, each
    cited, on top of `mt19937_64`, whose output the standard fixes ([rand.predef]).
  - Route every `log`/`exp`/`cos` call in RNG code through one internal wrapper, so a
    correctly rounded or portable maths library (CORE-MATH or OpenLibm) can be swapped in
    later without changing callers.
  - Give the algorithm set a version constant, recorded with experiment outputs next to
    the seed and library version.
  - Build `libscs_api` with floating-point contraction disabled (see Implies).
- **Guarantee:**
  - Uniform integer, uniform real, bernoulli, discrete and shuffle draws are bit-identical
    across platforms.
  - Normal, gamma and beta draws use fixed algorithms but may differ in the last bit
    across platforms, because system `log`/`exp`/`cos` are not required to round
    correctly.
  - Downstream results (distances, majority comparisons, competition trajectories) are
    identical on the same platform and build. Across platforms they can differ in rare
    cases where a last-bit difference changes a discrete outcome.

**Documentation requirement:** a user-facing reproducibility page, linked from the
`StreamManager` docs in both R and Python, must state in plain language:
- (a) what is guaranteed and what isn't;
- (b) what the project did to make results portable (own algorithms, versioned algorithm
  set, contraction disabled, golden-value tests on macOS and Linux);
- (c) what can still cause differences (last-bit maths-library differences in continuous
  draws, Eigen's explicit FMA on ARM if not disabled, compiler floating-point settings,
  different library versions);
- (d) what users should record to replicate a study (seed, stream names, library version,
  RNG-algorithm version, platform).

Precedents: GROMACS (own normal distribution), Rust `rand_distr` (portability limits for
floating-point distributions), Randompack (Jónasson 2026, arXiv 2605.05099; OpenLibm and
`-ffp-contract=off` for bit-identity), NumPy NEP 19 (no cross-platform promise), WG21
D2059R0 (`std` distributions differ: 53 / 82 / 92 on MSVC / libstdc++ / libc++).

Reviewers should report any use of `std` distributions or `std::shuffle`, any direct
`engine()` use outside `PRNG`, any transcendental call in RNG code that bypasses the
wrapper, any build of `libscs_api` without contraction disabled, and any doc promising
more or less than this.

Implies:
- `include/socialchoicelab/core/rng/prng.h:68–259` should use project-owned, cited
  algorithms.
- `tie_break.h:66`, `borda.h:126`, `social_ranking.h:69`, `profile_generators.h:65–151`,
  `step_policy.h:44–59` and `initialization.h:55–67` should draw only through `PRNG`
  methods.
- Add an RNG algorithm-version constant, exposed through the C API and recorded in
  experiment results.
- Add a cross-platform golden-value test in CI (macOS + Ubuntu): exact match for uniform,
  integer, bernoulli, discrete and shuffle; last-bit tolerance for normal, gamma and beta.
- `docs/architecture/stream_manager_design.md:14` should replace "always" with this
  scoped promise.
- Add the user-facing reproducibility page (none exists today) and link it from the R and
  Python `StreamManager` docs.
- Seeded expected values in existing tests will need regenerating once.
- `CMakeLists.txt` should compile `libscs_api` with floating-point contraction disabled
  (`-ffp-contract=off`) on all platforms; R and Python use the CMake-built library.
  Reasons: identical basic arithmetic across platforms, and exact symmetric ties (review
  scratchpad test, Apple Silicon, clang -O3: with contraction on, a voter at the origin
  was not equidistant from (a, b) and (b, a) in 11% of 100,000 cases, and `x*y − x*y` was
  non-zero in 100%; with it off, 0% for both; no measurable speed cost). Eigen's explicit
  FMA instructions on ARM (`Eigen/src/Core/arch/NEON/PacketMath.h`, `vfmaq_f64` under
  `__ARM_FEATURE_FMA`) are not controlled by this flag. Either disable Eigen
  vectorisation, or list this as a remaining cross-platform difference on the
  reproducibility page. Add a regression test for the symmetric-tie case.

### Q5. Winset "empty"
The header says the winset is empty if and only if a Condorcet winner exists, but the
winset is built from sampled polygons (`num_samples`). Should "empty" mean exactly empty,
or empty at the sampling resolution? Are the sampled polygons meant to be inscribed or
circumscribed?

**Answer:**
**Decision: winset emptiness is decided exactly; polygons are for drawing and area only.**
`is_empty` on a winset returned by `winset_2d`, and `core_2d`, must use an exact test at
the status quo, not the emptiness of the sampled polygon set.

- **The test.** For Euclidean simple majority, use the median-line condition: a point is a
  majority winner iff every median line passes through it (Feld & Grofman 1987, AJPS
  31(4), Theorem 8, p. 720, citing Davis, DeGroot & Hinich 1972 and McKelvey & Wendell
  1976). Equivalently, no line through the status quo has ≥ k ideal points strictly on one
  side. Evaluate it with exact predicates.
- **Extensions.** k-majority, weighted Euclidean and Minkowski 1 < p < ∞ are the review
  agent's own derivation (preferred-to sets are strictly convex with the status quo on
  their boundary), not from a source. They must be proved or checked against a source
  before being relied on. Until then, those cases are labelled "approximate" in the API
  docs.
- **Manhattan and Chebyshev** keep the polygon test, which is already exact for them
  (4-vertex regions).
- **Background facts.** Sampled polygons are inscribed (vertices on the indifference
  curve), so the computed winset is a subset of the true one: "non-empty" from polygons
  is always reliable, while "empty" can be false. Measured with the Python binding: five
  Plott-symmetric voters with one voter rotated by δ (true winset non-empty) were reported
  empty at δ = 0.03 with 64 samples, at δ = 0.01 with 256, and at δ = 0.003 with 1,024.
  Emptiness after winset boolean operations stays polygon-level and must be documented as
  approximate.
- **For reviewers.** Report any "empty iff Condorcet winner" claim that rests on polygon
  emptiness, and any Condorcet/core decision made from `winset_is_empty` on sampled
  polygons.

Implies:
- Add an exact core function (for example `is_majority_winner_2d(sq, voters, cfg, k)`),
  plus a C API wrapper, R/Python bindings and tests. Tests must include the near-Plott
  δ cases (0.03, 0.01, 0.003 rad, which must be non-empty) and exact symmetry (empty).
- `include/socialchoicelab/geometry/winset.h:196–198` (`winset_is_empty`) and
  `winset.h:252–254` (the "Empty iff" doc) should use or point to the exact test.
- `src/c_api/scs_api.cpp:1491` (`scs_winset_is_empty`) should report the exact result for
  `winset_2d` handles and document the boolean-op case as approximate.
- `include/socialchoicelab/geometry/core.h:165` should use the exact test instead of
  `winset_is_empty(winset_2d(...))`.
- `docs/architecture/geometry_design.md` §7 and §11 and `c_api_design.md:271` should be
  updated to match.
- Possible follow-up: Theorem 8 says the majority winner, if one exists, is at a voter's
  ideal point. If the paper's assumptions (for example odd n) hold for our setting,
  `core_2d` could test every voter ideal and become exact instead of heuristic. Check the
  assumptions before relying on this.
- Website: not affected (it doesn't use winsets).

### Q6. Weighted winset approximation
Weighted winsets expand each voter into copies, rounding weights to multiples of
min_weight/100. The expansion grows with the weight ratio × `weight_scale`. Is this an
accepted approximation, and what expansion size is acceptable?

**Answer:**
**Decision: weighted winsets use the weights exactly; no rounding or voter copying. The
default quota is more than half the total weight.**

- **Rule.** A point is in the weighted winset iff the voters (parties) who strictly prefer
  it to the status quo hold:
  - more than half the total weight (default, matching `winset_2d`'s ⌊n/2⌋+1); or
  - at least threshold × total weight, when the caller passes an explicit threshold in
    (0, 1].
- **Exact sums.** Comparing weight sums with the quota must not depend on rounding or
  summation order; for example, use exact rational arithmetic.
- **Algorithm.** Compute the region from coalitions of the original voters, not copies.
  Either:
  - extend the petal method to minimal winning coalitions (cost grows as 2ⁿ; fine for
    the typical handful of parties), or
  - overlay all preferred-to regions and sum the weights on each face.

  Above a documented size limit, fail with a clear end-user error rather than run
  indefinitely.
- **Background.** The current copying approach (`winset.h:509–580`) is not exact and is
  not usable for real inputs (measured with the Python binding):
  - Weights 1 / 1.37 / 2.5 / 3.1 / 4.9 expand to 1,287 copies and did not finish in
    120 s (an earlier run went 35 minutes).
  - Weights 1 / 1 / 2.004 put a losing coalition (2.0 of 4.004) inside the winset.
  - With equal weights, exactly half the weight counts as winning, unlike `winset_2d`.
  - Real legislatures are affected too: seat counts 206 / 197 / 152 / 118 / 92 / 39 / 28
    would expand to 2,972 copies, while an exact method checks only 2⁷ = 128 coalitions.
- **For reviewers.** Report any rounding of weights, voter-copying, "exact proportional
  representation" claims, or a default that lets exactly half the weight win.

Implies:
- `include/socialchoicelab/geometry/winset.h:474–580` should be replaced with an exact
  coalition-based method, a size limit, and no `int` weight arithmetic (lines 550–554
  can overflow).
- The default threshold in C++ (`winset.h:512`), the C API (`scs_weighted_winset_2d`,
  `scs_api.h:711`), Python (`_winset.py:321`, `threshold=0.5`) and R (`winset.R`
  `weighted_winset_2d`) should mean "more than half." For example, use a sentinel such as
  −1 / `"simple"`, mirroring `k = -1`, and document explicit thresholds as "at least".
- Remove `weight_scale`.
- `docs/architecture/geometry_design.md:248–252` and `:500` should drop "exact
  proportional representation" and describe the new method.
- Tests: the 1 / 1 / 2.004 near-quota case (point preferred by the two weight-1 voters
  must be outside); equal-weight even split (must match `winset_2d`); decimal vote-share
  weights for 6–7 parties finishing in under 1 s; the size-limit error.
- Website: not affected.

### Q7. Voronoi with coincident candidates
When two candidates are at the same point (common when competitors converge), the
bisector is degenerate and their cells overlap instead of splitting. What output is
expected — shared cell, error, or something else?

**Answer:**
**Decision: ties between equally close competitors are broken at random, per voter, from
a named stream (option 3).** A voter equidistant from two or more competitors (in
practice, competitors at the same position) supports one of them chosen uniformly at
random. The Predator's choice among equally large targets is also uniform at random.
Draws happen only when a tie occurs, so tie-free runs are unchanged. Vote totals stay
whole numbers.

- **Basis:**
  - It is consistent with Laver & Sergenti (2012). In all six chapter models (Ch5–Ch10 NetLogo
    files, local copies in `references/social_choice/electoral_competition/LaverNetLogo/`),
    `update-support` assigns each voter cell via `min-one-of parties [distance myself]`
    (Ch5 l.151, Ch6 l.168, Ch7 l.278, Ch8 l.281, Ch10 l.198; Ch9 l.290–291 uses
    `max-one-of` on valence-weighted utility). Predators pick targets via `max-one-of` /
    `min-one-of` (Ch10 l.176; Ch7–9 l.225/228/235). The NetLogo 4.1 dictionary says these
    report "one random agent" on a tie.
  - It matches the analytical literature's canonical assumption: indifferent voters
    randomize with equal probability (Duggan 2005, "A Survey of Equilibrium Analysis in
    Spatial Models of Elections", pp. 6, 11; Patty, Snyder & Ting 2008, "Two's Company,
    Three's an Equilibrium", p. 14 and fn. 18). Duggan & Jackson (2005) also require
    indifferent voters to treat candidates symmetrically (Duggan survey p. 11, fn. 3),
    which the current lowest-index rule violates.
  - Fowler & Laver (2008, JCR, p. 76) describe co-located "parasite" parties as "on
    average splitting the host's support down the middle."
  - Option 1 (equal fractional split) is the expected value of this rule and may be added
    later as an optional mode.
- **Measured motivation** (review scratchpad, Python binding, 1,000 voters N(0,1) in 2D,
  200 rounds, 20 seeds). With random starts, exact coincidence never occurred. With two
  competitors started at the same point, the higher-index one got zero votes in every
  coincident round. Two predators stayed locked together for all 4,000 rounds, with one
  at zero votes throughout.
- **Documentation requirement:**
  - `competition_design.md` gets a section "Ties between equally close competitors"
    stating the rule, the Laver–Sergenti code citations (file, procedure, line), the
    NetLogo tie semantics, the analytical sources above, and the difference from Laver's
    implementation: per voter rather than per grid cell, and from our named streams rather
    than NetLogo's global generator, so it's the same rule but not number-identical runs.
  - The R and Python `competition_run` docs state that a `StreamManager` is required and
    name the tie streams.
  - The Voronoi docs describe the shared cell.
- **Out of scope:** the general aggregation profile builder (`build_spatial_profile`,
  ties by index) is unchanged.
- **For reviewers.** Report lowest-index tie-breaking anywhere in competition voter
  assignment or Predator targeting, overlapping duplicate Voronoi cells, or a competition
  run that can proceed without a `StreamManager`.

Implies:
- `include/socialchoicelab/competition/election_feedback.h:84–97` should break distance
  ties uniformly at random via a new named stream (e.g. `competition_voter_ties`), not by
  sorting on (distance, index).
- `include/socialchoicelab/competition/adaptation.h:87–105` (`leader_index`) should break
  ties for largest uniformly at random via a named stream (e.g.
  `competition_adaptation_predator`).
- `include/socialchoicelab/geometry/voronoi_2d.h:48–63` should detect co-located sites and
  return one shared cell, marked as shared in the C API output (`c_api_design.md` §C2.8).
  The R/Python overlays should draw it once, labelled shared.
- `scs_competition_run` and `competition_run` in R/Python should require a
  `StreamManager`, with a clear end-user error.
- `docs/architecture/competition_design.md:175–178` (tie rule) and its stream map, plus
  `include/socialchoicelab/competition/experiment_runner.h:85–91` (allowlist), should add
  the new streams.
- **Website:** `socialchoicestudiowebsite/backend/app/simulation.py:14–18` registers an
  explicit stream list and must add the new streams.
- Add `references/REFERENCE_INDEX.md` entries for the Laver–Sergenti NetLogo models,
  Duggan (2005 survey) and Duggan & Jackson (2005).
- Tests:
  - Two co-located stickers split support roughly equally across seeds, and identically
    for a fixed seed.
  - Predator tie for largest.
  - Voronoi shared cell.
  - A tie-free run is bit-identical before and after the change.
  - The missing-`StreamManager` error.

### Q8. `hunter_sticker` strategy
This competition strategy exists in code but not in the design docs. What is its published
source (if any)? Should it be added to `competition_design.md`?

**Answer:**
**Decision: replace `hunter_sticker` with a general satisficing setting; Predator target
becomes an option.** Designed from first principles under the cross-cutting principle;
Laver & Sergenti (2012) used as a reference. No published source for `hunter_sticker`
was found; in Laver (2005, pp. 16, 18) and Fowler & Laver (2008, p. 6) "Hunter-Sticker"
means competition between Hunters and Stickers, not a hybrid rule.

- **Satisficing is a per-competitor setting, not a strategy kind.** Each competitor has a
  comfort metric (`none` by default, `vote_share` or `seat_share`) and a threshold κ.
  When the metric is at or above κ, the competitor does nothing that round; otherwise it
  applies its strategy.
  - R/Python accept a single value (model-wide treatment) or a vector (per competitor).
  - L&S's satisficing rule ("κ-satisficing-R: If the value of the output metric K < κ, do
    R; else do nothing", ch. 7, p. 137) is `vote_share` with κ. Their Ch7 code mixes κ
    values across parties (per-party `comfort-kappa`, l.35, l.428).
  - The old `hunter_sticker` is `seat_share` with κ = 1/seat_count, since a competitor
    holds a seat iff its seat share ≥ 1/S.
- **Satisficing is allowed for every strategy.** L&S consider satisficing Sticker and
  Aggregator meaningless ("Aggregator is not designed to increase party vote share",
  p. 137). We allow it for flexibility and document their objection. For Sticker it has
  no effect.
- **A satisfied competitor keeps everything:** position, heading and strategy memory.
- **When a satisficing Hunter resumes,** it judges its last actual move by comparing
  current support with its support just before that move, not with last round's support.
  This follows from the definitions (Hunter: "if the last move increased support…",
  p. 138, plus "do nothing"), and it matches the L&S code (`sat-hunt` → `hunt`/`stick`;
  `old-size` set only inside `hunt`, Ch7 l.208–219). So no compatibility switch is
  needed.
- **Predator target is an option:** `largest` (Laver 2005; L&S ch. 3, p. 29; current
  behaviour) or `nearest_larger` ("the closest of the set of parties with more support
  than you", L&S ch. 7, p. 135). The default is **not decided: flag both ways**.
- **For reviewers.** Report any strategy kind that duplicates another with a stopping
  rule, any loss of heading or Hunter memory while a competitor stands still, and
  satisficing documented without the L&S citation and the Aggregator objection.

Implies:
- Remove `kHunterSticker` / `"hunter_sticker"`: `competition_types.h:20`,
  `scs_api.h:1234`, `strategy_registry.h:164–200`, `r/R/competition.R:278`,
  `python/src/socialchoicelab/_competition.py:30, 389`, and the tests at
  `r/tests/testthat/test_competition.R:115–170` and `python/tests/test_competition.py:115ff`
  (rewrite them as `seat_share` κ = 1/S tests).
- Add per-competitor comfort metric and κ to the C++ competitor config, the C API
  (per-competitor arrays, like `strategy_kinds`) and both bindings.
- `include/socialchoicelab/competition/motion.h` (`updated.heading =
  motion.applied_heading`) should not zero the stored heading when a competitor stands
  still.
- The Hunter (`strategy_registry.h:86–140`) should keep its own "support before my last
  move" instead of using `previous_round_metrics`.
- Add a Predator target option at `strategy_registry.h:157–162` / `adaptation.h:87–105`,
  with the default pending.
- `docs/architecture/competition_design.md` should document satisficing (L&S ch. 7
  definition, our generalisation, the Aggregator objection), the heading and memory rule,
  and the Predator variants with citations.
- Website: not affected (`socialchoicestudiowebsite/backend/app/models.py:10` allows only
  sticker, hunter, aggregator and predator). Exposing satisficing there later would be a
  website change.
- Tests:
  - A satisficing Hunter stands still while share ≥ κ.
  - `seat_share` with κ = 1/S reproduces the old `hunter_sticker` behaviour.
  - Heading is preserved across a pause.
  - The wake-up comparison uses support from before the last move.
  - Both Predator targets.

### Q9. Thread safety
Docs claim the geometry functions are thread-safe. Is thread safety a requirement (e.g.
for parallel experiment runs or the website), and has it been checked under CGAL EPEC?

**Answer:**
**Decision: thread safety is a requirement, scoped to separate objects (option 1).**
Library functions may be called concurrently from multiple threads as long as each thread
uses its own objects. No handle (`SCS_Winset`, `SCS_Profile`, `SCS_StreamManager`,
`SCS_CompetitionTrace`, `SCS_CompetitionExperiment`, and their R/Python wrappers) may be
used from two threads at the same time, **including read-only calls**. Error buffers are
per call and are never shared.

- **Why reads are included.** CGAL's rule is that different objects may be used in
  different threads, but "it is not safe to access the same object from different threads
  at the same time, unless otherwise specified" (CGAL 6.1 manual, Preliminaries).
  Measured: CGAL's read-only `polygons_with_holes()` writes "visited" flags on arrangement
  faces (`Gps_face_base::set_visited`, `Gps_default_dcel.h:101`, reached from
  `winset_contains`, `winset.h:209`).
- **Evidence** (review scratchpad, ThreadSanitizer build of `scs_api.cpp`, 8 threads,
  macOS arm64, clang, CGAL 6.1.1 with `CGAL_HAS_THREADS` defined):
  - Separate objects per thread (`winset_2d`, convex hull, Voronoi, Yolk, and a
    competition run with a per-thread `StreamManager`): **0 data races**.
  - Concurrent reads of one shared winset (`contains_point`, `bbox`): **46 data races**.
  - These results are evidence, not proof: one platform, small workloads, and GMP/MPFR
    not instrumented.
- **Why it matters.** Python calls run truly in parallel on threads: cffi releases the GIL
  (measured: two threads gave a 2.00× speed-up on `winset_2d`). Users may parallelise
  sweeps with thread pools, and a future parallel experiment runner would rely on this
  contract.
- **For reviewers.** Report any new global or static mutable state (function-local
  `static const` initialised once, such as `StrategyRegistry::built_in()`, is fine), any
  API that shares mutable state between handles, and any doc that calls handle reads
  thread-safe.

Implies:
- `docs/architecture/c_api_design.md:594–603` (§C0.5) should state the scoped rule above,
  including "no shared handles, even for reads." Lines 96–110 should be reconciled with
  it: the "lazy caches" caveat is now confirmed for winsets.
- The R and Python docs for `Winset`, `Profile`, `StreamManager`, `CompetitionTrace` and
  `CompetitionExperiment` should carry the same one-thread-at-a-time warning.
- Add a ThreadSanitizer stress test to CI: concurrent calls on separate objects must
  report zero races.
- For Stage 8 / Q15, not a library change: the website runs simulations in FastAPI's
  thread pool (up to 40 threads by default, per AnyIO docs). It has no per-request
  timeout, and requests may ask for up to 5,000 voters, 500 rounds and 12 competitors
  (`socialchoicestudiowebsite/backend/app/models.py:34, 64, 85`). That's fine for its
  current local single-user use. For hosted use, consider fixed-size worker processes or a
  job queue (FastAPI docs point heavy computation to multi-process tools such as Celery).

---

## Needed before Stage 3 (C API)

### Q10. Competition bounds
The C API silently sets the competition space to the bounding box of competitors and
voters ± 1.0 in absolute units (`scs_api.cpp:4083–4103`). R and Python cannot set bounds.
Is this intended? Should the bindings expose bounds?

Stage 2 adds: bounds change results, because parties that hit the edge are stopped or
bounced. The ± 1.0 margin is in absolute units, so it is negligible on a 0–1000 scale but
100× the data range on a 0–0.01 scale. The same code is duplicated at
`scs_api.cpp:4378–4396`.

**Answer:**

### Q11. Version numbers
CMake says 0.1.0, `SCS_API_VERSION` says 0.2.0, and the packages are 0.3.0. Which is
authoritative? Should the C API version track the package version?

**Answer:**

### Q12. Deprecation language
`c_api_design.md` describes a deprecation period for API changes. Does that apply before
the project opens, given the no-backward-compatibility rule?

**Answer:**

---

## Needed before Stages 5–7 (bindings, parity, plotting)

### Q13. Visualization "renderers only" exceptions
The rule is that plotting layers only render what the core computes. Exceptions found:
R/Python draw Euclidean indifference circles themselves when no distance config is given;
the JS competition stats panel computes Euclidean distances regardless of metric; the JS
utility surface computes contours with marching squares. Are any of these acceptable?

**Answer:**

### Q14. Yolk and Heart in the bindings
`scs_yolk_2d` and the three Heart functions are in the C API but not bound in Python (and
not exposed in R). Should they stay unbound until replaced? Should `layer_yolk` (which
takes a user-supplied centre and radius) stay?

**Answer:**

---

## Needed before Stage 8 (app and website)

### Q15. Website integration
The website imports the library by inserting the sibling repo into `sys.path`, and
partly duplicates the plot payload builder. Is that the intended long-term design? Is the
website in scope for parity updates when the library API changes?

**Answer:**

---

## Needed before Stage 9 (docs and hygiene)

### Q16. Stale status docs
`WHERE_WE_ARE.md` and `.cursor/HANDOFF.md` say the October work is uncommitted, but it was
committed (fbc52da…aa19b05). Treat these as stale and report them?

**Answer:**

### Q17. Doc file names
Links point to `design_document.md` and `development.md`, but the files are
`Design_Document.md` and `DEVELOPMENT.md`. Which names are correct under the naming
convention?

**Answer:**

---

## Raised by Stage 2 (C++ core) — needed before the fix phase

These came out of the Stage 2 reports. None blocks Stage 3. Each lists the options the
reviewers saw; pick one, or write your own.

### Q18. A weight of 0 in 2D geometry
A salience weight of 0 means "ignore this dimension". That works for distances, but in 2D
geometry it turns a voter's preferred region into an endless strip, which crashed R or
Python (S02b-003). The alternative to an error was to cut the strip off at a box.

**Answer:** Error (approved 2026-10-29, implemented in PR #1). Every 2D function that builds
a bounded shape from salience weights (indifference curves, preferred-to sets, and
everything built on them: winsets, core, uncovered set) rejects a zero, negative or
non-finite weight with an end-user error. In distance and loss calculations, weight 0
still means "ignore this dimension". The Yolk ignores the distance config entirely and
Voronoi takes no weights, so this rule does not apply to them; Voronoi instead rejects
non-finite sites and bounding-box values.

### Q19. Threshold loss: the indifference curve through a point inside the "don't care" zone
With threshold loss, a voter doesn't care about any point within distance τ of their
ideal. If you ask for the indifference curve through such a point, the code returns the
edge of the zone, not a curve through the point (S02a open question 1).
Options: (a) keep returning the zone edge and document it; (b) return the circle through
the point; (c) return an error saying the point is inside the zone.

**Answer:**

### Q20. Random numbers in a range: can the top value come out?
`uniform_real(min, max)` is meant to give values from min up to, but not including, max.
For very narrow ranges, rounding can produce exactly max (S02a-011).
Options: (a) guarantee max never comes out, by clamping; (b) allow max and document the
narrow-range exception.

**Answer:**

### Q21. Source for the Gaussian and threshold loss formulas
The reviewer could not check the Gaussian loss formula against Conevska & Mutlu (2025):
the PDF text has no readable formula. Threshold loss has no citation at all. Neither
paper is in `REFERENCE_INDEX.md`.
What is the source for each formula? (The reviewer will add the papers to the index.)

**Answer:**

### Q22. Yolk with an even number of voters, or a supermajority
For an even number of voters there are two "median lines" per direction, with a band
between them. The code makes the yolk touch only one of them. Koehler (1992) says it
should touch both, for simple majority. For supermajority (k-majority) the reviewer found
no source.
Options: (a) touch both edges, as Koehler; (b) keep one edge and document why. And: which
source should the k-majority yolk follow?

**Answer:**

### Q23. "The yolk centre is always in the uncovered set"
Tests and docs rely on this, but the cited paper (Feld, Grofman & Miller 1988) does not
prove it. Should the project keep relying on it? If yes, what is the source?

**Answer:**

### Q24. Voronoi: when do two candidates count as being at the same place?
Q7 says candidates at the same place share a Voronoi cell. Should "same place" mean
exactly identical coordinates, or within a small tolerance?

**Answer:**

### Q25. Uncovered set with only one or two corner points
The uncovered-set boundary can come back as a single point or a line segment, not a
shape (S02b-008). Is that an acceptable result, or should it be flagged or raised as an
error?

**Answer:**

### Q26. Who wins a tied last seat?
Each seat method breaks ties in a fixed order, ending with "first listed wins":
- Plurality top-k: most votes, then first listed (`seat_allocation.h:16–25`).
- Hare largest remainder: largest remainder, then most votes, then first listed
  (`seat_allocation.h:109–115`). For example, votes [1, 3] with 2 seats give [0, 2]: the
  remainders are equal, so the vote total decides.
- Competition runs use these methods, so the same order applies there.

Q7 chose random tie-breaks for voters and the Predator so that earlier-listed parties
aren't favoured. Flag both ways.
Options: (a) replace only the final "first listed" step with a random draw from a named
stream, keeping each method's earlier rules (e.g. Hare's vote-total rule); (b) make any
remaining tie random, including Hare's vote-total rule; (c) keep the fixed order and
document it (ties are rare with many voters).

**Answer:**

### Q27. Copeland ranking: which definition of "beats"?
There are two Copeland functions, and they disagree (S02c-002). One counts x as beating
y when more voters prefer x than y (indifferent voters don't count). The other needs a
majority of all voters. Which should both use? And should the score be "wins minus
losses" (the standard Copeland score)?

**Answer:**

### Q28. `is_majority_consistent`: rename or redefine?
The name suggests "does this voting rule always pick the majority winner?". What it
actually checks is "is this candidate the Condorcet winner?" (S02c-003). Should the code
change to match the name, or the name change to match the code?

**Answer:**

### Q29. A Predator tied for largest party
The Predator moves toward the largest party. If the Predator itself is tied for largest,
should it (a) stand still, as in the Laver & Sergenti code, or (b) pick at random among
the tied parties, including itself (a literal reading of Q7)?

**Answer:**

### Q30. Bouncing off the wall
With the "reflect" boundary rule, a party that hits the edge bounces back. But its
direction isn't flipped, so a Hunter that just gained votes heads straight back into the
wall next round. Should the direction flip on a bounce?

**Answer:**

### Q31. Scenarios that share a seed
In a parameter sweep, every scenario with the same master seed gets identical random
numbers. That's a standard technique (common random numbers) that makes comparisons
fairer. Is it intended? If so, the reviewer suggests documenting it.

**Answer:**

### Q32. "Share-delta" step size
One step-size option never moves anyone (S02d-002). Was the step meant to depend on the
party's own change in vote share, or on its gap to the leading party
("share-gap-proportional" in `competition_plan.md`)?

**Answer:**

### Q33. Hunter "same move" with random step sizes
A Hunter that gained votes repeats its last move. With random step sizes, a new step
length is drawn every round, so the move isn't really the same. Should a Hunter that
gained votes reuse its last step length?

**Answer:**
