# SocialChoiceLab Documentation

This file is the single entry point for project documentation.

## Source-of-Truth Ownership

| What you need | Where to look |
|---------------|---------------|
| What's next / current position | `docs/status/WHERE_WE_ARE.md` |
| Backlog and phase/item status | `docs/status/CONSENSUS_PLAN.md` |
| Near-/mid-/long-term roadmap | `docs/status/ROADMAP.md` |
| Milestone "done" criteria (features, tests, docs, API stability) | `docs/status/MILESTONE_GATES.md` |
| Phase 2 edit record | `docs/status/Phase2Changes.md` |
| Chronological project history | `docs/status/PROJECT_LOG.md` |
| Architecture intent | `docs/architecture/Design_Document.md` |
| StreamManager design & rules | `docs/architecture/StreamManager_Design.md` |
| Code style, build, test | `docs/development/DEVELOPMENT.md` |
| Git and GitHub workflow | `docs/development/Git_Reference.md` |
| Simulation terminology & example | `docs/development/sample_simulation_description.md` |
| Academic reference library | `docs/references/README.md` |
| How to contribute | `CONTRIBUTING.md` (project root) |
| Security and vulnerability reporting | `SECURITY.md` (project root) |
| Release history | `CHANGELOG.md` (project root) |

**Note:** `docs/status/Phase2Changes.md` is a one-time record of Phase 2 edits; for ongoing work see WHERE_WE_ARE and PROJECT_LOG.

If two docs disagree for execution decisions, use this precedence:

1. `docs/status/WHERE_WE_ARE.md`
2. `docs/status/CONSENSUS_PLAN.md`
3. `docs/architecture/` (relevant design doc)
4. `docs/status/PROJECT_LOG.md` (history only)

## Documentation Layout

Project root: `CONTRIBUTING.md`, `SECURITY.md`, `CHANGELOG.md` (standard open-source hygiene).

```
docs/
├── README.md                              ← you are here (master index)
├── status/
│   ├── WHERE_WE_ARE.md                    ← current position + recent work log
│   ├── CONSENSUS_PLAN.md                  ← prioritized backlog (source of truth for item status)
│   ├── ROADMAP.md                         ← near-term, mid-term, long-term plans
│   ├── MILESTONE_GATES.md                 ← definition of done per milestone
│   ├── PROJECT_LOG.md                     ← chronological narrative history
│   └── Phase2Changes.md                   ← one-time Phase 2 edit record (ongoing: WHERE_WE_ARE, PROJECT_LOG)
├── architecture/
│   ├── Design_Document.md                 ← living system architecture
│   └── StreamManager_Design.md            ← RNG single-owner policy and rules
├── development/
│   ├── DEVELOPMENT.md                     ← code style, build, lint, test instructions
│   ├── Git_Reference.md                   ← solo git/github workflow
│   └── sample_simulation_description.md   ← illustrative walkthrough + glossary (non-governing)
└── references/
    ├── README.md                          ← reference library index
    ├── development/
    ├── foundation/
    ├── geometry/
    └── social_choice/
```

## Update Rules

For each completed development item:

1. Mark it ✅ Done in `docs/status/CONSENSUS_PLAN.md`.
2. Update `docs/status/WHERE_WE_ARE.md` (next item, last updated date, add to Recent Work).
3. Add a dated milestone note to `docs/status/PROJECT_LOG.md` when materially relevant.

At **end-of-milestone** (when running `scripts/end-of-milestone.sh`), the script prompts to confirm `docs/status/ROADMAP.md` is still accurate for near/mid/long-term; update it if horizons have shifted.

When **adding new docs**, add them to this index. Prefer a single source of truth (especially in `status/` and `architecture/`); avoid duplicating the same list or content in multiple files.
