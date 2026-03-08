"""scenarios.py — Built-in named scenario datasets for spatial voting.

Users access scenarios via convenience functions: list_scenarios() and load_scenario().
The JSON data is bundled in the package and never exposed to users.
"""

import json
import os
from typing import Any, Dict, List


def _scenarios_dir() -> str:
    """Return the absolute path to the bundled scenarios directory.

    Raises:
        RuntimeError: If the scenarios directory does not exist.
    """
    d = os.path.join(os.path.dirname(__file__), "data", "scenarios")
    if not os.path.isdir(d):
        raise RuntimeError(
            f"Scenario data directory not found at {d}. "
            "Reinstall the package to fix this issue."
        )
    return d


def list_scenarios() -> List[str]:
    """List available built-in scenarios.

    Returns:
        Sorted list of scenario identifiers that can be passed to load_scenario().

    Example:
        >>> scenarios = list_scenarios()
        >>> print(scenarios)
        ['laing_olmsted_bear', 'tovey_regular_polygon']
    """
    d = _scenarios_dir()
    files = [f for f in os.listdir(d) if f.endswith(".json")]
    return sorted([f[:-5] for f in files])  # strip .json


def load_scenario(name: str) -> Dict[str, Any]:
    """Load a built-in named scenario.

    Reads a bundled JSON scenario file and returns a dictionary ready to pass to
    plot_spatial_voting() and the computation functions.

    Args:
        name: Scenario identifier, one of the names returned by list_scenarios()
              (e.g., 'laing_olmsted_bear').

    Returns:
        A dictionary with keys:
            name (str): Full display name.
            description (str): Short description.
            source (str): Bibliographic source.
            n_voters (int): Number of voters.
            n_dimensions (int): Number of policy dimensions.
            space (dict): Contains 'x_range' and 'y_range', each a [min, max] pair.
            decision_rule (float): Majority threshold, e.g. 0.5.
            dim_names (list[str]): Axis labels (length n_dimensions).
            voters (numpy.ndarray): Flat array [x0, y0, x1, y1, ...] of shape
                (n_voters * n_dimensions,).
            voter_names (list[str]): Length n_voters.
            status_quo (numpy.ndarray or None): Length n_dimensions, or None.

    Raises:
        ValueError: If scenario name is unknown.

    Example:
        >>> import socialchoicelab as scl
        >>> sc = scl.load_scenario('laing_olmsted_bear')
        >>> sc['n_voters']  # 5
        5
        >>> sc['voters']
        array([ 20.8,  79.6, 100.4,  69.2, ...])
        >>> sc['status_quo']
        array([100., 100.])
    """
    available = list_scenarios()
    fpath = os.path.join(_scenarios_dir(), f"{name}.json")

    if not os.path.isfile(fpath):
        raise ValueError(
            f"Unknown scenario '{name}'. "
            f"Available scenarios: {', '.join(available)}."
        )

    import numpy as np

    with open(fpath, "r") as f:
        raw = json.load(f)

    # Convert voters from [[x, y], ...] to flat [x0, y0, x1, y1, ...]
    voters_flat = np.array(raw["voters"], dtype=float).flatten()

    # status_quo can be None or an array
    status_quo = None
    sq = raw.get("status_quo")
    if sq is not None and (isinstance(sq, list) and len(sq) > 0):
        status_quo = np.array(sq, dtype=float)

    return {
        "name": raw["name"],
        "description": raw.get("description", ""),
        "source": raw.get("source", ""),
        "n_voters": int(raw["n_voters"]),
        "n_dimensions": int(raw["n_dimensions"]),
        "space": raw["space"],
        "decision_rule": raw.get("decision_rule", 0.5),
        "dim_names": raw["dim_names"],
        "voters": voters_flat,
        "voter_names": raw["voter_names"],
        "status_quo": status_quo,
    }
