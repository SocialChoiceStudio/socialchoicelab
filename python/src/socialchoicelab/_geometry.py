"""Geometry functions: Copeland, Condorcet, core, uncovered set.

All indices are 0-based.  Array inputs accept any array-like.
"""

from __future__ import annotations

import numpy as np

from socialchoicelab._error import _check, new_err_buf
from socialchoicelab._loader import _ffi, _lib
from socialchoicelab._types import DistanceConfig, _c_double_array, _to_cffi_dist

__all__ = [
    "copeland_scores_2d",
    "copeland_winner_2d",
    "has_condorcet_winner_2d",
    "condorcet_winner_2d",
    "core_2d",
    "uncovered_set_2d",
    "uncovered_set_boundary_2d",
]

_ERR = 512


def _dist2(dist_config):
    if dist_config is None:
        dist_config = DistanceConfig()
    return _to_cffi_dist(dist_config, n_dims=2)


# ---------------------------------------------------------------------------
# Copeland
# ---------------------------------------------------------------------------


def copeland_scores_2d(
    alt_xy,
    voter_ideals_xy,
    dist_config: DistanceConfig | None = None,
    k: int = -1,
) -> np.ndarray:
    """Compute Copeland scores for each alternative.

    The Copeland score of alternative i is the number of alternatives it
    beats in pairwise comparisons minus the number it loses to.

    Parameters
    ----------
    alt_xy:
        Flat ``[x0, y0, …]`` alternative positions.
    voter_ideals_xy:
        Flat ``[x0, y0, …]`` voter ideal points.
    dist_config:
        Distance configuration.  ``None`` defaults to Euclidean.
    k:
        Majority threshold (``-1`` = simple majority).

    Returns
    -------
    numpy.ndarray of shape ``(n_alts,)`` and dtype ``int32``.
    """
    cfg, ka = _dist2(dist_config)
    alt_buf, alt_arr = _c_double_array(alt_xy)
    vtr_buf, vtr_arr = _c_double_array(voter_ideals_xy)
    n_alts = len(alt_arr) // 2
    n_voters = len(vtr_arr) // 2
    scores = np.zeros(n_alts, dtype=np.int32)
    s_buf = _ffi.cast("int *", _ffi.from_buffer(scores))
    err = new_err_buf()
    _check(
        _lib.scs_copeland_scores_2d(alt_buf, n_alts, vtr_buf, n_voters, cfg, int(k), s_buf, n_alts, err, _ERR),
        err,
    )
    return scores


def copeland_winner_2d(
    alt_xy,
    voter_ideals_xy,
    dist_config: DistanceConfig | None = None,
    k: int = -1,
) -> int:
    """Find the 0-based index of the Copeland winner.

    Ties are broken by smallest alternative index.

    Parameters
    ----------
    alt_xy:
        Flat ``[x0, y0, …]`` alternative positions.
    voter_ideals_xy:
        Flat ``[x0, y0, …]`` voter ideal points.
    dist_config:
        Distance configuration.  ``None`` defaults to Euclidean.
    k:
        Majority threshold (``-1`` = simple majority).

    Returns
    -------
    int
        0-based index of the winning alternative.
    """
    cfg, ka = _dist2(dist_config)
    alt_buf, alt_arr = _c_double_array(alt_xy)
    vtr_buf, vtr_arr = _c_double_array(voter_ideals_xy)
    n_alts = len(alt_arr) // 2
    n_voters = len(vtr_arr) // 2
    out = _ffi.new("int *")
    err = new_err_buf()
    _check(
        _lib.scs_copeland_winner_2d(alt_buf, n_alts, vtr_buf, n_voters, cfg, int(k), out, err, _ERR),
        err,
    )
    return int(out[0])


# ---------------------------------------------------------------------------
# Condorcet and core
# ---------------------------------------------------------------------------


def has_condorcet_winner_2d(
    alt_xy,
    voter_ideals_xy,
    dist_config: DistanceConfig | None = None,
    k: int = -1,
) -> bool:
    """Check whether a Condorcet winner exists in a finite alternative set.

    Parameters
    ----------
    alt_xy:
        Flat ``[x0, y0, …]`` alternative positions.
    voter_ideals_xy:
        Flat ``[x0, y0, …]`` voter ideal points.
    dist_config:
        Distance configuration.  ``None`` defaults to Euclidean.
    k:
        Majority threshold (``-1`` = simple majority).

    Returns
    -------
    bool
    """
    cfg, ka = _dist2(dist_config)
    alt_buf, alt_arr = _c_double_array(alt_xy)
    vtr_buf, vtr_arr = _c_double_array(voter_ideals_xy)
    n_alts = len(alt_arr) // 2
    n_voters = len(vtr_arr) // 2
    out = _ffi.new("int *")
    err = new_err_buf()
    _check(
        _lib.scs_has_condorcet_winner_2d(alt_buf, n_alts, vtr_buf, n_voters, cfg, int(k), out, err, _ERR),
        err,
    )
    return bool(out[0])


def condorcet_winner_2d(
    alt_xy,
    voter_ideals_xy,
    dist_config: DistanceConfig | None = None,
    k: int = -1,
) -> int | None:
    """Find the 0-based index of the Condorcet winner, or ``None`` if none exists.

    Parameters
    ----------
    alt_xy:
        Flat ``[x0, y0, …]`` alternative positions.
    voter_ideals_xy:
        Flat ``[x0, y0, …]`` voter ideal points.
    dist_config:
        Distance configuration.  ``None`` defaults to Euclidean.
    k:
        Majority threshold (``-1`` = simple majority).

    Returns
    -------
    int or None
    """
    cfg, ka = _dist2(dist_config)
    alt_buf, alt_arr = _c_double_array(alt_xy)
    vtr_buf, vtr_arr = _c_double_array(voter_ideals_xy)
    n_alts = len(alt_arr) // 2
    n_voters = len(vtr_arr) // 2
    out_found = _ffi.new("int *")
    out_idx = _ffi.new("int *")
    err = new_err_buf()
    _check(
        _lib.scs_condorcet_winner_2d(
            alt_buf, n_alts, vtr_buf, n_voters, cfg, int(k), out_found, out_idx, err, _ERR
        ),
        err,
    )
    if not out_found[0]:
        return None
    return int(out_idx[0])


def core_2d(
    voter_ideals_xy,
    dist_config: DistanceConfig | None = None,
    k: int = -1,
) -> dict | None:
    """Compute the core in continuous 2D space.

    The core is non-empty only when a Condorcet point exists.

    Parameters
    ----------
    voter_ideals_xy:
        Flat ``[x0, y0, …]`` voter ideal points.
    dist_config:
        Distance configuration.  ``None`` defaults to Euclidean.
    k:
        Majority threshold (``-1`` = simple majority).

    Returns
    -------
    dict or None
        ``{"x": float, "y": float}`` when a core point exists, else ``None``.
    """
    cfg, ka = _dist2(dist_config)
    vtr_buf, vtr_arr = _c_double_array(voter_ideals_xy)
    n_voters = len(vtr_arr) // 2
    out_found = _ffi.new("int *")
    out_x = _ffi.new("double *")
    out_y = _ffi.new("double *")
    err = new_err_buf()
    _check(
        _lib.scs_core_2d(vtr_buf, n_voters, cfg, int(k), out_found, out_x, out_y, err, _ERR),
        err,
    )
    if not out_found[0]:
        return None
    return {"x": float(out_x[0]), "y": float(out_y[0])}


# ---------------------------------------------------------------------------
# Uncovered set
# ---------------------------------------------------------------------------


def uncovered_set_2d(
    alt_xy,
    voter_ideals_xy,
    dist_config: DistanceConfig | None = None,
    k: int = -1,
) -> np.ndarray:
    """Compute the uncovered set over a finite alternative set.

    An alternative is uncovered if no other alternative beats it both
    directly and transitively (Miller 1980 covering relation).

    Parameters
    ----------
    alt_xy:
        Flat ``[x0, y0, …]`` alternative positions.
    voter_ideals_xy:
        Flat ``[x0, y0, …]`` voter ideal points.
    dist_config:
        Distance configuration.  ``None`` defaults to Euclidean.
    k:
        Majority threshold (``-1`` = simple majority).

    Returns
    -------
    numpy.ndarray of shape ``(n_uncovered,)`` and dtype ``int32``.
        0-based indices of uncovered alternatives.
    """
    cfg, ka = _dist2(dist_config)
    alt_buf, alt_arr = _c_double_array(alt_xy)
    vtr_buf, vtr_arr = _c_double_array(voter_ideals_xy)
    n_alts = len(alt_arr) // 2
    n_voters = len(vtr_arr) // 2

    # Size query
    out_n = _ffi.new("int *")
    err = new_err_buf()
    _check(
        _lib.scs_uncovered_set_2d(alt_buf, n_alts, vtr_buf, n_voters, cfg, int(k), _ffi.NULL, 0, out_n, err, _ERR),
        err,
    )
    n = int(out_n[0])

    # Fill
    idx = np.zeros(n, dtype=np.int32)
    idx_buf = _ffi.cast("int *", _ffi.from_buffer(idx))
    err = new_err_buf()
    _check(
        _lib.scs_uncovered_set_2d(alt_buf, n_alts, vtr_buf, n_voters, cfg, int(k), idx_buf, n, out_n, err, _ERR),
        err,
    )
    return idx


def uncovered_set_boundary_2d(
    voter_ideals_xy,
    dist_config: DistanceConfig | None = None,
    k: int = -1,
    grid_resolution: int = 15,
) -> np.ndarray:
    """Export the approximate boundary of the continuous uncovered set.

    Parameters
    ----------
    voter_ideals_xy:
        Flat ``[x0, y0, …]`` voter ideal points.
    dist_config:
        Distance configuration.  ``None`` defaults to Euclidean.
    k:
        Majority threshold (``-1`` = simple majority).
    grid_resolution:
        Grid resolution for the approximation (higher = finer).

    Returns
    -------
    numpy.ndarray of shape ``(n_pts, 2)``.
    """
    cfg, ka = _dist2(dist_config)
    vtr_buf, vtr_arr = _c_double_array(voter_ideals_xy)
    n_voters = len(vtr_arr) // 2

    # Size query
    out_n_pairs = _ffi.new("int *")
    err = new_err_buf()
    _check(
        _lib.scs_uncovered_set_boundary_size_2d(
            vtr_buf, n_voters, cfg, int(grid_resolution), int(k), out_n_pairs, err, _ERR
        ),
        err,
    )
    n_pts = int(out_n_pairs[0])

    if n_pts == 0:
        return np.empty((0, 2), dtype=np.float64)

    # Fill
    out_n = _ffi.new("int *")
    xy = np.zeros(n_pts * 2, dtype=np.float64)
    xy_buf = _ffi.cast("double *", _ffi.from_buffer(xy))
    err = new_err_buf()
    _check(
        _lib.scs_uncovered_set_boundary_2d(
            vtr_buf, n_voters, cfg, int(grid_resolution), int(k), xy_buf, n_pts, out_n, err, _ERR
        ),
        err,
    )
    return xy.reshape(int(out_n[0]), 2).copy()
