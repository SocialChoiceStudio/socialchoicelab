# Application service layer

**Status:** Implemented for the MVP workflow. Scopes step 3 of the
[nontechnical-user-release plan](../../.cursor/plans/nontechnical-user-release_61acb40c.plan.md).

**Purpose:** Give a hosted web front end (step 5, the Shiny for Python
MVP) one stable, serializable entry point per workflow, instead of
calling the lower-level R/Python API functions directly from the UI
layer. This isolates UI code from internal refactors to the plotting
and computation layers.

## Module

`python/src/socialchoicelab/service.py`, exported as
`socialchoicelab.run_spatial_voting_mvp`.

## Why Python-only

This module is application/presentation scaffolding for one specific
deployment target (the hosted Shiny for Python app), not a change to
core algorithms, data structures, or behavior in the R or Python
bindings. The R-Python Layer Parity rule governs the latter, not this
module. R users already have the equivalent high-level entry points
(`plot_spatial_voting()`, `layer_winset()`, `winset_2d()`, etc.)
directly in R; there is no R-side hosted app calling into this layer.

## Contract

`run_spatial_voting_mvp(request: dict) -> dict`

Both `request` and the return value are plain Python data: dicts,
lists, numbers, strings, and booleans only — no numpy arrays, cffi
handles, or other non-JSON-serializable objects. A JSON request body
can be passed in after `json.loads`, and the return value can be
passed straight to `json.dumps` for an HTTP response.

### Request shape

```python
{
    "scenario_name": "laing_olmsted_bear",   # OR "manual_voters" (mutually exclusive)
    "manual_voters": [[0.0, 0.0], [2.0, 0.0], [1.0, 2.0]],
    "manual_status_quo": [0.0, 0.0],          # required when manual_voters is given
    "decision_rule": {"kind": "majority"},     # or {"kind": "supermajority", "fraction": 0.67}
    "overlays": {
        "pareto": False,
        "indifference_curves": False,
        "preferred_regions": False,
    },
    "theme": "dark2",
}
```

Every key except `scenario_name`/`manual_voters`+`manual_status_quo`
is optional and defaults as shown. Invalid or missing required fields
raise `ValueError` with a message that states what failed, what was
provided, and how to fix it, per the project's end-user error message
rule.

### Response shape

```python
{
    "canvas": {...},    # the same canvas payload plot_spatial_voting()/save_plot() use
    "summary": {
        "source": "Laing-Olmsted-Bear",
        "n_voters": 5,
        "status_quo": [100.0, 100.0],
        "decision_rule": {"kind": "majority", "k": 3},
        "overlays": {"pareto": False, "indifference_curves": False, "preferred_regions": False},
        "winset_is_empty": False,
    },
}
```

## Scope

Implements exactly the one workflow defined in
[nontechnical_mvp_design.md](nontechnical_mvp_design.md): load a
bundled scenario or manually entered voters and status quo, compute a
majority or supermajority winset, and optionally draw the Pareto set
(convex hull), indifference curve, and preferred-region overlays.
Candidate competition, non-Euclidean distance configs, Yolk/Heart
overlays, CSV import, and saved projects are out of scope here for the
same reasons given in that design document's "Explicitly out of scope"
table; this layer does not expose them.

## Tests

`python/tests/test_service.py` covers the scenario path, the manual
entry path, the majority and supermajority decision rules, each
overlay, and every validation error.
