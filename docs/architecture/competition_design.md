# Competition Design

Design notes for Layer 7 multi-candidate electoral competition.

## Current status (2026-03-09)

The baseline Layer 7 stack is now substantially implemented locally:

- competition core engine
- strategy layer
- step and boundary policies
- election feedback and seat conversion
- experiment runner
- competition C API
- R and Python bindings
- first static and animated trajectory plots

The main open problem is now in the **R animation layer**, not in the
competition engine:

- the underlying round positions are correct;
- Python animation behaves correctly;
- R animation can still show oversized first jumps in some runs.

Once that bug is resolved, the next work item is **further animation
refinement**: trail toggles, fade tuning, layout polish, and remaining
R/Python parity cleanup.

This document starts with the **Phase 0 audit**: what the existing codebase
already supports that Layer 7 can safely build on, and where competition code
must avoid making stronger claims than the current stack justifies.

## Phase 0: 1D compatibility audit

### Goal

Establish whether the pre-Layer-7 dependency stack is sufficient for **1D
competition work** before implementation begins.

### Findings

#### Safe to build on

- **PRNG / StreamManager**
  - Dimension-agnostic. No 1D-specific risk.
  - Suitable for named-stream competition simulations.

- **Distance and loss infrastructure**
  - Already supports 1D naturally through the core vector/distance APIs.
  - Existing tests cover 1D distance behavior and 1D level-set behavior.

- **Indifference / level sets**
  - Real 1D support already exists via `level_set_1d`.
  - This is useful for validating threshold and utility behavior that future
    competition feedback may depend on.

#### Sufficient for Phase 0, but not 1D-native at the public type level

- **Aggregation / profile construction**
  - Current spatial profile construction is written around `Point2d`, not a
    dedicated 1D point type.
  - However, 1D preference-ordering behavior can already be represented
    faithfully by embedding 1D points on the x-axis with `y = 0`.
  - Phase 0 tests therefore confirm that the current dependency stack behaves
    correctly for line-based competition fixtures, even though the APIs are not
    yet presented as "1D-native."
  - This is enough to confirm 1D viability for candidate-competition
    prerequisites, but it should be described precisely: the current
    aggregation layer supports **1D competition inputs represented on a line in
    `Point2d`**, not a separate public 1D profile API.

- **Geometry and centrality helpers likely to be reused by competition**
  - Most existing geometry work is 2D-first.
  - For competition metrics that only need line-based fixtures, x-axis
    embedding is sufficient as a pre-Layer-7 dependency assumption.

### Conclusion

The existing codebase is sufficient to begin Layer 7 work **provided that**
competition implementation preserves 1D correctness in the core and does not
claim more than is currently true:

- The **competition core** should support 1D and 2D logic explicitly.
- Pre-existing dependencies used by the competition layer may rely on
  **1D embedded in `Point2d`** where they are still 2D-typed.
- Public wrappers may remain 2D-first initially, but 1D behavior must be
  protected by tests from the start.

No Phase 0 blocker was found that requires a pre-competition refactor of the
existing dependency stack. The main requirement is to keep the distinction
clear between:

- **true 1D-capable core logic**, which Layer 7 should preserve; and
- **existing 2D-typed helper APIs**, which currently express 1D through
  x-axis embedding.

### Phase 0 actions

- Add targeted 1D-on-a-line tests in aggregation-dependent areas where current
  2D-heavy coverage could miss bugs relevant to competition work.
- Avoid broad refactors before Layer 7 unless a concrete blocker appears.

## Phase A foundation decisions

Phase A establishes the Layer 7 core data model and deterministic
initialization without implementing adaptation yet.

### Core representation

- The competition core uses **dimension-aware `PointNd` vectors** for
  positions, headings, and bounds.
- Supported dimensions are currently **1D and 2D only**. This is explicit in
  validation, rather than relying on an implicit "2D but maybe line-embedded"
  convention inside the competition layer itself.
- Layer 7 may still adapt into existing 2D-typed helper APIs where needed, but
  its own state model is dimension-correct from the start.

### Competitor state

- Competitors are represented as **plain data structs**, not subclasses.
- Each competitor stores current position, heading, step size, active status,
  strategy kind, and a small amount of round-to-round performance memory.
- Full trace history remains an engine responsibility and will be added in the
  simulation-loop phase rather than being embedded inside each competitor.

### Strategy architecture

- Strategy identity remains a stable enum and stable string ID
  (`sticker`, `hunter`, `aggregator`, `predator`).
- Strategy execution is designed around a **service-layer registry** rather
  than a growing central switch and not around competitor subclasses.
- Phase A introduces the registry scaffold and the built-in baseline strategy
  registrations; Phase B will attach adaptation logic to that service layer.
- Adaptation decisions currently produce a **directional heading update** only.
  Step magnitude and boundary clipping remain separate concerns for the next
  motion-policy phase.

### Deterministic initialization

- Initialization uses dedicated `StreamManager` streams:
  - `competition_init_positions`
  - `competition_init_headings`
  - `competition_init_step_sizes`
- This keeps initialization reproducible and insulated from later changes to
  adaptation, tie-breaking, or analysis logic.
- Heading generation is dimension-aware:
  - in 1D, random headings are signed unit directions (`-1` or `+1`);
  - in 2D, random headings are normalized continuous direction vectors.

## Motion policy decisions

- Step-size resolution and boundary handling are separate from adaptation
  strategy services.
- Supported shared boundary policies are currently:
  - project to bounds;
  - stay put on invalid move;
  - reflect back into the feasible interval/box.
- Random step sizes use a dedicated `StreamManager` stream:
  - `competition_motion_step_sizes`

## Electoral evaluation bridge

- Round evaluation builds an `aggregation::Profile` from Layer 7 `PointNd`
  competitor/voter positions, then reuses aggregation-layer plurality scoring
  on that profile.
- Supporter assignment for the first competition milestone is defined as
  **plurality support**: each voter is attached to their top-ranked competitor
  under the round profile, with distance ties broken by ascending competitor
  index.
- The initial seat-conversion rules are:
  - plurality top-`k`;
  - Hare largest remainder.
- Hare largest remainder uses a deterministic fallback when total votes are all
  zero: seats are distributed as evenly as possible, breaking remaining ties by
  ascending competitor index.

## Engine semantics

- The first competition engine uses **synchronous round updates**:
  - evaluate votes and seats from positions at round `t`;
  - write round metrics onto the current competitor states;
  - derive all strategy decisions from that same shared snapshot;
  - apply motion to produce round `t+1` positions together.
- The fixed-round trace stores the **evaluated state** for each round plus the
  final competitor states after the last move.

## Termination diagnostics

- Early-stop diagnostics are configurable and optional.
- The first engine layer supports:
  - position convergence by epsilon;
  - cycle detection from rounded position signatures;
  - no-improvement windows over the chosen objective.
- Termination remains approximate and operational rather than claiming exact
  continuous-space equilibrium detection.

## Experiment runner semantics

- The first experiment runner is **serial and per-run deterministic**.
- Each run calls `StreamManager::reset_for_run(master_seed, run_index)` so
  rerunning a single replication by index reproduces the same result.
- Experiments return:
  - per-run summaries by default;
  - optional full traces when explicitly requested.
- Scenario sweeps are modeled as labeled collections of full experiment
  configs rather than as a bespoke mini-language. This keeps the first sweep
  layer simple and avoids freezing an overdesigned parameter-DSL too early.
- Parallel execution is intentionally deferred until the serial runner surface
  has proven stable.
