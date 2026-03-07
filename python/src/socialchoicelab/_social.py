"""Social rankings and consistency predicates.

All indices are 0-based.
"""

from __future__ import annotations

import numpy as np

from socialchoicelab._error import _check, new_err_buf
from socialchoicelab._loader import _ffi, _lib
from socialchoicelab._voting_rules import _resolve_tb

__all__ = [
    "rank_by_scores",
    "pairwise_ranking_from_matrix",
    "pareto_set",
    "is_pareto_efficient",
    "has_condorcet_winner_profile",
    "condorcet_winner_profile",
    "is_condorcet_consistent",
    "is_majority_consistent",
]

_ERR = 512


def _prof_ptr(profile):
    return profile._handle()


# ---------------------------------------------------------------------------
# Rankings
# ---------------------------------------------------------------------------


def rank_by_scores(
    scores,
    tie_break: str = "smallest_index",
    stream_manager=None,
    stream_name: str | None = None,
) -> np.ndarray:
    """Sort alternatives by descending score with tie-breaking.

    Parameters
    ----------
    scores:
        Float array of length n_alts.
    tie_break:
        ``"smallest_index"`` (default) or ``"random"``.
    stream_manager, stream_name:
        Required when ``tie_break='random'``.

    Returns
    -------
    numpy.ndarray of shape ``(n_alts,)`` and dtype ``int32``.
        0-based alternative indices, best first.
    """
    s = np.asarray(scores, dtype=np.float64).ravel()
    n_alts = len(s)
    s_buf = _ffi.cast("double *", _ffi.from_buffer(s))
    tb, mgr, sn = _resolve_tb(tie_break, stream_manager, stream_name)
    ranking = np.zeros(n_alts, dtype=np.int32)
    r_buf = _ffi.cast("int *", _ffi.from_buffer(ranking))
    err = new_err_buf()
    _check(
        _lib.scs_rank_by_scores(s_buf, n_alts, tb, mgr, sn or _ffi.NULL, r_buf, n_alts, err, _ERR),
        err,
    )
    return ranking


def pairwise_ranking_from_matrix(
    pairwise_matrix,
    tie_break: str = "smallest_index",
    stream_manager=None,
    stream_name: str | None = None,
) -> np.ndarray:
    """Rank alternatives by Copeland score derived from a pairwise matrix.

    Parameters
    ----------
    pairwise_matrix:
        Array of shape ``(n_alts, n_alts)`` and dtype ``int32`` (e.g. from
        :func:`~socialchoicelab.pairwise_matrix_2d`).
    tie_break:
        ``"smallest_index"`` (default) or ``"random"``.
    stream_manager, stream_name:
        Required when ``tie_break='random'``.

    Returns
    -------
    numpy.ndarray of shape ``(n_alts,)`` and dtype ``int32``.
        0-based alternative indices, best (highest Copeland score) first.
    """
    mat = np.asarray(pairwise_matrix, dtype=np.int32)
    if mat.ndim != 2 or mat.shape[0] != mat.shape[1]:
        raise ValueError(f"pairwise_matrix must be square, got shape {mat.shape}.")
    n_alts = mat.shape[0]
    mat = np.ascontiguousarray(mat)
    m_buf = _ffi.cast("int *", _ffi.from_buffer(mat))
    tb, mgr, sn = _resolve_tb(tie_break, stream_manager, stream_name)
    ranking = np.zeros(n_alts, dtype=np.int32)
    r_buf = _ffi.cast("int *", _ffi.from_buffer(ranking))
    err = new_err_buf()
    _check(
        _lib.scs_pairwise_ranking_from_matrix(
            m_buf, n_alts, tb, mgr, sn or _ffi.NULL, r_buf, n_alts, err, _ERR
        ),
        err,
    )
    return ranking


# ---------------------------------------------------------------------------
# Pareto efficiency
# ---------------------------------------------------------------------------


def pareto_set(profile) -> np.ndarray:
    """Return the Pareto set: all alternatives not Pareto-dominated.

    Parameters
    ----------
    profile:
        A :class:`~socialchoicelab.Profile` instance.

    Returns
    -------
    numpy.ndarray of shape ``(n_pareto,)`` and dtype ``int32``.
        0-based indices.
    """
    # Size query
    out_n = _ffi.new("int *")
    err = new_err_buf()
    _check(_lib.scs_pareto_set(_prof_ptr(profile), _ffi.NULL, 0, out_n, err, _ERR), err)
    n = int(out_n[0])
    result = np.zeros(n, dtype=np.int32)
    r_buf = _ffi.cast("int *", _ffi.from_buffer(result))
    err = new_err_buf()
    _check(_lib.scs_pareto_set(_prof_ptr(profile), r_buf, n, out_n, err, _ERR), err)
    return result


def is_pareto_efficient(profile, alt_idx: int) -> bool:
    """Check whether a single alternative is Pareto-efficient.

    Parameters
    ----------
    profile:
        A :class:`~socialchoicelab.Profile` instance.
    alt_idx:
        0-based index of the alternative to test.

    Returns
    -------
    bool
    """
    out = _ffi.new("int *")
    err = new_err_buf()
    _check(_lib.scs_is_pareto_efficient(_prof_ptr(profile), int(alt_idx), out, err, _ERR), err)
    return bool(out[0])


# ---------------------------------------------------------------------------
# Condorcet and majority-selection predicates
# ---------------------------------------------------------------------------


def has_condorcet_winner_profile(profile) -> bool:
    """Check whether the profile's majority relation has a Condorcet winner.

    Parameters
    ----------
    profile:
        A :class:`~socialchoicelab.Profile` instance.

    Returns
    -------
    bool
    """
    out = _ffi.new("int *")
    err = new_err_buf()
    _check(_lib.scs_has_condorcet_winner_profile(_prof_ptr(profile), out, err, _ERR), err)
    return bool(out[0])


def condorcet_winner_profile(profile) -> int | None:
    """Return the Condorcet winner from the profile, or ``None`` if none exists.

    Parameters
    ----------
    profile:
        A :class:`~socialchoicelab.Profile` instance.

    Returns
    -------
    int or None
        0-based index of the Condorcet winner, or ``None``.
    """
    out_found = _ffi.new("int *")
    out_winner = _ffi.new("int *")
    err = new_err_buf()
    _check(
        _lib.scs_condorcet_winner_profile(_prof_ptr(profile), out_found, out_winner, err, _ERR),
        err,
    )
    if not out_found[0]:
        return None
    return int(out_winner[0])


def is_condorcet_consistent(profile, alt_idx: int) -> bool:
    """Check whether an alternative is Condorcet-consistent.

    Returns ``True`` if no Condorcet winner exists (vacuously consistent)
    or if the alternative IS the Condorcet winner.

    Parameters
    ----------
    profile:
        A :class:`~socialchoicelab.Profile` instance.
    alt_idx:
        0-based index of the alternative to test.

    Returns
    -------
    bool
    """
    out = _ffi.new("int *")
    err = new_err_buf()
    _check(
        _lib.scs_is_selected_by_condorcet_consistent_rules(
            _prof_ptr(profile), int(alt_idx), out, err, _ERR
        ),
        err,
    )
    return bool(out[0])


def is_majority_consistent(profile, alt_idx: int) -> bool:
    """Check whether an alternative is majority-consistent.

    Returns ``True`` only if the alternative IS the Condorcet winner.
    Unlike :func:`is_condorcet_consistent`, this returns ``False`` for
    every candidate when no Condorcet winner exists.

    Parameters
    ----------
    profile:
        A :class:`~socialchoicelab.Profile` instance.
    alt_idx:
        0-based index of the alternative to test.

    Returns
    -------
    bool
    """
    out = _ffi.new("int *")
    err = new_err_buf()
    _check(
        _lib.scs_is_selected_by_majority_consistent_rules(
            _prof_ptr(profile), int(alt_idx), out, err, _ERR
        ),
        err,
    )
    return bool(out[0])
