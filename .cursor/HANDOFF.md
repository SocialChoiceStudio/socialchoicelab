# Handoff

**Date:** 2026-10-08
**Branch:** main, ahead of origin/main by 3 (unchanged since last handoff) — plus a large **uncommitted** working tree from this session.

## Goal

Implementing `.cursor/plans/nontechnical-user-release_61acb40c.plan.md`: turn the
developer-facing R/Python/C++ stack into a zero-command hosted web app. 4 of 5
plan todos are done; the 5th was cancelled at the user's direction (desktop
packaging is not wanted — the web app is the actual goal).

**Nothing from this session is committed.** The repo was already 3 commits
ahead of `origin/main` before this session (five-candidate demos,
Borda-Condorcet test fix, `plot_utility_surface()`) — those are unrelated
prior work, already committed, just not pushed.

## What's new and uncommitted

Run `git status` to see the full list. Summary by area:

- **C++ / C API:** `CompetitionTrace` now carries its own `DistanceConfig`;
  new `scs_competition_trace_dist_config` in `scs_api.h`/`scs_api.cpp`, with a
  new C++ test in `tests/unit/test_c_api.cpp`.
- **Python bindings:** `_competition.py` (`CompetitionTrace.dist_config()`),
  `plots.py` (overlay-guard rewrite in `animate_competition_canvas()` +
  new `render_spatial_voting_html()`), `_loader.py` (new cdef), new
  `service.py` (`run_spatial_voting_mvp()`), `__init__.py` (new export,
  version bump to `0.3.0`), `pyproject.toml` (version bump).
- **R bindings:** `competition.R`, `competition_canvas.R`, `r/src/competition.c`,
  `r/src/init.c` — same overlay-guard mirror. `DESCRIPTION` version bump to
  `0.3.0`.
- **Data fix (both languages):** `fiorina_plott_a_p_s_r1978_series1`/`series3`
  scenario JSON files were malformed in both
  `python/src/socialchoicelab/data/scenarios/` and
  `r/inst/extdata/scenarios/`; fixed in both.
- **New `app/` directory:** a working Shiny for Python app implementing the
  one MVP workflow. See `app/README.md` to run it locally.
- **New docs:** `docs/architecture/nontechnical_mvp_design.md`,
  `application_service_layer.md`, `hosted_mvp_design.md` — read
  `hosted_mvp_design.md` first, it has the fullest picture of what's
  verified vs. not.
- **Tests added:** `python/tests/test_service.py`,
  `app/tests/test_app.py`, new cases in `test_competition.py`,
  `test_competition_canvas_voronoi.py`, `test_plots.py`,
  `test_scenarios.py` (Python) and `test_competition.R`,
  `test_competition_canvas_voronoi.R`, `test_scenarios.R` (R).
- **`CHANGELOG.md` `[Unreleased]`** already has entries for all of the above
  — read it for the one-paragraph version of each change.

## Decisions made this session

- The application service layer (`service.py`) is Python-only on purpose —
  it's scaffolding for the hosted app, not a core-library change, so it
  doesn't need an R port. Reasoning is written into
  `application_service_layer.md` in case this gets questioned later.
- Desktop packaging (plan step 5) is cancelled. The user confirmed the goal
  is the web app, not a desktop app. Don't restart desktop packaging work
  without the user asking for it again.
- Scenario dropdown in the app is filtered to scenarios with a status quo
  point (currently only 3 of 34). `hosted_mvp_design.md` § "Expanding
  scenario coverage" lays out two ways to fix this later, neither done:
  let users click a status quo point, or research/add real ones to the
  literature-sourced scenarios. This needs a decision, not more code, next
  time it comes up.

## Tests run

All green as of this handoff:
- Python: `302 passed` (`cd python && pytest`)
- App: `6 passed` (`cd app && pytest tests/`)
- R: full `testthat::test_dir()` suite, all green
- C++: `34/34` via `ctest -LE benchmark`

`app/Dockerfile` was written but **not** build-verified — no Docker
available in this environment. Verify it builds before relying on it.

## Next step

1. Review the diff (`git status` / `git diff`), then decide whether to
   commit this as one batch or split it (service layer / overlay guard /
   scenario data fix / hosted app are each independently committable).
2. If continuing the hosted-MVP work: no hosting provider has been chosen
   yet (deferred this session), and the user pilot (plan step 6) hasn't
   been run. Both need the user's input before more infrastructure work
   makes sense.
3. Don't restart desktop packaging unless asked.
