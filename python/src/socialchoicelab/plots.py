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

import math
from pathlib import Path

import numpy as np

from socialchoicelab._geometry import centroid_2d, marginal_median_2d
from socialchoicelab.palette import (
    scl_palette,
    scl_theme_colors,
    _layer_fill_color,
    _layer_line_color,
    _voter_point_color,
    _alt_point_color,
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
            mode="markers+text" if show_labels else "markers",
            name="Status Quo",
            marker=dict(symbol="star", size=18, color=sq_col,
                        line=dict(color="white", width=1.5)),
            text=["SQ"] if show_labels else None,
            textposition="top center" if show_labels else None,
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
    trail="none",
    trail_length="medium",
):
    """Animate 2D competition trajectories from a CompetitionTrace."""
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


def layer_ic(
    fig,
    voters,
    sq,
    color_by_voter=False,
    fill_color=None,
    line_color=None,
    palette="auto",
    voter_names=None,
    name="Indifference Curves",
    theme="dark2",
):
    """Add voter indifference curves.

    Draws a circle for each voter centred at their ideal point with radius
    equal to the Euclidean distance to the status quo.

    Parameters
    ----------
    fig:
        Figure from :func:`plot_spatial_voting`.
    voters:
        Flat array of voter ideal points.
    sq:
        Length-2 array for the status quo.
    color_by_voter:
        ``False`` (default): all curves share a single neutral colour with
        one legend entry.  ``True``: each voter gets a unique colour from
        ``palette`` shown individually in the legend.
    fill_color:
        Explicit fill colour (overrides theme).  ``None``: no fill when
        ``color_by_voter=False``; faint per-voter colour when ``True``.
    line_color:
        Explicit uniform line colour (overrides theme for non-voter-colour mode).
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

    for i in range(n_v):
        r = math.sqrt((vx[i] - sqv[0]) ** 2 + (vy[i] - sqv[1]) ** 2)
        cx, cy = _circle_pts(float(vx[i]), float(vy[i]), r)
        lname = voter_names[i] if color_by_voter else name
        lgroup = voter_names[i] if color_by_voter else name
        show_lg = True if color_by_voter else (i == 0)
        use_fill = fill_color is not None or color_by_voter
        fig = _add_role_trace(fig, go.Scatter(
            x=cx, y=cy,
            mode="lines",
            fill="toself" if use_fill else "none",
            fillcolor=fill_colors[i],
            line=dict(color=line_colors[i], width=1, dash="dot"),
            name=lname,
            legendgroup=lgroup,
            showlegend=show_lg,
            hovertemplate=f"{voter_names[i]} IC (r={r:.3f})<extra></extra>",
        ), "region")
    return fig


def layer_preferred_regions(
    fig,
    voters,
    sq,
    color_by_voter=False,
    fill_color=None,
    line_color=None,
    palette="auto",
    voter_names=None,
    name="Preferred Region",
    theme="dark2",
):
    """Add voter preferred-to regions.

    Draws a filled circle for each voter centred at their ideal point with
    radius equal to the Euclidean distance to the status quo.  The interior
    is the set of policies the voter strictly prefers to the SQ.

    Parameters
    ----------
    fig:
        Figure from :func:`plot_spatial_voting`.
    voters:
        Flat array of voter ideal points.
    sq:
        Length-2 array for the status quo.
    color_by_voter:
        ``False``: all circles share one neutral colour.  ``True``: each voter
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

    for i in range(n_v):
        r = math.sqrt((vx[i] - sqv[0]) ** 2 + (vy[i] - sqv[1]) ** 2)
        cx, cy = _circle_pts(float(vx[i]), float(vy[i]), r)
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
            hovertemplate=f"{voter_names[i]} preferred region (r={r:.3f})<extra></extra>",
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
                               np.asarray(voters, dtype=float))

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
    labelled diamond marker.

    Parameters
    ----------
    fig:
        Plotly figure from :func:`plot_spatial_voting`.
    voters:
        Flat ``[x0, y0, …]`` voter ideal points.
    color:
        Marker colour string (CSS rgba).  ``None`` uses the theme default.
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
    color = color or _alt_point_color(theme)
    x, y = centroid_2d(voters)
    fig = _add_role_trace(
        fig,
        go.Scatter(
            x=[x], y=[y],
            mode="markers+text",
            marker=dict(symbol="diamond", size=12, color=color,
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
    cross marker (issue-by-issue median voter; Black 1948).

    Parameters
    ----------
    fig:
        Plotly figure from :func:`plot_spatial_voting`.
    voters:
        Flat ``[x0, y0, …]`` voter ideal points.
    color:
        Marker colour string (CSS rgba).  ``None`` uses the theme default.
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
    color = color or _alt_point_color(theme)
    x, y = marginal_median_2d(voters)
    fig = _add_role_trace(
        fig,
        go.Scatter(
            x=[x], y=[y],
            mode="markers+text",
            marker=dict(symbol="cross", size=14, color=color,
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
