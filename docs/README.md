# SocialChoiceLab Documentation

This file is the single entry point for project documentation.

## Source-of-Truth Ownership

| What you need | Where to look |
|---------------|---------------|
| What's next / current position | `docs/status/where_we_are.md` |
| Near-/mid-/long-term roadmap | `docs/status/roadmap.md` |
| Geometry plan (Foundation + Services, Steps A–E) | `docs/status/geometry_plan.md` |
| Milestone "done" criteria (features, tests, docs, API stability) | `docs/status/milestone_gates.md` |
| Archived plans (consensus reviews, core completion) | `docs/status/archive/` |
| Revisit before release or before opening to others | `docs/status/milestone_gates.md` § Revisit before release |
| Chronological project history | `docs/status/project_log.md` |
| Architecture intent | `docs/architecture/design_document.md` |
| StreamManager design & rules | `docs/architecture/stream_manager_design.md` |
| Indifference (level-set) API spec | `docs/architecture/indifference_design.md` |
| Code style, build, test | `docs/development/development.md` |
| Git and GitHub workflow | `docs/development/git_reference.md` |
| Simulation terminology & example | `docs/development/sample_simulation_description.md` |
| Academic reference library | `docs/references/README.md` |
| How to contribute | `CONTRIBUTING.md` (project root) |
| Security and vulnerability reporting | `SECURITY.md` (project root) |
| Release history | `CHANGELOG.md` (project root) |


If two docs disagree for execution decisions, use this precedence:

1. `docs/status/where_we_are.md`
2. `docs/architecture/` (relevant design doc)
3. `docs/status/roadmap.md` (near-/mid-/long-term)
4. `docs/status/project_log.md` (history only)

## Documentation Layout

Project root: `CONTRIBUTING.md`, `SECURITY.md`, `CHANGELOG.md` (standard open-source hygiene).

```
docs/
├── README.md                              ← you are here (master index)
├── status/
│   ├── where_we_are.md                    ← current position + recent work log
│   ├── roadmap.md                         ← near-term, mid-term, long-term plans
│   ├── c_api_plan.md                      ← c_api run plan (spec, implement, test, document)
│   ├── milestone_gates.md                 ← definition of done per milestone
│   ├── project_log.md                     ← chronological narrative history
│   └── archive/                           ← completed short-term plans (for reference)
│       ├── README.md
│       ├── consensus_plan_3.md
│       ├── consensus_plan_4.md
│       └── core_completion_plan.md
├── architecture/
│   ├── design_document.md                 ← living system architecture
│   ├── stream_manager_design.md            ← RNG single-owner policy and rules
│   └── indifference_design.md              ← level-set API spec (Layer 2)
├── development/
│   ├── development.md                     ← code style, build, lint, test instructions
│   ├── git_reference.md                   ← solo git/github workflow
│   └── sample_simulation_description.md   ← illustrative walkthrough + glossary (non-governing)
└── references/
    ├── README.md                          ← reference library index
    ├── reference_index.md                 ← annotated list of all papers and books
    └── implementation_priority.md         ← prioritized roadmap for social-choice features
```

## Where to Put a New Doc

Choose the subdirectory by the doc's purpose:

| Subdirectory | Put docs here when … | Examples |
|---|---|---|
| `status/` | The doc tracks *current state* — what is done, what is next, time horizons, or history. Agents read these every session. | `where_we_are.md`, `roadmap.md` |
| `architecture/` | The doc records a *design decision* that governs how code is written going forward. Should be stable once adopted. | `design_document.md`, `stream_manager_design.md` |
| `development/` | The doc is a *how-to* for working on the project — style guide, workflow, tools, illustrative examples. | `development.md`, `git_reference.md` |
| `references/` | The doc is a *bibliography or priority list* for academic/algorithmic sources. Not governing; just reference. | `reference_index.md`, `implementation_priority.md` |
| project root | Standard open-source hygiene files that GitHub renders prominently. | `CONTRIBUTING.md`, `SECURITY.md`, `CHANGELOG.md` |

**Naming:** All docs in `docs/` use `snake_case.md` (except `README.md` files). Root hygiene docs use `ALL_CAPS.md`. See the full convention in [`docs/development/development.md`](development/development.md) § File Naming Conventions.

**Single source of truth:** Before creating a new doc, check whether the content belongs in an existing file. Prefer adding a section to an existing doc over creating a new one, especially in `status/` and `architecture/`.

## Update Rules

For each completed development item:

1. Update `docs/status/where_we_are.md` (next item, last updated date, add to Recent Work).
2. If the work came from a plan, mark it ✅ Done in that plan (completed plans are in `docs/status/archive/`).
3. Add a dated milestone note to `docs/status/project_log.md` when materially relevant.

At **end-of-milestone** (when running `scripts/end-of-milestone.sh`), the script prompts to confirm `docs/status/roadmap.md` is still accurate for near/mid/long-term; update it if horizons have shifted.

When **adding new docs**, add them to this index. Prefer a single source of truth (especially in `status/` and `architecture/`); avoid duplicating the same list or content in multiple files.
