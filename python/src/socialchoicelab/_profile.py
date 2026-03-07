"""Profile: ordinal preference profile.

Wraps the ``SCS_Profile`` C handle. Create profiles via the factory functions
(:func:`profile_build_spatial`, :func:`profile_from_utility_matrix`, etc.)
rather than instantiating ``Profile`` directly.

All indices are 0-based throughout (no index translation; the C API is
0-indexed and the Python binding follows the same convention).
"""

from __future__ import annotations

import numpy as np

from socialchoicelab._error import SCSError, _check, new_err_buf
from socialchoicelab._loader import _ffi, _lib
from socialchoicelab._types import DistanceConfig, _to_cffi_dist, _c_double_array

__all__ = [
    "Profile",
    "profile_build_spatial",
    "profile_from_utility_matrix",
    "profile_impartial_culture",
    "profile_uniform_spatial",
    "profile_gaussian_spatial",
]

_ERR_BUF_SIZE = 512


class Profile:
    """Wraps an ``SCS_Profile`` C handle.

    Do not construct directly — use one of the factory functions.

    Implements context-manager protocol for deterministic cleanup.
    ``__del__`` provides a GC safety net.
    """

    def __init__(self, ptr) -> None:
        if ptr == _ffi.NULL:
            raise SCSError("Received a null SCS_Profile pointer.")
        self._ptr = ptr
        self._destroyed = False

    def __del__(self) -> None:
        self._destroy()

    def __enter__(self) -> "Profile":
        return self

    def __exit__(self, *_) -> None:
        self._destroy()

    def _destroy(self) -> None:
        if not self._destroyed and self._ptr != _ffi.NULL:
            _lib.scs_profile_destroy(self._ptr)
            self._ptr = _ffi.NULL
            self._destroyed = True

    # ------------------------------------------------------------------
    # Inspection
    # ------------------------------------------------------------------

    def dims(self) -> tuple[int, int]:
        """Return ``(n_voters, n_alts)``.

        Returns
        -------
        tuple[int, int]
        """
        n_voters = _ffi.new("int *")
        n_alts = _ffi.new("int *")
        err = new_err_buf()
        _check(_lib.scs_profile_dims(self._ptr, n_voters, n_alts, err, _ERR_BUF_SIZE), err)
        return int(n_voters[0]), int(n_alts[0])

    def get_ranking(self, voter: int) -> np.ndarray:
        """Return one voter's ranking as a 0-based integer array.

        Parameters
        ----------
        voter:
            0-based voter index.

        Returns
        -------
        numpy.ndarray of shape ``(n_alts,)`` and dtype ``int32``.
            ``result[r]`` is the 0-based alternative index at rank ``r``
            (rank 0 = most preferred).
        """
        n_voters, n_alts = self.dims()
        if voter < 0 or voter >= n_voters:
            raise IndexError(
                f"get_ranking: voter index {voter} is out of range [0, {n_voters - 1}]."
            )
        buf = _ffi.new("int[]", n_alts)
        err = new_err_buf()
        _check(
            _lib.scs_profile_get_ranking(self._ptr, int(voter), buf, n_alts, err, _ERR_BUF_SIZE),
            err,
        )
        return np.frombuffer(_ffi.buffer(buf, n_alts * 4), dtype=np.int32).copy()

    def export(self) -> np.ndarray:
        """Export all rankings as a 2D array of shape ``(n_voters, n_alts)``.

        Returns
        -------
        numpy.ndarray of shape ``(n_voters, n_alts)`` and dtype ``int32``.
            ``result[v, r]`` is the 0-based alternative index that voter ``v``
            ranks at position ``r`` (0 = most preferred).
        """
        n_voters, n_alts = self.dims()
        buf = _ffi.new("int[]", n_voters * n_alts)
        err = new_err_buf()
        _check(
            _lib.scs_profile_export_rankings(self._ptr, buf, n_voters * n_alts, err, _ERR_BUF_SIZE),
            err,
        )
        return np.frombuffer(_ffi.buffer(buf, n_voters * n_alts * 4), dtype=np.int32).reshape(
            n_voters, n_alts
        ).copy()

    def clone(self) -> "Profile":
        """Return a deep copy of this profile.

        Returns
        -------
        Profile
        """
        err = new_err_buf()
        ptr = _lib.scs_profile_clone(self._ptr, err, _ERR_BUF_SIZE)
        if ptr == _ffi.NULL:
            from socialchoicelab._error import _ffi_string
            raise SCSError(f"scs_profile_clone failed: {_ffi_string(err)}")
        return Profile(ptr)

    # ------------------------------------------------------------------
    # Internal accessor
    # ------------------------------------------------------------------

    def _handle(self):
        return self._ptr


# ---------------------------------------------------------------------------
# Factory functions
# ---------------------------------------------------------------------------


def profile_build_spatial(
    alt_xy,
    voter_ideals_xy,
    dist_config: DistanceConfig | None = None,
) -> Profile:
    """Build an ordinal preference profile from a 2D spatial model.

    Each voter ranks alternatives by distance (closest = most preferred).
    Distance ties are broken by smallest alternative index.

    Parameters
    ----------
    alt_xy:
        Flat ``[x0, y0, x1, y1, …]`` array of alternative positions,
        length ``2 * n_alts``.
    voter_ideals_xy:
        Flat ``[x0, y0, …]`` array of voter ideal points,
        length ``2 * n_voters``.
    dist_config:
        Distance configuration.  ``None`` defaults to Euclidean.

    Returns
    -------
    Profile
    """
    if dist_config is None:
        dist_config = DistanceConfig()
    cfg, ka = _to_cffi_dist(dist_config, n_dims=2)
    alt_buf, alt_arr = _c_double_array(alt_xy)
    vtr_buf, vtr_arr = _c_double_array(voter_ideals_xy)
    n_alts = len(alt_arr) // 2
    n_voters = len(vtr_arr) // 2
    err = new_err_buf()
    ptr = _lib.scs_profile_build_spatial(
        alt_buf, n_alts, vtr_buf, n_voters, cfg, err, _ERR_BUF_SIZE
    )
    if ptr == _ffi.NULL:
        from socialchoicelab._error import _ffi_string
        raise SCSError(f"scs_profile_build_spatial failed: {_ffi_string(err)}")
    return Profile(ptr)


def profile_from_utility_matrix(utilities, n_voters: int, n_alts: int) -> Profile:
    """Build a profile from a utility matrix.

    Parameters
    ----------
    utilities:
        Row-major array of shape ``(n_voters, n_alts)``. Higher utility
        means the alternative is preferred. Ties are broken by smallest
        alternative index.
    n_voters:
        Number of voters.
    n_alts:
        Number of alternatives.

    Returns
    -------
    Profile
    """
    u = np.asarray(utilities, dtype=np.float64).ravel()
    if len(u) != n_voters * n_alts:
        raise ValueError(
            f"utilities has {len(u)} elements but n_voters * n_alts = {n_voters * n_alts}."
        )
    u_buf = _ffi.cast("double *", _ffi.from_buffer(u))
    err = new_err_buf()
    ptr = _lib.scs_profile_from_utility_matrix(u_buf, int(n_voters), int(n_alts), err, _ERR_BUF_SIZE)
    if ptr == _ffi.NULL:
        from socialchoicelab._error import _ffi_string
        raise SCSError(f"scs_profile_from_utility_matrix failed: {_ffi_string(err)}")
    return Profile(ptr)


def profile_impartial_culture(
    n_voters: int,
    n_alts: int,
    stream_manager,
    stream_name: str,
) -> Profile:
    """Generate a profile under the impartial culture model.

    Each voter's ranking is drawn independently and uniformly from all
    ``n_alts!`` orderings (Fisher-Yates shuffle).

    Parameters
    ----------
    n_voters:
        Number of voters.
    n_alts:
        Number of alternatives.
    stream_manager:
        A :class:`~socialchoicelab.StreamManager` instance.
    stream_name:
        Named stream to use for randomness.

    Returns
    -------
    Profile
    """
    err = new_err_buf()
    ptr = _lib.scs_profile_impartial_culture(
        int(n_voters), int(n_alts),
        stream_manager._handle(), stream_name.encode(),
        err, _ERR_BUF_SIZE,
    )
    if ptr == _ffi.NULL:
        from socialchoicelab._error import _ffi_string
        raise SCSError(f"scs_profile_impartial_culture failed: {_ffi_string(err)}")
    return Profile(ptr)


def profile_uniform_spatial(
    n_voters: int,
    n_alts: int,
    stream_manager,
    stream_name: str,
    lo: float = -1.0,
    hi: float = 1.0,
    dist_config: DistanceConfig | None = None,
) -> Profile:
    """Generate a spatial profile with voters drawn from a uniform 2D distribution.

    Both voter ideal points and alternative positions are drawn from the
    uniform distribution on ``[lo, hi]²``. Only 2D is supported.

    Parameters
    ----------
    n_voters:
        Number of voters.
    n_alts:
        Number of alternatives.
    stream_manager:
        A :class:`~socialchoicelab.StreamManager` instance.
    stream_name:
        Named stream to use for randomness.
    lo:
        Lower bound of the uniform distribution.
    hi:
        Upper bound (must satisfy lo < hi).
    dist_config:
        Distance configuration.  ``None`` defaults to Euclidean.

    Returns
    -------
    Profile
    """
    if dist_config is None:
        dist_config = DistanceConfig()
    cfg, ka = _to_cffi_dist(dist_config, n_dims=2)
    err = new_err_buf()
    ptr = _lib.scs_profile_uniform_spatial(
        int(n_voters), int(n_alts), 2, float(lo), float(hi),
        cfg, stream_manager._handle(), stream_name.encode(),
        err, _ERR_BUF_SIZE,
    )
    if ptr == _ffi.NULL:
        from socialchoicelab._error import _ffi_string
        raise SCSError(f"scs_profile_uniform_spatial failed: {_ffi_string(err)}")
    return Profile(ptr)


def profile_gaussian_spatial(
    n_voters: int,
    n_alts: int,
    stream_manager,
    stream_name: str,
    mean: float = 0.0,
    stddev: float = 1.0,
    dist_config: DistanceConfig | None = None,
) -> Profile:
    """Generate a spatial profile with voters drawn from a Gaussian 2D distribution.

    Both voter ideal points and alternative positions are drawn from
    N(mean, stddev²) per dimension. Only 2D is supported.

    Parameters
    ----------
    n_voters:
        Number of voters.
    n_alts:
        Number of alternatives.
    stream_manager:
        A :class:`~socialchoicelab.StreamManager` instance.
    stream_name:
        Named stream to use for randomness.
    mean:
        Normal distribution mean (per dimension).
    stddev:
        Normal distribution standard deviation (per dimension; must be > 0).
    dist_config:
        Distance configuration.  ``None`` defaults to Euclidean.

    Returns
    -------
    Profile
    """
    if dist_config is None:
        dist_config = DistanceConfig()
    cfg, ka = _to_cffi_dist(dist_config, n_dims=2)
    err = new_err_buf()
    ptr = _lib.scs_profile_gaussian_spatial(
        int(n_voters), int(n_alts), 2, float(mean), float(stddev),
        cfg, stream_manager._handle(), stream_name.encode(),
        err, _ERR_BUF_SIZE,
    )
    if ptr == _ffi.NULL:
        from socialchoicelab._error import _ffi_string
        raise SCSError(f"scs_profile_gaussian_spatial failed: {_ffi_string(err)}")
    return Profile(ptr)
