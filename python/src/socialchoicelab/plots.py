"""Spatial voting visualization helpers (C10).

All functions return :class:`plotly.graph_objects.Figure` objects so they
can be displayed in Jupyter, saved to HTML, or embedded in reports.

Plotly is an optional dependency.  If it is not installed, importing
this module raises :class:`ImportError` with install instructions.

Layers compose by passing the figure returned by one function into the next::

    import socialchoicelab as scl
    import socialchoicelab.plots as sclp
    import numpy as np

    voters = np.array([-1.0, -0.5,  0.0, 0.0,  0.8, 0.6,
                       -0.4,  0.8,  0.5, -0.7])
    alts   = np.array([0.0,  0.0,  0.6, 0.4, -0.5, 0.3])
    sq     = np.array([0.0,  0.0])

    ws  = scl.winset_2d(sq[0], sq[1], voters)
    bnd = scl.uncovered_set_boundary_2d(voters)

    fig = sclp.plot_spatial_voting(voters, alts, sq=sq)
    fig = sclp.layer_winset(fig, ws)
    fig = sclp.layer_uncovered_set(fig, bnd)
    fig.show()
"""

from __future__ import annotations

import math

import numpy as np

try:
    import plotly.graph_objects as go
    _PLOTLY_AVAILABLE = True
except ImportError:
    _PLOTLY_AVAILABLE = False

__all__ = [
    "plot_spatial_voting",
    "layer_winset",
    "layer_yolk",
    "layer_uncovered_set",
    "layer_convex_hull",
]


def _require_plotly() -> None:
    if not _PLOTLY_AVAILABLE:
        raise ImportError(
            "plotly is required for visualization. "
            "Install it with: pip install plotly"
        )


def _flat_to_xy(v) -> tuple[np.ndarray, np.ndarray]:
    """Reshape flat [x0, y0, x1, y1, ...] to (xs, ys)."""
    arr = np.asarray(v, dtype=float).ravel()
    return arr[0::2], arr[1::2]


def _circle_pts(cx: float, cy: float, r: float, n: int = 64) -> tuple[np.ndarray, np.ndarray]:
    """Generate (n+1) points along a circle (closed path)."""
    theta = np.linspace(0.0, 2.0 * math.pi, n + 1)
    return cx + r * np.cos(theta), cy + r * np.sin(theta)


def plot_spatial_voting(
    voters,
    alternatives,
    sq=None,
    voter_names=None,
    alt_names=None,
    dim_names=("Dimension 1", "Dimension 2"),
    title="Spatial Voting Analysis",
    width=700,
    height=600,
):
    """Create a base 2D spatial voting plot.

    Parameters
    ----------
    voters:
        Flat array ``[x0, y0, x1, y1, …]`` of voter ideal points.
    alternatives:
        Flat array ``[x0, y0, …]`` of policy alternative coordinates.
    sq:
        Length-2 array ``[x, y]`` for the status quo, or ``None``.
    voter_names:
        List of voter labels.  ``None`` uses ``["V1", "V2", …]``.
    alt_names:
        List of alternative labels.  ``None`` uses ``["Alt 1", "Alt 2", …]``.
    dim_names:
        Pair of axis title strings.
    title:
        Plot title.
    width, height:
        Figure dimensions in pixels.

    Returns
    -------
    plotly.graph_objects.Figure
        Interactive Plotly figure.

    Examples
    --------
    >>> import socialchoicelab.plots as sclp
    >>> import numpy as np
    >>> voters = np.array([-1.0, -0.5, 0.0, 0.0, 0.8, 0.6])
    >>> alts   = np.array([0.0, 0.0, 0.6, 0.4])
    >>> fig = sclp.plot_spatial_voting(voters, alts, sq=np.array([0.0, 0.0]))
    """
    _require_plotly()
    vx, vy = _flat_to_xy(voters)
    ax, ay = _flat_to_xy(alternatives)
    n_v = len(vx)
    n_a = len(ax)
    if voter_names is None:
        voter_names = [f"V{i + 1}" for i in range(n_v)]
    if alt_names is None:
        alt_names = [f"Alt {i + 1}" for i in range(n_a)]

    fig = go.Figure()

    fig.add_trace(go.Scatter(
        x=vx, y=vy,
        mode="markers",
        name="Voters",
        marker=dict(
            symbol="circle", size=12,
            color="rgba(60,120,210,0.85)",
            line=dict(color="white", width=1.5),
        ),
        text=voter_names,
        hovertemplate="%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>",
    ))

    fig.add_trace(go.Scatter(
        x=ax, y=ay,
        mode="markers+text",
        name="Alternatives",
        marker=dict(
            symbol="diamond", size=14,
            color="rgba(210,70,30,0.9)",
            line=dict(color="white", width=1.5),
        ),
        text=alt_names,
        textposition="top center",
        hovertemplate="%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>",
    ))

    if sq is not None:
        sqv = np.asarray(sq, dtype=float).ravel()
        fig.add_trace(go.Scatter(
            x=[sqv[0]], y=[sqv[1]],
            mode="markers+text",
            name="Status Quo",
            marker=dict(
                symbol="star", size=18,
                color="rgba(20,20,20,0.9)",
                line=dict(color="white", width=1.5),
            ),
            text=["SQ"],
            textposition="top center",
            hovertemplate="Status Quo<br>(%{x:.3f}, %{y:.3f})<extra></extra>",
        ))

    fig.update_layout(
        title=dict(text=title, x=0.05),
        xaxis=dict(
            title=dim_names[0], zeroline=True,
            zerolinecolor="#cccccc", zerolinewidth=1,
        ),
        yaxis=dict(
            title=dim_names[1], zeroline=True,
            zerolinecolor="#cccccc", zerolinewidth=1,
            scaleanchor="x", scaleratio=1,
        ),
        legend=dict(x=1.02, y=1, xanchor="left"),
        hovermode="closest",
        plot_bgcolor="#f8f9fa",
        paper_bgcolor="white",
        width=width,
        height=height,
    )
    return fig


def layer_winset(
    fig,
    winset,
    fill_color="rgba(100,150,255,0.25)",
    line_color="rgba(50,100,220,0.8)",
    name="Winset",
):
    """Add a winset polygon layer.

    Overlays the boundary of a :class:`~socialchoicelab.Winset` object.
    If the winset is empty the figure is returned unchanged.

    Parameters
    ----------
    fig:
        Figure from :func:`plot_spatial_voting`.
    winset:
        A :class:`~socialchoicelab.Winset` object.
    fill_color:
        Fill colour (CSS rgba string).  Default: light blue.
    line_color:
        Outline colour (CSS rgba string).  Default: medium blue.
    name:
        Legend entry label.

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
    >>> fig = sclp.plot_spatial_voting(voters, np.array([0.0, 0.0]))
    >>> fig = sclp.layer_winset(fig, ws)
    """
    _require_plotly()
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
        fig.add_trace(go.Scatter(
            x=px, y=py,
            mode="lines",
            fill="toself",
            fillcolor=fc,
            line=dict(color=line_color, width=1.5),
            name=name,
            showlegend=show_legend,
            legendgroup=name,
            hoverinfo="skip",
        ))
        show_legend = False
    return fig


def layer_yolk(
    fig,
    center_x,
    center_y,
    radius,
    color="rgba(210,50,50,0.55)",
    name="Yolk",
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
    color:
        Fill/outline colour (CSS rgba string).  Default: semi-transparent red.
    name:
        Legend entry label.

    Returns
    -------
    plotly.graph_objects.Figure

    Examples
    --------
    >>> import socialchoicelab.plots as sclp
    >>> import numpy as np
    >>> voters = np.array([-1.0, -0.5, 0.0, 0.0, 0.8, 0.6])
    >>> fig = sclp.plot_spatial_voting(voters, np.array([0.0, 0.0]))
    >>> fig = sclp.layer_yolk(fig, center_x=0.1, center_y=0.05, radius=0.3)
    """
    _require_plotly()
    cx, cy = _circle_pts(float(center_x), float(center_y), float(radius))
    fig.add_trace(go.Scatter(
        x=cx, y=cy,
        mode="lines",
        fill="toself",
        fillcolor=color,
        line=dict(color=color, width=2),
        name=name,
        hovertemplate=f"{name} (r={radius:.4f})<extra></extra>",
    ))
    return fig


def layer_uncovered_set(
    fig,
    boundary_xy,
    fill_color="rgba(50,180,80,0.20)",
    line_color="rgba(30,150,60,0.85)",
    name="Uncovered Set",
):
    """Add an uncovered set boundary layer.

    Parameters
    ----------
    fig:
        Figure from :func:`plot_spatial_voting`.
    boundary_xy:
        ``ndarray`` of shape ``(n_pts, 2)`` from
        :func:`~socialchoicelab.uncovered_set_boundary_2d`.
        If empty, the figure is returned unchanged.
    fill_color:
        Fill colour.  Default: semi-transparent green.
    line_color:
        Outline colour.  Default: medium green.
    name:
        Legend entry label.

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
    >>> fig = sclp.plot_spatial_voting(voters, np.array([0.0, 0.0]))
    >>> fig = sclp.layer_uncovered_set(fig, bnd)
    """
    _require_plotly()
    bxy = np.asarray(boundary_xy, dtype=float)
    if len(bxy) == 0:
        return fig
    px = list(bxy[:, 0]) + [bxy[0, 0]]
    py = list(bxy[:, 1]) + [bxy[0, 1]]
    fig.add_trace(go.Scatter(
        x=px, y=py,
        mode="lines",
        fill="toself",
        fillcolor=fill_color,
        line=dict(color=line_color, width=1.5),
        name=name,
        hovertemplate=f"{name}<extra></extra>",
    ))
    return fig


def layer_convex_hull(
    fig,
    hull_xy,
    color="rgba(130,80,190,0.75)",
    name="Convex Hull",
):
    """Add a convex hull layer.

    Parameters
    ----------
    fig:
        Figure from :func:`plot_spatial_voting`.
    hull_xy:
        ``ndarray`` of shape ``(n_hull, 2)`` from
        :func:`~socialchoicelab.convex_hull_2d`.
        If empty, the figure is returned unchanged.
    color:
        Outline colour.  Default: semi-transparent purple.
    name:
        Legend entry label.

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
    >>> fig = sclp.plot_spatial_voting(voters, np.array([0.0, 0.0]))
    >>> fig = sclp.layer_convex_hull(fig, hull)
    """
    _require_plotly()
    hxy = np.asarray(hull_xy, dtype=float)
    if len(hxy) == 0:
        return fig
    px = list(hxy[:, 0]) + [hxy[0, 0]]
    py = list(hxy[:, 1]) + [hxy[0, 1]]
    fig.add_trace(go.Scatter(
        x=px, y=py,
        mode="lines",
        fill="toself",
        fillcolor="rgba(130,80,190,0.12)",
        line=dict(color=color, width=2),
        name=name,
        hovertemplate=f"{name}<extra></extra>",
    ))
    return fig
