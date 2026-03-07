"""Distance and loss configuration types for the socialchoicelab Python binding.

These Python dataclasses are the user-facing equivalents of the C structs
``SCS_DistanceConfig`` and ``SCS_LossConfig`` defined in ``scs_api.h``.
They are converted to cffi structs immediately before each C API call.

Helper functions
----------------
``_to_cffi_dist``  — convert a ``DistanceConfig`` to a cffi struct + keepalive.
``_to_cffi_loss``  — convert a ``LossConfig``     to a cffi struct.
``_new_err_buf``   — allocate a 512-byte error buffer.
``_check``         — raise an ``SCSError`` subclass on non-zero return codes.
``_c_double_array`` — convert an array-like to a cffi ``double[]`` buffer.
``_c_int_array``    — convert an array-like to a cffi ``int[]`` buffer.
"""

from __future__ import annotations

from dataclasses import dataclass, field

import numpy as np

from socialchoicelab._error import _check, new_err_buf  # noqa: F401  re-exported
from socialchoicelab._loader import _ffi, _lib

__all__ = [
    "DistanceConfig",
    "LossConfig",
    "make_dist_config",
    "make_loss_config",
]

# ---------------------------------------------------------------------------
# Error buffer size (must match _error.py)
# ---------------------------------------------------------------------------

_ERR_BUF_SIZE = 512

# ---------------------------------------------------------------------------
# DistanceConfig
# ---------------------------------------------------------------------------

_DIST_TYPE_MAP = {
    "euclidean": _ffi.cast("SCS_DistanceType", 0),
    "manhattan": _ffi.cast("SCS_DistanceType", 1),
    "chebyshev": _ffi.cast("SCS_DistanceType", 2),
    "minkowski": _ffi.cast("SCS_DistanceType", 3),
}

_DIST_TYPE_INT = {k: i for i, k in enumerate(["euclidean", "manhattan", "chebyshev", "minkowski"])}


@dataclass
class DistanceConfig:
    """Distance metric configuration.

    Parameters
    ----------
    distance_type:
        One of ``"euclidean"`` (L2), ``"manhattan"`` (L1),
        ``"chebyshev"`` (L∞), or ``"minkowski"`` (general Lp).
    salience_weights:
        Per-dimension salience weights (strictly positive).  If ``None``,
        uniform unit weights are used (the C API default behaviour).
    order_p:
        Minkowski exponent (≥ 1).  Ignored for other distance types.
    """

    distance_type: str = "euclidean"
    salience_weights: list[float] | None = None
    order_p: float = 2.0


@dataclass
class LossConfig:
    """Loss / utility function configuration.

    Parameters
    ----------
    loss_type:
        One of ``"linear"``, ``"quadratic"``, ``"gaussian"``,
        ``"threshold"``.
    sensitivity:
        Scale α for linear / quadratic / threshold loss.
    max_loss:
        Asymptote α for Gaussian loss.
    steepness:
        Rate β for Gaussian loss.
    threshold:
        Indifference radius τ for threshold loss.
    """

    loss_type: str = "quadratic"
    sensitivity: float = 1.0
    max_loss: float = 1.0
    steepness: float = 1.0
    threshold: float = 1.0


_LOSS_TYPE_INT = {
    "linear": 0,
    "quadratic": 1,
    "gaussian": 2,
    "threshold": 3,
}

# ---------------------------------------------------------------------------
# Convenience constructors (mirrors R make_dist_config / make_loss_config)
# ---------------------------------------------------------------------------


def make_dist_config(
    distance_type: str = "euclidean",
    salience_weights: list[float] | None = None,
    order_p: float = 2.0,
) -> DistanceConfig:
    """Create a :class:`DistanceConfig`.

    Parameters
    ----------
    distance_type:
        ``"euclidean"`` (default), ``"manhattan"``, ``"chebyshev"``,
        or ``"minkowski"``.
    salience_weights:
        Per-dimension salience weights.  ``None`` means uniform unit weights.
    order_p:
        Minkowski exponent (only used when ``distance_type="minkowski"``).

    Returns
    -------
    DistanceConfig
    """
    dt = distance_type.lower()
    if dt not in _DIST_TYPE_INT:
        raise ValueError(
            f"distance_type must be one of {list(_DIST_TYPE_INT)}, got {distance_type!r}."
        )
    return DistanceConfig(distance_type=dt, salience_weights=salience_weights, order_p=order_p)


def make_loss_config(
    loss_type: str = "quadratic",
    sensitivity: float = 1.0,
    max_loss: float = 1.0,
    steepness: float = 1.0,
    threshold: float = 1.0,
) -> LossConfig:
    """Create a :class:`LossConfig`.

    Parameters
    ----------
    loss_type:
        ``"linear"``, ``"quadratic"`` (default), ``"gaussian"``,
        or ``"threshold"``.
    sensitivity:
        α for linear / quadratic / threshold.
    max_loss:
        α for Gaussian.
    steepness:
        β for Gaussian.
    threshold:
        τ for threshold.

    Returns
    -------
    LossConfig
    """
    lt = loss_type.lower()
    if lt not in _LOSS_TYPE_INT:
        raise ValueError(
            f"loss_type must be one of {list(_LOSS_TYPE_INT)}, got {loss_type!r}."
        )
    return LossConfig(
        loss_type=lt,
        sensitivity=sensitivity,
        max_loss=max_loss,
        steepness=steepness,
        threshold=threshold,
    )


# ---------------------------------------------------------------------------
# Internal cffi conversion helpers (used by all binding modules)
# ---------------------------------------------------------------------------


def _to_cffi_dist(dist: DistanceConfig, n_dims: int = 2) -> tuple:
    """Convert a DistanceConfig to a (cffi SCS_DistanceConfig*, keepalive) pair.

    Parameters
    ----------
    dist:
        The DistanceConfig to convert.
    n_dims:
        Spatial dimension of the problem.  Used only when
        ``dist.salience_weights is None`` to provide uniform unit weights
        of the correct length (the C API requires n_weights == n_dims).

    The ``keepalive`` list must remain alive for the duration of the C call.
    """
    dt_str = dist.distance_type.lower()
    if dt_str not in _DIST_TYPE_INT:
        raise ValueError(
            f"distance_type must be one of {list(_DIST_TYPE_INT)}, got {dist.distance_type!r}."
        )
    cfg = _ffi.new("SCS_DistanceConfig *")
    keepalive: list = []
    cfg.distance_type = _DIST_TYPE_INT[dt_str]
    cfg.order_p = float(dist.order_p)
    if dist.salience_weights is not None:
        wbuf = _ffi.new("double[]", [float(w) for w in dist.salience_weights])
        keepalive.append(wbuf)
        cfg.salience_weights = wbuf
        cfg.n_weights = len(dist.salience_weights)
    else:
        # Uniform unit weights: C API still requires n_weights == n_dims.
        wbuf = _ffi.new("double[]", [1.0] * n_dims)
        keepalive.append(wbuf)
        cfg.salience_weights = wbuf
        cfg.n_weights = n_dims
    return cfg, keepalive


def _to_cffi_loss(loss: LossConfig):
    """Convert a LossConfig to a cffi SCS_LossConfig*."""
    lt_str = loss.loss_type.lower()
    if lt_str not in _LOSS_TYPE_INT:
        raise ValueError(
            f"loss_type must be one of {list(_LOSS_TYPE_INT)}, got {loss.loss_type!r}."
        )
    cfg = _ffi.new("SCS_LossConfig *")
    cfg.loss_type = _LOSS_TYPE_INT[lt_str]
    cfg.sensitivity = float(loss.sensitivity)
    cfg.max_loss = float(loss.max_loss)
    cfg.steepness = float(loss.steepness)
    cfg.threshold = float(loss.threshold)
    return cfg


def _c_double_array(arr) -> tuple:
    """Return (cffi double[], n_elements). Input can be any array-like."""
    a = np.asarray(arr, dtype=np.float64).ravel()
    buf = _ffi.cast("double *", _ffi.from_buffer(a))
    return buf, a  # keep `a` alive to keep `buf` valid


def _c_int_array(arr) -> tuple:
    """Return (cffi int[], n_elements). Input can be any array-like."""
    a = np.asarray(arr, dtype=np.int32).ravel()
    buf = _ffi.cast("int *", _ffi.from_buffer(a))
    return buf, a
