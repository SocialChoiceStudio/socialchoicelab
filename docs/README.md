# SocialChoiceLab Documentation

This file is the single entry point for project documentation.

## Source-of-Truth Ownership

| What you need | Where to look |
|---------------|---------------|
| What's next / current position | `docs/status/WHERE_WE_ARE.md` |
| Session / owner follow-ups (durable queue) | `docs/status/follow_up_queue.md` |
| Near-/mid-/long-term roadmap | `docs/status/ROADMAP.md` |
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


If two docs disagree for execution decisions, use this precedence:

1. `docs/status/WHERE_WE_ARE.md`
2. `docs/architecture/` (relevant design doc)
3. `docs/status/ROADMAP.md` (near-/mid-/long-term)
4. `docs/status/project_log.md` (history only)

## Documentation Layout

Project root: `CONTRIBUTING.md`, `SECURITY.md`, `CHANGELOG.md` (standard open-source hygiene).

```
docs/
├── README.md                              ← you are here (master index)
├── status/
│   ├── WHERE_WE_ARE.md                    ← current position + recent work log
│   ├── follow_up_queue.md                 ← explicit follow-ups (don’t lose across chats)
│   ├── ROADMAP.md                         ← near-term, mid-term, long-term plans
│   ├── competition_plan.md                ← Layer 7 competition implementation plan
│   │   ├── visualization_plan.md          ← visualization layer plan (complete, archived)
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
│       └── core_completion_plan.md
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
| `status/` | The doc tracks *current state* — what is done, what is next, time horizons, or history. Agents read these every session. | `WHERE_WE_ARE.md`, `ROADMAP.md` |
| `architecture/` | The doc records a *design decision* that governs how code is written going forward. Should be stable once adopted. | `design_document.md`, `stream_manager_design.md` |
| `development/` | The doc is a *how-to* for working on the project — style guide, workflow, tools, illustrative examples. | `development.md`, `git_reference.md` |
| `SocialChoiceStudioDev/references/` | Academic bibliography and priority lists — outside the repo in Dropbox; not tracked in git. | `reference_index.md`, `implementation_priority.md` |
| project root | Standard open-source hygiene files that GitHub renders prominently. | `CONTRIBUTING.md`, `SECURITY.md`, `CHANGELOG.md` |

**Naming:** All docs in `docs/` use `snake_case.md` (except `README.md` files). Root hygiene docs use `ALL_CAPS.md`. See the full convention in [`docs/development/development.md`](development/development.md) § File Naming Conventions.

**Single source of truth:** Before creating a new doc, check whether the content belongs in an existing file. Prefer adding a section to an existing doc over creating a new one, especially in `status/` and `architecture/`.

## Update Rules

For each completed development item:

1. Update `docs/status/WHERE_WE_ARE.md` (next item, last updated date, add to Recent Work).
2. If the work came from a plan, mark it ✅ Done in that plan (completed plans are in `docs/status/archive/`).
3. Add a dated milestone note to `docs/status/project_log.md` when materially relevant.

At **end-of-milestone** (when running `scripts/end-of-milestone.sh`), the script prompts to confirm `docs/status/ROADMAP.md` is still accurate for near/mid/long-term; update it if horizons have shifted.

When **adding new docs**, add them to this index. Prefer a single source of truth (especially in `status/` and `architecture/`); avoid duplicating the same list or content in multiple files.
