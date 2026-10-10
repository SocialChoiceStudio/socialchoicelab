# Hosted Shiny for Python MVP

**Status:** Built and verified locally. Not deployed to a public host; no
user pilot run yet. Scopes step 5 of the
[nontechnical-user-release plan](../../.cursor/plans/nontechnical-user-release_61acb40c.plan.md).

**Purpose:** Let a nontechnical user open a URL and complete the single
workflow in [nontechnical_mvp_design.md](nontechnical_mvp_design.md) without
installing software or writing code.

## What exists

`app/app.py` — a Shiny for Python app that collects inputs (bundled scenario
or manually entered voters, decision rule, overlay toggles), calls
`socialchoicelab.run_spatial_voting_mvp()` (the
[application service layer](application_service_layer.md)) for every
computation, and renders the result via
`socialchoicelab.plots.render_spatial_voting_html()` in an iframe. The
"Export HTML" button serves the same self-contained HTML file
`save_plot()` would write, as a browser download.

`app/Dockerfile` — builds the C++ `scs_api` shared library, installs the
Python bindings and `shiny`, and serves the app on port 8000.

`app/requirements.txt` — Python dependencies for the app layer
(`socialchoicelab` itself is installed from the repo's `python/` directory,
not from this file, since it is not yet published to PyPI).

`app/tests/test_app.py` — smoke tests for the app's pure-logic pieces
(request building from form inputs, scenario filtering) that run without a
server or browser.

## Verified by local browser testing

The app was run locally (`shiny run`) and driven through a real browser
(Chrome DevTools Protocol) to confirm, end to end:

- The scenario dropdown, decision rule controls, and overlay checkboxes
  render and the canvas updates reactively.
- A bundled scenario (`laing_olmsted_bear`) renders voters, the status quo,
  and the computed winset correctly.
- The Pareto set and indifference curve overlays both draw correctly when
  toggled on.
- Manual voter entry (typed `x,y` pairs) computes a winset without a
  bundled scenario, including a supermajority rule and an empty-winset
  result.
- Invalid manual input (a non-numeric coordinate) produces a clear,
  specific error banner instead of a crash or a blank page.
- The "Export HTML" download endpoint returns a complete, valid,
  self-contained HTML file matching `save_plot()`'s output format.

This testing found and led to fixing two real bugs (see `CHANGELOG.md`
"Unreleased"):

1. 32 of the 34 bundled scenarios have no status quo point, so
   `run_spatial_voting_mvp()` cannot compute a winset for them. The app's
   scenario dropdown is filtered to the 3 scenarios that do
   (`laing_olmsted_bear`, `laing_olmsted_skewed_star`,
   `tovey_regular_polygon`); the service layer still raises a clear error if
   a caller selects one without a status quo.
2. Two scenario data files (`fiorina_plott_a_p_s_r1978_series1`/`series3`)
   were malformed in both the Python and R bundled data directories and
   crashed `load_scenario()` entirely; both are now fixed and covered by a
   regression test in each language.

## Not yet done

- **Public deployment.** No hosting provider has been chosen yet. The
  `Dockerfile` has not been build-verified (no Docker available in the
  environment this was written in) or deployed to any host.
- **User pilot.** The plan's step 6 ("validate with real users") has not
  been run. This requires real pilot participants.
- **Resource limits and abuse protection** for a public deployment (request
  size limits, computation time limits, rate limiting) are not implemented;
  the app currently assumes a trusted, local, or small-scale environment.
- **CSV import, saved projects, and candidate competition** remain
  explicitly out of scope per `nontechnical_mvp_design.md`.

## Expanding scenario coverage (open question)

Only 3 of 34 bundled scenarios have a status quo point, so only 3 are
usable in this workflow today. Two ways to expand this, neither implemented
yet, pending a decision:

1. Let the user click a point on the plot to set an arbitrary status quo
   for scenarios that do not define one. This changes the workflow (the
   status quo would no longer come from the published scenario) and should
   be labeled as user-chosen, not canonical.
2. Research and add historically accurate status quo points to the other
   31 scenarios where the literature specifies one. This is a content
   task, not a code task, and should not invent values without a source.
