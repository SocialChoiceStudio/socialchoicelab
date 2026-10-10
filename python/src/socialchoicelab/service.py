"""service.py — High-level, serializable application service layer.

Implements step 3 ("Create a stable application service layer") of the
nontechnical-user-release plan. Every public function here accepts and
returns plain, JSON-serializable Python data (dicts, lists, numbers,
strings, booleans) only — no numpy arrays, cffi handles, or other
objects a web request body or response cannot carry as-is. This lets a
hosted Shiny for Python app (or any other HTTP front end) call these
functions directly with a parsed JSON request body, and serialize the
return value straight back into a JSON response.

This module intentionally implements only the single MVP workflow
documented in docs/architecture/nontechnical_mvp_design.md: "Explore a
spatial voting scenario and export a figure." It is deliberately a thin
layer over the existing public Python API (scenarios, winset, plots);
it does not add new computation. Scope is narrow on purpose so the
first hosted release makes no claim it cannot support — see that
design document's "Explicitly out of scope" table (candidate
competition, non-Euclidean distance, Yolk/Heart overlays, CSV import,
and saved projects are all out of scope for this function).

This module is Python-only by design, not an R/Python parity gap: it is
application/presentation scaffolding for one specific deployment (the
hosted Shiny for Python app), not a change to core algorithms, data
structures, or behavior in the R or Python bindings. R users already
have the equivalent high-level entry points (`plot_spatial_voting()`,
`layer_winset()`, etc.) directly in R.
"""

from __future__ import annotations

import math
from typing import Any, Dict, List, Optional

import numpy as np

from socialchoicelab._winset import winset_2d
from socialchoicelab.scenarios import list_scenarios, load_scenario
from socialchoicelab.plots import (
    plot_spatial_voting,
    layer_winset,
    layer_convex_hull,
    layer_ic,
    layer_preferred_regions,
)
from socialchoicelab._functions import convex_hull_2d

__all__ = ["run_spatial_voting_mvp"]

_VALID_DECISION_RULE_KINDS = ("majority", "supermajority")
_VALID_OVERLAY_KEYS = ("pareto", "indifference_curves", "preferred_regions")


def _flat_pairs(points: List[List[float]], field_name: str) -> np.ndarray:
    """Validate and flatten a list of [x, y] pairs into a flat float array."""
    if not isinstance(points, list) or len(points) == 0:
        raise ValueError(
            f"run_spatial_voting_mvp: '{field_name}' must be a non-empty list of "
            f"[x, y] pairs. You provided {points!r}. Enter at least one point."
        )
    flat: List[float] = []
    for i, p in enumerate(points):
        if (
            not isinstance(p, (list, tuple))
            or len(p) != 2
            or not all(isinstance(v, (int, float)) for v in p)
        ):
            raise ValueError(
                f"run_spatial_voting_mvp: '{field_name}[{i}]' must be a pair of "
                f"two numbers [x, y]. You provided {p!r}. "
                "Enter numeric x and y coordinates for each point."
            )
        flat.append(float(p[0]))
        flat.append(float(p[1]))
    return np.asarray(flat, dtype=float)


def _resolve_scenario_or_manual(request: Dict[str, Any]):
    """Return (voters_flat, status_quo_xy, voter_names, dim_names, source_label)."""
    scenario_name = request.get("scenario_name")
    manual_voters = request.get("manual_voters")

    if scenario_name is not None and manual_voters is not None:
        raise ValueError(
            "run_spatial_voting_mvp: provide either 'scenario_name' or "
            "'manual_voters', not both. You provided both. Remove one of them."
        )

    if scenario_name is not None:
        if not isinstance(scenario_name, str):
            raise ValueError(
                "run_spatial_voting_mvp: 'scenario_name' must be a string. "
                f"You provided {scenario_name!r}."
            )
        available = list_scenarios()
        if scenario_name not in available:
            raise ValueError(
                f"run_spatial_voting_mvp: unknown scenario '{scenario_name}'. "
                f"Choose one of: {', '.join(available)}."
            )
        sc = load_scenario(scenario_name)
        if sc["status_quo"] is None:
            raise ValueError(
                f"run_spatial_voting_mvp: scenario '{scenario_name}' has no status "
                "quo point, so a winset cannot be computed. Choose a different "
                "scenario."
            )
        return (
            np.asarray(sc["voters"], dtype=float),
            np.asarray(sc["status_quo"], dtype=float),
            list(sc["voter_names"]),
            list(sc["dim_names"]),
            sc["name"],
        )

    if manual_voters is not None:
        manual_sq = request.get("manual_status_quo")
        if manual_sq is None:
            raise ValueError(
                "run_spatial_voting_mvp: 'manual_status_quo' is required when "
                "'manual_voters' is provided. Enter a status quo point [x, y]."
            )
        voters_flat = _flat_pairs(manual_voters, "manual_voters")
        if len(voters_flat) // 2 < 2:
            raise ValueError(
                "run_spatial_voting_mvp: 'manual_voters' must contain at least "
                f"2 voters to run a majority analysis. You provided "
                f"{len(voters_flat) // 2}. Add more voters."
            )
        if (
            not isinstance(manual_sq, (list, tuple))
            or len(manual_sq) != 2
            or not all(isinstance(v, (int, float)) for v in manual_sq)
        ):
            raise ValueError(
                "run_spatial_voting_mvp: 'manual_status_quo' must be a pair of "
                f"two numbers [x, y]. You provided {manual_sq!r}."
            )
        sq = np.asarray([float(manual_sq[0]), float(manual_sq[1])], dtype=float)
        n_v = len(voters_flat) // 2
        voter_names = [f"V{i + 1}" for i in range(n_v)]
        dim_names = ["Dimension 1", "Dimension 2"]
        return voters_flat, sq, voter_names, dim_names, "Manual entry"

    raise ValueError(
        "run_spatial_voting_mvp: provide either 'scenario_name' (one of "
        f"{', '.join(list_scenarios())}) or 'manual_voters' with "
        "'manual_status_quo'. You provided neither."
    )


def _resolve_decision_rule(request: Dict[str, Any], n_voters: int):
    """Return (k_count, rule_summary_dict) for scl.winset_2d's k argument."""
    rule = request.get("decision_rule", {"kind": "majority"})
    if not isinstance(rule, dict) or "kind" not in rule:
        raise ValueError(
            "run_spatial_voting_mvp: 'decision_rule' must be a dict with a "
            "'kind' key, e.g. {'kind': 'majority'}. "
            f"You provided {rule!r}."
        )
    kind = rule["kind"]
    if kind not in _VALID_DECISION_RULE_KINDS:
        raise ValueError(
            f"run_spatial_voting_mvp: decision_rule.kind must be one of "
            f"{_VALID_DECISION_RULE_KINDS}. You provided '{kind}'."
        )
    if kind == "majority":
        return -1, {"kind": "majority", "k": math.floor(n_voters / 2) + 1}

    fraction = rule.get("fraction")
    if not isinstance(fraction, (int, float)) or not (0.5 < fraction <= 1.0):
        raise ValueError(
            "run_spatial_voting_mvp: decision_rule.fraction must be a number "
            f"greater than 0.5 and at most 1.0 for a supermajority. You "
            f"provided {fraction!r}. For example, 0.67 means a two-thirds "
            "majority."
        )
    k = math.ceil(fraction * n_voters)
    k = min(k, n_voters)
    return k, {"kind": "supermajority", "fraction": float(fraction), "k": k}


def _resolve_overlays(request: Dict[str, Any]) -> Dict[str, bool]:
    overlays_in = request.get("overlays", {})
    if not isinstance(overlays_in, dict):
        raise ValueError(
            "run_spatial_voting_mvp: 'overlays' must be a dict of booleans "
            f"with keys from {_VALID_OVERLAY_KEYS}. You provided {overlays_in!r}."
        )
    unknown = set(overlays_in) - set(_VALID_OVERLAY_KEYS)
    if unknown:
        raise ValueError(
            f"run_spatial_voting_mvp: unknown overlay key(s) {sorted(unknown)}. "
            f"Valid overlay keys are: {_VALID_OVERLAY_KEYS}."
        )
    return {key: bool(overlays_in.get(key, False)) for key in _VALID_OVERLAY_KEYS}


def run_spatial_voting_mvp(request: Dict[str, Any]) -> Dict[str, Any]:
    """Run the single nontechnical-user MVP workflow end to end.

    Loads a bundled scenario or manually entered voters, computes the
    majority or supermajority winset of the status quo, builds the
    spatial voting canvas payload (optionally with Pareto set,
    indifference curve, and preferred-region overlays), and returns a
    plain, JSON-serializable dict with both the canvas payload (ready
    for ``save_plot`` / the existing canvas JavaScript) and a small
    text summary.

    Parameters
    ----------
    request:
        A plain dict with the following keys:

        scenario_name (str, optional):
            One of ``list_scenarios()``. Mutually exclusive with
            ``manual_voters``.
        manual_voters (list[[float, float]], optional):
            Voter ideal points as ``[[x, y], ...]``. Requires
            ``manual_status_quo``. Mutually exclusive with
            ``scenario_name``.
        manual_status_quo ([float, float], optional):
            Required when ``manual_voters`` is given.
        decision_rule (dict, optional):
            ``{"kind": "majority"}`` (default) or
            ``{"kind": "supermajority", "fraction": 0.5 < f <= 1.0}``.
        overlays (dict, optional):
            Booleans for ``"pareto"``, ``"indifference_curves"``, and
            ``"preferred_regions"``. All default to ``False``.
        theme (str, optional):
            Passed through to the plot layers. Defaults to ``"dark2"``.

    Returns
    -------
    dict
        ``{"canvas": <canvas payload dict>, "summary": {...}}``. Every
        value in the returned dict is a plain Python ``str``, ``int``,
        ``float``, ``bool``, ``list``, ``dict``, or ``None`` — safe to
        pass through ``json.dumps``.

    Raises
    ------
    ValueError
        If the request is missing required fields or contains invalid
        values. Every message states what was wrong, what was
        provided, and how to fix it.

    Examples
    --------
    >>> import socialchoicelab as scl
    >>> result = scl.run_spatial_voting_mvp({
    ...     "scenario_name": "laing_olmsted_bear",
    ...     "decision_rule": {"kind": "majority"},
    ...     "overlays": {"pareto": True},
    ... })
    >>> sorted(result.keys())
    ['canvas', 'summary']
    """
    if not isinstance(request, dict):
        raise ValueError(
            "run_spatial_voting_mvp: 'request' must be a dict. You provided "
            f"{type(request).__name__}."
        )

    voters_flat, sq, voter_names, dim_names, source_label = _resolve_scenario_or_manual(
        request
    )
    n_voters = len(voters_flat) // 2
    k, rule_summary = _resolve_decision_rule(request, n_voters)
    overlays = _resolve_overlays(request)
    theme = request.get("theme", "dark2")
    if not isinstance(theme, str):
        raise ValueError(
            f"run_spatial_voting_mvp: 'theme' must be a string. You provided {theme!r}."
        )

    fig = plot_spatial_voting(
        voters_flat,
        sq=sq,
        voter_names=voter_names,
        dim_names=tuple(dim_names) if len(dim_names) == 2 else ("Dimension 1", "Dimension 2"),
        title=source_label,
        theme=theme,
    )

    with winset_2d(float(sq[0]), float(sq[1]), voters_flat, k=k) as winset:
        winset_is_empty = winset.is_empty()
        layer_winset(fig, winset=winset, theme=theme)

    if overlays["pareto"]:
        hull = convex_hull_2d(voters_flat)
        layer_convex_hull(fig, hull, theme=theme)
    if overlays["indifference_curves"]:
        layer_ic(fig, voters_flat, sq, theme=theme)
    if overlays["preferred_regions"]:
        layer_preferred_regions(fig, voters_flat, sq, theme=theme)

    summary = {
        "source": source_label,
        "n_voters": n_voters,
        "status_quo": [float(sq[0]), float(sq[1])],
        "decision_rule": rule_summary,
        "overlays": overlays,
        "winset_is_empty": bool(winset_is_empty),
    }
    return {"canvas": fig, "summary": summary}
