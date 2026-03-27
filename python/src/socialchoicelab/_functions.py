"""Stateless utility functions: version, distance, loss, level-set, geometry.

All indices are 0-based.  Array inputs accept any array-like and are
converted to numpy internally.  Array outputs are numpy ndarrays.
"""

from __future__ import annotations

import numpy as np

from socialchoicelab._error import SCSError, _check, new_err_buf
from socialchoicelab._loader import _ffi, _lib
from socialchoicelab._types import (
    DistanceConfig,
    LossConfig,
    _c_double_array,
    _to_cffi_dist,
    _to_cffi_loss,
)

__all__ = [
    "scs_version",
    "calculate_distance",
    "distance_to_utility",
    "normalize_utility",
    "level_set_1d",
    "ic_interval_1d",
    "level_set_2d",
    "level_set_to_polygon",
    "ic_polygon_2d",
    "convex_hull_2d",
    "majority_prefers_2d",
    "weighted_majority_prefers_2d",
    "pairwise_matrix_2d",
]

_ERR = 512

_LEVEL_SET_TYPE_NAMES = {0: "circle", 1: "ellipse", 2: "superellipse", 3: "polygon"}


# ---------------------------------------------------------------------------
# API version
# ---------------------------------------------------------------------------


def scs_version() -> dict:
    """Return the compiled C API version as a dict.

    Returns
    -------
    dict
        ``{"major": int, "minor": int, "patch": int}``
    """
    major = _ffi.new("int *")
    minor = _ffi.new("int *")
    patch = _ffi.new("int *")
    err = new_err_buf()
    _check(_lib.scs_api_version(major, minor, patch, err, _ERR), err)
    return {"major": int(major[0]), "minor": int(minor[0]), "patch": int(patch[0])}


# ---------------------------------------------------------------------------
# Distance and utility functions
# ---------------------------------------------------------------------------


def calculate_distance(
    ideal_point,
    alt_point,
    dist_config: DistanceConfig | None = None,
) -> float:
    """Compute the distance between two N-dimensional points.

    Parameters
    ----------
    ideal_point:
        Array of n_dims floats (voter ideal point).
    alt_point:
        Array of n_dims floats (alternative position).
    dist_config:
        Distance configuration.  ``None`` defaults to Euclidean.

    Returns
    -------
    float
    """
    ip = np.asarray(ideal_point, dtype=np.float64).ravel()
    ap = np.asarray(alt_point, dtype=np.float64).ravel()
    n_dims = len(ip)
    if len(ap) != n_dims:
        raise ValueError(f"ideal_point has {n_dims} elements but alt_point has {len(ap)}.")
    if dist_config is None:
        dist_config = DistanceConfig()
    cfg, ka = _to_cffi_dist(dist_config, n_dims=n_dims)
    ip_buf = _ffi.cast("double *", _ffi.from_buffer(ip))
    ap_buf = _ffi.cast("double *", _ffi.from_buffer(ap))
    out = _ffi.new("double *")
    err = new_err_buf()
    _check(_lib.scs_calculate_distance(ip_buf, ap_buf, n_dims, cfg, out, err, _ERR), err)
    return float(out[0])


def distance_to_utility(
    distance: float,
    loss_config: LossConfig | None = None,
) -> float:
    """Convert a distance value to utility using the given loss function.

    Parameters
    ----------
    distance:
        Non-negative distance value.
    loss_config:
        Loss function configuration.  ``None`` defaults to quadratic loss.

    Returns
    -------
    float
        Utility value (≤ 0 for these loss functions).
    """
    if loss_config is None:
        loss_config = LossConfig()
    cfg = _to_cffi_loss(loss_config)
    out = _ffi.new("double *")
    err = new_err_buf()
    _check(_lib.scs_distance_to_utility(float(distance), cfg, out, err, _ERR), err)
    return float(out[0])


def normalize_utility(
    utility: float,
    max_distance: float,
    loss_config: LossConfig | None = None,
) -> float:
    """Normalize utility to [0, 1] given the maximum possible distance.

    Parameters
    ----------
    utility:
        Raw utility (from :func:`distance_to_utility`).
    max_distance:
        Maximum distance used to define the worst utility.
    loss_config:
        Loss function configuration (must match how utility was computed).
        ``None`` defaults to quadratic loss.

    Returns
    -------
    float
        Normalized utility in [0, 1].
    """
    if loss_config is None:
        loss_config = LossConfig()
    cfg = _to_cffi_loss(loss_config)
    out = _ffi.new("double *")
    err = new_err_buf()
    _check(
        _lib.scs_normalize_utility(float(utility), float(max_distance), cfg, out, err, _ERR),
        err,
    )
    return float(out[0])


# ---------------------------------------------------------------------------
# Level-set functions
# ---------------------------------------------------------------------------


def level_set_1d(
    ideal: float,
    weight: float,
    utility_level: float,
    loss_config: LossConfig | None = None,
) -> np.ndarray:
    """Compute the 1D level set: up to 2 points where u(x) = utility_level.

    Parameters
    ----------
    ideal:
        Ideal point coordinate (1D).
    weight:
        Salience weight (> 0).
    utility_level:
        Target utility level.
    loss_config:
        Loss configuration.  ``None`` defaults to quadratic.

    Returns
    -------
    numpy.ndarray
        Array of 0, 1, or 2 floats.
    """
    if loss_config is None:
        loss_config = LossConfig()
    cfg = _to_cffi_loss(loss_config)
    out_pts = _ffi.new("double[2]")
    out_n = _ffi.new("int *")
    err = new_err_buf()
    _check(
        _lib.scs_level_set_1d(
            float(ideal), float(weight), float(utility_level), cfg, out_pts, out_n, err, _ERR
        ),
        err,
    )
    n = int(out_n[0])
    return np.array([float(out_pts[i]) for i in range(n)], dtype=np.float64)


def ic_interval_1d(
    ideal: float,
    reference_x: float,
    loss_config: LossConfig | None = None,
    dist_config: DistanceConfig | None = None,
) -> np.ndarray:
    """1D indifference interval: one C call (distance → utility → level_set_1d).

    ``dist_config`` must be one-dimensional (``n_weights == 1``).
    """
    if loss_config is None:
        loss_config = LossConfig()
    if dist_config is None:
        dist_config = DistanceConfig(salience_weights=[1.0])
    lcfg = _to_cffi_loss(loss_config)
    dcfg, _ka = _to_cffi_dist(dist_config, n_dims=1)
    out_pts = _ffi.new("double[2]")
    out_n = _ffi.new("int *")
    err = new_err_buf()
    _check(
        _lib.scs_ic_interval_1d(
            float(ideal),
            float(reference_x),
            lcfg,
            dcfg,
            out_pts,
            out_n,
            err,
            _ERR,
        ),
        err,
    )
    n = int(out_n[0])
    return np.array([float(out_pts[i]) for i in range(n)], dtype=np.float64)


def level_set_2d(
    ideal_x: float,
    ideal_y: float,
    utility_level: float,
    loss_config: LossConfig | None = None,
    dist_config: DistanceConfig | None = None,
) -> dict:
    """Compute the 2D level set (exact shape).

    Parameters
    ----------
    ideal_x, ideal_y:
        Ideal point coordinates.
    utility_level:
        Target utility level.
    loss_config:
        Loss configuration.  ``None`` defaults to quadratic.
    dist_config:
        Distance configuration.  ``None`` defaults to Euclidean.

    Returns
    -------
    dict with keys:
        ``type`` — ``"circle"``, ``"ellipse"``, ``"superellipse"``, or
        ``"polygon"``.
        ``center_x``, ``center_y`` — centre of the level set.
        ``param0`` — radius (circle) or first semi-axis.
        ``param1`` — second semi-axis (ellipse / superellipse).
        ``exponent_p`` — Minkowski exponent (superellipse).
        ``vertices`` — ``ndarray`` of shape ``(n_vertices, 2)`` for
        polygon type; ``None`` otherwise.
    """
    if loss_config is None:
        loss_config = LossConfig()
    if dist_config is None:
        dist_config = DistanceConfig()
    loss_cfg = _to_cffi_loss(loss_config)
    dist_cfg, ka = _to_cffi_dist(dist_config, n_dims=2)
    out = _ffi.new("SCS_LevelSet2d *")
    err = new_err_buf()
    _check(
        _lib.scs_level_set_2d(
            float(ideal_x), float(ideal_y), float(utility_level),
            loss_cfg, dist_cfg, out, err, _ERR,
        ),
        err,
    )
    type_name = _LEVEL_SET_TYPE_NAMES.get(int(out.type), "unknown")
    verts = None
    if type_name == "polygon" and int(out.n_vertices) > 0:
        nv = int(out.n_vertices)
        verts = np.array(
            [[float(out.vertices[2 * i]), float(out.vertices[2 * i + 1])] for i in range(nv)],
            dtype=np.float64,
        )
    return {
        "type": type_name,
        "center_x": float(out.center_x),
        "center_y": float(out.center_y),
        "param0": float(out.param0),
        "param1": float(out.param1),
        "exponent_p": float(out.exponent_p),
        "vertices": verts,
    }


def level_set_to_polygon(level_set: dict, num_samples: int = 64) -> np.ndarray:
    """Sample a level set as a closed polygon.

    Parameters
    ----------
    level_set:
        Dict returned by :func:`level_set_2d`.
    num_samples:
        Number of samples for smooth shapes (circle/ellipse/superellipse).
        Ignored for polygon (exact 4 vertices are returned).

    Returns
    -------
    numpy.ndarray of shape ``(n_pts, 2)``.
    """
    ls = _ffi.new("SCS_LevelSet2d *")
    type_map = {"circle": 0, "ellipse": 1, "superellipse": 2, "polygon": 3}
    t = type_map.get(level_set.get("type", ""), -1)
    if t < 0:
        raise ValueError(f"Unknown level_set type: {level_set.get('type')!r}")
    ls.type = t
    ls.center_x = float(level_set.get("center_x", 0.0))
    ls.center_y = float(level_set.get("center_y", 0.0))
    ls.param0 = float(level_set.get("param0", 0.0))
    ls.param1 = float(level_set.get("param1", 0.0))
    ls.exponent_p = float(level_set.get("exponent_p", 2.0))
    verts = level_set.get("vertices")
    if verts is not None:
        nv = len(verts)
        ls.n_vertices = nv
        for i in range(nv):
            ls.vertices[2 * i] = float(verts[i][0])
            ls.vertices[2 * i + 1] = float(verts[i][1])
    else:
        ls.n_vertices = 0

    # Size query
    out_n = _ffi.new("int *")
    err = new_err_buf()
    _check(_lib.scs_level_set_to_polygon(ls, int(num_samples), _ffi.NULL, 0, out_n, err, _ERR), err)
    n_pts = int(out_n[0])

    # Fill
    buf = _ffi.new("double[]", n_pts * 2)
    err = new_err_buf()
    _check(
        _lib.scs_level_set_to_polygon(ls, int(num_samples), buf, n_pts, out_n, err, _ERR), err
    )
    return np.frombuffer(_ffi.buffer(buf, n_pts * 2 * 8), dtype=np.float64).reshape(n_pts, 2).copy()


def ic_polygon_2d(
    ideal_x: float,
    ideal_y: float,
    sq_x: float,
    sq_y: float,
    loss_config: LossConfig | None = None,
    dist_config: DistanceConfig | None = None,
    num_samples: int = 64,
) -> np.ndarray:
    """Compute the IC boundary polygon in a single C API call.

    Compound convenience function equivalent to calling
    :func:`calculate_distance`, :func:`distance_to_utility`,
    :func:`level_set_2d`, and :func:`level_set_to_polygon` in sequence,
    but without intermediate round-trips across the C API boundary.

    Parameters
    ----------
    ideal_x, ideal_y:
        Voter's ideal point coordinates.
    sq_x, sq_y:
        Reference point coordinates (status quo or seat position).
    loss_config:
        Loss configuration. ``None`` defaults to quadratic.
    dist_config:
        Distance configuration. ``None`` defaults to Euclidean.
    num_samples:
        Number of boundary samples for smooth shapes (>= 3).
        Ignored for polygon shapes (exact 4 vertices are returned).

    Returns
    -------
    numpy.ndarray of shape ``(n_pts, 2)``.
    """
    if loss_config is None:
        loss_config = LossConfig()
    if dist_config is None:
        dist_config = DistanceConfig()
    loss_cfg = _to_cffi_loss(loss_config)
    dist_cfg, ka = _to_cffi_dist(dist_config, n_dims=2)
    out_n = _ffi.new("int *")
    err = new_err_buf()
    _check(
        _lib.scs_ic_polygon_2d(
            float(ideal_x), float(ideal_y), float(sq_x), float(sq_y),
            loss_cfg, dist_cfg, int(num_samples),
            _ffi.NULL, 0, out_n, err, _ERR,
        ),
        err,
    )
    n_pts = int(out_n[0])
    buf = _ffi.new("double[]", n_pts * 2)
    err = new_err_buf()
    _check(
        _lib.scs_ic_polygon_2d(
            float(ideal_x), float(ideal_y), float(sq_x), float(sq_y),
            loss_cfg, dist_cfg, int(num_samples),
            buf, n_pts, out_n, err, _ERR,
        ),
        err,
    )
    return np.frombuffer(_ffi.buffer(buf, n_pts * 2 * 8), dtype=np.float64).reshape(n_pts, 2).copy()


# ---------------------------------------------------------------------------
# Geometry — convex hull and majority preference
# ---------------------------------------------------------------------------


def convex_hull_2d(points) -> np.ndarray:
    """Compute the 2D convex hull of a set of points.

    Parameters
    ----------
    points:
        Flat array ``[x0, y0, x1, y1, …]`` or array of shape ``(n, 2)``.

    Returns
    -------
    numpy.ndarray of shape ``(n_hull, 2)``
        Hull vertices in counter-clockwise order.
    """
    pts = np.asarray(points, dtype=np.float64).ravel()
    n_points = len(pts) // 2
    pts_buf = _ffi.cast("double *", _ffi.from_buffer(pts))
    out_xy = _ffi.new("double[]", n_points * 2)
    out_n = _ffi.new("int *")
    err = new_err_buf()
    _check(_lib.scs_convex_hull_2d(pts_buf, n_points, out_xy, out_n, err, _ERR), err)
    n_hull = int(out_n[0])
    return np.frombuffer(_ffi.buffer(out_xy, n_hull * 2 * 8), dtype=np.float64).reshape(n_hull, 2).copy()


def majority_prefers_2d(
    point_a_x: float,
    point_a_y: float,
    point_b_x: float,
    point_b_y: float,
    voter_ideals_xy,
    dist_config: DistanceConfig | None = None,
    k: int = -1,
) -> bool:
    """Test whether a k-majority of voters prefer point A to point B.

    Parameters
    ----------
    point_a_x, point_a_y:
        First alternative.
    point_b_x, point_b_y:
        Second alternative.
    voter_ideals_xy:
        Flat ``[x0, y0, …]`` array of voter ideal points.
    dist_config:
        Distance configuration.  ``None`` defaults to Euclidean.
    k:
        Majority threshold: ``-1`` means simple majority.

    Returns
    -------
    bool
        ``True`` if A defeats B under k-majority.
    """
    if dist_config is None:
        dist_config = DistanceConfig()
    cfg, ka = _to_cffi_dist(dist_config, n_dims=2)
    xy_buf, xy_arr = _c_double_array(voter_ideals_xy)
    n_voters = len(xy_arr) // 2
    out = _ffi.new("int *")
    err = new_err_buf()
    _check(
        _lib.scs_majority_prefers_2d(
            float(point_a_x), float(point_a_y),
            float(point_b_x), float(point_b_y),
            xy_buf, n_voters, cfg, int(k), out, err, _ERR,
        ),
        err,
    )
    return bool(out[0])


def weighted_majority_prefers_2d(
    point_a_x: float,
    point_a_y: float,
    point_b_x: float,
    point_b_y: float,
    voter_ideals_xy,
    weights,
    dist_config: DistanceConfig | None = None,
    threshold: float = 0.5,
) -> bool:
    """Test whether the weighted majority of voters prefer point A to point B.

    Parameters
    ----------
    point_a_x, point_a_y:
        First alternative.
    point_b_x, point_b_y:
        Second alternative.
    voter_ideals_xy:
        Flat ``[x0, y0, …]`` voter ideal points.
    weights:
        Voter weights (length n_voters; all must be > 0).
    dist_config:
        Distance configuration.  ``None`` defaults to Euclidean.
    threshold:
        Weight fraction in ``(0, 1]``.  Use 0.5 for simple weighted majority.

    Returns
    -------
    bool
    """
    if dist_config is None:
        dist_config = DistanceConfig()
    cfg, ka = _to_cffi_dist(dist_config, n_dims=2)
    xy_buf, xy_arr = _c_double_array(voter_ideals_xy)
    w_buf, w_arr = _c_double_array(weights)
    n_voters = len(xy_arr) // 2
    out = _ffi.new("int *")
    err = new_err_buf()
    _check(
        _lib.scs_weighted_majority_prefers_2d(
            float(point_a_x), float(point_a_y),
            float(point_b_x), float(point_b_y),
            xy_buf, n_voters, w_buf, cfg, float(threshold), out, err, _ERR,
        ),
        err,
    )
    return bool(out[0])


def pairwise_matrix_2d(
    alt_xy,
    voter_ideals_xy,
    dist_config: DistanceConfig | None = None,
    k: int = -1,
) -> np.ndarray:
    """Compute the n_alts × n_alts pairwise preference matrix.

    Entry ``[i, j]`` is:
    - ``1``  (SCS_PAIRWISE_WIN)  if alt i is preferred by more voters than j.
    - ``0``  (SCS_PAIRWISE_TIE)  if equal numbers prefer each.
    - ``-1`` (SCS_PAIRWISE_LOSS) if alt j is preferred by more voters than i.

    The matrix is antisymmetric: ``M[i,j] = -M[j,i]``, ``M[i,i] = 0``.

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
    numpy.ndarray of shape ``(n_alts, n_alts)`` and dtype ``int32``.
    """
    if dist_config is None:
        dist_config = DistanceConfig()
    cfg, ka = _to_cffi_dist(dist_config, n_dims=2)
    alt_buf, alt_arr = _c_double_array(alt_xy)
    vtr_buf, vtr_arr = _c_double_array(voter_ideals_xy)
    n_alts = len(alt_arr) // 2
    n_voters = len(vtr_arr) // 2
    # C API returns row-major; numpy is also row-major, so no transposition needed.
    mat = np.zeros((n_alts, n_alts), dtype=np.int32)
    mat_buf = _ffi.cast("int *", _ffi.from_buffer(mat))
    err = new_err_buf()
    _check(
        _lib.scs_pairwise_matrix_2d(
            alt_buf, n_alts, vtr_buf, n_voters, cfg, int(k),
            mat_buf, n_alts * n_alts, err, _ERR,
        ),
        err,
    )
    return mat
