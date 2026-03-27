"""Spatial voting visualization helpers (C10/C13).

Static 2D spatial plots use a canvas payload dict from :func:`plot_spatial_voting`
and :func:`layer_*` functions; :func:`save_plot` writes standalone HTML.

Competition animations use :func:`animate_competition_canvas` (self-contained HTML).

Layers compose by passing the dict from one function into the next::

    import socialchoicelab.plots as sclp
    import numpy as np

    voters = np.array([-1.0, -0.5, 0.0, 0.0, 0.8, 0.6, -0.4, 0.8, 0.5, -0.7])
    sq = np.array([0.0, 0.0])
    fig = sclp.plot_spatial_voting(voters, sq=sq, theme="dark2")
    fig = sclp.layer_ic(fig, voters, sq, theme="dark2")
    _ = sclp.save_plot(fig, "/tmp/p.html")  # writes HTML
"""

from __future__ import annotations

import json
import math
import warnings
from pathlib import Path

import numpy as np

_PACKAGE_DIR = Path(__file__).parent
_CORE_JS_PATH = _PACKAGE_DIR / "scs_canvas_core.js"
_COMPETITION_JS_PATH = _PACKAGE_DIR / "competition_canvas.js"
_SPATIAL_JS_PATH = _PACKAGE_DIR / "spatial_voting_canvas.js"

from socialchoicelab._functions import (
    ic_interval_1d,
    ic_polygon_2d,
    level_set_to_polygon,
)
from socialchoicelab._geometry import centroid_2d, marginal_median_2d
from socialchoicelab._winset import winset_2d
from socialchoicelab._types import DistanceConfig, LossConfig, _to_cffi_dist
from socialchoicelab._error import _check, new_err_buf
from socialchoicelab._loader import _ffi, _lib
from socialchoicelab.palette import (
    scl_palette,
    scl_theme_colors,
    _layer_fill_color,
    _layer_line_color,
    _voter_point_color,
    _alt_point_color,
    _candidate_colors_1d,
    _centroid_overlay_color,
    _marginal_median_overlay_color,
    _overlay_triangle_outline_color,
    _ic_uniform_line,
    _preferred_uniform_fill,
    _preferred_uniform_line,
)

__all__ = [
    "plot_spatial_voting",
    "animate_competition_canvas",
    "load_competition_canvas",
    "layer_ic",
    "layer_preferred_regions",
    "layer_winset",
    "layer_yolk",
    "layer_uncovered_set",
    "layer_convex_hull",
    "layer_centroid",
    "layer_marginal_median",
    "save_plot",
    "scl_palette",
    "scl_theme_colors",
]


def _flat_to_xy(v) -> tuple[np.ndarray, np.ndarray]:
    arr = np.asarray(v, dtype=float).ravel()
    return arr[0::2], arr[1::2]


def _circle_pts(cx: float, cy: float, r: float, n: int = 64) -> tuple[np.ndarray, np.ndarray]:
    theta = np.linspace(0.0, 2.0 * math.pi, n + 1)
    return cx + r * np.cos(theta), cy + r * np.sin(theta)


def _padded_range(vals: np.ndarray, pad_frac: float = 0.12, min_pad: float = 0.5) -> list[float]:
    lo, hi = float(np.min(vals)), float(np.max(vals))
    pad = max((hi - lo) * pad_frac, min_pad)
    return [lo - pad, hi + pad]


def _range_with_origin(vals: np.ndarray, pad_frac: float = 0.12, min_pad: float = 0.5) -> list[float]:
    lo, hi = _padded_range(vals, pad_frac=pad_frac, min_pad=min_pad)
    return [min(lo, 0.0), max(hi, 0.0)]


def _names_from_strategies(kinds: list[str]) -> list[str]:
    """Generate display names from strategy kinds.

    If a strategy appears once, just use its title-cased name (e.g. "Hunter").
    If it appears multiple times, append a letter: "Hunter A", "Hunter B", etc.
    """
    from collections import Counter
    counts = Counter(kinds)
    labels: list[str] = []
    counters: dict[str, int] = {}
    for k in kinds:
        nice = k.title()
        if counts[k] == 1:
            labels.append(nice)
        else:
            idx = counters.get(k, 0)
            counters[k] = idx + 1
            labels.append(f"{nice} {chr(65 + idx)}")
    return labels


def _annotate_names_with_strategies(names: list[str], kinds: list[str]) -> list[str]:
    """Append strategy kind to user-supplied names: 'Biden' → 'Biden (Hunter)'."""
    return [f"{name} ({kind.title()})" for name, kind in zip(names, kinds)]


def _spatial_canvas_theme_colors(theme: str) -> dict[str, str]:
    """Mirror R .spatial_canvas_theme_colors for JSON theme_colors."""
    ws_fill, ws_line = scl_theme_colors("winset", theme=theme)
    hull_fill, hull_line = scl_theme_colors("convex_hull", theme=theme)
    yolk_fill, yolk_line = scl_theme_colors("yolk", theme=theme)
    unc_fill, unc_line = scl_theme_colors("uncovered_set", theme=theme)
    return {
        "plot_bg": "#ffffff" if theme == "bw" else "#fafafa",
        "grid": "rgba(60,60,60,0.15)" if theme == "bw" else "rgba(140,140,140,0.18)",
        "axis": "rgba(40,40,40,0.5)" if theme == "bw" else "rgba(100,100,100,0.45)",
        "text": "#111111" if theme == "bw" else "#2d2d2d",
        "text_light": "#555555" if theme == "bw" else "#888888",
        "border": "#bbbbbb" if theme == "bw" else "#d4d4d4",
        "voter_fill": _voter_point_color(theme),
        "voter_stroke": "rgba(255,255,255,0.7)" if theme == "bw" else "rgba(255,255,255,0.65)",
        "alt_fill": _alt_point_color(theme),
        "sq_fill": "rgba(40,40,40,0.92)" if theme == "bw" else "rgba(255,200,0,0.95)",
        "winset_fill": ws_fill,
        "winset_line": ws_line,
        "hull_fill": hull_fill,
        "hull_line": hull_line,
        "ic_line": _ic_uniform_line(theme),
        "pref_fill": _preferred_uniform_fill(theme),
        "pref_line": _preferred_uniform_line(theme),
        "yolk_fill": yolk_fill,
        "yolk_line": yolk_line,
        "uncovered_fill": unc_fill,
        "uncovered_line": unc_line,
    }


def _assert_spatial_payload(fig: object) -> dict:
    if not isinstance(fig, dict):
        raise TypeError(f"Expected dict from plot_spatial_voting(), got {type(fig).__name__}.")
    if not fig.get("_scs_spatial_canvas"):
        raise TypeError("Expected canvas payload dict from plot_spatial_voting() and layer_*().")
    return fig


def _poly_to_flat_xy(px: list[float], py: list[float]) -> list[float]:
    arr = np.column_stack((px, py)).astype(float)
    return arr.ravel(order="C").tolist()


def plot_spatial_voting(
    voters,
    alternatives=None,
    sq=None,
    voter_names=None,
    alt_names=None,
    dim_names=("Dimension 1", "Dimension 2"),
    title="Spatial Voting Analysis",
    show_labels=False,
    layer_toggles=True,
    xlim=None,
    ylim=None,
    theme="dark2",
    width=700,
    height=600,
):
    """Create a base 2D spatial voting plot.

    Parameters
    ----------
    voters:
        Flat array ``[x0, y0, x1, y1, …]`` of voter ideal points.
    alternatives:
        Flat array of alternative coordinates, or ``None``.
    sq:
        Length-2 array ``[x, y]`` for the status quo, or ``None``.
    voter_names:
        List of voter labels.  ``None`` uses ``["V1", "V2", …]``.
    alt_names:
        List of alternative labels.  ``None`` uses ``["Alt 1", …]``.
    dim_names:
        Pair of axis title strings.
    title:
        Plot title.
    show_labels:
        Draw text labels directly on the graph for SQ and alternatives.
    layer_toggles:
        If ``True`` (default), show a bottom checkbox bar to toggle visibility
        (like the competition canvas; crop below the plot for slides). The legend
        with symbols stays on the right of the plot on the canvas. If ``False``,
        omit the bottom bar for publication figures; all layers remain visible.
    xlim, ylim:
        ``[min, max]`` for explicit axis ranges.  ``None`` auto-computes.
    theme:
        Colour theme: ``"dark2"`` (default, ColorBrewer Dark2, colorblind-safe),
        ``"set2"``, ``"okabe_ito"``, ``"paired"``, or ``"bw"`` (black-and-white
        print).  Pass the same value to every ``layer_*`` call for coordinated
        colours across the whole plot.
    width, height:
        Widget dimensions in pixels (used when saving HTML).

    Returns
    -------
    dict
        Canvas payload for :func:`layer_*` and :func:`save_plot`.

    Examples
    --------
    >>> import socialchoicelab.plots as sclp
    >>> import numpy as np
    >>> voters = np.array([-1.0, -0.5, 0.0, 0.0, 0.8, 0.6])
    >>> fig = sclp.plot_spatial_voting(voters, sq=np.array([0.0, 0.0]))
    """
    vx, vy = _flat_to_xy(voters)
    n_v = len(vx)
    if voter_names is None:
        voter_names = [f"V{i + 1}" for i in range(n_v)]

    all_x = list(vx)
    all_y = list(vy)
    sqv = None
    if sq is not None:
        sqv = np.asarray(sq, dtype=float).ravel()
        all_x.append(float(sqv[0]))
        all_y.append(float(sqv[1]))
    alts_flat: list[float] = []
    alt_names_out: list[str] = []
    if alternatives is not None:
        ax, ay = _flat_to_xy(np.asarray(alternatives, dtype=float).ravel())
        n_a = len(ax)
        if alt_names is None:
            alt_names = [f"Alt {i + 1}" for i in range(n_a)]
        all_x.extend(ax.tolist())
        all_y.extend(ay.tolist())
        alts_flat = np.ravel(np.column_stack((ax, ay)), order="C").tolist()
        alt_names_out = list(alt_names)

    if xlim is None and len(all_x) > 0:
        xlim = _range_with_origin(np.array(all_x))
    if ylim is None and len(all_y) > 0:
        ylim = _range_with_origin(np.array(all_y))

    payload = {
        "voters_x": vx.astype(float).tolist(),
        "voters_y": vy.astype(float).tolist(),
        "voter_names": list(voter_names),
        "alternatives": alts_flat,
        "alternative_names": alt_names_out,
        "sq": None if sq is None else [float(sqv[0]), float(sqv[1])],
        "xlim": [float(xlim[0]), float(xlim[1])],
        "ylim": [float(ylim[0]), float(ylim[1])],
        "dim_names": [str(dim_names[0]), str(dim_names[1])],
        "title": str(title),
        "theme": str(theme),
        "theme_colors": _spatial_canvas_theme_colors(theme),
        "show_labels": bool(show_labels),
        "layer_toggles": bool(layer_toggles),
        "layers": {},
    }
    out = dict(payload)
    out["_scs_spatial_canvas"] = True
    out["_width"] = int(width)
    out["_height"] = int(height)
    return out



def _serialise_overlays_static(overlays):
    """Convert an overlays dict to the overlays_static JSON structure."""
    if not overlays:
        return None
    _POINT_KEYS   = {"centroid", "marginal_median", "geometric_mean"}
    _POLYGON_KEYS = {"pareto_set"}
    result = {}
    for key, obj in overlays.items():
        if key in _POINT_KEYS:
            if not (isinstance(obj, (tuple, list)) and len(obj) == 2):
                raise TypeError(
                    f"Overlay '{key}' must be a (x, y) tuple from the "
                    "corresponding _2d() function."
                )
            result[key] = {"x": float(obj[0]), "y": float(obj[1])}
        elif key in _POLYGON_KEYS:
            arr = np.asarray(obj, dtype=float)
            if arr.ndim != 2 or arr.shape[1] != 2:
                raise TypeError(
                    f"Overlay '{key}' must be an (n, 2) array from convex_hull_2d()."
                )
            result[key] = {"polygon": arr.tolist()}
        else:
            valid = sorted(_POINT_KEYS | _POLYGON_KEYS)
            raise ValueError(
                f"Unknown overlay key '{key}'. Valid keys: {valid}."
            )
    return result


def _htmlwidgets_stub_script() -> str:
    return (
        "    window.HTMLWidgets = (function() {\n"
        "      var defs = [];\n"
        "      return {\n"
        "        widget: function(d) { defs.push(d); },\n"
        "        _init:  function(el, w, h, data) {\n"
        "          defs.forEach(function(d) {\n"
        '            if (d.type === "output") { d.factory(el, w, h).renderValue(data); }\n'
        "          });\n"
        "        }\n"
        "      };\n"
        "    })();\n"
    )


def _html_from_payload(payload, width, height):
    """Build self-contained HTML for the competition canvas widget."""
    if not _CORE_JS_PATH.exists() or not _COMPETITION_JS_PATH.exists():
        raise FileNotFoundError(
            "Competition canvas JS missing: expected "
            f"{_CORE_JS_PATH} and {_COMPETITION_JS_PATH} (copy from r/inst/htmlwidgets/)."
        )
    js_core = _CORE_JS_PATH.read_text(encoding="utf-8")
    js_player = _COMPETITION_JS_PATH.read_text(encoding="utf-8")
    json_payload = json.dumps(payload)
    title = payload.get("title", "Competition Trajectories")
    w = str(int(width))
    h = str(int(height))
    return "\n".join([
        "<!DOCTYPE html>",
        "<html>",
        "<head>",
        '  <meta charset="utf-8">',
        '  <meta name="viewport" content="width=device-width, initial-scale=1">',
        "  <title>" + title + "</title>",
        "  <style>",
        "    * { box-sizing: border-box; margin: 0; padding: 0; }",
        "    body { background: #fff; }",
        '    #competition-canvas { width: ' + w + 'px; height: ' + h + 'px; }',
        "  </style>",
        "</head>",
        "<body>",
        '  <div id="competition-canvas"></div>',
        "  <script>",
        _htmlwidgets_stub_script(),
        "  </script>",
        "  <script>",
        js_core,
        "  </script>",
        "  <script>",
        js_player,
        "  </script>",
        "  <script>",
        "    HTMLWidgets._init(",
        '      document.getElementById("competition-canvas"),',
        "      " + w + ", " + h + ",",
        "      " + json_payload,
        "    );",
        "  </script>",
        "</body>",
        "</html>",
    ])


def _html_from_spatial_payload(payload: dict, width: int, height: int) -> str:
    """Build self-contained HTML for the static spatial voting canvas widget."""
    if not _CORE_JS_PATH.exists() or not _SPATIAL_JS_PATH.exists():
        raise FileNotFoundError(
            "Spatial canvas JS missing: expected "
            f"{_CORE_JS_PATH} and {_SPATIAL_JS_PATH} (copy from r/inst/htmlwidgets/)."
        )
    js_core = _CORE_JS_PATH.read_text(encoding="utf-8")
    js_spatial = _SPATIAL_JS_PATH.read_text(encoding="utf-8")
    json_payload = json.dumps(payload)
    title = str(payload.get("title", "Spatial Voting Analysis"))
    w = str(int(width))
    h = str(int(height))
    return "\n".join([
        "<!DOCTYPE html>",
        "<html>",
        "<head>",
        '  <meta charset="utf-8">',
        '  <meta name="viewport" content="width=device-width, initial-scale=1">',
        "  <title>" + title + "</title>",
        "  <style>",
        "    * { box-sizing: border-box; margin: 0; padding: 0; }",
        "    body { background: #fff; }",
        '    #spatial-voting-canvas { width: ' + w + 'px; height: ' + h + 'px; }',
        "  </style>",
        "</head>",
        "<body>",
        '  <div id="spatial-voting-canvas"></div>',
        "  <script>",
        _htmlwidgets_stub_script(),
        "  </script>",
        "  <script>",
        js_core,
        "  </script>",
        "  <script>",
        js_spatial,
        "  </script>",
        "  <script>",
        "    HTMLWidgets._init(",
        '      document.getElementById("spatial-voting-canvas"),',
        "      " + w + ", " + h + ",",
        "      " + json_payload,
        "    );",
        "  </script>",
        "</body>",
        "</html>",
    ])


def _save_scscanvas(payload, width, height, payload_path):
    """Write the canvas payload envelope to a .scscanvas JSON file."""
    import datetime

    try:
        from importlib.metadata import version as _pkg_version
        gen_ver = _pkg_version("socialchoicelab")
    except Exception:
        gen_ver = "unknown"
    created = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    envelope = {
        "format":    "scscanvas",
        "version":   "1",
        "created":   created,
        "generator": f"socialchoicelab/python/{gen_ver}",
        "width":     int(width),
        "height":    int(height),
        "payload":   payload,
    }
    Path(payload_path).write_text(json.dumps(envelope), encoding="utf-8")


def _animate_competition_canvas_1d(
    trace,
    n_rounds,
    n_competitors,
    competitor_names,
    voters,
    title,
    theme,
    width,
    height,
    trail,
    trail_len,
    dim_names,
    overlays,
    path,
    compute_ic,
    ic_loss_config,
    ic_dist_config,
    ic_num_samples,
    ic_max_curves,
    compute_winset=False,
    payload_path=None,
):
    """Build the 1D canvas animation payload and HTML."""
    import warnings

    positions = [trace.round_positions(r) for r in range(n_rounds)]
    positions.append(trace.final_positions())
    frame_names = [f"Round {r + 1}" for r in range(n_rounds)] + ["Final"]

    vote_shares = [trace.round_vote_shares(r).tolist() for r in range(n_rounds)]
    vote_shares.append(trace.final_vote_shares().tolist())

    # Voters — 1D flat array (each element is one voter's ideal point)
    voter_flat = np.asarray(voters if voters is not None else [], dtype=float).ravel()
    voters_x = voter_flat.tolist()

    # Axis range
    all_x = voters_x + [float(p[c, 0]) for p in positions for c in range(n_competitors)]
    xlim = _range_with_origin(np.array(all_x)) if all_x else [-1.0, 1.0]

    # 1D candidate colours use a green-free palette (voter dots are lime green).
    colors = _candidate_colors_1d(n_competitors, alpha=0.95)
    voter_color = _voter_point_color(theme)

    # 1D overlays — centroid (mean) and marginal median; others are skipped with a warning
    _VALID_1D_OVERLAYS = {"centroid", "marginal_median"}
    overlays_out = {}
    if overlays:
        for key in overlays:
            if key == "centroid":
                overlays_out["centroid"] = {
                    "x": float(np.mean(voters_x)) if voters_x else 0.0
                }
            elif key == "marginal_median":
                overlays_out["marginal_median"] = {
                    "x": float(np.median(voters_x)) if voters_x else 0.0
                }
            else:
                warnings.warn(
                    f"Overlay '{key}' is not supported for 1D traces and will be "
                    f"skipped. Valid 1D overlays: {sorted(_VALID_1D_OVERLAYS)}.",
                    stacklevel=4,
                )
    overlays_serialised = overlays_out if overlays_out else None

    # 1D IC computation — single C call per voter (ic_interval_1d)
    ic_data_payload = None
    ic_indices_payload = None
    if compute_ic:
        n_voters = len(voters_x)
        if n_voters == 0:
            raise ValueError("compute_ic=True requires voters to be provided.")
        if ic_loss_config is None:
            ic_loss_config = LossConfig(loss_type="linear", sensitivity=1.0)
        if ic_dist_config is None:
            ic_dist_config = DistanceConfig(salience_weights=[1.0])
        ic_num_samples = int(ic_num_samples)
        if ic_num_samples < 3:
            raise ValueError(f"ic_num_samples must be >= 3, got {ic_num_samples}.")

        n_frames = n_rounds + 1
        seat_idxs_per_frame = []
        for r in range(n_rounds):
            st = trace.round_seat_totals(r)
            seat_idxs_per_frame.append([int(i) for i in np.where(st > 0)[0]])
        final_ss = trace.final_seat_shares()
        seat_idxs_per_frame.append([int(i) for i in np.where(final_ss > 0)[0]])
        # No ic_max_curves guard for 1D: each IC is just two numbers (lo, hi),
        # so the computation is negligible regardless of voter/frame count.

        ic_data_payload = []
        ic_indices_payload = []
        for frame_i, pos_mat in enumerate(positions):
            seat_idxs = seat_idxs_per_frame[frame_i]
            frame_curves = []
            frame_cidxs = []
            for s in seat_idxs:
                seat_x = float(pos_mat[s, 0])
                frame_cidxs.append(s)
                voter_curves = []
                for v in range(n_voters):
                    pts = ic_interval_1d(
                        float(voters_x[v]), float(seat_x),
                        ic_loss_config, ic_dist_config,
                    )
                    voter_curves.append(pts.tolist())
                frame_curves.append(voter_curves)
            ic_data_payload.append(frame_curves)
            ic_indices_payload.append(frame_cidxs)

    # Seat-holder indices (unconditional, used by JS stats block)
    seat_holder_indices = []
    for r in range(n_rounds):
        st = trace.round_seat_totals(r)
        seat_holder_indices.append([int(i) for i in np.where(st > 0)[0]])
    final_ss_all = trace.final_seat_shares()
    seat_holder_indices.append([int(i) for i in np.where(final_ss_all > 0)[0]])

    # 1D dim name — use first element of dim_names tuple
    if isinstance(dim_names, (list, tuple)) and len(dim_names) >= 1:
        dim_name_1d = str(dim_names[0])
    else:
        dim_name_1d = "Dimension 1"

    payload = {
        "n_dims":            1,
        "voters_x":          voters_x,
        "voter_color":       voter_color,
        "xlim":              xlim,
        "dim_names":         [dim_name_1d],
        "rounds":            frame_names,
        "positions": [
            [[float(p[c, 0])] for c in range(n_competitors)]
            for p in positions
        ],
        "competitor_names":  list(competitor_names),
        "competitor_colors": list(colors),
        "vote_shares":       vote_shares,
        "trail":             trail,
        "trail_length":      trail_len,
        "title":             title,
        "seat_holder_indices": seat_holder_indices,
    }
    if overlays_serialised is not None:
        payload["overlays_static"] = overlays_serialised
    if ic_data_payload is not None:
        payload["indifference_curves"]   = ic_data_payload
        payload["ic_competitor_indices"] = ic_indices_payload
    # WinSet intervals per frame (Euclidean 1D, exact O(n^2) sweep)
    if compute_winset:
        if not voters_x:
            raise ValueError("compute_winset=True requires voters to be provided.")
        voter_arr = np.asarray(voters_x, dtype=np.float64)
        n_frames_ws = n_rounds + 1
        seat_idxs_per_frame_ws = []
        for r in range(n_rounds):
            st = trace.round_seat_totals(r)
            seat_idxs_per_frame_ws.append([int(i) for i in np.where(st > 0)[0]])
        final_ss_ws = trace.final_seat_shares()
        seat_idxs_per_frame_ws.append([int(i) for i in np.where(final_ss_ws > 0)[0]])

        ws_data = []
        out_lo = _ffi.new("double *")
        out_hi = _ffi.new("double *")
        err_buf = new_err_buf()
        voter_ptr = _ffi.cast("double *", _ffi.from_buffer(voter_arr))
        for frame_i, pos_mat in enumerate(positions):
            seat_idxs = seat_idxs_per_frame_ws[frame_i]
            frame_ws = []
            for s in seat_idxs:
                seat_x = float(pos_mat[s, 0])
                rc = _lib.scs_winset_interval_1d(
                    voter_ptr, len(voters_x), seat_x,
                    out_lo, out_hi, err_buf, len(err_buf)
                )
                _check(rc, err_buf)
                import math
                frame_ws.append({
                    "lo": float(out_lo[0]),
                    "hi": float(out_hi[0]),
                    "competitor_idx": s,
                })
            ws_data.append(frame_ws)
        payload["winset_intervals_1d"] = ws_data

    if payload_path is not None:
        _save_scscanvas(payload, width, height, payload_path)

    html = _html_from_payload(payload, width, height)
    if path is not None:
        Path(path).write_text(html, encoding="utf-8")
    return html


def animate_competition_canvas(
    trace,
    voters=None,
    competitor_names=None,
    title="Competition Trajectories",
    theme="dark2",
    width=900,
    height=800,
    trail="fade",
    trail_length="medium",
    dim_names=("Dimension 1", "Dimension 2"),
    overlays=None,
    path=None,
    compute_ic=False,
    ic_loss_config=None,
    ic_dist_config=None,
    ic_num_samples=32,
    ic_max_curves=5000,
    compute_winset=False,
    winset_dist_config=None,
    winset_k=-1,
    winset_num_samples=64,
    winset_max=1000,
    compute_voronoi=False,
    voronoi_dist_config=None,
    payload_path=None,
):
    """Animate competition trajectories using the canvas-based HTML player.

    Returns a self-contained HTML string (and optionally writes it to ``path``).
    Data is sent once; the browser draws frames on demand, so long runs stay
    lightweight compared to frame-heavy animation formats.

    Parameters
    ----------
    trace:
        A :class:`socialchoicelab.CompetitionTrace`. Must be 2D.
    voters:
        Flat array or ``(n, 2)`` array of voter ideal points, or ``None``.
    competitor_names:
        Display labels for competitors. Defaults to ``["Competitor 1", ...]``.
    title:
        Plot title drawn on the canvas.
    theme:
        Colour theme — same options as :func:`plot_spatial_voting`.
    width, height:
        Widget dimensions in pixels.
    trail:
        ``"fade"`` (default), ``"full"``, or ``"none"``.
    trail_length:
        ``"short"`` (1/3 rounds), ``"medium"`` (1/2 rounds), ``"long"``
        (3/4 rounds), or a positive integer. Ignored when trail is not
        ``"fade"``.
    dim_names:
        Pair of axis title strings.
    path:
        If given, write the HTML to this file path in addition to returning it.
    compute_ic:
        When ``True``, compute per-frame indifference curves for every voter
        through each seat holder's position and bundle them into the widget
        payload. A toggle is then shown in the player UI. Requires ``voters``
        to be provided.
    ic_loss_config:
        :class:`LossConfig` for IC computation. ``None`` uses linear loss with
        sensitivity 1, producing pure iso-distance curves that match competition
        vote logic.
    ic_dist_config:
        :class:`DistanceConfig` for IC computation. Should match the
        ``dist_config`` used in :func:`competition_run`. ``None`` uses 2D
        Euclidean.
    ic_num_samples:
        Number of polygon vertices per indifference curve for smooth shapes.
        Exact polygons always use 4 vertices. Default 32.
    ic_max_curves:
        Maximum total indifference curves allowed across all frames. Exceeding
        this raises a ``ValueError``. Default 2000.
    compute_winset:
        When ``True``, compute per-frame WinSet boundaries for each seat
        holder's current position and bundle them into the widget payload. A
        toggle is then shown in the player UI. Requires ``voters`` to be
        provided. The WinSet is computed using ``winset_k`` and
        ``winset_dist_config``; no assumption is made about which voting rule
        is in use.
    winset_dist_config:
        :class:`DistanceConfig` for WinSet computation. Should match the
        ``dist_config`` used in :func:`competition_run`. ``None`` uses 2D
        Euclidean.
    winset_k:
        Voting rule threshold passed to :func:`winset_2d`. ``-1`` means
        simple majority. Default ``-1``.
    winset_num_samples:
        Boundary approximation quality (number of sample points on the WinSet
        boundary). Must be >= 4. Default 64.
    winset_max:
        Maximum total WinSet computations allowed (``seats_per_frame ×
        n_frames``). Exceeding this raises a ``ValueError``. Default 1000.

    payload_path:
        If given, write the pre-computed payload (positions, ICs, WinSet,
        Cutlines, etc.) to this path as a ``.scscanvas`` JSON file before
        building the HTML.  The file can be reloaded later with
        :func:`load_competition_canvas` without recomputing anything.
    compute_voronoi:
        When ``True``, compute Euclidean Voronoi (candidate) regions per frame
        and show a "Voronoi" toggle in the canvas. Currently only Euclidean
        (L2, uniform weights) is supported; a non-Euclidean
        ``voronoi_dist_config`` raises a ``ValueError``. Default ``False``.
    voronoi_dist_config:
        :class:`DistanceConfig` for Voronoi (must be Euclidean when
        ``compute_voronoi`` is ``True``). ``None`` uses 2D Euclidean. If
        provided, must be Euclidean with uniform salience.

    Returns
    -------
    str
        A self-contained HTML string. If ``path`` is provided the same string
        is written to that file.
    """
    n_rounds, n_competitors, n_dims = trace.dims()

    # Validate common parameters before dispatching on dimensionality.
    strategy_kinds = trace.strategy_kinds()
    if competitor_names is None:
        competitor_names = _names_from_strategies(strategy_kinds)
    else:
        if len(competitor_names) != n_competitors:
            raise ValueError("competitor_names length must match n_competitors.")
        competitor_names = _annotate_names_with_strategies(
            list(competitor_names), strategy_kinds
        )
    if trail not in {"none", "full", "fade"}:
        raise ValueError("trail must be one of: 'none', 'full', 'fade'.")
    _TRAIL_SHORTHANDS = {"short", "medium", "long"}
    if isinstance(trail_length, str) and trail_length not in _TRAIL_SHORTHANDS:
        raise ValueError(
            f"trail_length {trail_length!r} is not recognised. "
            "Use 'short' (1/3 rounds), 'medium' (1/2 rounds), 'long' (3/4 rounds), "
            "or a positive integer."
        )

    n_segments_max = max(n_rounds, 0)
    if isinstance(trail_length, str):
        trail_len_common = max(1, {
            "short":  n_segments_max // 3,
            "medium": n_segments_max // 2,
            "long":   (n_segments_max * 3) // 4,
        }[trail_length])
    else:
        trail_len_common = int(trail_length)
        if trail_len_common < 1:
            raise ValueError(f"trail_length must be >= 1, got {trail_len_common}.")

    if n_dims == 1:
        return _animate_competition_canvas_1d(
            trace, n_rounds, n_competitors, competitor_names,
            voters, title, theme, width, height,
            trail, trail_len_common, dim_names, overlays, path,
            compute_ic, ic_loss_config, ic_dist_config, ic_num_samples, ic_max_curves,
            compute_winset=compute_winset,
            payload_path=payload_path,
        )
    if n_dims != 2:
        raise ValueError(
            f"animate_competition_canvas supports 1D and 2D traces, "
            f"got n_dims={n_dims}."
        )

    if compute_voronoi:
        vdc = voronoi_dist_config if voronoi_dist_config is not None else DistanceConfig(salience_weights=[1.0, 1.0])
        type_ok = (getattr(vdc, "distance_type", "euclidean") == "euclidean")
        w = getattr(vdc, "salience_weights", [1.0, 1.0])
        uniform_ok = len(w) >= 2 and len(set(w)) == 1
        if not type_ok or not uniform_ok:
            raise ValueError(
                "compute_voronoi=True requires Euclidean (L2) distance with uniform "
                "salience. Voronoi for non-Euclidean metrics is planned; use "
                "voronoi_dist_config=None or DistanceConfig() for Euclidean."
            )

    # Positions — mirrors R: list of (n_rounds + 1) matrices, last is final.
    positions = [trace.round_positions(r) for r in range(n_rounds)]
    positions.append(trace.final_positions())
    frame_names = [f"Round {r + 1}" for r in range(n_rounds)] + ["Final"]

    vote_shares = [trace.round_vote_shares(r).tolist() for r in range(n_rounds)]
    vote_shares.append(trace.final_vote_shares().tolist())

    # Voters
    voter_flat = np.asarray(voters if voters is not None else [], dtype=float).ravel()
    voters_x = voter_flat[0::2].tolist()
    voters_y = voter_flat[1::2].tolist()

    # Axis ranges
    all_x = voters_x + [float(p[c, 0]) for p in positions for c in range(n_competitors)]
    all_y = voters_y + [float(p[c, 1]) for p in positions for c in range(n_competitors)]
    xlim = _range_with_origin(np.array(all_x)) if all_x else [-1.0, 1.0]
    ylim = _range_with_origin(np.array(all_y)) if all_y else [-1.0, 1.0]

    trail_len = trail_len_common

    colors = scl_palette(theme, n_competitors, alpha=0.95)
    voter_color = _voter_point_color(theme)

    # ── Indifference-curve computation ─────────────────────────────────────────
    ic_data_payload = None
    ic_indices_payload = None
    if compute_ic:
        n_voters = len(voters_x)
        if n_voters == 0:
            raise ValueError("compute_ic=True requires voters to be provided.")
        if ic_loss_config is None:
            ic_loss_config = LossConfig(loss_type="linear", sensitivity=1.0)
        if ic_dist_config is None:
            ic_dist_config = DistanceConfig(salience_weights=[1.0, 1.0])
        ic_num_samples = int(ic_num_samples)
        ic_max_curves = int(ic_max_curves)
        if ic_num_samples < 3:
            raise ValueError(f"ic_num_samples must be >= 3, got {ic_num_samples}.")

        # Identify seat holders per frame.
        n_frames = n_rounds + 1
        seat_idxs_per_frame = []
        for r in range(n_rounds):
            st = trace.round_seat_totals(r)
            seat_idxs_per_frame.append([int(i) for i in np.where(st > 0)[0]])
        final_ss = trace.final_seat_shares()
        seat_idxs_per_frame.append([int(i) for i in np.where(final_ss > 0)[0]])

        total_ics = sum(n_voters * len(s) for s in seat_idxs_per_frame)
        if total_ics > ic_max_curves:
            n_seats_max = max(len(s) for s in seat_idxs_per_frame)
            raise ValueError(
                f"compute_ic: total IC count ({total_ics}) exceeds ic_max_curves "
                f"({ic_max_curves}). Got n_voters={n_voters}, max seats per "
                f"frame={n_seats_max}, n_frames={n_frames}. Reduce the number of "
                f"voters, seats, or rounds, or increase ic_max_curves."
            )

        ic_data_payload = []
        ic_indices_payload = []
        for frame_i, pos_mat in enumerate(positions):
            seat_idxs = seat_idxs_per_frame[frame_i]
            frame_curves = []
            frame_cidxs = []
            for s in seat_idxs:
                seat_pos = [float(pos_mat[s, 0]), float(pos_mat[s, 1])]
                frame_cidxs.append(s)
                voter_curves = []
                for v in range(n_voters):
                    poly = ic_polygon_2d(
                        voters_x[v], voters_y[v],
                        seat_pos[0], seat_pos[1],
                        ic_loss_config, ic_dist_config,
                        ic_num_samples,
                    )
                    voter_curves.append(poly.ravel().tolist())
                frame_curves.append(voter_curves)
            ic_data_payload.append(frame_curves)
            ic_indices_payload.append(frame_cidxs)

    # ── WinSet computation ──────────────────────────────────────────────────────
    ws_data_payload = None
    ws_indices_payload = None
    if compute_winset:
        n_voters = len(voters_x)
        if n_voters == 0:
            raise ValueError("compute_winset=True requires voters to be provided.")
        if winset_dist_config is None:
            winset_dist_config = DistanceConfig(salience_weights=[1.0, 1.0])
        winset_num_samples = int(winset_num_samples)
        winset_max = int(winset_max)
        if winset_num_samples < 4:
            raise ValueError(
                f"winset_num_samples must be >= 4, got {winset_num_samples}."
            )

        n_frames = n_rounds + 1
        seat_idxs_per_frame = []
        for r in range(n_rounds):
            st = trace.round_seat_totals(r)
            seat_idxs_per_frame.append([int(i) for i in np.where(st > 0)[0]])
        final_ss = trace.final_seat_shares()
        seat_idxs_per_frame.append([int(i) for i in np.where(final_ss > 0)[0]])

        total_winsets = sum(len(s) for s in seat_idxs_per_frame)
        if total_winsets > winset_max:
            n_seats_max = max(len(s) for s in seat_idxs_per_frame)
            raise ValueError(
                f"compute_winset: total WinSet count ({total_winsets}) exceeds "
                f"winset_max ({winset_max}). Got max seats per frame={n_seats_max}, "
                f"n_frames={n_frames}. Reduce the number of seats or rounds, or "
                f"increase winset_max."
            )

        voter_ideals_flat = np.empty(n_voters * 2, dtype=float)
        voter_ideals_flat[0::2] = voters_x
        voter_ideals_flat[1::2] = voters_y
        vif_buf = _ffi.cast("double *", _ffi.from_buffer(voter_ideals_flat))
        wcfg, _wka = _to_cffi_dist(winset_dist_config, n_dims=2)
        _WS_ERR = 512

        ws_data_payload = []
        ws_indices_payload = []
        for frame_i, pos_mat in enumerate(positions):
            seat_idxs = seat_idxs_per_frame[frame_i]
            frame_ws = []
            frame_cidxs = []
            for s in seat_idxs:
                frame_cidxs.append(s)
                sq_x = float(pos_mat[s, 0])
                sq_y = float(pos_mat[s, 1])
                is_empty = _ffi.new("int *")
                nx = _ffi.new("int *")
                np_paths = _ffi.new("int *")
                err = new_err_buf()
                _check(
                    _lib.scs_winset_2d_export_boundary(
                        sq_x, sq_y, vif_buf, n_voters, wcfg,
                        int(winset_k), int(winset_num_samples),
                        is_empty, _ffi.NULL, 0, _ffi.NULL, 0, _ffi.NULL,
                        nx, np_paths, err, _WS_ERR,
                    ),
                    err,
                )
                if is_empty[0]:
                    frame_ws.append(None)
                    continue
                n_x = int(nx[0])
                n_p = int(np_paths[0])
                tmp_xy = _ffi.new("double[]", n_x * 2)
                tmp_starts = _ffi.new("int[]", n_p)
                tmp_holes = _ffi.new("int[]", n_p)
                nx2 = _ffi.new("int *")
                np2 = _ffi.new("int *")
                is_empty2 = _ffi.new("int *")
                err = new_err_buf()
                _check(
                    _lib.scs_winset_2d_export_boundary(
                        sq_x, sq_y, vif_buf, n_voters, wcfg,
                        int(winset_k), int(winset_num_samples),
                        is_empty2, tmp_xy, n_x, tmp_starts, n_p, tmp_holes,
                        nx2, np2, err, _WS_ERR,
                    ),
                    err,
                )
                xy_flat = np.frombuffer(
                    _ffi.buffer(tmp_xy, int(nx2[0]) * 2 * 8), dtype=np.float64
                )
                path_starts = np.frombuffer(
                    _ffi.buffer(tmp_starts, int(np2[0]) * 4), dtype=np.int32
                )
                path_is_hole = np.frombuffer(
                    _ffi.buffer(tmp_holes, int(np2[0]) * 4), dtype=np.int32
                )
                n_paths = int(np2[0])
                ends = list(path_starts[1:]) + [int(nx2[0])]
                paths_list = []
                for p_i in range(n_paths):
                    s_idx = int(path_starts[p_i])
                    e_idx = int(ends[p_i])
                    paths_list.append(xy_flat[2 * s_idx : 2 * e_idx].tolist())
                frame_ws.append({
                    "paths":          paths_list,
                    "is_hole":        [int(h) for h in path_is_hole],
                    "competitor_idx": s,
                })
            ws_data_payload.append(frame_ws)
            ws_indices_payload.append(frame_cidxs)

    # ── Voronoi (Euclidean) per frame ─────────────────────────────────────────
    voronoi_cells_payload = None
    if compute_voronoi:
        _VORONOI_ERR_BUF = 512
        bbox = [float(xlim[0]), float(ylim[0]), float(xlim[1]), float(ylim[1])]
        voronoi_cells_payload = []
        for frame_i, pos_mat in enumerate(positions):
            sites_xy = np.asarray(pos_mat, dtype=np.float64).ravel()
            n_sites = pos_mat.shape[0]
            heap = _ffi.new("SCS_VoronoiCellsHeap *")
            err = new_err_buf()
            rc = _lib.scs_voronoi_cells_2d_heap(
                _ffi.cast("double *", _ffi.from_buffer(sites_xy)), n_sites,
                bbox[0], bbox[1], bbox[2], bbox[3],
                heap, err, _VORONOI_ERR_BUF,
            )
            if rc != 0:
                _lib.scs_voronoi_cells_heap_destroy(heap)
                _check(rc, err)
            try:
                h = heap[0]
                n_cells = h.cell_start_len - 1 if h.cell_start_len > 0 else 0
                cell_starts = np.frombuffer(
                    _ffi.buffer(h.cell_start, (n_cells + 1) * 4), dtype=np.int32
                ).copy()
                if h.n_xy_pairs > 0 and h.xy != _ffi.NULL:
                    xy_flat = np.frombuffer(
                        _ffi.buffer(h.xy, h.n_xy_pairs * 2 * 8), dtype=np.float64
                    ).copy()
                else:
                    xy_flat = np.zeros(0, dtype=np.float64)
                frame_cells = []
                for c in range(n_cells):
                    start = int(cell_starts[c])
                    end = int(cell_starts[c + 1])
                    if start >= end:
                        frame_cells.append(None)
                        continue
                    path_xy = xy_flat[2 * start : 2 * end].tolist()
                    frame_cells.append({"paths": [path_xy], "competitor_idx": c})
                voronoi_cells_payload.append(frame_cells)
            finally:
                _lib.scs_voronoi_cells_heap_destroy(heap)

    # ── Seat-holder indices (unconditional, used by JS stats block) ────────────
    # 0-based competitor indices of seat holders per frame; minimal payload cost.
    seat_holder_indices = []
    for r in range(n_rounds):
        st = trace.round_seat_totals(r)
        seat_holder_indices.append([int(i) for i in np.where(st > 0)[0]])
    final_ss_all = trace.final_seat_shares()
    seat_holder_indices.append([int(i) for i in np.where(final_ss_all > 0)[0]])

    # JSON payload — identical structure to the R binding.
    payload = {
        "voters_x":          voters_x,
        "voters_y":          voters_y,
        "voter_color":       voter_color,
        "xlim":              xlim,
        "ylim":              ylim,
        "dim_names":         list(dim_names),
        "rounds":            frame_names,
        "positions":         [
            [[float(p[c, 0]), float(p[c, 1])] for c in range(n_competitors)]
            for p in positions
        ],
        "competitor_names":  list(competitor_names),
        "competitor_colors": list(colors),
        "vote_shares":       vote_shares,
        "trail":                trail,
        "trail_length":         trail_len,
        "title":                title,
        "overlays_static":      _serialise_overlays_static(overlays),
        "seat_holder_indices":  seat_holder_indices,
    }
    if ic_data_payload is not None:
        payload["indifference_curves"]   = ic_data_payload
        payload["ic_competitor_indices"] = ic_indices_payload
    if ws_data_payload is not None:
        payload["winsets"]                   = ws_data_payload
        payload["winset_competitor_indices"] = ws_indices_payload
    if voronoi_cells_payload is not None:
        payload["voronoi_cells"] = voronoi_cells_payload

    if payload_path is not None:
        _save_scscanvas(payload, width, height, payload_path)

    html = _html_from_payload(payload, width, height)
    if path is not None:
        Path(path).write_text(html, encoding="utf-8")
    return html


def load_competition_canvas(path, width=None, height=None):
    """Load a competition canvas animation from a ``.scscanvas`` file.

    Reads a file written by :func:`animate_competition_canvas` (via the
    ``payload_path`` argument) or the R equivalent ``save_competition_canvas()``,
    and returns the self-contained HTML string — identical to what
    :func:`animate_competition_canvas` would return — without recomputing
    anything.

    Parameters
    ----------
    path:
        Path to a ``.scscanvas`` JSON file.
    width, height:
        Widget dimensions in pixels.  ``None`` uses the values stored in the
        file.

    Returns
    -------
    str
        Self-contained HTML string, ready to write to a ``.html`` file or
        display in a Jupyter notebook via ``IPython.display.HTML``.

    Examples
    --------
    >>> html = load_competition_canvas("run_200.scscanvas")
    >>> with open("run_200.html", "w") as f:
    ...     f.write(html)
    """
    p = Path(path)
    if not p.exists():
        raise FileNotFoundError(
            f"load_competition_canvas: file not found: {path}"
        )
    envelope = json.loads(p.read_text(encoding="utf-8"))
    fmt = envelope.get("format")
    if fmt != "scscanvas":
        raise ValueError(
            f"load_competition_canvas: unexpected format {fmt!r} "
            f"(expected 'scscanvas'). Is '{path}' a .scscanvas file?"
        )
    payload = envelope["payload"]
    w = width  if width  is not None else envelope.get("width")  or 900
    h = height if height is not None else envelope.get("height") or 800
    return _html_from_payload(payload, w, h)


def layer_ic(
    fig,
    voters,
    sq,
    dist_config=None,
    color_by_voter=False,
    fill_color=None,
    line_color=None,
    line_width=1,
    palette="auto",
    voter_names=None,
    name="Indifference Curves",
    theme="dark2",
):
    """Add voter indifference curves to a canvas payload dict."""
    _assert_spatial_payload(fig)
    vx, vy = _flat_to_xy(voters)
    sqv = np.asarray(sq, dtype=float).ravel()
    n_v = len(vx)
    if voter_names is None:
        voter_names = [f"V{i + 1}" for i in range(n_v)]

    pal_name = theme if palette == "auto" else palette

    if color_by_voter:
        line_colors = scl_palette(pal_name, n_v, alpha=0.70)
        fill_colors = (
            [fill_color] * n_v if fill_color is not None
            else scl_palette(pal_name, n_v, alpha=0.06)
        )
    else:
        lc = line_color if line_color is not None else _ic_uniform_line(theme)
        line_colors = [lc] * n_v
        fill_colors = [fill_color if fill_color is not None else "rgba(0,0,0,0)"] * n_v

    linear_loss = LossConfig(loss_type="linear")
    curves = list(fig["layers"].get("ic_curves") or [])

    for i in range(n_v):
        if dist_config is None:
            r = math.sqrt((vx[i] - sqv[0]) ** 2 + (vy[i] - sqv[1]) ** 2)
            cx, cy = _circle_pts(float(vx[i]), float(vy[i]), r)
            px, py = cx.tolist(), cy.tolist()
        else:
            poly = ic_polygon_2d(
                float(vx[i]), float(vy[i]),
                float(sqv[0]), float(sqv[1]),
                linear_loss, dist_config, 64,
            )
            px = poly[:, 0].tolist() + [float(poly[0, 0])]
            py = poly[:, 1].tolist() + [float(poly[0, 1])]
        flat = _poly_to_flat_xy(px, py)
        entry = {
            "xy": flat,
            "line": line_colors[i],
            "fill": fill_colors[i] if (color_by_voter or fill_color is not None) else None,
        }
        curves.append(entry)
    fig["layers"]["ic_curves"] = curves
    return fig


def layer_preferred_regions(
    fig,
    voters,
    sq,
    dist_config=None,
    color_by_voter=False,
    fill_color=None,
    line_color=None,
    palette="auto",
    voter_names=None,
    name="Preferred Region",
    theme="dark2",
):
    """Add voter preferred-to regions to a canvas payload dict."""
    _assert_spatial_payload(fig)
    vx, vy = _flat_to_xy(voters)
    sqv = np.asarray(sq, dtype=float).ravel()
    n_v = len(vx)
    if voter_names is None:
        voter_names = [f"V{i + 1}" for i in range(n_v)]

    pal_name = theme if palette == "auto" else palette

    if color_by_voter:
        line_colors = scl_palette(pal_name, n_v, alpha=0.65)
        fill_colors = scl_palette(pal_name, n_v, alpha=0.10)
    else:
        fill_colors = [fill_color if fill_color is not None
                       else _preferred_uniform_fill(theme)] * n_v
        line_colors = [line_color if line_color is not None
                       else _preferred_uniform_line(theme)] * n_v

    linear_loss = LossConfig(loss_type="linear")
    regs = list(fig["layers"].get("preferred_regions") or [])

    for i in range(n_v):
        if dist_config is None:
            r = math.sqrt((vx[i] - sqv[0]) ** 2 + (vy[i] - sqv[1]) ** 2)
            cx, cy = _circle_pts(float(vx[i]), float(vy[i]), r)
            px, py = cx.tolist(), cy.tolist()
        else:
            poly = ic_polygon_2d(
                float(vx[i]), float(vy[i]),
                float(sqv[0]), float(sqv[1]),
                linear_loss, dist_config, 64,
            )
            px = poly[:, 0].tolist() + [float(poly[0, 0])]
            py = poly[:, 1].tolist() + [float(poly[0, 1])]
        flat = _poly_to_flat_xy(px, py)
        regs.append({"xy": flat, "fill": fill_colors[i], "line": line_colors[i]})
    fig["layers"]["preferred_regions"] = regs
    return fig


def layer_winset(
    fig,
    winset=None,
    fill_color=None,
    line_color=None,
    name="Winset",
    voters=None,
    sq=None,
    dist_config=None,
    theme="dark2",
):
    """Add a winset polygon layer to a canvas payload dict."""
    _assert_spatial_payload(fig)

    if winset is None:
        if voters is None or sq is None:
            raise ValueError(
                "layer_winset: provide either 'winset' (a Winset object) "
                "or both 'voters' and 'sq' to compute the winset automatically."
            )
        import socialchoicelab as scl
        sqv = np.asarray(sq, dtype=float).ravel()
        winset = scl.winset_2d(float(sqv[0]), float(sqv[1]),
                               np.asarray(voters, dtype=float),
                               dist_config=dist_config)

    fill_c = fill_color if fill_color is not None else _layer_fill_color("winset", theme)
    line_c = line_color if line_color is not None else _layer_line_color("winset", theme)

    if winset.is_empty():
        return fig

    bnd = winset.boundary()
    xy = bnd["xy"]
    starts = bnd["path_starts"]
    is_hole = bnd["path_is_hole"]
    n_paths = len(starts)
    ends = list(starts[1:]) + [len(xy)]

    paths = list(fig["layers"].get("winset_paths") or [])
    hole_fill = "rgba(248,249,250,0.95)"

    for i in range(n_paths):
        fc_i = hole_fill if is_hole[i] else fill_c
        s, e = int(starts[i]), int(ends[i])
        px = list(xy[s:e, 0]) + [xy[s, 0]]
        py = list(xy[s:e, 1]) + [xy[s, 1]]
        flat = _poly_to_flat_xy(px, py)
        paths.append({"xy": flat, "fill": fc_i, "line": line_c})
    fig["layers"]["winset_paths"] = paths
    return fig


def layer_yolk(
    fig,
    center_x,
    center_y,
    radius,
    fill_color=None,
    line_color=None,
    name="Yolk",
    theme="dark2",
):
    """Add a yolk circle layer to a canvas payload dict."""
    _assert_spatial_payload(fig)
    fill_c = fill_color if fill_color is not None else _layer_fill_color("yolk", theme)
    line_c = line_color if line_color is not None else _layer_line_color("yolk", theme)
    fig["layers"]["yolk"] = {
        "cx": float(center_x),
        "cy": float(center_y),
        "r": float(radius),
        "fill": fill_c,
        "line": line_c,
    }
    return fig


def layer_uncovered_set(
    fig,
    boundary_xy=None,
    fill_color=None,
    line_color=None,
    name="Uncovered Set",
    voters=None,
    grid_resolution=20,
    theme="dark2",
):
    """Add an uncovered set boundary layer to a canvas payload dict."""
    _assert_spatial_payload(fig)

    if boundary_xy is None:
        if voters is None:
            raise ValueError(
                "layer_uncovered_set: provide either 'boundary_xy' "
                "or 'voters' to compute the boundary automatically."
            )
        import socialchoicelab as scl
        boundary_xy = scl.uncovered_set_boundary_2d(
            np.asarray(voters, dtype=float), grid_resolution=int(grid_resolution)
        )

    fill_c = fill_color if fill_color is not None else _layer_fill_color("uncovered_set", theme)
    line_c = line_color if line_color is not None else _layer_line_color("uncovered_set", theme)

    bxy = np.asarray(boundary_xy, dtype=float)
    if len(bxy) == 0:
        return fig
    px = list(bxy[:, 0]) + [bxy[0, 0]]
    py = list(bxy[:, 1]) + [bxy[0, 1]]
    fig["layers"]["uncovered_xy"] = {
        "xy": _poly_to_flat_xy(px, py),
        "fill": fill_c,
        "line": line_c,
    }
    return fig


def layer_convex_hull(
    fig,
    hull_xy,
    fill_color=None,
    line_color=None,
    name="Pareto Set",
    theme="dark2",
):
    """Add a Pareto set layer (convex hull of ideals for Euclidean preferences).

    The canvas labels this layer "Pareto Set". Payload key remains ``convex_hull_xy``.
    """
    _assert_spatial_payload(fig)
    fill_c = fill_color if fill_color is not None else _layer_fill_color("convex_hull", theme)
    line_c = line_color if line_color is not None else _layer_line_color("convex_hull", theme)

    hxy = np.asarray(hull_xy, dtype=float)
    if len(hxy) == 0:
        return fig
    px = list(hxy[:, 0]) + [hxy[0, 0]]
    py = list(hxy[:, 1]) + [hxy[0, 1]]
    fig["layers"]["convex_hull_xy"] = {
        "xy": _poly_to_flat_xy(px, py),
        "fill": fill_c,
        "line": line_c,
    }
    return fig


def layer_centroid(fig, voters, color=None, name="Centroid", theme="dark2"):
    """Add a centroid marker to a canvas payload dict."""
    _assert_spatial_payload(fig)
    stroke = color or _centroid_overlay_color(theme)
    x, y = centroid_2d(voters)
    fig["layers"]["centroid"] = [float(x), float(y)]
    fig["layers"]["centroid_stroke"] = stroke
    return fig


def layer_marginal_median(fig, voters, color=None, name="Marginal Median", theme="dark2"):
    """Add a marginal median marker to a canvas payload dict."""
    _assert_spatial_payload(fig)
    fill_c = color or _marginal_median_overlay_color(theme)
    stroke_c = _overlay_triangle_outline_color(theme)
    x, y = marginal_median_2d(voters)
    fig["layers"]["marginal_median"] = [float(x), float(y)]
    fig["layers"]["marginal_median_fill"] = fill_c
    fig["layers"]["marginal_median_stroke"] = stroke_c
    return fig


def save_plot(fig, path, width=None, height=None):
    """Save a spatial voting canvas payload to a standalone HTML file."""
    _assert_spatial_payload(fig)
    ext = Path(path).suffix.lower()
    w = int(width if width is not None else fig.get("_width", 700))
    h = int(height if height is not None else fig.get("_height", 600))
    export = {k: v for k, v in fig.items() if not str(k).startswith("_")}
    if ext == ".html":
        html = _html_from_spatial_payload(export, w, h)
        Path(path).write_text(html, encoding="utf-8")
    elif ext in {".png", ".svg", ".pdf", ".jpeg", ".jpg", ".webp"}:
        raise NotImplementedError(
            "PNG/SVG export is not yet supported for canvas plots; save as HTML "
            "and use the browser's print/screenshot tools."
        )
    else:
        raise ValueError(
            f"save_plot: unsupported file extension '{ext}'. Supported formats: .html."
        )
    return str(path)
