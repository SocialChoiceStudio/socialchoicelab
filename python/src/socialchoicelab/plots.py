"""Spatial voting visualization helpers (C10/C13).

All functions return :class:`plotly.graph_objects.Figure` objects so they
can be displayed in Jupyter, saved to HTML, or embedded in reports.

Layers compose by passing the figure returned by one function into the next::

    import socialchoicelab as scl
    import socialchoicelab.plots as sclp
    import numpy as np

    voters = np.array([-1.0, -0.5,  0.0, 0.0,  0.8, 0.6, -0.4,  0.8,  0.5, -0.7])
    sq     = np.array([0.0, 0.0])

    fig = sclp.plot_spatial_voting(voters, sq=sq, theme="dark2")
    fig = sclp.layer_ic(fig, voters, sq, theme="dark2")
    fig = sclp.layer_winset(fig, voters=voters, sq=sq, theme="dark2")
    fig.show()
"""

from __future__ import annotations

import json
import math
from pathlib import Path

import numpy as np

# Path to the canvas player JS bundled with the Python package.
# The canonical source lives in r/inst/htmlwidgets/competition_canvas.js;
# a copy is kept here so it is available after pip install.
_CANVAS_JS_PATH = Path(__file__).parent / "competition_canvas.js"

from socialchoicelab._functions import (
    calculate_distance,
    distance_to_utility,
    level_set_1d,
    level_set_2d,
    level_set_to_polygon,
)
from socialchoicelab._geometry import centroid_2d, marginal_median_2d
from socialchoicelab._winset import winset_2d
from socialchoicelab._types import DistanceConfig, LossConfig
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

try:
    import plotly.graph_objects as go
    _PLOTLY_AVAILABLE = True
except ImportError:
    _PLOTLY_AVAILABLE = False

__all__ = [
    "plot_spatial_voting",
    "plot_competition_trajectories",
    "animate_competition_trajectories",
    "animate_competition_canvas",
    "layer_ic",
    "layer_preferred_regions",
    "layer_winset",
    "layer_yolk",
    "layer_uncovered_set",
    "layer_convex_hull",
    "layer_centroid",
    "layer_marginal_median",
    "save_plot",
    "finalize_plot",
    # Re-exported for convenience so users can do `sclp.scl_palette(...)`
    "scl_palette",
    "scl_theme_colors",
]

_TRACE_ROLE_PRIORITY = {
    "region": 1,
    "line": 2,
    "point": 3,
    "overlay": 4,
}


def _require_plotly() -> None:
    if not _PLOTLY_AVAILABLE:
        raise ImportError(
            "plotly is required for visualization. "
            "Install it with: pip install plotly"
        )


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


def _trace_role(trace) -> str:
    meta = getattr(trace, "meta", None)
    if isinstance(meta, dict):
        return str(meta.get("scl_role", "point"))
    return "point"


def _set_trace_role(trace, role: str):
    meta = getattr(trace, "meta", None)
    trace.meta = dict(meta) if isinstance(meta, dict) else {}
    trace.meta["scl_role"] = role
    return trace


def _normalize_trace_roles(fig):
    ordered = sorted(
        enumerate(fig.data),
        key=lambda item: (_TRACE_ROLE_PRIORITY.get(_trace_role(item[1]), 99), item[0]),
    )
    fig.data = tuple(trace for _, trace in ordered)
    return fig


def _add_role_trace(fig, trace, role: str):
    fig.add_trace(_set_trace_role(trace, role))
    return _normalize_trace_roles(fig)


def _set_rgba_alpha(color: str, alpha: float) -> str:
    if color.startswith("rgba(") and color.endswith(")"):
        parts = [p.strip() for p in color[5:-1].split(",")]
        if len(parts) == 4:
            parts[3] = f"{alpha:.3f}"
            return f"rgba({', '.join(parts)})"
    return color


def _apply_plot_config(fig):
    fig.update_layout(modebar=dict(remove=["select2d", "lasso2d", "autoScale2d"]))
    fig._config = {
        "displaylogo": False,
        "modeBarButtonsToRemove": ["select2d", "lasso2d", "autoScale2d"],
    }
    return fig


def plot_spatial_voting(
    voters,
    alternatives=None,
    sq=None,
    voter_names=None,
    alt_names=None,
    dim_names=("Dimension 1", "Dimension 2"),
    title="Spatial Voting Analysis",
    show_labels=False,
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
    xlim, ylim:
        ``[min, max]`` for explicit axis ranges.  ``None`` auto-computes.
    theme:
        Colour theme: ``"dark2"`` (default, ColorBrewer Dark2, colorblind-safe),
        ``"set2"``, ``"okabe_ito"``, ``"paired"``, or ``"bw"`` (black-and-white
        print).  Pass the same value to every ``layer_*`` call for coordinated
        colours across the whole plot.
    width, height:
        Figure dimensions in pixels.

    Returns
    -------
    plotly.graph_objects.Figure

    Examples
    --------
    >>> import socialchoicelab.plots as sclp
    >>> import numpy as np
    >>> voters = np.array([-1.0, -0.5, 0.0, 0.0, 0.8, 0.6])
    >>> fig = sclp.plot_spatial_voting(voters, sq=np.array([0.0, 0.0]))
    """
    _require_plotly()
    vx, vy = _flat_to_xy(voters)
    n_v = len(vx)
    if voter_names is None:
        voter_names = [f"V{i + 1}" for i in range(n_v)]

    voter_col = _voter_point_color(theme)
    alt_col   = _alt_point_color(theme)
    sq_col    = "rgba(0,0,0,0.90)" if theme == "bw" else "rgba(20,20,20,0.90)"

    all_x = list(vx)
    all_y = list(vy)
    if sq is not None:
        sqv = np.asarray(sq, dtype=float).ravel()
        all_x.append(float(sqv[0]))
        all_y.append(float(sqv[1]))
    if xlim is None and len(all_x) > 0:
        xlim = _range_with_origin(np.array(all_x))
    if ylim is None and len(all_y) > 0:
        ylim = _range_with_origin(np.array(all_y))

    fig = go.Figure()

    fig = _add_role_trace(fig, go.Scatter(
        x=vx, y=vy,
        mode="markers",
        name="Voters",
        marker=dict(symbol="circle", size=8, color=voter_col,
                    line=dict(color="white", width=1.5)),
        text=voter_names,
        hovertemplate="%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>",
    ), "point")

    if alternatives is not None:
        ax, ay = _flat_to_xy(np.asarray(alternatives, dtype=float).ravel())
        n_a = len(ax)
        if alt_names is None:
            alt_names = [f"Alt {i + 1}" for i in range(n_a)]
        all_x.extend(list(ax))
        all_y.extend(list(ay))
        fig = _add_role_trace(fig, go.Scatter(
            x=ax, y=ay,
            mode="markers+text" if show_labels else "markers",
            name="Alternatives",
            marker=dict(symbol="diamond", size=14, color=alt_col,
                        line=dict(color="white", width=1.5)),
            text=alt_names,
            textposition="top center",
            hovertemplate="%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>",
        ), "point")

    if sq is not None:
        fig = _add_role_trace(fig, go.Scatter(
            x=[sqv[0]], y=[sqv[1]],
            mode="markers",
            name="Status Quo",
            marker=dict(symbol="star", size=18, color=sq_col,
                        line=dict(color="white", width=1.5)),
            hovertemplate="Status Quo<br>(%{x:.3f}, %{y:.3f})<extra></extra>",
        ), "point")

    fig.update_layout(
        title=dict(text=title, x=0.5, xanchor="center"),
        xaxis=dict(title=dim_names[0], zeroline=True,
                   zerolinecolor="rgba(140,140,140,0.45)", zerolinewidth=1,
                   showgrid=True, gridcolor="rgba(140,140,140,0.25)",
                   gridwidth=1, range=xlim),
        yaxis=dict(title=dim_names[1], zeroline=True,
                   zerolinecolor="rgba(140,140,140,0.45)", zerolinewidth=1,
                   showgrid=True, gridcolor="rgba(140,140,140,0.25)",
                   gridwidth=1, scaleanchor="x", scaleratio=1, range=ylim),
        legend=dict(x=1.02, y=1, xanchor="left"),
        hovermode="closest",
        plot_bgcolor="white",
        paper_bgcolor="white",
        width=width,
        height=height,
    )
    return _apply_plot_config(fig)


def plot_competition_trajectories(
    trace,
    voters=None,
    competitor_names=None,
    title="Competition Trajectories",
    theme="dark2",
    width=700,
    height=600,
):
    """Plot 2D competition trajectories from a CompetitionTrace.

    Parameters
    ----------
    trace:
        A :class:`socialchoicelab.CompetitionTrace`.
    voters:
        Optional flat or ``(n, 2)`` voter array for background points.
    competitor_names:
        Optional labels for competitors.
    title, theme, width, height:
        Standard plotting options.
    """
    _require_plotly()
    n_rounds, n_competitors, n_dims = trace.dims()
    if n_dims != 2:
        raise ValueError(
            f"plot_competition_trajectories currently supports only 2D traces, got n_dims={n_dims}."
        )
    if competitor_names is None:
        competitor_names = [f"Competitor {i + 1}" for i in range(n_competitors)]
    if len(competitor_names) != n_competitors:
        raise ValueError("competitor_names length must match n_competitors.")

    voter_flat = np.asarray(voters if voters is not None else [], dtype=float).ravel()
    fig = plot_spatial_voting(
        voter_flat,
        alternatives=None,
        sq=None,
        title=title,
        theme=theme,
        width=width,
        height=height,
    )

    round_positions = [trace.round_positions(r) for r in range(n_rounds)]
    round_positions.append(trace.final_positions())
    colors = scl_palette(theme, n_competitors, alpha=0.95)

    for idx in range(n_competitors):
        path = np.vstack([pos[idx, :] for pos in round_positions])
        fig = _add_role_trace(
            fig,
            go.Scatter(
                x=path[:, 0],
                y=path[:, 1],
                mode="lines+markers",
                name=competitor_names[idx],
                marker=dict(symbol="diamond", size=12, color=colors[idx], line=dict(color="white", width=1.0)),
                line=dict(color=colors[idx], width=3),
                text=[f"Round {r}" for r in range(n_rounds)] + ["Final"],
                hovertemplate="%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>",
            )
            ,
            "overlay",
        )

    return _apply_plot_config(fig)


def animate_competition_trajectories(
    trace,
    voters=None,
    competitor_names=None,
    title="Competition Trajectories",
    theme="dark2",
    width=700,
    height=600,
    trail="fade",
    trail_length="medium",
):
    """Animate 2D competition trajectories from a CompetitionTrace.

    Parameters
    ----------
    trace:
        A ``CompetitionTrace`` object from ``competition_run``. Must be 2D.
    voters:
        Flat array or array-like of voter ideal points (row-major, 2 columns).
        Passed directly to ``plot_spatial_voting``.
    competitor_names:
        List of display names for each competitor. Defaults to
        ``["Competitor 1", "Competitor 2", ...]``.
    title:
        Plot title string.
    theme:
        Colour theme name (see ``scl_palette``).
    width, height:
        Figure dimensions in pixels.
    trail:
        Trail style for animated paths. Three modes are supported:

        ``"fade"`` *(default)*
            Each animation step shows a short fading trail of recent segments
            behind each competitor. Opacity decays exponentially from the
            current position outward; segments older than ``trail_length`` are
            hidden. Conveys direction and recent history without cluttering
            the plot.

        ``"full"``
            The complete path from round 1 to the current step is drawn as a
            continuous line. Useful for inspecting the entire trajectory, but
            can become visually busy in long competitions.

        ``"none"``
            Markers only; no path segments are drawn. Cleanest output,
            suitable when only final positions matter or the animation is
            embedded in a space-constrained layout.

    trail_length:
        Controls how many past segments are shown when ``trail="fade"``.
        Accepts a string shorthand or a positive integer:

        ``"short"``
            The most recent 1/3 of all rounds. Good for dense or fast-moving
            competitions.

        ``"medium"`` *(default)*
            The most recent 1/2 of all rounds. Balances recency and context
            for most competition lengths.

        ``"long"``
            The most recent 3/4 of all rounds. Useful when the full trajectory
            arc matters but ``"full"`` would be too cluttered.

        *positive integer*
            Exact number of segments to retain, independent of total round
            count.

        Ignored when ``trail`` is ``"none"`` or ``"full"``.

    Returns
    -------
    plotly.graph_objects.Figure
        A Plotly figure with animation frames, a slider, and Play/Pause
        controls. The slider starts paused (``active=-1``); press Play or
        drag the slider to animate.
    """
    _require_plotly()
    n_rounds, n_competitors, n_dims = trace.dims()
    if n_dims != 2:
        raise ValueError(
            f"animate_competition_trajectories currently supports only 2D traces, got n_dims={n_dims}."
        )
    if competitor_names is None:
        competitor_names = [f"Competitor {i + 1}" for i in range(n_competitors)]
    if len(competitor_names) != n_competitors:
        raise ValueError("competitor_names length must match n_competitors.")
    if trail not in {"none", "full", "fade"}:
        raise ValueError("trail must be one of: 'none', 'full', 'fade'.")
    _TRAIL_LENGTH_SHORTHANDS = {"short", "medium", "long"}
    if isinstance(trail_length, str) and trail_length not in _TRAIL_LENGTH_SHORTHANDS:
        raise ValueError(
            f"trail_length {trail_length!r} is not recognised. "
            "Use 'short' (1/3 rounds), 'medium' (1/2 rounds), 'long' (3/4 rounds), "
            "or a positive integer."
        )
    if not isinstance(trail_length, (str, int)):
        raise ValueError("trail_length must be a positive integer or one of 'short', 'medium', 'long'.")

    voter_flat = np.asarray(voters if voters is not None else [], dtype=float).ravel()
    fig = plot_spatial_voting(
        voter_flat,
        alternatives=None,
        sq=None,
        title=title,
        theme=theme,
        width=width,
        height=height,
    )
    positions = [trace.round_positions(r) for r in range(n_rounds)]
    positions.append(trace.final_positions())
    frame_names = [f"Round {r + 1}" for r in range(n_rounds)] + ["Final"]
    colors = scl_palette(theme, n_competitors, alpha=0.95)
    n_segments_max = max(len(frame_names) - 1, 0)
    if isinstance(trail_length, str):
        trail_length = max(1, {
            "short":  n_segments_max // 3,
            "medium": n_segments_max // 2,
            "long":   (n_segments_max * 3) // 4,
        }[trail_length])
    else:
        trail_length = int(trail_length)
        if trail_length < 1:
            raise ValueError(f"trail_length must be >= 1, got {trail_length}.")

    initial = positions[0]
    if trail == "none":
        static_count = len(fig.data)
        anim_records = []
        for frame_idx, frame_name in enumerate(frame_names):
            for idx in range(n_competitors):
                anim_records.append(
                    {
                        "frame": frame_name,
                        "competitor": competitor_names[idx],
                        "x": float(positions[frame_idx][idx, 0]),
                        "y": float(positions[frame_idx][idx, 1]),
                        "hover_text": frame_name,
                        "color": colors[idx],
                    }
                )
        for idx in range(n_competitors):
            fig = _add_role_trace(
                fig,
                go.Scatter(
                    x=[initial[idx, 0]],
                    y=[initial[idx, 1]],
                    mode="markers",
                    name=competitor_names[idx],
                    marker=dict(symbol="diamond", size=12, color=colors[idx], line=dict(color="white", width=1.0)),
                    text=[frame_names[0]],
                    hovertemplate="%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>",
                ),
                "overlay",
            )

        frames = []
        for frame_name in frame_names:
            frame_rows = [row for row in anim_records if row["frame"] == frame_name]
            frame_data = []
            for idx in range(n_competitors):
                row = frame_rows[idx]
                frame_data.append(
                    go.Scatter(
                        x=[row["x"]],
                        y=[row["y"]],
                        mode="markers",
                        name=row["competitor"],
                        marker=dict(
                            symbol="diamond",
                            size=12,
                            color=row["color"],
                            line=dict(color="white", width=1.0),
                        ),
                        text=[row["hover_text"]],
                        hovertemplate="%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>",
                    )
                )
            frames.append(
                go.Frame(
                    data=frame_data,
                    name=frame_name,
                    traces=list(range(static_count, static_count + n_competitors)),
                )
            )
        fig.frames = frames
    elif trail == "fade":
        overlay_start = len(fig.data)
        for idx in range(n_competitors):
            for _ in range(n_segments_max):
                fig = _add_role_trace(
                    fig,
                    go.Scatter(
                        x=[],
                        y=[],
                        mode="lines",
                        name=competitor_names[idx],
                        showlegend=False,
                        hoverinfo="skip",
                        line=dict(color=colors[idx], width=3),
                    ),
                    "overlay",
                )
            fig = _add_role_trace(
                fig,
                go.Scatter(
                    x=[initial[idx, 0]],
                    y=[initial[idx, 1]],
                    mode="markers",
                    name=competitor_names[idx],
                    marker=dict(symbol="diamond", size=12, color=colors[idx], line=dict(color="white", width=1.0)),
                    text=[frame_names[0]],
                    hovertemplate="%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>",
                ),
                "overlay",
            )
        frames = []
        for frame_idx, frame_name in enumerate(frame_names):
            frame_data = []
            for idx in range(n_competitors):
                actual_segments = frame_idx
                for age in range(1, n_segments_max + 1):
                    if age <= actual_segments and age <= trail_length:
                        seg_idx = actual_segments - age
                        p0 = positions[seg_idx][idx, :]
                        p1 = positions[seg_idx + 1][idx, :]
                        alpha = max(0.12, 0.88 * (0.55 ** (age - 1)))
                        frame_data.append(
                            go.Scatter(
                                x=[p0[0], p1[0]],
                                y=[p0[1], p1[1]],
                                mode="lines",
                                name=competitor_names[idx],
                                showlegend=False,
                                hoverinfo="skip",
                                line=dict(color=_set_rgba_alpha(colors[idx], alpha), width=3),
                            )
                        )
                    else:
                        frame_data.append(
                            go.Scatter(x=[], y=[], mode="lines", showlegend=False, hoverinfo="skip")
                        )
                current = positions[frame_idx][idx, :]
                frame_data.append(
                    go.Scatter(
                        x=[current[0]],
                        y=[current[1]],
                        mode="markers",
                        name=competitor_names[idx],
                        marker=dict(symbol="diamond", size=12, color=colors[idx], line=dict(color="white", width=1.0)),
                        text=[frame_name],
                        hovertemplate="%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>",
                    )
                )
            traces = list(range(overlay_start, overlay_start + n_competitors * (n_segments_max + 1)))
            frames.append(go.Frame(data=frame_data, name=frame_name, traces=traces))
        fig.frames = frames
    else:
        # trail == "full": one lines+markers trace per competitor, accumulating positions.
        overlay_start = len(fig.data)
        for idx in range(n_competitors):
            fig = _add_role_trace(
                fig,
                go.Scatter(
                    x=[initial[idx, 0]],
                    y=[initial[idx, 1]],
                    mode="lines+markers",
                    name=competitor_names[idx],
                    marker=dict(symbol="diamond", size=12, color=colors[idx], line=dict(color="white", width=1.0)),
                    line=dict(color=colors[idx], width=3),
                    text=[frame_names[0]],
                    hovertemplate="%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>",
                ),
                "overlay",
            )
        frames = []
        for frame_idx, frame_name in enumerate(frame_names):
            frame_data = []
            for idx in range(n_competitors):
                current_path = [pos[idx, :] for pos in positions[: frame_idx + 1]]
                path = np.vstack(current_path)
                frame_data.append(
                    go.Scatter(
                        x=path[:, 0],
                        y=path[:, 1],
                        mode="lines+markers",
                        name=competitor_names[idx],
                        marker=dict(symbol="diamond", size=12, color=colors[idx], line=dict(color="white", width=1.0)),
                        line=dict(color=colors[idx], width=3),
                        text=[f"Round {r + 1}" for r in range(min(frame_idx + 1, n_rounds))]
                        + (["Final"] if frame_idx == len(frame_names) - 1 else []),
                        hovertemplate="%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>",
                    )
                )
            traces = list(range(overlay_start, overlay_start + n_competitors))
            frames.append(go.Frame(data=frame_data, name=frame_name, traces=traces))
        fig.frames = frames
    fig.update_layout(
        margin=dict(t=95, r=20, b=220, l=65),
        updatemenus=[
            dict(
                type="buttons",
                showactive=False,
                x=0.5,
                y=-0.36,
                xanchor="center",
                yanchor="top",
                pad={"t": 0, "r": 8},
                direction="right",
                buttons=[
                    dict(
                        label="Play",
                        method="animate",
                        args=[frame_names, {"frame": {"duration": 700, "redraw": True},
                                     "transition": {"duration": 0},
                                     "fromcurrent": False,
                                     "mode": "next"}],
                    ),
                    dict(
                        label="Pause",
                        method="animate",
                        args=[[None], {"frame": {"duration": 0, "redraw": True},
                                       "transition": {"duration": 0},
                                       "mode": "immediate"}],
                    ),
                ],
            )
        ],
        sliders=[
            dict(
                active=0,
                x=0.08,
                y=-0.17,
                len=0.84,
                currentvalue={"visible": False},
                pad={"t": 0, "b": 0},
                steps=[
                    dict(
                        label="",
                        method="animate",
                        args=[[frame_name], {"frame": {"duration": 0, "redraw": True},
                                             "transition": {"duration": 0},
                                             "mode": "immediate"}],
                    )
                    for frame_name in frame_names
                ],
            )
        ],
    )
    # Plotly.js accepts active=-1 (no step executed on initial render, preventing
    # autoplay on load), but the Python wrapper validates active >= 0. Patch the
    # internal props dict directly so the correct value reaches the browser.
    if fig.layout.sliders:
        fig.layout.sliders[0]._props["active"] = -1
    return fig


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


def _html_from_payload(payload, width, height):
    """Build a self-contained HTML string from a computed canvas payload dict.

    The title is read from ``payload["title"]``.  Both the 1D and 2D internal
    builders delegate here so the HTML structure is defined in exactly one place.
    """
    if not _CANVAS_JS_PATH.exists():
        raise FileNotFoundError(
            f"Canvas player JS not found at {_CANVAS_JS_PATH}. "
            "Ensure r/inst/htmlwidgets/competition_canvas.js is present."
        )
    js_player = _CANVAS_JS_PATH.read_text(encoding="utf-8")
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
        "    window.HTMLWidgets = (function() {",
        "      var defs = [];",
        "      return {",
        "        widget: function(d) { defs.push(d); },",
        "        _init:  function(el, w, h, data) {",
        "          defs.forEach(function(d) {",
        '            if (d.type === "output") { d.factory(el, w, h).renderValue(data); }',
        "          });",
        "        }",
        "      };",
        "    })();",
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

    # 1D IC computation — uses level_set_1d to get interval boundaries per voter
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

        # Extract salience weight for level_set_1d
        weight_1d = (
            float(ic_dist_config.salience_weights[0])
            if ic_dist_config.salience_weights is not None
            else 1.0
        )

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
                    voter_x_v = voters_x[v]
                    dist_val = calculate_distance([voter_x_v], [seat_x], ic_dist_config)
                    util_level = distance_to_utility(dist_val, ic_loss_config)
                    pts = level_set_1d(voter_x_v, weight_1d, util_level, ic_loss_config)
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
    """Animate 2D competition trajectories using the canvas-based player.

    Produces the same animation as :func:`animate_competition_trajectories` but
    delivers it as a self-contained HTML file using the shared Canvas 2D player.
    Data is stored once and frames are drawn on demand, so it scales to long
    runs without the large payload that Plotly frame-based animations require.

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
                    voter_ideal = [voters_x[v], voters_y[v]]
                    dist_val = calculate_distance(voter_ideal, seat_pos, ic_dist_config)
                    util_level = distance_to_utility(dist_val, ic_loss_config)
                    ls = level_set_2d(
                        voters_x[v], voters_y[v], util_level,
                        ic_loss_config, ic_dist_config,
                    )
                    poly = level_set_to_polygon(ls, ic_num_samples)
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
                ws = winset_2d(
                    sq_x, sq_y, voter_ideals_flat,
                    dist_config=winset_dist_config,
                    k=winset_k,
                    num_samples=winset_num_samples,
                )
                if ws.is_empty():
                    frame_ws.append(None)
                else:
                    bnd = ws.boundary()
                    xy = bnd["xy"]
                    path_starts = bnd["path_starts"]
                    path_is_hole = bnd["path_is_hole"]
                    n_paths = len(path_starts)
                    ends = list(path_starts[1:]) + [len(xy)]
                    paths_list = []
                    for p_i in range(n_paths):
                        s_idx = int(path_starts[p_i])
                        e_idx = int(ends[p_i])
                        paths_list.append(xy[s_idx:e_idx].ravel().tolist())
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
            err = new_err_buf()
            out_total = _ffi.new("int *")
            out_n_cells = _ffi.new("int *")
            _check(
                _lib.scs_voronoi_cells_2d_size(
                    _ffi.cast("double *", _ffi.from_buffer(sites_xy)), n_sites,
                    bbox[0], bbox[1], bbox[2], bbox[3],
                    out_total, out_n_cells, err, _VORONOI_ERR_BUF,
                ),
                err,
            )
            total_pairs = int(out_total[0])
            n_cells = int(out_n_cells[0])
            xy_buf = _ffi.new("double[]", 2 * total_pairs) if total_pairs > 0 else _ffi.new("double[]", 1)
            cell_start_buf = _ffi.new("int[]", n_cells + 1)
            out_xy_n = _ffi.new("int *")
            err = new_err_buf()
            _check(
                _lib.scs_voronoi_cells_2d(
                    _ffi.cast("double *", _ffi.from_buffer(sites_xy)), n_sites,
                    bbox[0], bbox[1], bbox[2], bbox[3],
                    xy_buf, total_pairs, out_xy_n, cell_start_buf, n_cells + 1,
                    err, _VORONOI_ERR_BUF,
                ),
                err,
            )
            xy_flat = np.frombuffer(_ffi.buffer(xy_buf, int(out_xy_n[0]) * 2 * 8), dtype=np.float64)
            cell_starts = np.frombuffer(_ffi.buffer(cell_start_buf, (n_cells + 1) * 4), dtype=np.int32)
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
    """Add voter indifference curves.

    Draws an indifference contour for each voter centred at their ideal point,
    passing through the status quo.  Under Euclidean distance (the default)
    each contour is a circle; other metrics (Manhattan, Chebyshev, Minkowski)
    produce their respective iso-distance shapes.

    Parameters
    ----------
    fig:
        Figure from :func:`plot_spatial_voting`.
    voters:
        Flat array of voter ideal points.
    sq:
        Length-2 array for the status quo.
    dist_config:
        Distance metric configuration (:class:`~socialchoicelab.DistanceConfig`).
        ``None`` (default) uses Euclidean distance and draws an efficient circle.
        Pass :func:`~socialchoicelab.make_dist_config` for other metrics.
    color_by_voter:
        ``False`` (default): all curves share a single neutral colour with
        one legend entry.  ``True``: each voter gets a unique colour from
        ``palette`` shown individually in the legend.
    fill_color:
        Explicit fill colour (overrides theme).  ``None``: no fill when
        ``color_by_voter=False``; faint per-voter colour when ``True``.
    line_color:
        Explicit uniform line colour (overrides theme for non-voter-colour mode).
    line_width:
        Stroke width of the IC contours in pixels.  Default ``1``.
    palette:
        Palette name for ``color_by_voter`` mode.  ``"auto"`` (default) uses
        the ``theme``'s palette.
    voter_names:
        Labels used in the legend when ``color_by_voter=True``.
    name:
        Legend group label (when ``color_by_voter=False``).
    theme:
        Colour theme — same options as :func:`plot_spatial_voting`.

    Returns
    -------
    plotly.graph_objects.Figure

    Examples
    --------
    >>> import socialchoicelab.plots as sclp
    >>> import numpy as np
    >>> voters = np.array([-1.0, -0.5, 0.0, 0.0, 0.8, 0.6])
    >>> sq = np.array([0.1, 0.1])
    >>> fig = sclp.plot_spatial_voting(voters, sq=sq)
    >>> fig = sclp.layer_ic(fig, voters, sq)
    """
    _require_plotly()
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

    _linear_loss = LossConfig(loss_type="linear")

    for i in range(n_v):
        if dist_config is None:
            r = math.sqrt((vx[i] - sqv[0]) ** 2 + (vy[i] - sqv[1]) ** 2)
            cx, cy = _circle_pts(float(vx[i]), float(vy[i]), r)
            hover_d = r
        else:
            d = calculate_distance([float(vx[i]), float(vy[i])], sqv.tolist(), dist_config)
            ul = distance_to_utility(d, _linear_loss)
            ls = level_set_2d(float(vx[i]), float(vy[i]), ul, _linear_loss, dist_config)
            poly = level_set_to_polygon(ls, 64)
            cx = poly[:, 0].tolist() + [float(poly[0, 0])]
            cy = poly[:, 1].tolist() + [float(poly[0, 1])]
            hover_d = d
        lname = voter_names[i] if color_by_voter else name
        lgroup = voter_names[i] if color_by_voter else name
        show_lg = True if color_by_voter else (i == 0)
        use_fill = fill_color is not None or color_by_voter
        fig = _add_role_trace(fig, go.Scatter(
            x=cx, y=cy,
            mode="lines",
            fill="toself" if use_fill else "none",
            fillcolor=fill_colors[i],
            line=dict(color=line_colors[i], width=line_width, dash="dot"),
            name=lname,
            legendgroup=lgroup,
            showlegend=show_lg,
            hovertemplate=f"{voter_names[i]} IC (d={hover_d:.3f})<extra></extra>",
        ), "region")
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
    """Add voter preferred-to regions.

    Draws a filled region for each voter centred at their ideal point bounded
    by the indifference contour through the status quo.  The interior is the
    set of policies the voter strictly prefers to the SQ.  Under Euclidean
    distance (the default) each region is a circle; other metrics produce
    their respective iso-distance shapes.

    Parameters
    ----------
    fig:
        Figure from :func:`plot_spatial_voting`.
    voters:
        Flat array of voter ideal points.
    sq:
        Length-2 array for the status quo.
    dist_config:
        Distance metric configuration (:class:`~socialchoicelab.DistanceConfig`).
        ``None`` (default) uses Euclidean distance and draws an efficient circle.
        Pass :func:`~socialchoicelab.make_dist_config` for other metrics.
    color_by_voter:
        ``False``: all regions share one neutral colour.  ``True``: each voter
        gets a unique colour from ``palette``.
    fill_color:
        Explicit fill colour (overrides theme).
    line_color:
        Explicit outline colour (overrides theme).
    palette:
        Palette name for ``color_by_voter`` mode.
    voter_names:
        Labels used in the legend when ``color_by_voter=True``.
    name:
        Legend group label (when ``color_by_voter=False``).
    theme:
        Colour theme — same options as :func:`plot_spatial_voting`.

    Returns
    -------
    plotly.graph_objects.Figure

    Examples
    --------
    >>> import socialchoicelab.plots as sclp
    >>> import numpy as np
    >>> voters = np.array([-1.0, -0.5, 0.0, 0.0, 0.8, 0.6])
    >>> sq = np.array([0.1, 0.1])
    >>> fig = sclp.plot_spatial_voting(voters, sq=sq)
    >>> fig = sclp.layer_preferred_regions(fig, voters, sq)
    """
    _require_plotly()
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

    _linear_loss = LossConfig(loss_type="linear")

    for i in range(n_v):
        if dist_config is None:
            r = math.sqrt((vx[i] - sqv[0]) ** 2 + (vy[i] - sqv[1]) ** 2)
            cx, cy = _circle_pts(float(vx[i]), float(vy[i]), r)
            hover_d = r
        else:
            d = calculate_distance([float(vx[i]), float(vy[i])], sqv.tolist(), dist_config)
            ul = distance_to_utility(d, _linear_loss)
            ls = level_set_2d(float(vx[i]), float(vy[i]), ul, _linear_loss, dist_config)
            poly = level_set_to_polygon(ls, 64)
            cx = poly[:, 0].tolist() + [float(poly[0, 0])]
            cy = poly[:, 1].tolist() + [float(poly[0, 1])]
            hover_d = d
        lname = voter_names[i] if color_by_voter else name
        lgroup = voter_names[i] if color_by_voter else name
        show_lg = True if color_by_voter else (i == 0)
        fig = _add_role_trace(fig, go.Scatter(
            x=cx, y=cy,
            mode="lines",
            fill="toself",
            fillcolor=fill_colors[i],
            line=dict(color=line_colors[i], width=1, dash="dot"),
            name=lname,
            legendgroup=lgroup,
            showlegend=show_lg,
            hovertemplate=f"{voter_names[i]} preferred region (d={hover_d:.3f})<extra></extra>",
        ), "region")
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
    """Add a winset polygon layer.

    Pass either a pre-computed :class:`~socialchoicelab.Winset` or raw
    ``voters`` + ``sq`` for auto-compute.  Empty winsets are ignored.

    Parameters
    ----------
    fig:
        Figure from :func:`plot_spatial_voting`.
    winset:
        A :class:`~socialchoicelab.Winset` object, or ``None`` for auto-compute.
    fill_color:
        Explicit fill colour (overrides theme).
    line_color:
        Explicit outline colour (overrides theme).
    name:
        Legend entry label.
    voters:
        Flat voter array (required when ``winset=None``).
    sq:
        Status quo ``[x, y]`` (required when ``winset=None``).
    dist_config:
        Distance metric configuration (:class:`~socialchoicelab.DistanceConfig`).
        ``None`` (default) uses Euclidean distance.  Only used in the
        auto-compute path (``winset=None``); ignored when a pre-computed
        ``winset`` is supplied.
    theme:
        Colour theme — same options as :func:`plot_spatial_voting`.

    Returns
    -------
    plotly.graph_objects.Figure

    Examples
    --------
    >>> import socialchoicelab as scl
    >>> import socialchoicelab.plots as sclp
    >>> import numpy as np
    >>> voters = np.array([-1.0, -0.5, 0.0, 0.0, 0.8, 0.6, -0.4, 0.8, 0.5, -0.7])
    >>> ws  = scl.winset_2d(0.0, 0.0, voters)
    >>> fig = sclp.plot_spatial_voting(voters)
    >>> fig = sclp.layer_winset(fig, ws)
    """
    _require_plotly()

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

    fill_color = fill_color if fill_color is not None else _layer_fill_color("winset", theme)
    line_color = line_color if line_color is not None else _layer_line_color("winset", theme)

    if winset.is_empty():
        return fig

    bnd = winset.boundary()
    xy = bnd["xy"]
    starts = bnd["path_starts"]
    is_hole = bnd["path_is_hole"]
    n_paths = len(starts)
    ends = list(starts[1:]) + [len(xy)]

    show_legend = True
    for i in range(n_paths):
        fc = "rgba(248,249,250,0.95)" if is_hole[i] else fill_color
        s, e = int(starts[i]), int(ends[i])
        px = list(xy[s:e, 0]) + [xy[s, 0]]
        py = list(xy[s:e, 1]) + [xy[s, 1]]
        fig = _add_role_trace(fig, go.Scatter(
            x=px, y=py,
            mode="lines",
            fill="toself",
            fillcolor=fc,
            line=dict(color=line_color, width=2, dash="solid"),
            name=name,
            showlegend=show_legend,
            legendgroup=name,
            hoverinfo="skip",
        ), "region")
        show_legend = False
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
    """Add a yolk circle layer.

    Parameters
    ----------
    fig:
        Figure from :func:`plot_spatial_voting`.
    center_x, center_y:
        Yolk center coordinates.
    radius:
        Yolk radius (same units as plot axes).
    fill_color:
        Explicit fill colour (overrides theme).
    line_color:
        Explicit outline colour (overrides theme).
    name:
        Legend entry label.
    theme:
        Colour theme — same options as :func:`plot_spatial_voting`.

    Returns
    -------
    plotly.graph_objects.Figure

    Examples
    --------
    >>> import socialchoicelab.plots as sclp
    >>> import numpy as np
    >>> voters = np.array([-1.0, -0.5, 0.0, 0.0, 0.8, 0.6])
    >>> fig = sclp.plot_spatial_voting(voters)
    >>> fig = sclp.layer_yolk(fig, center_x=0.1, center_y=0.05, radius=0.3)
    """
    _require_plotly()
    fill_color = fill_color if fill_color is not None else _layer_fill_color("yolk", theme)
    line_color = line_color if line_color is not None else _layer_line_color("yolk", theme)
    cx, cy = _circle_pts(float(center_x), float(center_y), float(radius))
    fig = _add_role_trace(fig, go.Scatter(
        x=cx, y=cy,
        mode="lines",
        fill="toself",
        fillcolor=fill_color,
        line=dict(color=line_color, width=2, dash="longdashdot"),
        name=name,
        hovertemplate=f"{name} (r={radius:.4f})<extra></extra>",
    ), "region")
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
    """Add an uncovered set boundary layer.

    Pass either a pre-computed boundary or raw ``voters`` for auto-compute.
    Empty boundaries are ignored.

    Parameters
    ----------
    fig:
        Figure from :func:`plot_spatial_voting`.
    boundary_xy:
        ``ndarray`` of shape ``(n_pts, 2)``, or ``None`` for auto-compute.
    fill_color:
        Explicit fill colour (overrides theme).
    line_color:
        Explicit outline colour (overrides theme).
    name:
        Legend entry label.
    voters:
        Flat voter array (required when ``boundary_xy=None``).
    grid_resolution:
        Grid resolution for auto-compute (default 20).
    theme:
        Colour theme — same options as :func:`plot_spatial_voting`.

    Returns
    -------
    plotly.graph_objects.Figure

    Examples
    --------
    >>> import socialchoicelab as scl
    >>> import socialchoicelab.plots as sclp
    >>> import numpy as np
    >>> voters = np.array([-1.0, -0.5, 0.0, 0.0, 0.8, 0.6, -0.4, 0.8, 0.5, -0.7])
    >>> bnd = scl.uncovered_set_boundary_2d(voters, grid_resolution=8)
    >>> fig = sclp.plot_spatial_voting(voters)
    >>> fig = sclp.layer_uncovered_set(fig, bnd)
    """
    _require_plotly()

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

    fill_color = fill_color if fill_color is not None else _layer_fill_color("uncovered_set", theme)
    line_color = line_color if line_color is not None else _layer_line_color("uncovered_set", theme)

    bxy = np.asarray(boundary_xy, dtype=float)
    if len(bxy) == 0:
        return fig
    px = list(bxy[:, 0]) + [bxy[0, 0]]
    py = list(bxy[:, 1]) + [bxy[0, 1]]
    fig = _add_role_trace(fig, go.Scatter(
        x=px, y=py,
        mode="lines",
        fill="toself",
        fillcolor=fill_color,
        line=dict(color=line_color, width=1.5, dash="longdash"),
        name=name,
        hovertemplate=f"{name}<extra></extra>",
    ), "region")
    return fig


def layer_convex_hull(
    fig,
    hull_xy,
    fill_color=None,
    line_color=None,
    name="Convex Hull",
    theme="dark2",
):
    """Add a convex hull layer.

    Parameters
    ----------
    fig:
        Figure from :func:`plot_spatial_voting`.
    hull_xy:
        ``ndarray`` of shape ``(n_hull, 2)``.  Empty arrays are ignored.
    fill_color:
        Explicit fill colour (overrides theme).
    line_color:
        Explicit outline colour (overrides theme).
    name:
        Legend entry label.
    theme:
        Colour theme — same options as :func:`plot_spatial_voting`.

    Returns
    -------
    plotly.graph_objects.Figure

    Examples
    --------
    >>> import socialchoicelab as scl
    >>> import socialchoicelab.plots as sclp
    >>> import numpy as np
    >>> voters = np.array([-1.0, -0.5, 0.0, 0.0, 0.8, 0.6, -0.4, 0.8, 0.5, -0.7])
    >>> hull = scl.convex_hull_2d(voters)
    >>> fig  = sclp.plot_spatial_voting(voters)
    >>> fig  = sclp.layer_convex_hull(fig, hull)
    """
    _require_plotly()
    fill_color = fill_color if fill_color is not None else _layer_fill_color("convex_hull", theme)
    line_color = line_color if line_color is not None else _layer_line_color("convex_hull", theme)

    hxy = np.asarray(hull_xy, dtype=float)
    if len(hxy) == 0:
        return fig
    px = list(hxy[:, 0]) + [hxy[0, 0]]
    py = list(hxy[:, 1]) + [hxy[0, 1]]
    fig = _add_role_trace(fig, go.Scatter(
        x=px, y=py,
        mode="lines",
        fill="toself",
        fillcolor=fill_color,
        line=dict(color=line_color, width=1.5, dash="dash"),
        name=name,
        hovertemplate=f"{name}<extra></extra>",
    ), "region")
    return fig


def save_plot(fig, path, width=None, height=None):
    """Save a spatial voting plot to an HTML or image file.

    HTML export uses ``fig.write_html()`` and requires no extra packages.
    Image export (``.png``, ``.svg``, ``.pdf``) uses ``fig.write_image()``
    which requires the ``kaleido`` package::

        pip install kaleido

    Parameters
    ----------
    fig:
        A Plotly figure.
    path:
        Output file path.  Extension sets format: ``.html`` (recommended),
        ``.png``, ``.svg``, ``.pdf``, ``.jpeg``, or ``.webp``.
    width, height:
        Optional pixel dimensions (override figure's own size).

    Returns
    -------
    str
        The output path.

    Examples
    --------
    >>> import socialchoicelab.plots as sclp
    >>> import numpy as np
    >>> voters = np.array([-1.0, -0.5, 0.0, 0.0, 0.8, 0.6])
    >>> fig = sclp.plot_spatial_voting(voters, sq=np.array([0.0, 0.0]))
    >>> _ = sclp.save_plot(fig, "/tmp/my_plot.html")
    """
    _require_plotly()
    ext = Path(path).suffix.lower()
    if width is not None or height is not None:
        kw = {k: v for k, v in [("width", width), ("height", height)] if v is not None}
        fig = fig.update_layout(**kw)

    if ext == ".html":
        fig.write_html(str(path))
    elif ext in {".png", ".svg", ".pdf", ".jpeg", ".jpg", ".webp"}:
        try:
            fig.write_image(str(path))
        except Exception as exc:
            raise RuntimeError(
                f"save_plot: failed to write image: {exc}\n"
                "Image export requires the kaleido package. "
                "Install it with: pip install kaleido"
            ) from exc
    else:
        raise ValueError(
            f"save_plot: unsupported file extension '{ext}'. "
            "Supported formats: .html, .png, .svg, .pdf, .jpeg, .webp."
        )
    return str(path)


def finalize_plot(fig):
    """Enforce the canonical layer stack order.

    Parameters
    ----------
    fig:
        A Plotly figure.

    Returns
    -------
    plotly.graph_objects.Figure

    Examples
    --------
    >>> import socialchoicelab.plots as sclp
    >>> import numpy as np
    >>> voters = np.array([-1.0, -0.5, 0.0, 0.0, 0.8, 0.6])
    >>> fig = sclp.plot_spatial_voting(voters, sq=np.array([0.0, 0.0]))
    >>> fig = sclp.finalize_plot(fig)
    """
    return _normalize_trace_roles(fig)


def layer_centroid(fig, voters, color=None, name="Centroid", theme="dark2"):
    """Add a centroid (mean voter position) marker layer.

    Displays the coordinate-wise arithmetic mean of voter ideal points as a
    labelled cross (+) marker, matching the competition-canvas overlay style.

    Parameters
    ----------
    fig:
        Plotly figure from :func:`plot_spatial_voting`.
    voters:
        Flat ``[x0, y0, …]`` voter ideal points.
    color:
        Marker colour string (CSS rgba).  ``None`` uses the canvas-matched
        crimson (or grayscale when ``theme="bw"``).
    name:
        Legend entry label.
    theme:
        Colour theme — see :func:`plot_spatial_voting`.

    Returns
    -------
    plotly.graph_objects.Figure

    Examples
    --------
    >>> import numpy as np
    >>> import socialchoicelab.plots as sclp
    >>> voters = np.array([-1.0, -0.5, 0.0, 0.0, 0.8, 0.6])
    >>> fig = sclp.plot_spatial_voting(voters)
    >>> fig = sclp.layer_centroid(fig, voters)
    """
    if not _PLOTLY_AVAILABLE:
        raise ImportError("plotly is required: pip install plotly")
    color = color or _centroid_overlay_color(theme)
    x, y = centroid_2d(voters)
    fig = _add_role_trace(
        fig,
        go.Scatter(
            x=[x], y=[y],
            mode="markers+text",
            marker=dict(symbol="cross", size=11, color=color,
                        line=dict(color=color, width=2)),
            text=[name],
            textposition="top center",
            name=name,
            hovertemplate=f"{name} ({x:.4f}, {y:.4f})<extra></extra>",
        )
        ,
        "point",
    )
    return fig


def layer_marginal_median(fig, voters, color=None, name="Marginal Median",
                          theme="dark2"):
    """Add a marginal median marker layer.

    Displays the coordinate-wise median of voter ideal points as a labelled
    upward-pointing filled triangle, matching the competition-canvas overlay
    (issue-by-issue median voter; Black 1948).

    Parameters
    ----------
    fig:
        Plotly figure from :func:`plot_spatial_voting`.
    voters:
        Flat ``[x0, y0, …]`` voter ideal points.
    color:
        Marker fill colour (CSS rgba).  ``None`` uses the canvas-matched
        indigo-violet (or grayscale when ``theme="bw"``).  Outline always uses
        a light stroke for contrast, as on the canvas.
    name:
        Legend entry label.
    theme:
        Colour theme — see :func:`plot_spatial_voting`.

    Returns
    -------
    plotly.graph_objects.Figure

    Examples
    --------
    >>> import numpy as np
    >>> import socialchoicelab.plots as sclp
    >>> voters = np.array([-1.0, -0.5, 0.0, 0.0, 0.8, 0.6])
    >>> fig = sclp.plot_spatial_voting(voters)
    >>> fig = sclp.layer_marginal_median(fig, voters)
    """
    if not _PLOTLY_AVAILABLE:
        raise ImportError("plotly is required: pip install plotly")
    fill_c = color or _marginal_median_overlay_color(theme)
    stroke_c = _overlay_triangle_outline_color(theme)
    x, y = marginal_median_2d(voters)
    fig = _add_role_trace(
        fig,
        go.Scatter(
            x=[x], y=[y],
            mode="markers+text",
            marker=dict(symbol="triangle-up", size=14, color=fill_c,
                        line=dict(color=stroke_c, width=2)),
            text=[name],
            textposition="top center",
            name=name,
            hovertemplate=f"{name} ({x:.4f}, {y:.4f})<extra></extra>",
        )
        ,
        "point",
    )
    return fig
