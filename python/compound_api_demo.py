"""compound_api_demo.py — Exercise compound C API entry points (Python bindings)
================================================================================

Demonstrates the same four paths used by the competition / spatial stack:

  1. ``scs_ic_interval_1d``           → :func:`socialchoicelab.ic_interval_1d`
  2. ``scs_winset_2d_export_boundary``  → low-level ``_lib`` (no public wrapper yet)
  3. ``scs_uncovered_set_boundary_2d_heap`` → :func:`socialchoicelab.uncovered_set_boundary_2d`
  4. ``scs_voronoi_cells_2d_heap``    → low-level ``_lib`` (same as ``animate_competition_canvas``)

Run from the repository root::

    export SCS_LIB_PATH=$(pwd)/build
    python python/compound_api_demo.py

For **sliders + canvas** in the browser, see ``compound_api_interactive_demo.py``.
"""

from __future__ import annotations

import numpy as np

import socialchoicelab as scl
from socialchoicelab._error import _check, new_err_buf
from socialchoicelab._loader import _ffi, _lib
from socialchoicelab._types import DistanceConfig, _to_cffi_dist

_ERR = 512


def _demo_ic_interval_1d() -> None:
    print("\n--- 1. ic_interval_1d (compound 1D IC) ---")
    lc = scl.make_loss_config(loss_type="linear", sensitivity=1.0)
    dc = scl.make_dist_config(salience_weights=[1.0])
    ideal, ref = 0.5, 1.2
    pts = scl.ic_interval_1d(ideal, ref, lc, dc)
    d = scl.calculate_distance([ideal], [ref], dc)
    ul = scl.distance_to_utility(d, lc)
    pts_step = scl.level_set_1d(ideal, weight=1.0, utility_level=ul, loss_config=lc)
    np.testing.assert_allclose(pts, pts_step, rtol=0, atol=1e-12)
    print(f"  ideal={ideal}, ref={ref}  →  interval points: {pts.tolist()}")


def _demo_winset_export_boundary() -> None:
    print("\n--- 2. scs_winset_2d_export_boundary (no Winset handle) ---")
    voters = np.array([0.0, 0.0, 2.0, 0.0, 1.0, 2.0], dtype=np.float64)
    n_voters = len(voters) // 2
    sq_x, sq_y = 1.0, 2.0 / 3.0
    k, num_samples = -1, 64
    wcfg, _ka = _to_cffi_dist(DistanceConfig(salience_weights=[1.0, 1.0]), n_dims=2)
    vbuf = _ffi.cast("double *", _ffi.from_buffer(voters))

    is_empty = _ffi.new("int *")
    nx = _ffi.new("int *")
    np_paths = _ffi.new("int *")
    err = new_err_buf()
    _check(
        _lib.scs_winset_2d_export_boundary(
            sq_x, sq_y, vbuf, n_voters, wcfg, k, num_samples,
            is_empty, _ffi.NULL, 0, _ffi.NULL, 0, _ffi.NULL, nx, np_paths, err, _ERR,
        ),
        err,
    )
    if is_empty[0]:
        print("  (unexpected) empty winset")
        return
    n_x, n_p = int(nx[0]), int(np_paths[0])
    tmp_xy = _ffi.new("double[]", n_x * 2)
    tmp_starts = _ffi.new("int[]", n_p)
    tmp_holes = _ffi.new("int[]", n_p)
    nx2 = _ffi.new("int *")
    np2 = _ffi.new("int *")
    is_empty2 = _ffi.new("int *")
    err = new_err_buf()
    _check(
        _lib.scs_winset_2d_export_boundary(
            sq_x, sq_y, vbuf, n_voters, wcfg, k, num_samples,
            is_empty2, tmp_xy, n_x, tmp_starts, n_p, tmp_holes, nx2, np2, err, _ERR,
        ),
        err,
    )
    n_paths = int(np2[0])
    print(f"  status_quo=({sq_x}, {sq_y}), n_voters={n_voters}")
    print(f"  boundary: {int(nx2[0])} (x,y) pairs, {n_paths} path(s)")


def _demo_uncovered_boundary_heap() -> None:
    print("\n--- 3. uncovered_set_boundary_2d (heap export inside binding) ---")
    voters = np.array(
        [-1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, -1.0],
        dtype=np.float64,
    )
    bnd = scl.uncovered_set_boundary_2d(voters, grid_resolution=8, k=-1)
    print(f"  n boundary vertices: {bnd.shape[0]}  (shape {bnd.shape})")


def _demo_voronoi_heap() -> None:
    print("\n--- 4. scs_voronoi_cells_2d_heap (single internal compute) ---")
    sites = np.array([0.0, 0.0, 1.0, 0.0, 0.5, 1.0], dtype=np.float64)
    n_sites = 3
    bbox = (-0.5, -0.5, 1.5, 1.5)
    heap = _ffi.new("SCS_VoronoiCellsHeap *")
    err = new_err_buf()
    rc = _lib.scs_voronoi_cells_2d_heap(
        _ffi.cast("double *", _ffi.from_buffer(sites)),
        n_sites,
        bbox[0], bbox[1], bbox[2], bbox[3],
        heap, err, _ERR,
    )
    if rc != 0:
        _lib.scs_voronoi_cells_heap_destroy(heap)
        _check(rc, err)
    try:
        h = heap[0]
        n_cells = h.cell_start_len - 1 if h.cell_start_len > 0 else 0
        print(f"  n_sites={n_sites}, bbox={bbox}")
        print(f"  total (x,y) pairs in heap: {h.n_xy_pairs}, cells: {n_cells}")
    finally:
        _lib.scs_voronoi_cells_heap_destroy(heap)


def main() -> None:
    print("compound_api_demo — Python bindings for composite C API paths")
    _demo_ic_interval_1d()
    _demo_winset_export_boundary()
    _demo_uncovered_boundary_heap()
    _demo_voronoi_heap()
    print("\nDone.\n")


if __name__ == "__main__":
    main()
