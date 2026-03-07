"""Winset: 2D majority winset region.

Wraps the ``SCS_Winset`` C handle. Create winset objects via the factory
functions (:func:`winset_2d`, :func:`weighted_winset_2d`,
:func:`winset_with_veto_2d`) rather than instantiating ``Winset`` directly.

Boolean operations (:meth:`~Winset.union`, :meth:`~Winset.intersection`,
:meth:`~Winset.difference`, :meth:`~Winset.symmetric_difference`) return new
``Winset`` objects; the inputs are never modified.
"""

from __future__ import annotations

import numpy as np

from socialchoicelab._error import SCSError, _check, new_err_buf
from socialchoicelab._loader import _ffi, _lib
from socialchoicelab._types import DistanceConfig, _to_cffi_dist, _c_double_array

__all__ = [
    "Winset",
    "winset_2d",
    "weighted_winset_2d",
    "winset_with_veto_2d",
]

_ERR_BUF_SIZE = 512


class Winset:
    """Wraps an ``SCS_Winset`` C handle.

    Do not construct directly — use :func:`winset_2d`,
    :func:`weighted_winset_2d`, or :func:`winset_with_veto_2d`.

    Implements context-manager protocol for deterministic cleanup.
    ``__del__`` provides a GC safety net.
    """

    def __init__(self, ptr) -> None:
        if ptr == _ffi.NULL:
            raise SCSError("Received a null SCS_Winset pointer.")
        self._ptr = ptr
        self._destroyed = False

    def __del__(self) -> None:
        self._destroy()

    def __enter__(self) -> "Winset":
        return self

    def __exit__(self, *_) -> None:
        self._destroy()

    def _destroy(self) -> None:
        if not self._destroyed and self._ptr != _ffi.NULL:
            _lib.scs_winset_destroy(self._ptr)
            self._ptr = _ffi.NULL
            self._destroyed = True

    # ------------------------------------------------------------------
    # Queries
    # ------------------------------------------------------------------

    def is_empty(self) -> bool:
        """Return ``True`` if no policy beats the status quo.

        Returns
        -------
        bool
        """
        out = _ffi.new("int *")
        err = new_err_buf()
        _check(_lib.scs_winset_is_empty(self._ptr, out, err, _ERR_BUF_SIZE), err)
        return bool(out[0])

    def contains(self, x: float, y: float) -> bool:
        """Test whether the point *(x, y)* lies strictly inside the winset.

        Parameters
        ----------
        x, y:
            Point coordinates.

        Returns
        -------
        bool
        """
        out = _ffi.new("int *")
        err = new_err_buf()
        _check(
            _lib.scs_winset_contains_point_2d(self._ptr, float(x), float(y), out, err, _ERR_BUF_SIZE),
            err,
        )
        return bool(out[0])

    def bbox(self) -> dict | None:
        """Axis-aligned bounding box of the winset.

        Returns
        -------
        dict or None
            ``{"min_x": ..., "min_y": ..., "max_x": ..., "max_y": ...}``
            when the winset is non-empty, ``None`` when it is empty.
        """
        out_found = _ffi.new("int *")
        min_x = _ffi.new("double *")
        min_y = _ffi.new("double *")
        max_x = _ffi.new("double *")
        max_y = _ffi.new("double *")
        err = new_err_buf()
        _check(
            _lib.scs_winset_bbox_2d(
                self._ptr, out_found, min_x, min_y, max_x, max_y, err, _ERR_BUF_SIZE
            ),
            err,
        )
        if not out_found[0]:
            return None
        return {
            "min_x": float(min_x[0]),
            "min_y": float(min_y[0]),
            "max_x": float(max_x[0]),
            "max_y": float(max_y[0]),
        }

    def boundary(self) -> np.ndarray:
        """Export the winset boundary as a 2D array of shape *(n_pts, 2)*.

        The boundary consists of one or more closed paths (outer rings and
        holes).  All paths are returned in a single flat array with a
        companion ``path_starts`` array indicating where each path begins.

        Returns
        -------
        dict
            ``{"xy": ndarray of shape (n_pts, 2),
               "path_starts": ndarray of shape (n_paths,),
               "path_is_hole": ndarray of shape (n_paths,)}``
        """
        out_xy_pairs = _ffi.new("int *")
        out_n_paths = _ffi.new("int *")
        err = new_err_buf()
        _check(
            _lib.scs_winset_boundary_size_2d(self._ptr, out_xy_pairs, out_n_paths, err, _ERR_BUF_SIZE),
            err,
        )
        n_xy = int(out_xy_pairs[0])
        n_paths = int(out_n_paths[0])

        if n_xy == 0:
            return {
                "xy": np.empty((0, 2), dtype=np.float64),
                "path_starts": np.empty(0, dtype=np.int32),
                "path_is_hole": np.empty(0, dtype=np.int32),
            }

        xy_buf = _ffi.new("double[]", n_xy * 2)
        starts_buf = _ffi.new("int[]", n_paths)
        holes_buf = _ffi.new("int[]", n_paths)
        out_xy_n = _ffi.new("int *")
        out_np = _ffi.new("int *")
        err = new_err_buf()
        _check(
            _lib.scs_winset_sample_boundary_2d(
                self._ptr,
                xy_buf, n_xy, out_xy_n,
                starts_buf, n_paths, holes_buf, out_np,
                err, _ERR_BUF_SIZE,
            ),
            err,
        )
        xy = np.frombuffer(_ffi.buffer(xy_buf, int(out_xy_n[0]) * 2 * 8), dtype=np.float64).reshape(-1, 2).copy()
        return {
            "xy": xy,
            "path_starts": np.frombuffer(_ffi.buffer(starts_buf, int(out_np[0]) * 4), dtype=np.int32).copy(),
            "path_is_hole": np.frombuffer(_ffi.buffer(holes_buf, int(out_np[0]) * 4), dtype=np.int32).copy(),
        }

    # ------------------------------------------------------------------
    # Boolean set operations
    # ------------------------------------------------------------------

    def union(self, other: "Winset") -> "Winset":
        """Return the union of this winset and *other* (self ∪ other).

        Returns
        -------
        Winset
            New owned handle; inputs are not modified.
        """
        err = new_err_buf()
        ptr = _lib.scs_winset_union(self._ptr, other._ptr, err, _ERR_BUF_SIZE)
        if ptr == _ffi.NULL:
            from socialchoicelab._error import _ffi_string
            raise SCSError(f"scs_winset_union failed: {_ffi_string(err)}")
        return Winset(ptr)

    def intersection(self, other: "Winset") -> "Winset":
        """Return the intersection of this winset and *other* (self ∩ other).

        Returns
        -------
        Winset
        """
        err = new_err_buf()
        ptr = _lib.scs_winset_intersection(self._ptr, other._ptr, err, _ERR_BUF_SIZE)
        if ptr == _ffi.NULL:
            from socialchoicelab._error import _ffi_string
            raise SCSError(f"scs_winset_intersection failed: {_ffi_string(err)}")
        return Winset(ptr)

    def difference(self, other: "Winset") -> "Winset":
        """Return the set difference (self \\ other).

        Returns
        -------
        Winset
        """
        err = new_err_buf()
        ptr = _lib.scs_winset_difference(self._ptr, other._ptr, err, _ERR_BUF_SIZE)
        if ptr == _ffi.NULL:
            from socialchoicelab._error import _ffi_string
            raise SCSError(f"scs_winset_difference failed: {_ffi_string(err)}")
        return Winset(ptr)

    def symmetric_difference(self, other: "Winset") -> "Winset":
        """Return the symmetric difference (self △ other).

        Returns
        -------
        Winset
        """
        err = new_err_buf()
        ptr = _lib.scs_winset_symmetric_difference(self._ptr, other._ptr, err, _ERR_BUF_SIZE)
        if ptr == _ffi.NULL:
            from socialchoicelab._error import _ffi_string
            raise SCSError(f"scs_winset_symmetric_difference failed: {_ffi_string(err)}")
        return Winset(ptr)

    def clone(self) -> "Winset":
        """Return a deep copy of this winset.

        Returns
        -------
        Winset
        """
        err = new_err_buf()
        ptr = _lib.scs_winset_clone(self._ptr, err, _ERR_BUF_SIZE)
        if ptr == _ffi.NULL:
            from socialchoicelab._error import _ffi_string
            raise SCSError(f"scs_winset_clone failed: {_ffi_string(err)}")
        return Winset(ptr)

    # ------------------------------------------------------------------
    # Internal accessor
    # ------------------------------------------------------------------

    def _handle(self):
        return self._ptr


# ---------------------------------------------------------------------------
# Factory functions
# ---------------------------------------------------------------------------


def winset_2d(
    status_quo_x: float,
    status_quo_y: float,
    voter_ideals_xy,
    dist_config: DistanceConfig | None = None,
    k: int = -1,
    num_samples: int = 64,
) -> Winset:
    """Compute the 2D k-majority winset of a status quo point.

    Parameters
    ----------
    status_quo_x, status_quo_y:
        Status quo coordinates.
    voter_ideals_xy:
        Flat array ``[x0, y0, x1, y1, …]`` of voter ideal points, length
        ``2 * n_voters``.
    dist_config:
        Distance configuration.  ``None`` defaults to Euclidean.
    k:
        Majority threshold: ``-1`` means simple majority (⌊n/2⌋ + 1).
        Pass an integer in ``[1, n_voters]`` for a k-majority.
    num_samples:
        Boundary approximation quality (≥ 4).

    Returns
    -------
    Winset
        Caller owns; use as a context manager for deterministic cleanup.
    """
    if dist_config is None:
        dist_config = DistanceConfig()
    cfg, ka = _to_cffi_dist(dist_config, n_dims=2)
    xy_buf, xy_arr = _c_double_array(voter_ideals_xy)
    n_voters = len(xy_arr) // 2
    err = new_err_buf()
    ptr = _lib.scs_winset_2d(
        float(status_quo_x), float(status_quo_y),
        xy_buf, n_voters, cfg, int(k), int(num_samples),
        err, _ERR_BUF_SIZE,
    )
    if ptr == _ffi.NULL:
        from socialchoicelab._error import _ffi_string
        raise SCSError(f"scs_winset_2d failed: {_ffi_string(err)}")
    return Winset(ptr)


def weighted_winset_2d(
    status_quo_x: float,
    status_quo_y: float,
    voter_ideals_xy,
    weights,
    dist_config: DistanceConfig | None = None,
    threshold: float = 0.5,
    num_samples: int = 64,
) -> Winset:
    """Compute the weighted-majority 2D winset of a status quo point.

    Parameters
    ----------
    status_quo_x, status_quo_y:
        Status quo coordinates.
    voter_ideals_xy:
        Flat ``[x0, y0, …]`` voter ideal points.
    weights:
        Voter weights (length ``n_voters``; all must be > 0).
    dist_config:
        Distance configuration.  ``None`` defaults to Euclidean.
    threshold:
        Required weight fraction in ``(0, 1]``.  Use 0.5 for simple
        weighted majority.
    num_samples:
        Boundary approximation quality (≥ 4).

    Returns
    -------
    Winset
    """
    if dist_config is None:
        dist_config = DistanceConfig()
    cfg, ka = _to_cffi_dist(dist_config, n_dims=2)
    xy_buf, xy_arr = _c_double_array(voter_ideals_xy)
    w_buf, w_arr = _c_double_array(weights)
    n_voters = len(xy_arr) // 2
    err = new_err_buf()
    ptr = _lib.scs_weighted_winset_2d(
        float(status_quo_x), float(status_quo_y),
        xy_buf, n_voters, w_buf, cfg,
        float(threshold), int(num_samples),
        err, _ERR_BUF_SIZE,
    )
    if ptr == _ffi.NULL:
        from socialchoicelab._error import _ffi_string
        raise SCSError(f"scs_weighted_winset_2d failed: {_ffi_string(err)}")
    return Winset(ptr)


def winset_with_veto_2d(
    status_quo_x: float,
    status_quo_y: float,
    voter_ideals_xy,
    dist_config: DistanceConfig | None = None,
    veto_ideals_xy=None,
    k: int = -1,
    num_samples: int = 64,
) -> Winset:
    """Compute the k-majority winset constrained by veto players.

    Parameters
    ----------
    status_quo_x, status_quo_y:
        Status quo coordinates.
    voter_ideals_xy:
        Flat ``[x0, y0, …]`` voter ideal points.
    dist_config:
        Distance configuration.  ``None`` defaults to Euclidean.
    veto_ideals_xy:
        Flat ``[x0, y0, …]`` veto player ideal points, or ``None`` for
        no veto constraint (equivalent to :func:`winset_2d`).
    k:
        Majority threshold (``-1`` = simple majority).
    num_samples:
        Boundary approximation quality (≥ 4).

    Returns
    -------
    Winset
    """
    if dist_config is None:
        dist_config = DistanceConfig()
    cfg, ka = _to_cffi_dist(dist_config, n_dims=2)
    xy_buf, xy_arr = _c_double_array(voter_ideals_xy)
    n_voters = len(xy_arr) // 2

    if veto_ideals_xy is None or len(veto_ideals_xy) == 0:
        veto_buf = _ffi.NULL
        n_veto = 0
        veto_arr = None
    else:
        veto_buf, veto_arr = _c_double_array(veto_ideals_xy)
        n_veto = len(veto_arr) // 2

    err = new_err_buf()
    ptr = _lib.scs_winset_with_veto_2d(
        float(status_quo_x), float(status_quo_y),
        xy_buf, n_voters, cfg,
        veto_buf, n_veto,
        int(k), int(num_samples),
        err, _ERR_BUF_SIZE,
    )
    if ptr == _ffi.NULL:
        from socialchoicelab._error import _ffi_string
        raise SCSError(f"scs_winset_with_veto_2d failed: {_ffi_string(err)}")
    return Winset(ptr)
