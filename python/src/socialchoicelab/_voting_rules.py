"""Voting rules: plurality, Borda, anti-plurality, scoring rule, approval.

All indices are 0-based.  Tie-breaking is controlled by the ``tie_break``
parameter (``"smallest_index"`` or ``"random"``).
"""

from __future__ import annotations

import numpy as np

from socialchoicelab._error import _check, new_err_buf
from socialchoicelab._loader import _ffi, _lib
from socialchoicelab._types import DistanceConfig, _c_double_array, _to_cffi_dist

__all__ = [
    "plurality_scores",
    "plurality_all_winners",
    "plurality_one_winner",
    "borda_scores",
    "borda_all_winners",
    "borda_one_winner",
    "borda_ranking",
    "antiplurality_scores",
    "antiplurality_all_winners",
    "antiplurality_one_winner",
    "scoring_rule_scores",
    "scoring_rule_all_winners",
    "scoring_rule_one_winner",
    "approval_scores_spatial",
    "approval_all_winners_spatial",
    "approval_scores_topk",
    "approval_all_winners_topk",
]

_ERR = 512

# ---------------------------------------------------------------------------
# Tie-break helper
# ---------------------------------------------------------------------------

_TB_MAP = {"smallest_index": 1, "random": 0}


def _resolve_tb(tie_break: str, stream_manager, stream_name):
    """Return (tb_int, mgr_ptr, sn_bytes).

    ``mgr_ptr`` and ``sn_bytes`` are cffi NULL / None when not needed.
    """
    tb_str = tie_break.lower()
    tb = _TB_MAP.get(tb_str)
    if tb is None:
        raise ValueError(
            f"tie_break must be 'smallest_index' or 'random', got {tie_break!r}."
        )
    if tb == 0:  # random
        if stream_manager is None:
            raise ValueError("tie_break='random' requires a StreamManager instance.")
        if stream_name is None:
            raise ValueError("tie_break='random' requires a stream_name.")
        return tb, stream_manager._handle(), stream_name.encode()
    return tb, _ffi.NULL, _ffi.NULL


def _prof_ptr(profile):
    return profile._handle()


def _all_winners_from(call_fn, *args_before_buf):
    """Hidden two-call pattern for *_all_winners functions.

    ``call_fn(args_before_buf..., buf, capacity, out_n, err, ERR)``
    """
    out_n = _ffi.new("int *")
    err = new_err_buf()
    _check(call_fn(*args_before_buf, _ffi.NULL, 0, out_n, err, _ERR), err)
    n = int(out_n[0])
    result = np.zeros(n, dtype=np.int32)
    r_buf = _ffi.cast("int *", _ffi.from_buffer(result))
    err = new_err_buf()
    _check(call_fn(*args_before_buf, r_buf, n, out_n, err, _ERR), err)
    return result


# ---------------------------------------------------------------------------
# Plurality
# ---------------------------------------------------------------------------


def plurality_scores(profile) -> np.ndarray:
    """Compute plurality scores (first-place vote counts per alternative).

    Parameters
    ----------
    profile:
        A :class:`~socialchoicelab.Profile` instance.

    Returns
    -------
    numpy.ndarray of shape ``(n_alts,)`` and dtype ``int32``.
    """
    _, n_alts = profile.dims()
    scores = np.zeros(n_alts, dtype=np.int32)
    s_buf = _ffi.cast("int *", _ffi.from_buffer(scores))
    err = new_err_buf()
    _check(_lib.scs_plurality_scores(_prof_ptr(profile), s_buf, n_alts, err, _ERR), err)
    return scores


def plurality_all_winners(profile) -> np.ndarray:
    """Return all plurality winners (tied for the most first-place votes).

    Parameters
    ----------
    profile:
        A :class:`~socialchoicelab.Profile` instance.

    Returns
    -------
    numpy.ndarray of shape ``(n_winners,)`` and dtype ``int32``.
        0-based alternative indices.
    """
    return _all_winners_from(_lib.scs_plurality_all_winners, _prof_ptr(profile))


def plurality_one_winner(
    profile,
    tie_break: str = "smallest_index",
    stream_manager=None,
    stream_name: str | None = None,
) -> int:
    """Return a single plurality winner, applying tie-breaking if needed.

    Parameters
    ----------
    profile:
        A :class:`~socialchoicelab.Profile` instance.
    tie_break:
        ``"smallest_index"`` (default) or ``"random"``.
    stream_manager:
        Required when ``tie_break='random'``.
    stream_name:
        Named stream for randomness; required with ``"random"``.

    Returns
    -------
    int
        0-based index of the winner.
    """
    tb, mgr, sn = _resolve_tb(tie_break, stream_manager, stream_name)
    out = _ffi.new("int *")
    err = new_err_buf()
    _check(
        _lib.scs_plurality_one_winner(_prof_ptr(profile), tb, mgr, sn or _ffi.NULL, out, err, _ERR),
        err,
    )
    return int(out[0])


# ---------------------------------------------------------------------------
# Borda
# ---------------------------------------------------------------------------


def borda_scores(profile) -> np.ndarray:
    """Compute Borda scores.

    The Borda score of alt j is the sum over voters of (n_alts - 1 - rank_of_j).

    Parameters
    ----------
    profile:
        A :class:`~socialchoicelab.Profile` instance.

    Returns
    -------
    numpy.ndarray of shape ``(n_alts,)`` and dtype ``int32``.
    """
    _, n_alts = profile.dims()
    scores = np.zeros(n_alts, dtype=np.int32)
    s_buf = _ffi.cast("int *", _ffi.from_buffer(scores))
    err = new_err_buf()
    _check(_lib.scs_borda_scores(_prof_ptr(profile), s_buf, n_alts, err, _ERR), err)
    return scores


def borda_all_winners(profile) -> np.ndarray:
    """Return all Borda winners.

    Returns
    -------
    numpy.ndarray of shape ``(n_winners,)`` and dtype ``int32``.
    """
    return _all_winners_from(_lib.scs_borda_all_winners, _prof_ptr(profile))


def borda_one_winner(
    profile,
    tie_break: str = "smallest_index",
    stream_manager=None,
    stream_name: str | None = None,
) -> int:
    """Return a single Borda winner with tie-breaking.

    Returns
    -------
    int
    """
    tb, mgr, sn = _resolve_tb(tie_break, stream_manager, stream_name)
    out = _ffi.new("int *")
    err = new_err_buf()
    _check(
        _lib.scs_borda_one_winner(_prof_ptr(profile), tb, mgr, sn or _ffi.NULL, out, err, _ERR),
        err,
    )
    return int(out[0])


def borda_ranking(
    profile,
    tie_break: str = "smallest_index",
    stream_manager=None,
    stream_name: str | None = None,
) -> np.ndarray:
    """Full social ordering by descending Borda score.

    Parameters
    ----------
    profile:
        A :class:`~socialchoicelab.Profile` instance.
    tie_break:
        Tie-breaking rule within score groups.
    stream_manager, stream_name:
        Required when ``tie_break='random'``.

    Returns
    -------
    numpy.ndarray of shape ``(n_alts,)`` and dtype ``int32``.
        0-based alternative indices, most preferred first.
    """
    tb, mgr, sn = _resolve_tb(tie_break, stream_manager, stream_name)
    _, n_alts = profile.dims()
    ranking = np.zeros(n_alts, dtype=np.int32)
    r_buf = _ffi.cast("int *", _ffi.from_buffer(ranking))
    err = new_err_buf()
    _check(
        _lib.scs_borda_ranking(_prof_ptr(profile), tb, mgr, sn or _ffi.NULL, r_buf, n_alts, err, _ERR),
        err,
    )
    return ranking


# ---------------------------------------------------------------------------
# Anti-plurality
# ---------------------------------------------------------------------------


def antiplurality_scores(profile) -> np.ndarray:
    """Compute anti-plurality scores.

    score[j] = number of voters for whom alternative j is NOT last.

    Parameters
    ----------
    profile:
        A :class:`~socialchoicelab.Profile` instance.

    Returns
    -------
    numpy.ndarray of shape ``(n_alts,)`` and dtype ``int32``.
    """
    _, n_alts = profile.dims()
    scores = np.zeros(n_alts, dtype=np.int32)
    s_buf = _ffi.cast("int *", _ffi.from_buffer(scores))
    err = new_err_buf()
    _check(_lib.scs_antiplurality_scores(_prof_ptr(profile), s_buf, n_alts, err, _ERR), err)
    return scores


def antiplurality_all_winners(profile) -> np.ndarray:
    """Return all anti-plurality winners.

    Returns
    -------
    numpy.ndarray of shape ``(n_winners,)`` and dtype ``int32``.
    """
    return _all_winners_from(_lib.scs_antiplurality_all_winners, _prof_ptr(profile))


def antiplurality_one_winner(
    profile,
    tie_break: str = "smallest_index",
    stream_manager=None,
    stream_name: str | None = None,
) -> int:
    """Return a single anti-plurality winner with tie-breaking.

    Returns
    -------
    int
    """
    tb, mgr, sn = _resolve_tb(tie_break, stream_manager, stream_name)
    out = _ffi.new("int *")
    err = new_err_buf()
    _check(
        _lib.scs_antiplurality_one_winner(_prof_ptr(profile), tb, mgr, sn or _ffi.NULL, out, err, _ERR),
        err,
    )
    return int(out[0])


# ---------------------------------------------------------------------------
# Generic positional scoring rule
# ---------------------------------------------------------------------------


def scoring_rule_scores(profile, score_weights) -> np.ndarray:
    """Compute scores under a generic positional scoring rule.

    Parameters
    ----------
    profile:
        A :class:`~socialchoicelab.Profile` instance.
    score_weights:
        Non-increasing array of length n_alts. ``score_weights[r]`` is the
        score awarded to the alternative ranked ``r``-th (0 = best).

    Returns
    -------
    numpy.ndarray of shape ``(n_alts,)`` and dtype ``float64``.
    """
    sw = np.asarray(score_weights, dtype=np.float64).ravel()
    sw_buf = _ffi.cast("double *", _ffi.from_buffer(sw))
    _, n_alts = profile.dims()
    scores = np.zeros(n_alts, dtype=np.float64)
    s_buf = _ffi.cast("double *", _ffi.from_buffer(scores))
    err = new_err_buf()
    _check(
        _lib.scs_scoring_rule_scores(
            _prof_ptr(profile), sw_buf, len(sw), s_buf, n_alts, err, _ERR
        ),
        err,
    )
    return scores


def scoring_rule_all_winners(profile, score_weights) -> np.ndarray:
    """Return all scoring-rule winners.

    Parameters
    ----------
    score_weights:
        Non-increasing weight array of length n_alts.

    Returns
    -------
    numpy.ndarray of shape ``(n_winners,)`` and dtype ``int32``.
    """
    sw = np.asarray(score_weights, dtype=np.float64).ravel()
    sw_buf = _ffi.cast("double *", _ffi.from_buffer(sw))
    return _all_winners_from(_lib.scs_scoring_rule_all_winners, _prof_ptr(profile), sw_buf, len(sw))


def scoring_rule_one_winner(
    profile,
    score_weights,
    tie_break: str = "smallest_index",
    stream_manager=None,
    stream_name: str | None = None,
) -> int:
    """Return a single scoring-rule winner with tie-breaking.

    Returns
    -------
    int
    """
    sw = np.asarray(score_weights, dtype=np.float64).ravel()
    sw_buf = _ffi.cast("double *", _ffi.from_buffer(sw))
    tb, mgr, sn = _resolve_tb(tie_break, stream_manager, stream_name)
    out = _ffi.new("int *")
    err = new_err_buf()
    _check(
        _lib.scs_scoring_rule_one_winner(
            _prof_ptr(profile), sw_buf, len(sw), tb, mgr, sn or _ffi.NULL, out, err, _ERR
        ),
        err,
    )
    return int(out[0])


# ---------------------------------------------------------------------------
# Approval voting — spatial threshold model
# ---------------------------------------------------------------------------


def approval_scores_spatial(
    alt_xy,
    voter_ideals_xy,
    threshold: float,
    dist_config: DistanceConfig | None = None,
) -> np.ndarray:
    """Compute approval scores under the spatial threshold model.

    A voter approves an alternative if its distance from the voter's ideal
    point is at most ``threshold``.

    Parameters
    ----------
    alt_xy:
        Flat ``[x0, y0, …]`` alternative positions.
    voter_ideals_xy:
        Flat ``[x0, y0, …]`` voter ideal points.
    threshold:
        Non-negative approval radius.
    dist_config:
        Distance configuration.  ``None`` defaults to Euclidean.

    Returns
    -------
    numpy.ndarray of shape ``(n_alts,)`` and dtype ``int32``.
    """
    if dist_config is None:
        dist_config = DistanceConfig()
    cfg, ka = _to_cffi_dist(dist_config, n_dims=2)
    alt_buf, alt_arr = _c_double_array(alt_xy)
    vtr_buf, vtr_arr = _c_double_array(voter_ideals_xy)
    n_alts = len(alt_arr) // 2
    n_voters = len(vtr_arr) // 2
    scores = np.zeros(n_alts, dtype=np.int32)
    s_buf = _ffi.cast("int *", _ffi.from_buffer(scores))
    err = new_err_buf()
    _check(
        _lib.scs_approval_scores_spatial(
            alt_buf, n_alts, vtr_buf, n_voters, float(threshold), cfg, s_buf, n_alts, err, _ERR
        ),
        err,
    )
    return scores


def approval_all_winners_spatial(
    alt_xy,
    voter_ideals_xy,
    threshold: float,
    dist_config: DistanceConfig | None = None,
) -> np.ndarray:
    """Return all spatial-approval winners.

    Parameters
    ----------
    alt_xy:
        Flat ``[x0, y0, …]`` alternative positions.
    voter_ideals_xy:
        Flat ``[x0, y0, …]`` voter ideal points.
    threshold:
        Approval radius.
    dist_config:
        Distance configuration.  ``None`` defaults to Euclidean.

    Returns
    -------
    numpy.ndarray of shape ``(n_winners,)`` and dtype ``int32``.
        Empty if no alternative receives any approval.
    """
    if dist_config is None:
        dist_config = DistanceConfig()
    cfg, ka = _to_cffi_dist(dist_config, n_dims=2)
    alt_buf, alt_arr = _c_double_array(alt_xy)
    vtr_buf, vtr_arr = _c_double_array(voter_ideals_xy)
    n_alts = len(alt_arr) // 2
    n_voters = len(vtr_arr) // 2
    return _all_winners_from(
        _lib.scs_approval_all_winners_spatial,
        alt_buf, n_alts, vtr_buf, n_voters, float(threshold), cfg,
    )


# ---------------------------------------------------------------------------
# Approval voting — ordinal top-k
# ---------------------------------------------------------------------------


def approval_scores_topk(profile, k: int) -> np.ndarray:
    """Compute ordinal top-k approval scores.

    Each voter approves their top ``k`` ranked alternatives.

    Parameters
    ----------
    profile:
        A :class:`~socialchoicelab.Profile` instance.
    k:
        Number of alternatives each voter approves (in ``[1, n_alts]``).

    Returns
    -------
    numpy.ndarray of shape ``(n_alts,)`` and dtype ``int32``.
    """
    _, n_alts = profile.dims()
    scores = np.zeros(n_alts, dtype=np.int32)
    s_buf = _ffi.cast("int *", _ffi.from_buffer(scores))
    err = new_err_buf()
    _check(
        _lib.scs_approval_scores_topk(_prof_ptr(profile), int(k), s_buf, n_alts, err, _ERR),
        err,
    )
    return scores


def approval_all_winners_topk(profile, k: int) -> np.ndarray:
    """Return all top-k approval winners.

    Parameters
    ----------
    profile:
        A :class:`~socialchoicelab.Profile` instance.
    k:
        Approval set size.

    Returns
    -------
    numpy.ndarray of shape ``(n_winners,)`` and dtype ``int32``.
    """
    return _all_winners_from(_lib.scs_approval_all_winners_topk, _prof_ptr(profile), int(k))
