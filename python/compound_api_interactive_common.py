"""Shared payload builder + validation for compound API interactive HTML demos.

Used by ``compound_api_interactive_demo.py`` and ``tests/test_compound_api_interactive.py``.
"""

from __future__ import annotations

import json
from typing import Any

import numpy as np

import socialchoicelab as scl
from socialchoicelab._error import _check, new_err_buf
from socialchoicelab._loader import _ffi, _lib
from socialchoicelab._types import DistanceConfig, _to_cffi_dist

_ERR = 512

# Human-readable guide embedded in JSON for the browser (mirrored in R).
INTERACTIVE_EXPECTATIONS: dict[str, dict[str, list[str]]] = {
    "ic": {
        "healthy": [
            "Green vertical line at x = 0 is the voter ideal (fixed).",
            "Orange dashed vertical line is the reference point; it moves with the slider.",
            "Cyan horizontal segment on the midline is the 1D indifference interval; it shrinks as the reference approaches the ideal.",
        ],
        "broken": [
            "No cyan segment or pts length wrong: ic_interval_1d or JSON embedding failed.",
            "Slider does nothing: DATA.ic.frames missing or empty.",
        ],
    },
    "winset": {
        "healthy": [
            "Three gray dots are fixed voters (triangle).",
            "Orange triangle is the status quo moving on a circle around the centroid.",
            "Blue outline (filled with even–odd rule for holes) is the winset boundary from scs_winset_2d_export_boundary.",
        ],
        "broken": [
            "No blue region on any frame: all exports empty or path data not serialised.",
            "Jagged or self-crossing-only shapes: sample count / hole flags may be wrong.",
        ],
    },
    "uncovered": {
        "healthy": [
            "Four gray dots are voters (square).",
            "Green closed loop is the approximate uncovered boundary; it should grow smoother as grid resolution increases.",
        ],
        "broken": [
            "No green loop but grid label changes: boundary array empty or xy not interleaved x,y.",
            "Nothing moves with the slider: uncovered.frames missing.",
        ],
    },
    "voronoi": {
        "healthy": [
            "Three white dots are Voronoi sites.",
            "Colored polygons are clipped Voronoi cells; changing scale zooms the bbox.",
        ],
        "broken": [
            "Missing cells or null cell: heap export or JSON mismatch.",
            "Sites drawn but no polygons: cell coordinates empty.",
        ],
    },
}


def _winset_export_paths(
    sq_x: float,
    sq_y: float,
    voters_flat: np.ndarray,
    *,
    k: int = -1,
    num_samples: int = 64,
) -> dict:
    """Return {empty: true} or {empty: false, sq, paths: [{xy, hole}, ...]}."""
    n_voters = len(voters_flat) // 2
    wcfg, _ka = _to_cffi_dist(DistanceConfig(salience_weights=[1.0, 1.0]), n_dims=2)
    vbuf = _ffi.cast("double *", _ffi.from_buffer(voters_flat))

    is_empty = _ffi.new("int *")
    nx = _ffi.new("int *")
    np_paths = _ffi.new("int *")
    err = new_err_buf()
    _check(
        _lib.scs_winset_2d_export_boundary(
            float(sq_x), float(sq_y), vbuf, n_voters, wcfg, int(k), int(num_samples),
            is_empty, _ffi.NULL, 0, _ffi.NULL, 0, _ffi.NULL, nx, np_paths, err, _ERR,
        ),
        err,
    )
    if is_empty[0]:
        return {"empty": True, "sq": [sq_x, sq_y]}
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
            float(sq_x), float(sq_y), vbuf, n_voters, wcfg, int(k), int(num_samples),
            is_empty2, tmp_xy, n_x, tmp_starts, n_p, tmp_holes, nx2, np2, err, _ERR,
        ),
        err,
    )
    xy_flat = np.frombuffer(_ffi.buffer(tmp_xy, int(nx2[0]) * 16), dtype=np.float64)
    starts = np.frombuffer(_ffi.buffer(tmp_starts, int(np2[0]) * 4), dtype=np.int32)
    holes = np.frombuffer(_ffi.buffer(tmp_holes, int(np2[0]) * 4), dtype=np.int32)
    n_paths = int(np2[0])
    ends = list(starts[1:]) + [int(nx2[0])]
    paths_out = []
    for p_i in range(n_paths):
        s_idx = int(starts[p_i])
        e_idx = int(ends[p_i])
        chunk = xy_flat[2 * s_idx : 2 * e_idx].copy().tolist()
        paths_out.append({"xy": chunk, "hole": int(holes[p_i])})
    return {"empty": False, "sq": [sq_x, sq_y], "paths": paths_out}


def _voronoi_cells_json(sites: np.ndarray, bbox: tuple[float, float, float, float]) -> list:
    n_sites = len(sites) // 2
    heap = _ffi.new("SCS_VoronoiCellsHeap *")
    err = new_err_buf()
    rc = _lib.scs_voronoi_cells_2d_heap(
        _ffi.cast("double *", _ffi.from_buffer(sites)), n_sites,
        bbox[0], bbox[1], bbox[2], bbox[3],
        heap, err, _ERR,
    )
    if rc != 0:
        _lib.scs_voronoi_cells_heap_destroy(heap)
        _check(rc, err)
    cells = []
    try:
        h = heap[0]
        n_cells = h.cell_start_len - 1 if h.cell_start_len > 0 else 0
        cs = np.frombuffer(_ffi.buffer(h.cell_start, (n_cells + 1) * 4), dtype=np.int32).copy()
        if h.n_xy_pairs > 0 and h.xy != _ffi.NULL:
            xy_flat = np.frombuffer(_ffi.buffer(h.xy, h.n_xy_pairs * 16), dtype=np.float64).copy()
        else:
            xy_flat = np.zeros(0, dtype=np.float64)
        for c in range(n_cells):
            start, end = int(cs[c]), int(cs[c + 1])
            if start >= end:
                cells.append(None)
            else:
                cells.append(xy_flat[2 * start : 2 * end].tolist())
    finally:
        _lib.scs_voronoi_cells_heap_destroy(heap)
    return cells


def build_payload() -> dict[str, Any]:
    lc = scl.make_loss_config(loss_type="linear", sensitivity=1.0)
    dc1 = scl.make_dist_config(salience_weights=[1.0])
    ideal_1d = 0.0
    refs = np.linspace(-2.4, 2.4, 97)
    ic_frames = []
    for ref in refs:
        pts = scl.ic_interval_1d(float(ideal_1d), float(ref), lc, dc1)
        ic_frames.append({"ref": float(ref), "pts": pts.tolist()})

    voters_w = np.array([0.0, 0.0, 2.0, 0.0, 1.0, 2.0], dtype=np.float64)
    cx, cy = 1.0, 2.0 / 3.0
    rad = 0.82
    angles = np.linspace(0, 2 * np.pi, 49)[:-1]
    winset_frames = []
    for th in angles:
        sx, sy = cx + rad * np.cos(th), cy + rad * np.sin(th)
        fr = _winset_export_paths(float(sx), float(sy), voters_w)
        fr["angle_deg"] = float(np.degrees(th))
        winset_frames.append(fr)

    voters_u = np.array([-1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, -1.0], dtype=np.float64)
    uncov_frames = []
    for g in range(4, 19):
        bnd = scl.uncovered_set_boundary_2d(voters_u, grid_resolution=int(g), k=-1)
        uncov_frames.append({"grid": int(g), "xy": bnd.reshape(-1).tolist()})

    sites_v = np.array([0.0, 0.0, 1.0, 0.0, 0.5, 1.0], dtype=np.float64)
    cx_v, cy_v = 0.5, 1.0 / 3.0
    hw = 1.15
    vor_frames = []
    for s in np.linspace(0.65, 1.35, 36):
        h = hw * float(s)
        bbox = (cx_v - h, cy_v - h, cx_v + h, cy_v + h)
        vor_frames.append({"scale": float(s), "bbox": list(bbox), "cells": _voronoi_cells_json(sites_v, bbox)})

    payload: dict[str, Any] = {
        "ic": {"ideal": ideal_1d, "frames": ic_frames},
        "winset": {"voters": voters_w.tolist(), "frames": winset_frames},
        "uncovered": {"voters": voters_u.tolist(), "frames": uncov_frames},
        "voronoi": {"sites": sites_v.tolist(), "frames": vor_frames},
    }
    checks, all_ok = validate_interactive_payload(payload)
    payload["meta"] = {
        "binding": "python",
        "structure_version": 1,
        "expectations": INTERACTIVE_EXPECTATIONS,
        "build_checks": checks,
        "build_all_ok": all_ok,
    }
    return payload


def validate_interactive_payload(p: dict[str, Any]) -> tuple[list[dict[str, Any]], bool]:
    """Return (list of {id, ok, message}, all_ok). Does not mutate p."""
    out: list[dict[str, Any]] = []
    ok_all = True

    def chk(scenario: str, cid: str, cond: bool, good: str, bad: str) -> None:
        nonlocal ok_all
        if cond:
            out.append({"scenario": scenario, "id": cid, "ok": True, "message": good})
        else:
            ok_all = False
            out.append({"scenario": scenario, "id": cid, "ok": False, "message": bad})

    ic = p.get("ic")
    if not isinstance(ic, dict) or "frames" not in ic:
        ok_all = False
        out.append({"scenario": "ic", "id": "structure", "ok": False, "message": "missing ic.frames"})
    else:
        frames = ic["frames"]
        chk("ic", "frame_count", len(frames) == 97, "97 reference frames (slider 0–96).", f"expected 97 ic frames, got {len(frames)}")
        if frames:
            r0, r1 = frames[0].get("ref"), frames[-1].get("ref")
            chk("ic", "ref_range", r0 is not None and r1 is not None and r0 < r1, f"ref from {r0} to {r1}.", "ic ref range invalid")
        ic_pts_ok = True
        for i, fr in enumerate(frames):
            pts = fr.get("pts")
            if not isinstance(pts, list):
                ok_all = False
                ic_pts_ok = False
                out.append({"scenario": "ic", "id": f"pts_{i}", "ok": False, "message": f"frame {i} pts not a list"})
                break
            if len(pts) >= 2 and not (pts[0] <= pts[1]):
                ok_all = False
                ic_pts_ok = False
                out.append({"scenario": "ic", "id": "order", "ok": False, "message": f"frame {i}: pts not sorted left≤right"})
                break
        if ic_pts_ok and frames:
            chk("ic", "pts_shape", True, "Each frame has list pts; when length≥2 endpoints satisfy left≤right.", "")

    ws = p.get("winset")
    if not isinstance(ws, dict) or "frames" not in ws:
        ok_all = False
        out.append({"scenario": "winset", "id": "structure", "ok": False, "message": "missing winset.frames"})
    else:
        wf = ws["frames"]
        chk("winset", "frame_count", len(wf) == 48, "48 orbit frames (one per angle step).", f"expected 48 winset frames, got {len(wf)}")
        nonempty = sum(1 for fr in wf if isinstance(fr, dict) and not fr.get("empty") and fr.get("paths"))
        chk("winset", "nonempty_exports", nonempty > 0, f"{nonempty} frames with non-empty boundary paths.", "all winset frames empty — export or geometry failure")
        for i, fr in enumerate(wf):
            if isinstance(fr, dict) and fr.get("angle_deg") is None:
                ok_all = False
                out.append({"scenario": "winset", "id": "angle", "ok": False, "message": f"frame {i} missing angle_deg"})
                break

    unc = p.get("uncovered")
    if not isinstance(unc, dict) or "frames" not in unc:
        ok_all = False
        out.append({"scenario": "uncovered", "id": "structure", "ok": False, "message": "missing uncovered.frames"})
    else:
        uf = unc["frames"]
        grids = [fr.get("grid") for fr in uf if isinstance(fr, dict)]
        chk("uncovered", "frame_count", len(uf) == 15, "15 frames (grid 4…18).", f"expected 15 uncovered frames, got {len(uf)}")
        chk("uncovered", "grid_sequence", grids == list(range(4, 19)), "grid runs 4,5,…,18.", f"grid sequence wrong: {grids}")
        bad_xy = next((i for i, fr in enumerate(uf) if isinstance(fr.get("xy"), list) and len(fr["xy"]) % 2 != 0), None)
        chk("uncovered", "xy_pairs", bad_xy is None, "uncovered xy arrays have even length (x,y pairs).", f"frame {bad_xy} xy length not even")

    vo = p.get("voronoi")
    if not isinstance(vo, dict) or "frames" not in vo:
        ok_all = False
        out.append({"scenario": "voronoi", "id": "structure", "ok": False, "message": "missing voronoi.frames"})
    else:
        vf = vo["frames"]
        chk("voronoi", "frame_count", len(vf) == 36, "36 bbox scale steps.", f"expected 36 voronoi frames, got {len(vf)}")
        bad_cells = None
        for i, fr in enumerate(vf):
            cells = fr.get("cells") if isinstance(fr, dict) else None
            if not isinstance(cells, list) or len(cells) != 3:
                bad_cells = (i, "cell count")
                break
            for j, c in enumerate(cells):
                if c is None or (isinstance(c, list) and len(c) < 4):
                    bad_cells = (i, f"cell {j} empty or too short")
                    break
            if bad_cells:
                break
        chk("voronoi", "cells", bad_cells is None, "Each frame has 3 non-null cell polygons (≥2 vertices).", f"frame {bad_cells[0]}: {bad_cells[1]}" if bad_cells else "")

    return out, ok_all


def format_validation_report(checks: list[dict[str, Any]], all_ok: bool) -> str:
    lines = ["compound_api_interactive — data checks"]
    for c in checks:
        tag = "PASS" if c["ok"] else "FAIL"
        lines.append(f"  [{tag}] {c['scenario']}/{c['id']}: {c['message']}")
    lines.append("  SUMMARY: " + ("all checks passed" if all_ok else "one or more checks failed — see above; page may still render for debugging"))
    return "\n".join(lines)


def payload_to_json(payload: dict[str, Any]) -> str:
    return json.dumps(payload)
