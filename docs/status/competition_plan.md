# Competition Plan

Roadmap for adding Layer 7 multi-candidate electoral competition to SocialChoiceLab.

This plan is based on the current project architecture, milestone-gate style, the deprecated `voteR` implementation, and the main competition references: Laver and Sergenti's party-competition ABM baseline, Fowler and Laver's tournament framework, and related work on convergence, centrifugal/centripetal incentives, and multicandidate equilibrium.

## Implementation status (2026-03-09)

This roadmap began as a forward plan. It now also needs to reflect the fact
that most of the baseline Layer 7 stack is already implemented locally.

### Largely implemented

- Phase 0: 1D compatibility audit and test backfill
- Phase A: competition domain model and initialization
- Phase B: electoral evaluation bridge and seat allocation service
- Phase C: adaptation strategy framework and baseline strategies
- Phase D: step-size policies and boundary handling
- Phase E: stateful competition engine and trace recording
- Phase F: termination, convergence, and cycle diagnostics
- Phase G: competition C API
- Phase H: R and Python bindings
- Phase I: experiment runner baseline
- Phase J: first static and animated trajectory plotting

### Complete

- R animation jump bug: fixed.
- Animation UX refinement: complete (trail toggles, fade-trail behavior, layout polish).
- R/Python parity: audited and fixed (`jitter` gap, `n_runs` rename).

### Practical next step

All Layer 7 implementation work is done. The remaining work is the **`0.3.0`
release gate**: citation verification in `competition_design.md`, CI green
check, and API stability declaration.

### Planned engine extension: per-run position re-randomization

The experiment runner currently uses the same `initial_competitors` for every
run, so only RNG streams vary across runs. This is appropriate for studying how
strategy stochasticity affects outcomes from a fixed starting configuration, but
it does not capture sensitivity to initial conditions.

**Feature:** Add an optional `InitializationConfig` field to
`CompetitionExperimentConfig`. When present, the runner re-initializes
competitor positions (and headings and step sizes) from that config for each
run, using the run's RNG stream for the random draws. When absent, the existing
`initial_competitors` field governs as today — so the change is fully backward
compatible.

**Sequencing:** Implement after the R animation bug and animation UX
refinements are resolved. This is an engine feature, so it touches
`experiment_runner.h`, the C API (`scs_api.h` / `scs_api.cpp`), and the
R/Python experiment wrappers. Do not implement until the animation layer is
stable.

## Scope and sequencing

This roadmap departs slightly from the user-suggested phase order in three places:

- It puts **electoral evaluation and seat allocation before adaptation strategies**, because Hunter/Aggregator/Predator all need a clean, tested notion of round feedback before their behavior can be implemented correctly.
- It puts **all public C API work after the core C++ engine stabilizes**, to avoid freezing an ABI around churn-prone internal types.
- It treats **visualization as the last implementation phase**, because the existing Plotly layer should consume a stable exported trace format rather than drive core design.

## Recommended design decisions

### 1. Internal dimensionality vs public API

Recommendation: make the C++ core explicitly support **1D and 2D**, but ship a **2D-first public API** in the first milestone.

- Why: 1D competition is theoretically important, simpler to benchmark, and should be supported in core logic from the start; current geometry and plotting infrastructure are still primarily 2D-centered at the public boundary.
- Consequence: the core should not bake in 2D assumptions where 1D has a natural interpretation, while the first `scs_api.h` entry points can still be `*_2d` to match existing conventions.
- Constraint: docs and API names should not imply that ND competition is already supported end-to-end just because some internal types are dimension-ready; similarly, a 2D-first public API must not be mistaken for a 2D-only core.
- Follow-on: add 1D wrappers once the 1D-capable core is stable and the 2D API/bindings are in place.

### 2. Synchronous vs asynchronous adaptation

Recommendation: use **synchronous round updates** by default.

- Why: this matches the Laver-style interpretation that all competitors observe the same round outcome and then move in response to that shared feedback.
- Consequence: round `t` vote/seat outcomes are computed from positions at `t`, all next moves are derived from that same snapshot, and positions at `t+1` are committed together.
- Follow-on: asynchronous or randomized update orders can be added later as alternative engine modes.

### 3. Vote rule vs seat rule

Recommendation: model these as **separate components**.

- `vote_rule`: how votes/scores are computed from a profile.
- `seat_rule`: how vote totals/scores are turned into seats.

Why: plurality and PR are not the same kind of operation. The engine should reuse Layer 5 profile/rule logic where that fits cleanly, then apply a reusable seat-allocation service for turning round vote totals into seats. This also keeps the architecture open to later combinations such as approval voting with PR seat conversion.

Clarification: Layer 5 currently provides ordinal-profile construction and social-choice rules, not a full multi-seat competition layer. The competition engine therefore needs a small additional bridge from spatial round state to vote totals/supporter sets, plus a dedicated seat-allocation service for electoral competition.

### 4. Equilibrium detection

Recommendation: implement **convergence detection**, **cycle detection**, and an **approximate local-equilibrium diagnostic**, not an exact continuous-space Nash solver.

- Why: exact Nash detection in multidimensional multicandidate competition is not a tractable first competition-release deliverable, and the literature on equilibrium with strategic voting highlights multiplicity rather than simple exact detection.
- Consequence: the engine should expose a bounded deviation test such as "no sampled unilateral deviation improves the competitor's objective by more than epsilon."

### 5. Result shape

Recommendation: store a **full competition trace handle** in core/API layers.

- Why: visualization, diagnostics, and experiment summaries all need round histories, not just terminal states.
- Consequence: use an opaque result/trace handle in the C API with size-query export functions for positions, votes, seats, and derived metrics.

## Source-driven requirements

- The baseline strategy set must include Sticker, Hunter, Aggregator, and Predator, with an extension path for future heuristics (Laver and Sergenti 2012).
- The engine must be able to reproduce fixed-party-count baseline runs before any birth/death or tournament mechanics are attempted (Laver and Sergenti 2012; Fowler and Laver 2008).
- The architecture should remain open to tournament-style or evolutionary decision-rule competitions without rewriting the engine (Fowler and Laver 2008).
- The simulation must use named RNG streams and deterministic run-level seeding per `StreamManager`.
- Every binding-visible feature must land through `scs_api.h` first.
- The first competition release should state its electoral formulas precisely: plurality seat assignment and one named PR seat-conversion rule, not an undifferentiated claim of supporting "PR" in general.
- The core competition logic should support **1D and 2D** from the outset; where public wrappers are 2D-first, 1D support must still be preserved and tested internally.

## Phase 0: 1D compatibility audit and test backfill

**Goal**

Audit the existing codebase for 1D compatibility in the components the competition layer will depend on, and add missing 1D tests wherever current 2D-heavy coverage is not enough to protect later competition work.

**Prerequisites**

- None beyond the current repository state.

**Deliverables**

- Read-through and compatibility notes for the Layer 1-5 components that the competition engine will rely on:
  - PRNG / `StreamManager`
  - distance / loss infrastructure
  - profile construction and aggregation rules
  - any geometry/centrality helpers intended for competition metrics
- New or expanded tests in existing suites where 1D behavior is material and not already covered.
- `docs/architecture/competition_design.md`
  - short note documenting what the project already supports in 1D before Layer 7 work begins, and which Layer 7 components are required to preserve that support.

**C API surface**

- None. This is a read-through and test-backfill phase.

**Tests**

- Add 1D tests for existing dependencies where competition will rely on them and current 2D tests are insufficient.
- Prefer targeted additions to existing test files rather than inventing a new “1D compatibility” abstraction.
- Minimum expectation:
  - ranking/profile behavior in 1D where ties and left/right ordering matter
  - distance/loss combinations used by competition feedback
  - any movement/math helpers introduced early for Layer 7 should be designed with 1D cases in mind before 2D wrappers are added

**Milestone gate**

- Features: the dependency audit is complete and documents which existing components are already safe for 1D use.
- Tests: missing 1D coverage needed by Layer 7 has been added before competition implementation begins.
- Docs: `competition_design.md` records the 1D/2D support stance for Layer 7.
- API stability: N/A — no new public API in this phase.

## Phase A: Competition domain model and initialization

**Goal**

Establish the core Layer 7 types, validation rules, and deterministic initialization pipeline so later phases can build on strong, testable contracts instead of ad hoc data frames, while preserving 1D and 2D support in the core.

**Prerequisites**

- Phase 0 complete.
- Layers 1-5 complete.
- `scs_api` patterns and `StreamManager` conventions stable.

**Deliverables**

- `include/socialchoicelab/simulation/competition/competitor.h`
  - `Competitor`, `CompetitorType`, `CompetitorObjective`, and immutable competitor IDs.
- `include/socialchoicelab/simulation/competition/competition_config.h`
  - global engine config, initialization modes, round limits, dimension/bounds metadata for 1D or 2D runs.
- `include/socialchoicelab/simulation/competition/initialization.h`
  - fixed-position initialization, random initialization, and validation helpers for 1D and 2D competitor sets.
- `include/socialchoicelab/simulation/competition/competition_state.h`
  - per-round state snapshot with positions, headings, step sizes, vote totals, seat totals, and termination metadata.
- `docs/architecture/competition_design.md`
  - Layer 7 design document, stream map, terminology, and citations.

**C API surface**

- None in this phase. Keep the work C++-only until the core types stop moving.

**Tests**

- `tests/unit/test_competitor.cpp`
  - validates IDs, type/objective enums, bounds, and dimensional consistency.
- `tests/unit/test_competition_initialization.cpp`
  - verifies fixed and random initialization in 1D and 2D, reproducibility under a named stream, and failure on invalid bounds or duplicate IDs.

**Milestone gate**

- Features: typed competitor/config/state model exists; initialization supports fixed and random competitor placement.
- Tests: new unit tests pass; deterministic initialization is reproducible with the same seed and stream map.
- Docs: `competition_design.md` exists and defines domain vocabulary and stream responsibilities.
- API stability: internal-only; no public ABI promised yet.

## Phase B: Electoral evaluation bridge and seat allocation service

**Goal**

Create the stateless "round evaluation" layer that turns voter/candidate positions into vote totals, vote shares, seat allocations, and supporter assignments using existing aggregation infrastructure wherever possible, without hard-coding 2D assumptions into core competition feedback.

**Prerequisites**

- Phase A complete.

**Deliverables**

- `include/socialchoicelab/simulation/competition/election_feedback.h`
  - builds the round-level evaluation bridge from voter ideals and competitor positions in 1D or 2D;
  - computes vote totals, shares, supporter sets, and leader rankings;
  - uses Layer 5 profile/ranking logic where appropriate, but owns the competition-specific mapping from spatial state to round feedback.
- `include/socialchoicelab/aggregation/seat_allocation.h`
  - new reusable stateless seat-allocation helpers:
    - plurality top-`k` seat allocation;
    - proportional allocation by **Hare largest remainder** for the first PR milestone.
- `tests/unit/test_seat_allocation.cpp`
  - aggregation-level tests for plurality and PR seat conversion.
- `tests/unit/test_competition_election_feedback.cpp`
  - end-to-end feedback tests from positions -> profile -> votes -> seats in both 1D and 2D where behavior differs materially.

**C API surface**

- None in this phase; the service remains internal until the trace format is settled.

**Tests**

- Verify plurality vote totals are delegated through existing profile/rule logic rather than recomputed separately.
- Verify PR seat totals preserve seat count and behave correctly under exact ties and zero-vote competitors.
- Verify supporter assignment needed by Aggregator is stable and well-defined under ties.
- Verify objective metrics can be computed by vote share or seat share.
- Verify 1D fixtures where left/right ordering and exact midpoint ties could mask bugs that 2D tests would not catch.

**Milestone gate**

- Features: a stateless round-evaluation service exists; plurality and Hare-largest-remainder PR seat conversion are both implemented and reusable.
- Tests: vote, share, and seat outputs match hand-worked toy examples and deterministic tie-break cases.
- Docs: `competition_design.md` documents the vote-rule/seat-rule split and names Hare largest remainder as the initial PR formula rather than implying a generic PR layer.
- API stability: internal-only; no public ABI promised yet.

## Phase C: Adaptation strategy framework and baseline strategies

**Goal**

Implement a pluggable adaptation framework with the four baseline Laver strategies: Sticker, Hunter, Aggregator, and Predator, with explicit 1D and 2D interpretations in core logic.

**Prerequisites**

- Phase B complete.

**Deliverables**

- `include/socialchoicelab/simulation/competition/adaptation_strategy.h`
  - strategy interface and adaptation context.
- `include/socialchoicelab/simulation/competition/strategies/sticker.h`
- `include/socialchoicelab/simulation/competition/strategies/hunter.h`
- `include/socialchoicelab/simulation/competition/strategies/aggregator.h`
- `include/socialchoicelab/simulation/competition/strategies/predator.h`
- `include/socialchoicelab/simulation/competition/strategy_factory.h`
  - maps enum/config inputs to strategy objects without R/Python involvement.

**C API surface**

- None in this phase; strategy enums are added when the C API is frozen in Phase G.

**Tests**

- `tests/unit/test_competition_strategies.cpp`
  - Sticker never moves.
  - Hunter keeps heading after improvement and redraws heading after non-improvement in 1D and 2D.
  - Aggregator moves toward the weighted mean of current supporters in 1D and 2D.
  - Predator moves toward the current leader under the chosen objective metric in 1D and 2D.
- Seeded tests confirm random Hunter behavior is reproducible through named streams.
- Mixed-strategy tests confirm the factory can host heterogeneous competitor types in the same run.

**Milestone gate**

- Features: all four baseline strategies are implemented and selectable per competitor.
- Tests: each strategy passes isolated behavior tests and mixed-population tests.
- Docs: `competition_design.md` cites and defines each strategy, including objective semantics.
- API stability: internal-only; strategy interface stable enough for engine integration, but still not public.

## Phase D: Step-size policies and boundary handling

**Goal**

Separate movement magnitude and feasible-space handling from strategy logic so strategies choose directions/objectives while shared motion policies handle geometry consistently in both 1D and 2D.

**Prerequisites**

- Phase C complete.

**Deliverables**

- `include/socialchoicelab/simulation/competition/step_policy.h`
  - fixed step;
  - share-gap-proportional step;
  - share-gap-random step.
- `include/socialchoicelab/simulation/competition/boundary_policy.h`
  - project-to-box;
  - stay-put;
  - reflect (optional in the first competition release if time allows, but the interface should allow it).
- `include/socialchoicelab/simulation/competition/motion.h`
  - shared movement helpers and heading updates, with explicit 1D left/right semantics and 2D angle/vector semantics.

**C API surface**

- None in this phase.

**Tests**

- `tests/unit/test_competition_motion.cpp`
  - verifies motion length, deterministic projection, and boundary clipping/projection in 1D and 2D.
- `tests/unit/test_step_policy.cpp`
  - verifies fixed and share-dependent steps, including zero-share, leader-share, and minimum-step cases.
- 1D edge tests are required, not optional, because fixed-axis competition is part of the intended core support surface.

**Milestone gate**

- Features: strategies no longer own ad hoc motion math; step and boundary behavior are centrally configurable.
- Tests: movement remains deterministic under fixed seeds; every boundary policy has direct unit coverage.
- Docs: default step and boundary policies are documented with trade-offs.
- API stability: internal-only.

## Phase E: Stateful competition engine and trace recording

**Goal**

Build the actual Layer 7 engine: initialize -> evaluate -> adapt -> move -> record -> terminate.

**Prerequisites**

- Phases A-D complete.

**Deliverables**

- `include/socialchoicelab/simulation/competition/competition_engine.h`
  - one-run stateful engine API for 1D and 2D core runs.
- `include/socialchoicelab/simulation/competition/competition_trace.h`
  - full round history for analysis and later FFI export.
- `include/socialchoicelab/simulation/competition/voter_sources.h`
  - fixed voters and generated voters (uniform and Gaussian at minimum, using named streams).
- `include/socialchoicelab/simulation/competition/round_metrics.h`
  - per-round derived measures: vote shares, seat shares, leader IDs, support centroids, plus 1D/2D-appropriate central-reference metrics.
- `tests/unit/test_competition_engine.cpp`
  - end-to-end engine tests.

Note: the first engine milestone should target **fixed-party-count** competition only. Party birth/death, survival thresholds, and tournament-style rule churn remain later extensions even though the architecture should leave room for them.

**C API surface**

- None yet. Phase E is the point where the internal trace contract should be frozen, but public wrappers wait for Phase G.

**Tests**

- Deterministic end-to-end runs for:
  - 1D all-Sticker;
  - 1D all-Aggregator;
  - 1D all-Hunter;
  - all-Sticker;
  - all-Aggregator;
  - all-Hunter;
  - mixed populations.
- Tests that fixed voters remain fixed across rounds and generated-voter modes consume only the documented voter stream.
- Tests that synchronous updates use round-`t` feedback for all competitors before any round-`t+1` positions are committed.
- Tests that recorded traces contain every round, including round 0 initialization.
- Tests that 1D runs do not depend on 2D-only metrics being present.

**Milestone gate**

- Features: one full competition run can be executed in C++ and yields a complete trace.
- Tests: end-to-end deterministic fixtures pass, including mixed strategies and both electoral formulas.
- Docs: `competition_design.md` documents the round order and trace schema.
- API stability: internal trace schema frozen for the upcoming C API phase.

## Phase F: Termination, convergence, equilibrium, and cycle diagnostics

**Goal**

Add principled stopping conditions and diagnostics so the engine can support both fixed-horizon simulations and theory-oriented stopping rules.

**Prerequisites**

- Phase E complete.

**Deliverables**

- `include/socialchoicelab/simulation/competition/termination.h`
  - fixed-round termination;
  - geometric convergence by position/share epsilon;
  - no-improvement windows;
  - approximate cycle detection via rounded state signatures;
  - approximate local-equilibrium probe sets.
- `include/socialchoicelab/simulation/competition/state_signature.h`
  - canonicalized state summaries for cycle detection.
- `tests/unit/test_competition_termination.cpp`

**C API surface**

- None in this phase; termination diagnostics become public with the trace handle in Phase G.

**Tests**

- Convergence detection on deterministic Aggregator fixtures.
- Non-convergence / cycling detection on seeded Hunter fixtures.
- Approximate local-equilibrium tests showing "no profitable sampled deviation" in simple stable toy cases.
- Regression tests ensuring termination checks do not consume behavioral RNG streams.

**Milestone gate**

- Features: the engine supports fixed-horizon and diagnostic-driven termination.
- Tests: convergence and cycle detectors behave predictably on seeded fixtures.
- Docs: diagnostics are clearly labeled as exact or approximate; local Nash checks are explicitly described as heuristic.
- API stability: internal diagnostics frozen for FFI exposure.

## Phase G: C API surface for competition runs and trace export

**Goal**

Expose Layer 7 through the stable C ABI using the same patterns already used for StreamManager, Winset, and Profile handles. First public wrappers remain 2D-first even though the core is intended to support 1D as well.

**Prerequisites**

- Phases A-F complete and internally stable.

**Deliverables**

- `include/scs_api.h`
  - new enums and POD structs:
    - `SCS_CompetitionStrategy`
    - `SCS_CompetitionObjective`
    - `SCS_CompetitionVoteRule`
    - `SCS_CompetitionSeatRule`
    - `SCS_CompetitionStepPolicy`
    - `SCS_CompetitionBoundaryPolicy`
    - `SCS_CompetitionTerminationReason`
    - `SCS_CompetitorSpec2d`
    - `SCS_CompetitionConfig2d`
    - `SCS_CompetitionTerminationConfig`
  - new opaque handle:
    - `SCS_CompetitionTrace*`
- `src/c_api/scs_api.cpp`
  - wrapper implementations and error translation.
- `tests/unit/test_c_api.cpp`
  - new competition coverage.
- `docs/architecture/c_api_design.md`
  - competition section, lifecycle contracts, size-query examples.

**C API surface**

- Run entry points:
  - `scs_competition_run_fixed_voters_2d(...)`
  - `scs_competition_run_uniform_voters_2d(...)`
  - `scs_competition_run_gaussian_voters_2d(...)`
- Trace lifecycle:
  - `scs_competition_trace_destroy(...)`
  - `scs_competition_trace_dims(...)`
  - `scs_competition_trace_round_count(...)`
  - `scs_competition_trace_termination(...)`
- Trace export/query helpers with size-query patterns:
  - `scs_competition_trace_positions_2d(...)`
  - `scs_competition_trace_vote_totals(...)`
  - `scs_competition_trace_vote_shares(...)`
  - `scs_competition_trace_seat_totals(...)`
  - `scs_competition_trace_round_metrics(...)`

**Tests**

- Null-handle and invalid-argument coverage.
- Buffer-too-small coverage for trace export functions.
- Reproducibility tests proving identical seeds and stream registrations reproduce identical traces.
- Tests proving unknown stream names are rejected cleanly when allowlists are active.

**Milestone gate**

- Features: a stable C ABI exists for running competition simulations and exporting traces.
- Tests: `test_c_api.cpp` covers success paths, failure paths, and size-query contracts.
- Docs: `c_api_design.md` documents every new type and function, including ownership and error semantics.
- API stability: competition C API frozen for the milestone tag.

## Phase H: R and Python bindings

**Goal**

Expose the competition engine in both language packages with interfaces consistent with the existing binding style.

**Prerequisites**

- Phase G complete.

**Deliverables**

- R:
  - `r/src/scs_bindings.c`
    - `.Call()` wrappers for new competition C API functions.
  - `r/R/competition.R`
    - `CompetitionTrace` R6 wrapper and high-level run/export helpers.
  - `r/tests/testthat/test-competition.R`
- Python:
  - `python/src/socialchoicelab/_loader.py`
    - add competition declarations.
  - `python/src/socialchoicelab/_competition.py`
    - `CompetitionTrace` wrapper and run/export helpers.
  - `python/tests/test_competition.py`

**C API surface**

- No new C API functions unless binding ergonomics uncover a real gap. Prefer using the Phase G surface unchanged.

**Tests**

- Binding-level smoke tests for running fixed-voter and generated-voter simulations.
- Export tests ensuring trace matrices/data frames have the expected shapes and index conventions.
- Error translation tests verifying C API failures become idiomatic R/Python errors.

**Milestone gate**

- Features: R and Python can both run competition simulations and inspect traces.
- Tests: binding test suites pass in CI.
- Docs: user-facing binding docs and examples exist in both languages.
- API stability: binding APIs stable for the milestone tag; breaking changes require a documented migration note.

## Phase I: Experiment runner and reproducible parameter sweeps

**Goal**

Add a stateful experiment runner for replicated sweeps over voters, competitors, rules, seats, and strategy mixtures, with serial/parallel equivalence under deterministic seeds.

**Prerequisites**

- Phases A-H complete for single-run functionality.

**Deliverables**

- `include/socialchoicelab/simulation/competition/experiment_runner.h`
  - run grids, replication plans, and per-run seed derivation.
- `include/socialchoicelab/simulation/competition/experiment_result.h`
  - summary tables and optional retained trace subsets.
- `tests/unit/test_competition_experiment_runner.cpp`
- Binding-facing helpers:
  - `r/R/competition_experiments.R`
  - `python/src/socialchoicelab/_competition_experiments.py`

**C API surface**

- Add only if needed after the core experiment contract settles:
  - `SCS_CompetitionExperimentResult*`
  - run/query/destroy functions using the same handle pattern as traces.

**Tests**

- Serial vs parallel replication equivalence under identical master seeds and run indices.
- Tests for sweep expansion, retained metrics, and optional trace sampling.
- Tests that analysis-only randomness uses separate streams and cannot perturb behavioral outcomes.

**Milestone gate**

- Features: parameter sweeps and replicated experiments run reproducibly and can summarize outcomes without hand-written loops in bindings.
- Tests: serial/parallel equivalence and run-index replay are proven by tests.
- Docs: experiment seeding hierarchy and stream assignments are documented.
- API stability: experiment config/result contracts stable if a C API is added.

## Phase J: Visualization and trajectory animation

**Goal**

Expose animated competition trajectories through the existing Plotly-based visualization layer in R and Python.

**Prerequisites**

- Phase H complete for single-run traces.
- Phase I complete if experiment-summary visualizations are included in the same milestone.

**Deliverables**

- R:
  - `r/R/competition_plots.R`
    - `plot_competition_trace()`
    - `animate_competition_trace()`
    - optional summary plots for sweeps.
- Python:
  - `python/src/socialchoicelab/competition_plots.py`
    - matching API.
- Tests:
  - `r/tests/testthat/test-competition-plots.R`
  - `python/tests/test_competition_plots.py`
- Documentation/examples showing:
  - static trajectories;
  - round sliders/frames;
  - overlays with voters, centroid, marginal median, and optional geometry layers.

**C API surface**

- None. Visualization should consume exported trace data from Phase G/H.

**Tests**

- Plot object construction tests.
- Frame-count and trace-shape tests.
- Theme and label consistency tests across R and Python.

**Milestone gate**

- Features: users can animate party trajectories and inspect round-by-round outcomes in both bindings.
- Tests: plotting tests pass in both languages.
- Docs: examples demonstrate baseline Laver-style runs and mixed-strategy runs.
- API stability: plotting API stable for the milestone tag.

## Phase K: Documentation, citations, and release hardening

**Goal**

Finish the competition layer to the same standard as earlier milestones: complete docs, checked citations, and explicit milestone criteria.

**Prerequisites**

- Phases A-J complete.

**Deliverables**

- `docs/architecture/competition_design.md`
  - finalized with citations and extension notes.
- `docs/status/milestone_gates.md`
  - add a new competition milestone entry.
- `docs/status/where_we_are.md`
  - update current/next state after implementation.
- Reference validation pass for:
  - Laver and Sergenti (2012)
  - Fowler and Laver (2008)
  - Gallego and Schofield (2016)
  - Calvo and Hellwig (2011)
  - Patty, Snyder, and Ting (2008)
  - Merrill and Adams (2002), where applicable to narrative claims.

**C API surface**

- None, except narrowly additive fixes discovered during documentation or release review.

**Tests**

- Full CI green across C++, C API, R, and Python.
- One end-to-end regression test reproducing a published baseline scenario configuration.
- One regression test ensuring the same run can be replayed exactly from stored metadata.

**Milestone gate**

- Features: Layer 7 competition engine, experiment runner, and bindings are complete for the documented scope.
- Tests: full CI green; baseline reproduction and replay tests pass.
- Docs: design docs, user docs, milestone gates, and citations are complete and verified.
- API stability: semantic-versioning expectations are clear for the new competition surface.

## Recommended file layout

Recommended new C++ subtree:

- `include/socialchoicelab/simulation/competition/`
- `tests/unit/test_competition_*.cpp`

Recommended binding files:

- R:
  - `r/R/competition*.R`
- Python:
  - `python/src/socialchoicelab/_competition*.py`

This keeps Layer 7 clearly separate from Layer 5 aggregation and avoids embedding stateful engine code inside geometry or aggregation headers.

## Open follow-on extensions after the baseline roadmap

- Endogenous party birth/death and survival thresholds from the Fowler-Laver tournament framework.
- Alternative tournament/evolutionary strategy ecosystems.
- Additional electoral formulas such as D'Hondt, Sainte-Lague, STV-like experimental adapters, and mixed-member abstractions where theoretically justified.
- Strategic-voting or stochastic-choice voter models, which likely need a separate voter-behavior module rather than being folded into the baseline competition engine.
- Valence-augmented competition and convergence-coefficient diagnostics informed by the Schofield/Gallego and Calvo/Hellwig literatures.

## Bottom-line implementation order

1. Domain types and initialization.
2. Electoral evaluation and seat allocation bridge.
3. Baseline adaptation strategies.
4. Motion policies.
5. Stateful engine and trace recording.
6. Termination and diagnostic logic.
7. Stable C API.
8. R and Python bindings.
9. Experiment runner.
10. Visualization.
11. Documentation and milestone hardening.

That order gives SocialChoiceLab a clean Layer 7 foundation that matches the existing architecture, reproduces the core Laver heuristics, and stays extensible enough for later tournament, strategic-voting, and valence-based competition models.
