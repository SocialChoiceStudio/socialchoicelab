# socialchoicelab

The project rules live in `.cursor/rules/*.mdc` and are shared with Cursor. They are the single source of truth; do not copy them here. They are imported below so every Claude Code session loads them automatically. Follow them all; in Cursor, the ones marked `alwaysApply: false` load only when relevant, so apply those when the task touches their area.

@.cursor/rules/Approval-Before-Implementing.mdc
@.cursor/rules/Check-State-And-Tailor.mdc
@.cursor/rules/Cpp-Core-And-C-API-Workflow.mdc
@.cursor/rules/End-User-Error-Messages.mdc
@.cursor/rules/File-Naming-Conventions.mdc
@.cursor/rules/New-Functionality-means-New-Tests.mdc
@.cursor/rules/No-Backward-Compat-Until-Open.mdc
@.cursor/rules/R-Python-Layer-Parity.mdc
@.cursor/rules/Verify-Library-Behaviour-From-Docs.mdc

Orientation: docs/README.md, docs/status/WHERE_WE_ARE.md, and .cursor/HANDOFF.md for the latest session state. Open items: docs/status/follow_up_queue.md.

Before pushing, run `./scripts/check-all.sh` (also enforced by the pre-push hook; see `.cursor/skills/check-before-push/SKILL.md`).

Never read or print `.env` or `.github_token`.
