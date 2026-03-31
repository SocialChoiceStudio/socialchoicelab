# SocialChoiceLab Documentation

This file is the single entry point for project documentation.

## Start here (humans and agents)

1. **Open this file (`docs/README.md`)** — it maps questions to a single path.
2. **Right now (tags, phase, blockers):** [`docs/status/WHERE_WE_ARE.md`](status/WHERE_WE_ARE.md).
3. **Ordered planned work (what to do next, in sequence):** [`docs/status/ROADMAP.md`](status/ROADMAP.md) — **only this file should hold the main prioritized list.** Do not copy that list into other docs; link here instead.
4. **Scratch reminders from chat (optional):** [`docs/status/follow_up_queue.md`](status/follow_up_queue.md). When something becomes real work, **move it into `ROADMAP.md` and delete the scratch line** so two lists do not drift.
5. **What actually shipped:** [`CHANGELOG.md`](../CHANGELOG.md) at the repo root.
6. **`docs/status/archive/`** is old plans — if anything disagrees with `ROADMAP.md` or `WHERE_WE_ARE.md`, **the non-archive files win.**

## Source-of-Truth Ownership

| What you need | Where to look |
|---------------|---------------|
| Current snapshot (phase, tags, known blockers) | `docs/status/WHERE_WE_ARE.md` |
| Ordered plan / prioritized next work | `docs/status/ROADMAP.md` |
| Short scratch list (promote into ROADMAP when real) | `docs/status/follow_up_queue.md` |
| Layer 7 competition implementation plan | `docs/status/competition_plan.md` |
| Visualization layer implementation plan (archived) | `docs/status/archive/visualization_plan.md` |
| Milestone "done" criteria (features, tests, docs, API stability) | `docs/status/MILESTONE_GATES.md` |
| Archived plans (geometry, C API, bindings, aggregation, consensus reviews) | `docs/status/archive/` |
| Revisit before release or before opening to others | `docs/status/MILESTONE_GATES.md` § Revisit before release |
| Chronological project history | `docs/status/project_log.md` |
| Architecture intent | `docs/architecture/design_document.md` |
| Geometry services design (CGAL 2D, winsets, Yolk, Heart, etc.) | `docs/architecture/geometry_design.md` |
| Aggregation design (profiles, voting rules, Pareto, Condorcet) | `docs/architecture/aggregation_design.md` |
| C API design and contracts | `docs/architecture/c_api_design.md` |
| Competition layer (Layer 7) design | `docs/architecture/competition_design.md` |
| StreamManager design & rules | `docs/architecture/stream_manager_design.md` |
| Indifference (level-set) API spec | `docs/architecture/indifference_design.md` |
| Visualization layer design (color system, layer stack, theme decisions) | `docs/development/visualization_design.md` |
| Code style, build, test | `docs/development/development.md` |
| Setup on new or synced machine | `docs/development/setup_checklist.md` |
| Git and GitHub workflow | `docs/development/git_reference.md` |
| Simulation terminology & example | `docs/development/sample_simulation_description.md` |
| Academic reference library | `../references/README.md` (outside repo, in `SocialChoiceStudioDev/`) |
| How to contribute | `CONTRIBUTING.md` (project root) |
| Security and vulnerability reporting | `SECURITY.md` (project root) |
| Release history | `CHANGELOG.md` (project root) |


If two docs disagree, use this order:

1. **`CHANGELOG.md`** — facts about what was released.
2. **`docs/status/ROADMAP.md`** — **order and priorities** for planned work.
3. **`docs/status/WHERE_WE_ARE.md`** — **current snapshot** (should be kept consistent with the roadmap; if they diverge, fix one or the other in the same edit).
4. **`docs/architecture/`** — technical contracts and design intent for code.
5. **`docs/status/project_log.md`** — narrative history only, not a plan.

## Documentation Layout

Project root: `CONTRIBUTING.md`, `SECURITY.md`, `CHANGELOG.md` (standard open-source hygiene).

```
docs/
├── README.md                              ← you are here (master index)
├── status/
│   ├── WHERE_WE_ARE.md                    ← current snapshot (tags, phase, blockers)
│   ├── ROADMAP.md                         ← ordered prioritized plan (main list)
│   ├── follow_up_queue.md                 ← optional scratch; promote into ROADMAP
│   ├── competition_plan.md                ← Layer 7 competition implementation plan
│   ├── MILESTONE_GATES.md                 ← definition of done per milestone
│   ├── project_log.md                     ← chronological narrative history
│   └── archive/                           ← completed short-term plans (for reference)
│       ├── README.md
│       ├── c_api_plan.md
│       ├── c_api_extensions_plan.md
│       ├── geometry_plan.md
│       ├── profiles_and_aggregation_plan.md
│       ├── binding_plan_completed.md
│       ├── binding_decisions_resolved.md
│       ├── consensus_plan_3.md
│       ├── consensus_plan_4.md
│       ├── core_completion_plan.md
│       └── visualization_plan.md          ← visualization layer plan (complete)
├── architecture/
│   ├── design_document.md                 ← living system architecture
│   ├── geometry_design.md                 ← CGAL 2D geometry API and kernel policy
│   ├── aggregation_design.md              ← profiles, voting rules, aggregation properties
│   ├── c_api_design.md                    ← stable C ABI design and contracts
│   ├── competition_design.md              ← Layer 7 competition engine design
│   ├── stream_manager_design.md           ← RNG single-owner policy and rules
│   └── indifference_design.md             ← level-set API spec (Layer 2)
├── development/
│   ├── development.md                     ← code style, build, lint, test instructions
│   ├── setup_checklist.md                 ← new or synced machine: prerequisites, build, test
│   ├── git_reference.md                   ← solo git/github workflow
│   ├── visualization_design.md            ← color system, layer stack, theme decisions
│   └── sample_simulation_description.md   ← illustrative walkthrough + glossary (non-governing)
```

> **Academic reference library** (papers, PDFs, annotated bibliography) lives at
> `../references/` relative to this repo — i.e. `SocialChoiceStudioDev/references/` in Dropbox.
> It is not tracked in git.

## Where to Put a New Doc

Choose the subdirectory by the doc's purpose:

| Subdirectory | Put docs here when … | Examples |
|---|---|---|
| `status/` | Snapshot, ordered plan, milestones, or history. **Prioritized “what next” belongs in `ROADMAP.md` only**; `WHERE_WE_ARE.md` is the current snapshot. | `ROADMAP.md`, `WHERE_WE_ARE.md`, `MILESTONE_GATES.md` |
| `architecture/` | The doc records a *design decision* that governs how code is written going forward. Should be stable once adopted. | `design_document.md`, `stream_manager_design.md` |
| `development/` | The doc is a *how-to* for working on the project — style guide, workflow, tools, illustrative examples. | `development.md`, `git_reference.md` |
| `SocialChoiceStudioDev/references/` | Academic bibliography and priority lists — outside the repo in Dropbox; not tracked in git. | `reference_index.md`, `implementation_priority.md` |
| project root | Standard open-source hygiene files that GitHub renders prominently. | `CONTRIBUTING.md`, `SECURITY.md`, `CHANGELOG.md` |

**Naming:** All docs in `docs/` use `snake_case.md` (except `README.md` files). Root hygiene docs use `ALL_CAPS.md`. See the full convention in [`docs/development/development.md`](development/development.md) § File Naming Conventions.

**Single source of truth:** Before creating a new doc, check whether the content belongs in an existing file. Prefer adding a section to an existing doc over creating a new one, especially in `status/` and `architecture/`.

## Update Rules

For each completed development item:

1. Update **`docs/status/ROADMAP.md`** if priorities or ordering changed; update **`docs/status/WHERE_WE_ARE.md`** for the current snapshot (date, phase, blockers). Do not paste the full roadmap into `WHERE_WE_ARE.md` — link to `ROADMAP.md`.
2. If the work came from a plan, mark it ✅ Done in that plan (completed plans are in `docs/status/archive/`).
3. Add a dated milestone note to `docs/status/project_log.md` when materially relevant.
4. Record shipped behavior in **`CHANGELOG.md`**.

At **end-of-milestone** (when running `scripts/end-of-milestone.sh`), the script prompts to confirm `docs/status/ROADMAP.md` is still accurate for near/mid/long-term; update it if horizons have shifted.

When **adding new docs**, add them to this index. Prefer a single source of truth (especially in `status/` and `architecture/`); avoid duplicating the same list or content in multiple files.
