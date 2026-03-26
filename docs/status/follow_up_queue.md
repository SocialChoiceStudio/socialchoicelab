# Follow-up queue

Explicit tasks the maintainer asked to remember **outside** of normal roadmap grooming.
**Purpose:** chat sessions do not persist; this file is the durable reminder.

How to use:

- Add a bullet when you decide “we should do X next” in conversation or review.
- Prefer a **one-line outcome** plus optional link to a design doc or issue.
- When an item ships, **delete it** or move a one-line note to `project_log.md` / `CHANGELOG.md`.
- If an item already lives in `ROADMAP.md` with full detail, add only a **pointer** here (avoid two sources of truth).

---

## Owner-requested (fill in)

_Add bullets below. Nothing here yet — previous session requests were not written to the repo._

---

## Pointers (canonical detail elsewhere)

These are **not** duplicates of full specs; they are reminders to read the governing doc.

| Topic | Where it lives |
|-------|----------------|
| Utility / loss → utility diagnostic plot on canvas (R/Python parity) | `ROADMAP.md` — § Next work sequence (visual stack) |
| `save_plot` PNG/SVG or other static export | `ROADMAP.md` + `CHANGELOG.md` `[Unreleased]` + `rendering_consolidation_evaluation.md` |
| Competition engine extensions (e.g. per-run re-randomization) | `ROADMAP.md` post-tag follow-on + `competition_plan.md` |
| Yolk / Heart correctness (approximation vs exact) | `ROADMAP.md`, `WHERE_WE_ARE.md` known issues, `MILESTONE_GATES.md` |
| Composite C API before 1.0 | `ROADMAP.md` release ladder |
