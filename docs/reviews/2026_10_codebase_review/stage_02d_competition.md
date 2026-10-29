# Stage 02d — C++ core: competition
Model / effort: Opus 5.5 / high   Commit: 3f19579 (no source change up to HEAD 3a4e908)   Files reviewed: 13, 1,933 lines
Not read (if any): none. All 13 headers in `include/socialchoicelab/competition/` were read in full.

Scratch programs: `SCR=/private/tmp/claude-501/-Users-robiragan-Dropbox-SocialChoiceStudioDev/2402f97e-221e-494b-88cb-fc63c41efc50/scratchpad/stage2d/`
(build with `$SCR/build.sh <name>`: Apple clang, `-std=c++20 -O2 -DNDEBUG`, repo `include/`,
`build/_deps/eigen-src`, Homebrew CGAL/GMP/MPFR, plus `src/core/rng/stream_manager.cpp`; shared
helpers in `$SCR/common.h`).

Sources checked: Laver & Sergenti (2012) book PDF (ch. 3, pp. 28–29, strategy definitions),
the L&S Ch5/Ch7/Ch10 NetLogo models (Ch5 `hunt`/`aggregate`/`predate`/`update-support`, lines
120–155 after CR→LF conversion), and Fowler & Laver (2008, JCR 52(1), p. 70). **Laver (2005,
APSR) is not in `references/`**, so claims attributed only to it were not checked.

## Summary

- **0 P0, 6 P1, 6 P2, 6 P3** (18 findings). Every P1 and most P2 findings were reproduced with
  scratch programs.
- **Most important:** an **Aggregator with no supporters moves towards the coordinate origin**.
  It does not stand still as L&S specify, so the result depends on where the user puts (0, 0)
  (S02d-001). The **`share_delta_proportional` step policy can never move anyone**: every step
  is 0 from round 0 onwards, so the run is identical to all-Sticker (S02d-002).
- **Termination can stop runs wrongly.** `state_signature` prints coordinates to 6 significant
  digits whatever `signature_resolution` is, so distinct states collide and a run stops early
  with `kCycleDetected` (S02d-004). A stationary state is also reported as a "cycle"
  (S02d-013). Position convergence itself compares exactly copied doubles and is sound.
- **Robustness:** `kReflect` hangs forever for a large or infinite step (S02d-003).
  Step/termination validation accepts NaN/Inf (S02d-010). One trace holds about 282 MB at the
  website's maximum sizes, almost none of it exported (S02d-009).
- **Owner decisions:** two more Q4 sites outside the Implies ranges (S02d-005). The core engine
  still runs without a `StreamManager` and then silently drops jitter (S02d-006, Q7). Seat
  ties are lowest-index, which Q7's literal scope does not cover (S02d-012; owner to confirm).
  Q10 (bounds) is reported as an open question.
- **Strategy definitions vs L&S / Fowler & Laver:** Hunter matches: a uniform draw from the
  backward half-space equals L&S's `heading + 90 + random-float 180`. Aggregator diverges
  (it takes a fixed step towards the centroid instead of jumping to it, overshoots, and never
  converges; S02d-007). Predator matches L&S except for ties (S02d-014). None of these
  definitions or divergences are documented (S02d-015).

## Contracts this layer provides

Namespace `socialchoicelab::competition`. Header-only, no global or static mutable state.
`StrategyRegistry::built_in()` is a function-local `static const` and its services are
stateless (Q9-compliant). Separate engine runs on separate `StreamManager`s may run
concurrently.

- **Dimensions:** competition supports 1 or 2 dimensions only (`validate_bounds`,
  competition_types.h:100). Bounds must satisfy `lower[d] < upper[d]` strictly. Every
  competitor position/heading and every voter ideal must have that dimension. The distance
  config must have exactly `dim` salience weights (engine.h:126–132). Positions and voters are
  **not** checked for finiteness or for lying inside the bounds. The core relies on the C API
  for finiteness (NaN reaches `calculate_distance`, which throws "alternative_point[0] is not
  finite").
- **Errors:** every precondition failure throws `std::invalid_argument` with a
  function-prefixed message. Unknown stream names under an allowlist throw from
  `StreamManager::get_stream`. A `seat_count` larger than the number of *active* competitors
  under `kPluralityTopK` throws from inside round 0 (S02d-018). Hunter in 2D throws mid-run
  ("Hunter requires a StreamManager…") if `stream_manager == nullptr` and a random draw is
  needed. Nothing returns error codes.
- **Ownership:** all inputs are taken by const reference and copied. Outputs
  (`CompetitionTrace`, `CompetitionExperimentResult`, `CompetitionSweepResult`) are plain
  value types owned by the caller. `AdaptationContext` holds references that are valid only
  during one round. `StrategyRegistry` stores **non-owning** pointers, so registered services
  must outlive the registry (S02d-017).
- **Units:** positions, step sizes, bounds and epsilons are in the caller's coordinate units.
  Vote/seat shares are in [0, 1] (vote share = votes / n_voters; seat share = seats /
  seat_count, all 0 when seat_count = 0). Vote totals are whole numbers.
- **Round semantics (synchronous):** in round t, active competitors are evaluated at their
  round-t positions. `previous_round_metrics ← current_round_metrics` and the new metrics are
  written. Every active competitor adapts from that same snapshot, then moves (step from
  `resolve_step_size`, then boundary policy). Inactive competitors are left off the ballot,
  get zero metrics and never move. Competitors are processed in index order. Each Hunter
  heading draw (`competition_adaptation_hunter`) is followed by that competitor's step-size
  draw (`competition_motion_step_sizes`). Because the streams are separate, the interleaving
  does not matter. Step-size draws are consumed for every active competitor every round, even
  when its heading is zero.
- **Trace:** `rounds[t].evaluated_competitors` hold the round-t positions with round-t
  metrics. `rounds[t].feedback` vectors are indexed by total competitor index (inactive
  slots are 0). `feedback.profile` holds the active-only election. `final_competitors` hold
  the positions **after** the last move, still carrying the **last evaluated round's metrics**
  (S02d-008). `rounds.size()` is `max_rounds`, or fewer after an early stop. With
  `max_rounds = 0` the trace has no rounds and `final_competitors` are the inputs with no
  metrics. `trace.distance_config` is a copy of the election distance config.
- **Ordering and ties (current, pre-Q7):** each voter supports their nearest competitor, and
  exact distance ties go to the lowest active index. `leader_ranking` sorts by votes and then
  index. Predator's target is the first competitor with the maximum objective. Seat ties go
  to the lowest index (S02d-012).
- **Termination:** checks run after each round in the order convergence → no-improvement →
  cycle, and the first to fire sets `termination_reason`. Convergence means max ‖Δposition‖₂
  ≤ `position_epsilon` between evaluated and next positions; exact comparison of copied
  doubles makes "nobody moved" reliable. No-improvement means no competitor's objective rose
  by more than `objective_epsilon`. Because shares sum to 1, this effectively means "shares
  unchanged". Cycle means the post-move position signature equals one of the last
  `cycle_memory` signatures, including the initial one. It is position-only and unreliable
  beyond 6 significant digits (S02d-004, S02d-013). A fixed point is reported as a cycle when
  convergence detection is off.
- **Determinism:** for the same inputs, registry, stream seeds and platform/build, results
  are bit-identical. Cross-platform: Hunter headings in 2D use `std::normal_distribution`, and
  `std` distributions are used for step sizes, so draws differ across standard libraries
  until Q4 lands.
- **Experiment runner:** serial. It owns a local `StreamManager(master_seed)`. Each run calls
  `reset_for_run(master_seed, i)` and then sets the allowlist to exactly
  `{competition_adaptation_hunter, competition_motion_step_sizes}`, so a custom strategy
  using any other stream throws. `run_seed` equals the seed actually used. Run i can be
  reproduced alone with `reset_for_run(master_seed, i)`. Note that run 0 is **not** the same
  as a single run with `StreamManager(master_seed)`. All runs share the same
  `initial_competitors`. Sweeps reuse each scenario's own `master_seed`, so scenarios with
  equal seeds get common random numbers. Means are plain arithmetic means over runs.
- **Not reachable from the C API:** `initialize_competitors` and the `competition_init_*`
  streams, `run_competition_sweep`, inactive competitors, and custom registries.

## Findings

### S02d-001 [P1, confirmed] Aggregator with zero supporters moves towards the coordinate origin
- **Where:** include/socialchoicelab/competition/adaptation.h:110–118 (`supporter_mean` returns
  `PointNd::Zero` for an empty list); strategy_registry.h:149–155 (heading =
  `normalize(mean − position)`).
- **What:** With no supporters, the Aggregator's target is the origin (0, …, 0), so it walks
  towards the origin. L&S (2012, ch. 3, p. 28) and their code (Ch5 `aggregate`: "maintain
  current position if zero supporters") keep the party where it is. Where it ends up depends
  on the coordinate system.
- **Failure scenario:** Sticker at 5, Aggregator at 8, voters {4, 5, 6} (1D, step 1). The
  Aggregator has 0 votes and moves 8 → 7 → 6. Mirrored (−5, −8, voters −4…−6), it moves
  −8 → −6. Either way it moves towards 0, passing through the Sticker's support.
  `$SCR/d02_aggregator.cpp`, output: `(a) zero-supporter Aggregator positions: 8(votes=0)
  7(votes=0) 6(votes=1) final=6 [L&S: stays at 8]` and `(a') mirrored: … final=-6`. This is
  reachable whenever an Aggregator loses all voters. It is common today with co-located
  competitors, where the higher index gets zero votes (Q7 measured motivation).
- **Suggested fix:** Return a zero heading when the supporter list is empty. Add a test
  ("Aggregator with no supporters stands still").
- **Also affects:** R/Python `competition_run` and the website (all expose Aggregator).

### S02d-002 [P1, confirmed] `share_delta_proportional` step policy never moves anyone
- **Where:** include/socialchoicelab/competition/step_policy.h:17–30, 61–63;
  initialization.h:93–94.
- **What:** Step = c·|share_t − share_{t−1}|. In round 0 nobody has previous metrics, so
  every step is 0. Nobody moves, so in round 1 every share is unchanged and every step is 0
  again, and so on forever. The policy is a fixed point from any start, and selecting it
  silently gives an all-Sticker run. The test `ProportionalStepIsZeroOnFirstRound`
  (test_competition_motion.cpp:106–120) locks in the first step, but nothing checks a
  multi-round run. competition_plan.md Phase D describes a "share-gap-proportional" step,
  which suggests a gap (e.g. to the leader) was intended rather than the own-share change.
- **Failure scenario:** Hunter, Aggregator and Predator in 2D, 441 voters, c = 10, 50 rounds.
  Output: `max displacement after 50 rounds = 0` and step sizes `0 0 0` in rounds 0, 1 and 49.
  `$SCR/d01_share_delta_frozen.cpp`.
- **Suggested fix:** Redefine the policy so it is not degenerate (for example, gap to the
  leader's share, or a minimum step / fallback step when there is no history), and document
  it. Add a multi-round engine test that asserts motion.
- **Also affects:** R `make_competition_step_config(kind = "share_delta_proportional")`
  (utils.R:111), Python `_STEP_POLICY_INT` (_competition.py:36), C API
  `SCS_COMPETITION_STEP_SHARE_DELTA_PROPORTIONAL`.

### S02d-003 [P1, confirmed] `kReflect` boundary policy loops forever for large or infinite steps
- **Where:** include/socialchoicelab/competition/boundary_policy.h:13–24 (`reflect_coordinate`).
- **What:** Reflection is done one bounce at a time (`value = 2·upper − value`, then
  `2·lower − value`). The loop takes about |value|/width iterations. Once |value| ≫ width, the
  ± width updates are lost to rounding, so it never ends. ±Inf never ends either. Step sizes
  are only checked for `< 0` (S02d-010), and the C API does not check their finiteness, so
  both cases are reachable.
- **Failure scenario:** 1D box [−1, 1], one Hunter, fixed step. Step 1e6 takes 2.9 ms per
  move. Step 1e9 takes 1,580 ms per move. Step 1e17 or `inf`: `still looping after 3 s:
  HANG`. `$SCR/d03_reflect_hang.cpp` (run with the step as argv[1]). From R/Python:
  `boundary_policy = "reflect"` with `fixed_step_size = 1e17`.
- **Suggested fix:** Use the closed form (fold `value − lower` modulo `2·width` with
  `std::fmod`, then mirror). Reject non-finite step sizes in `validate_step_policy`.
- **Also affects:** C API, R, Python and website runs that use reflect.

### S02d-004 [P1, confirmed] Cycle signature keeps only 6 significant digits, so runs stop early on false cycles
- **Where:** include/socialchoicelab/competition/state_signature.h:22–30; used at
  engine.h:297–350.
- **What:** Coordinates are rounded to `resolution` and then written with `operator<<` at the
  default precision of 6 significant digits. The effective resolution is therefore
  max(resolution, ~5e-6·|x|), not `signature_resolution`. Distinct states collide, and
  `has_cycle_signature` reports `kCycleDetected`. The reverse failure also happens: values
  that round to −0 and +0 print differently ("-0" vs "0"), so equal states can miss each
  other.
- **Failure scenario:** `$SCR/d04_signature.cpp`: `sig(10.000000)` equals `sig(10.000040)` at
  resolution 1e-6, and `sig(1234.5671)` equals `sig(1234.5674)` at 1e-9. In an engine run, a
  1D Predator at 900 chases a larger Sticker at 400 with step 1e-4 (cycle detection on,
  resolution 1e-6). It moves 900 → 899.9999 and the run stops after round 0: `rounds=1
  early=1 reason=2 (kCycleDetected)`. This stops early on any data whose coordinates have
  more than ~5 significant digits relative to the step (e.g. 0–1000 scales with small steps).
- **Suggested fix:** Build the signature from the integer `llround(x / resolution)` values
  (or the raw bits of the rounded double, with −0 normalised), not from formatted doubles.
  Add tests at large magnitude and for −0.
- **Also affects:** C API `stop_on_cycle` / `signature_resolution`, R/Python termination
  config.

### S02d-005 [P1, confirmed] Two more `std::` distribution / `engine()` sites missed by Q4's Implies list
- **Where:** include/socialchoicelab/competition/initialization.h:71–80
  (`std::normal_distribution` in `random_unit_heading`, used by every 2D Hunter draw via
  adaptation.h:132 and strategy_registry.h:105, 134, 146) and initialization.h:88–92
  (`std::uniform_real_distribution` in `initial_step_size`). Decision: Q4.
- **What:** Q4's Implies lists `initialization.h:55–67` and `step_policy.h:44–59`. These two
  further sites also draw through `prng.engine()` with `std` distributions, so their output
  is not portable across standard libraries. The first one drives every stochastic Hunter
  move in 2D.
- **Failure scenario:** On libc++ and libstdc++, the same seed gives different 2D Hunter
  headings from round 1 onwards, and so different trajectories (logic is unambiguous:
  `std::normal_distribution` is implementation-defined per [rand.dist.general]).
- **Suggested fix:** Route all draws through `PRNG` methods. Pitfall for the fix:
  `PRNG::uniform_real` throws unless min < max (prng.h:86–90), but `validate_step_policy`
  allows `min_step_size == max_step_size` (competition_types.h:136). A plain swap would make
  `random_uniform` with equal bounds throw, so handle min == max explicitly or relax one of
  the two.
- **Also affects:** Seeded expected values in competition tests; the website (stochastic
  Hunters).

### S02d-006 [P1, confirmed] Core engine runs without a `StreamManager`; jitter is then silently dropped
- **Where:** include/socialchoicelab/competition/engine.h:285–291 (accepts
  `stream_manager == nullptr`); step_policy.h:42–48 (`jitter > 0 && stream_manager != nullptr`,
  otherwise exact step); strategy_registry.h:98–102, 127–131, 139–143 (Hunter throws mid-run
  only when a draw is needed). Decision: Q7.
- **What:** Q7 says a competition run must not proceed without a `StreamManager`; its
  Implies cover only `scs_competition_run` and the bindings. The core entry point still
  accepts `nullptr`. Configured jitter is then silently ignored with no error. A 2D Hunter
  instead fails partway through the run, at the first round it needs a random heading.
- **Failure scenario:** `jitter = 0.5`, no StreamManager, 5 rounds. Every step is exactly
  1.0 and there is no error (`$SCR/d07_validation.cpp`, last line: `jitter=0.5, no
  StreamManager: step sizes 0 1 1 1 1 final 1 (no error, no jitter)`).
- **Suggested fix:** Take `StreamManager&` (or throw at entry when it is null) in
  `run_fixed_round_competition` / `run_competition_round`, so the requirement is enforced in
  one place below the C API. Remove the silent jitter fallback.
- **Also affects:** R `make_competition_step_config` docs (utils.R:118–119) say jitter is
  "uniform noise magnitude added to each step after the primary step size is computed". The
  code instead multiplies by (1 + U(−j, j)), only for `kind = "fixed"`. Python does not
  document jitter at all.

### S02d-007 [P2, confirmed] Aggregator takes fixed steps towards the centroid instead of moving to it, so it overshoots and never settles
- **Where:** include/socialchoicelab/competition/strategy_registry.h:149–155; motion.h:39–42.
- **What:** L&S (2012, ch. 3, p. 28; Ch5 `aggregate` sets `xcor`/`ycor` to the supporter
  mean) and Fowler & Laver (2008, p. 70) define Aggregator as setting policy *at* the
  supporters' mean. Here the Aggregator produces a heading towards the mean, and the shared
  step policy decides how far it moves. When the step is larger than the distance to the mean
  it overshoots, and it can oscillate around the mean indefinitely. Position convergence then
  never triggers. L&S's headline all-Aggregator result (convergence to a CVT, ch. 3) cannot
  be reproduced, and no L&S-compatible option exists. The cross-cutting principle asks for
  one "where feasible", and here it is trivially feasible.
- **Failure scenario:** One Aggregator at 0, voters {0.2, 0.3, 0.4}, step 1, convergence
  stop on. Trajectory `0 1 0 1 0 1 …` for 20 rounds, `terminated_early=0`
  (`$SCR/d02_aggregator.cpp`, part b). L&S would be at 0.3 after one round.
- **Suggested fix:** Either let the strategy return a target (and cap the step at the
  distance to it), or add an L&S-compatible "jump to centroid" Aggregator mode. Document the
  default in `competition_design.md`.
- **Also affects:** R/Python/website runs with Aggregators.

### S02d-008 [P2, confirmed] "Final vote/seat shares" do not belong to the "final positions"
- **Where:** include/socialchoicelab/competition/engine.h:355 (`final_competitors` =
  post-move states that still carry pre-move metrics); experiment_runner.h:112–124;
  src/c_api/scs_api.cpp:548–562 (`final_metric_shares_from_trace`).
- **What:** `final_positions` are after the last move, while `final_vote_shares` /
  `final_seat_shares` (trace and experiment, including the experiment means) are the shares
  earned at the previous positions. Nothing documents this, and the getters' names suggest
  the pair matches.
- **Failure scenario:** 1D Predator at −4 vs Sticker at 1, 11 voters, 1 round, step 2.
  Final positions are (−2, 1). Reported final shares are 0.3636/0.6364, but the shares at
  (−2, 1) are 0.4545/0.5455. The experiment run summary reports the same mismatched pair
  (`$SCR/d05_final_shares.cpp`).
- **Suggested fix:** Either evaluate the final positions once more and report those shares,
  or rename/document them as "last evaluated round" shares in the C API, R and Python.
- **Also affects:** `scs_competition_trace_final_*`, `scs_competition_experiment_*final*`,
  R `CompetitionTrace$final_vote_shares()`, Python equivalents.

### S02d-009 [P2, confirmed] Each trace round stores the full profile, copies of every voter point, and unread fields
- **Where:** include/socialchoicelab/competition/election_feedback.h:31–40, 154–161;
  engine.h:19–23, 306.
- **What:** Every `CompetitionRoundRecord` keeps `ElectionFeedback` in full: the
  n_voters × n_competitors `Profile`, `supporter_ideal_points` (one heap-allocated
  `Eigen::VectorXd` copy per voter per round), `supporter_indices` and `leader_ranking`. The
  C API reads none of these (grep of `src/` for `profile`, `supporter_*` and
  `leader_ranking` on trace feedback finds nothing). `leader_ranking` is not read anywhere.
- **Failure scenario:** At the website's caps (5,000 voters, 12 competitors, 500 rounds; Q9
  notes), one trace holds **282 MB**: 120 MB of rankings plus 2.5 M `PointNd` copies. The
  exported data is about 0.24 MB (`$SCR/d06_trace_memory.cpp`). An experiment with
  `retain_traces` multiplies this by `num_runs`, and the website can run up to 40 such
  requests at once.
- **Suggested fix:** Keep only per-competitor vectors in the trace record. Keep the full
  feedback only for the current round, as the strategies need it. Drop `leader_ranking`, or
  compute it on demand.
- **Also affects:** website memory use (Stage 8), R/Python experiments with
  `retain_traces = TRUE`.

### S02d-010 [P2, confirmed] Step and termination validation accepts NaN and ±Inf
- **Where:** include/socialchoicelab/competition/competition_types.h:116–150;
  termination.h:40–63.
- **What:** All checks are ordered comparisons (`x < 0.0`, `jitter > 1.0`, `<= 0.0`), so NaN
  passes all of them and +Inf passes most of them. Downstream effects: a NaN step turns
  positions into NaN, and the run then fails one round later with an unrelated message
  ("alternative_point[0] is not finite"). An infinite step hangs `kReflect` (S02d-003). A NaN
  epsilon disables convergence and no-improvement detection without any warning.
- **Failure scenario:** `$SCR/d07_validation.cpp`: `fixed_step_size=nan accepted`,
  `fixed_step_size=inf accepted`, `jitter=nan accepted`, `termination eps=nan,
  resolution=inf accepted`, `engine with NaN step: engine threw: alternative_point[0] is not
  finite`.
- **Suggested fix:** Require `std::isfinite` on every double field in `validate_step_policy`
  and `validate_termination_config`. Use end-user messages that state the entered value and
  the valid range.
- **Also affects:** C API (no finiteness check on `SCS_CompetitionStepConfig` /
  termination config; see Out-of-slice notes).

### S02d-011 [P2, confirmed] `build_spatial_profile_nd` duplicates `build_spatial_profile` and has already drifted
- **Where:** include/socialchoicelab/competition/election_feedback.h:44–100 vs
  include/socialchoicelab/aggregation/profile.h:96–126.
- **What:** The two functions run the same (distance, index) sort loop, one for `PointNd` and
  one for `Point2d`. They already differ: the competition copy rejects `m == 0` and checks
  dimensions, while the aggregation copy allows `m == 0`. The competition copy also builds
  full rankings (O(n·m log m)), although only `rankings[v][0]` is used. Q7 will change
  competition voter ties to random (per voter, from a stream) while the aggregation builder
  stays index-tied. Keeping the competition path as a copy of the profile builder invites
  further drift.
- **Failure scenario:** A fix applied to one builder (e.g. the Q2 distance change, or a
  validation change) is not applied to the other. Competition and aggregation then rank the
  same voters differently. There is no failing input today; the drift is in validation only.
- **Suggested fix:** Replace it with a competition-specific "nearest competitor per voter"
  routine that owns the Q7 tie rule and returns supporters directly. Alternatively,
  generalise `build_spatial_profile` to `PointNd` and keep one copy.
- **Also affects:** none outside the core.

### S02d-012 [P2, confirmed] Seat ties in competition go to the lowest index
- **Where:** include/socialchoicelab/competition/election_feedback.h:141–150 →
  aggregation/seat_allocation.h:13–22 (top-k: votes, then index) and :107–113 (Hare:
  remainder, votes, then index). Decision: Q7 (extension; owner to confirm scope).
- **What:** Under the `seat_share` objective, a tie for the last seat always goes to the
  lower-index competitor. That feeds Hunter's improvement test and Predator's target. Q7
  forbids lowest-index ties in voter assignment and Predator targeting and cites the
  symmetry requirement (Duggan & Jackson). Seat ties are the same kind of bias, but Q7 does
  not mention them.
- **Failure scenario:** Two co-located competitors with equal votes (after the Q7 voter
  split, an exact tie is likely with an even electorate), `seat_count = 1`, plurality top-k.
  Competitor 0 holds the seat in every tied round, so seat-share Hunters/Predators behave
  asymmetrically.
- **Suggested fix:** Owner to decide whether Q7 extends to seat ties. If so, break exact
  seat ties at random from a named stream (e.g. `competition_seat_ties`) and add it to the
  stream map and the runner allowlist.
- **Also affects:** stage 2c (`seat_allocation.h` contract); website stream list.

### S02d-013 [P3, confirmed] A stationary state is reported as `kCycleDetected`; the signature ignores heading and memory
- **Where:** include/socialchoicelab/competition/engine.h:297–345;
  tests/unit/test_competition_termination.cpp:135–150 (asserts this for two Stickers).
- **What:** The initial signature is in the window, so a run in which nobody moves stops
  with "cycle detected" whenever `stop_on_convergence` is off. Because the signature is
  positions only, a 2D Hunter whose move is blocked (`kStayPut`) while everyone else is
  stationary also reads as a cycle, although its next heading is random.
  `competition_design.md` §Termination calls this "cycle detection".
- **Failure scenario:** All Stickers, `stop_on_cycle` only → `kCycleDetected` after round 0
  (the existing test).
- **Suggested fix:** Report a period-1 repeat as `kConverged` (or a distinct
  `kStationary`), and document that signatures are positions only.
- **Also affects:** R/Python termination reason strings.

### S02d-014 [P3, confirmed] A Predator tied for largest moves; L&S's tied largest party stands still
- **Where:** include/socialchoicelab/competition/adaptation.h:87–108;
  strategy_registry.h:157–162. Decision: Q7, Q8.
- **What:** L&S (ch. 3, p. 29: "if this is you, stand still"; Ch5 `predate`:
  `if (mysize < [mysize] of largest-party)`) never move a Predator that is jointly largest.
  Here the first maximum is the target, so a tied Predator with a higher index chases the
  tied lower-index party. A literal reading of Q7 (uniform random among the equally large)
  would sometimes pick itself and sometimes the other party, which also differs from L&S.
  Neither behaviour is documented.
- **Failure scenario:** Sticker at −3 and Predator at 3, 2 votes each. The Predator moves to
  2 (`$SCR/d08_predator_tie.cpp`: `votes 2 vs 2; Predator moved from 3 to 2`).
- **Suggested fix:** See Open question 2. Document the chosen rule and the L&S difference
  under the Q7/Q8 sections.
- **Also affects:** R/Python/website Predator runs.

### S02d-015 [P3, confirmed] Strategy definitions, their L&S divergences and the citations are not documented
- **Where:** docs/architecture/competition_design.md:5–8, 15, 135–145;
  strategy_registry.h:118–122; adaptation.h:120–126.
- **What:** The design doc names the four strategies but defines none of them. It does not
  say that:
  - Aggregator steps rather than jumps (S02d-007);
  - Hunter in 1D reverses deterministically;
  - Hunter/Predator moves use the shared step policy instead of a "unit move";
  - zero-supporter and tied-leader behaviour (S02d-001, S02d-014).

  Citations disagree: the design doc has "Laver and Sergenti 2011", the code has "Laver
  2012, ch.3", and the PDF in `references/` and Q8 have Laver & Sergenti 2012. The code cites
  "Fowler & Laver 2008, §2", but the rule list is on p. 70. Laver (2005) is cited as the
  primary source but is not in `references/`. `REFERENCE_INDEX.md` has no Laver or Fowler
  entries at all. Q7 Implies covers only the NetLogo models, Duggan, and Duggan & Jackson.
  This is in addition to Q8's documentation items (satisficing, heading/memory, Predator
  variants).
- **Failure scenario:** A user comparing results with L&S's all-Aggregator chapter gets
  non-converging Aggregators and cannot find the reason in the docs.
- **Suggested fix:** Add a "Strategy definitions" section quoting L&S ch. 3 and Fowler &
  Laver p. 70, with a "differences from L&S" list. Unify the citation as Laver & Sergenti
  (2012). Add Laver (2005) to `references/` and the index entries.
- **Also affects:** R/Python `competition_run` docs.

### S02d-016 [P3, confirmed] `initialize_competitors` cannot be reached from any binding, and its stream use is undocumented
- **Where:** include/socialchoicelab/competition/initialization.h:144–197;
  competition_design.md:26–36, 147–154.
- **What:**
  - `initialize_competitors` and the three `competition_init_*` streams are used only by
    tests. The C API builds competitors directly (scs_api.cpp:486–503), and the experiment
    runner's allowlist omits these streams. Yet the stream map lists all five streams as
    "used by Layer 7".
  - `value_or(random_…())` evaluates its argument eagerly (:163–164, :177–178), so position
    and heading draws are consumed even for competitors with supplied values. This keeps
    streams aligned, which may be intended, but it is undocumented.
  - `kZero` mode passes a supplied heading through un-normalised (:173–175), unlike the
    other modes (harmless: motion normalises).
- **Failure scenario:** A reader of the stream map registers the init streams for a
  binding run and expects them to be used; they are never drawn.
- **Suggested fix:** Mark the init streams as "C++ only (planned per-run re-randomisation)"
  in the stream map. Document the eager draws (or make them explicit). Normalise in `kZero`
  mode.
- **Also affects:** website stream list (consistent today).

### S02d-017 [P3, confirmed] `StrategyRegistry::find(std::string_view)` is `noexcept` but allocates; registry pointers are non-owning
- **Where:** include/socialchoicelab/competition/strategy_registry.h:54–58 (clang-tidy
  `bugprone-exception-escape`), :29–46.
- **What:** `std::string(stable_id)` can throw `bad_alloc` inside a `noexcept` function,
  which calls `std::terminate`. Only tests call it. `register_strategy` stores raw pointers
  without documenting that the service must outlive the registry.
- **Failure scenario:** Out of memory during `find("hunter")` → `std::terminate`. Registering
  a temporary service → dangling pointer on the next `adapt`.
- **Suggested fix:** Drop `noexcept`, or use heterogeneous lookup
  (`std::unordered_map<std::string, …, transparent hash, std::equal_to<>>`). Document the
  lifetime requirement.
- **Also affects:** none.

### S02d-018 [P3, confirmed] `seat_count` is not checked against the number of active competitors up front
- **Where:** include/socialchoicelab/competition/engine.h:103–147 (no check);
  election_feedback.h:129–150.
- **What:** With `kPluralityTopK`, `seat_count` larger than the number of active competitors
  throws from inside round 0, with a message that counts "competitors" (meaning the active
  ones). `seat_count = 0` is accepted and makes every seat share 0.
- **Failure scenario:** 2 competitors, one inactive, `seat_count = 2` → `plurality_top_k_seat_allocation:
  seat_count must not exceed the number of competitors.` (`$SCR/d08_predator_tie.cpp`).
- **Suggested fix:** Validate `seat_count` (≥ 1 and ≤ active count for top-k) in
  `validate_engine_inputs`, with an end-user message giving both numbers.
- **Also affects:** C API error text.

## Tool leads checked

- clang-tidy `bugprone-exception-escape` strategy_registry.h:54 → S02d-017. Real, but only on
  out-of-memory.
- clang-tidy `bugprone-unchecked-optional-access` initialization.h:167 → false positive.
  `validate_initialization_config` (called at :147) throws at :114–118 when a
  user-specified position is missing.
- clang-tidy `bugprone-unchecked-optional-access` initialization.h:181 → false positive
  (same check at :124–128 for headings).
- Coverage `termination.h` 60% → correct but untested: the `validate_termination_config`
  throw branches and the `max_position_delta` size/dimension throws. Stage 4 should add the
  S02d-004 and S02d-013 cases.
- Coverage `competition_types.h` 64% → correct but untested: `stable_strategy_id` /
  `strategy_kind_from_stable_id` for hunter/aggregator, and `validate_step_policy` error
  branches. Missing NaN/Inf checks → S02d-010.
- Coverage `adaptation.h` 71% / `strategy_registry.h` 74% → correct but untested: Hunter
  fallback branches (zero heading after a loss), the `leader_index` missing-metrics throw,
  and `kHunterSticker` (to be removed under Q8). The zero-supporter Aggregator branch is
  untested and wrong → S02d-001.
- Stage 1 hotspot `build_spatial_profile_nd` duplicates `build_spatial_profile` → S02d-011.
- Stage 1 hotspot direct `engine()` use (`step_policy.h:46`) → known (Q4). More sites →
  S02d-005.
- Prompt lead "termination/state signatures and floating-point comparison" → S02d-004
  (early stop), S02d-013. Position convergence and Hunter's improvement test compare exactly
  copied or identically computed doubles: no defect.
- Prompt lead "experiment runner: per-run seeding, allowlist, aggregation" → seeding correct
  (run i can be replayed alone; reported `run_seed` equals the seed used). Allowlist missing
  the Q7 tie streams is known (Q7). Aggregation pairs pre-move shares with post-move
  positions → S02d-008.
- Prompt lead "strategies vs L&S (2012) and Laver (2005)" → Hunter matches L&S/F&L. Aggregator
  → S02d-001, S02d-007. Predator → S02d-014. Docs → S02d-015. Laver (2005) not available.

## Open questions

1. **Q10 (unanswered): competition bounds.** The core requires explicit bounds and
   enforces them through the boundary policy, so bounds change results. The C API always
   uses bbox(initial competitors ∪ voters) ± 1.0 in absolute units (scs_api.cpp:4083–4103,
   duplicated at :4378–4396). On a 0–1000 scale the margin is negligible. On a 0–0.01 scale
   it is 100× the data range, so boundary policies rarely bind. R/Python cannot set bounds.
   `initialize_competitors` uses bounds for random starts, but no binding reaches it.
   Intended behaviour is unknown.
2. **Predator tied for largest** (S02d-014): should a jointly-largest Predator stand still
   (L&S code), or choose uniformly among the tied largest including itself (literal Q7)?
3. **Reflect and heading:** `kReflect` reflects the position but keeps the outward heading,
   so an improving Hunter "repeats the same move" back into the wall each round. Should the
   reflected heading component be flipped?
4. **Seat ties** (S02d-012): does Q7's random tie rule extend to seat allocation inside
   competition?
5. **Sweeps and seeds:** scenarios with the same `master_seed` reuse identical streams
   (common random numbers). Is that intended, and should it be documented?
6. **Share-delta step** (S02d-002): was the intended quantity the own-share change or the
   gap to the leader ("share-gap-proportional", competition_plan.md Phase D)?
7. **Hunter "same move" under random step sizes:** L&S's Hunter repeats the same move. With
   `random_uniform` steps, a new step length is drawn every round. Should a Hunter that
   improved reuse its last step length?

## Out-of-slice notes

- `scs_competition_run_experiment` (scs_api.cpp:4354–4420) does not call
  `validate_finite_doubles` on positions, headings or voters, unlike `scs_competition_run`
  (:4068–4076). Stage 3.
- The C API does not check that `SCS_CompetitionStepConfig` or termination doubles are
  finite (scs_api.cpp:357–380, 448–461). `to_cpp_termination_config` silently replaces
  `signature_resolution <= 0` with 1e-6 where the core would throw. Stage 3.
- Bounds inference is copy-pasted between `scs_competition_run` and
  `scs_competition_run_experiment` (scs_api.cpp:4083–4103, :4378–4396). Stage 3.
- R `make_competition_step_config` jitter doc is wrong, and Python does not document jitter
  (see S02d-006). Stages 5/6.
- `hare_largest_remainder_allocation` compares double remainders with `!=` for ties
  (seat_allocation.h:110). Stage 2c.
