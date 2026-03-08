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
        xlim = _padded_range(np.array(all_x))
    if ylim is None and len(all_y) > 0:
        ylim = _padded_range(np.array(all_y))

    fig = go.Figure()

    fig.add_trace(go.Scatter(
        x=vx, y=vy,
        mode="markers",
        name="Voters",
        marker=dict(symbol="circle", size=12, color=voter_col,
                    line=dict(color="white", width=1.5)),
        text=voter_names,
        hovertemplate="%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>",
        zorder=8,
    ))

    if alternatives is not None:
        ax, ay = _flat_to_xy(np.asarray(alternatives, dtype=float).ravel())
        n_a = len(ax)
        if alt_names is None:
            alt_names = [f"Alt {i + 1}" for i in range(n_a)]
        all_x.extend(list(ax))
        all_y.extend(list(ay))
        fig.add_trace(go.Scatter(
            x=ax, y=ay,
            mode="markers+text" if show_labels else "markers",
            name="Alternatives",
            marker=dict(symbol="diamond", size=14, color=alt_col,
                        line=dict(color="white", width=1.5)),
            text=alt_names,
            textposition="top center",
            hovertemplate="%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>",
            zorder=9,
        ))

    if sq is not None:
        fig.add_trace(go.Scatter(
            x=[sqv[0]], y=[sqv[1]],
            mode="markers+text" if show_labels else "markers",
            name="Status Quo",
            marker=dict(symbol="star", size=18, color=sq_col,
                        line=dict(color="white", width=1.5)),
            text=["SQ"],
            textposition="top center",
            hovertemplate="Status Quo<br>(%{x:.3f}, %{y:.3f})<extra></extra>",
            zorder=9,
        ))

    fig.update_layout(
        title=dict(text=title, x=0.05),
        xaxis=dict(title=dim_names[0], zeroline=True,
                   zerolinecolor="#cccccc", zerolinewidth=1, range=xlim),
        yaxis=dict(title=dim_names[1], zeroline=True,
                   zerolinecolor="#cccccc", zerolinewidth=1,
                   scaleanchor="x", scaleratio=1, range=ylim),
        legend=dict(x=1.02, y=1, xanchor="left"),
        hovermode="closest",
        plot_bgcolor="#f8f9fa",
        paper_bgcolor="white",
        width=width,
        height=height,
    )
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
        fig.add_trace(go.Scatter(
            x=cx, y=cy,
            mode="lines",
            fill="toself" if use_fill else "none",
            fillcolor=fill_colors[i],
            line=dict(color=line_colors[i], width=1, dash="dot"),
            name=lname,
            legendgroup=lgroup,
            showlegend=show_lg,
            hovertemplate=f"{voter_names[i]} IC (r={r:.3f})<extra></extra>",
            zorder=1,
        ))
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
        fig.add_trace(go.Scatter(
            x=cx, y=cy,
            mode="lines",
            fill="toself",
            fillcolor=fill_colors[i],
            line=dict(color=line_colors[i], width=1, dash="dot"),
            name=lname,
            legendgroup=lgroup,
            showlegend=show_lg,
            hovertemplate=f"{voter_names[i]} preferred region (r={r:.3f})<extra></extra>",
            zorder=1,
        ))
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
        fig.add_trace(go.Scatter(
            x=px, y=py,
            mode="lines",
            fill="toself",
            fillcolor=fc,
            line=dict(color=line_color, width=2, dash="solid"),
            name=name,
            showlegend=show_legend,
            legendgroup=name,
            hoverinfo="skip",
            zorder=6,
        ))
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
    fig.add_trace(go.Scatter(
        x=cx, y=cy,
        mode="lines",
        fill="toself",
        fillcolor=fill_color,
        line=dict(color=line_color, width=2, dash="longdashdot"),
        name=name,
        hovertemplate=f"{name} (r={radius:.4f})<extra></extra>",
        zorder=5,
    ))
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
    fig.add_trace(go.Scatter(
        x=px, y=py,
        mode="lines",
        fill="toself",
        fillcolor=fill_color,
        line=dict(color=line_color, width=1.5, dash="longdash"),
        name=name,
        hovertemplate=f"{name}<extra></extra>",
        zorder=4,
    ))
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
    fig.add_trace(go.Scatter(
        x=px, y=py,
        mode="lines",
        fill="toself",
        fillcolor=fill_color,
        line=dict(color=line_color, width=1.5, dash="dash"),
        name=name,
        hovertemplate=f"{name}<extra></extra>",
        zorder=2,
    ))
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
    """Enforce the canonical layer stack order (no-op; kept for API compatibility).

    Layer ordering is handled natively via the ``zorder`` attribute on each trace.

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
    return fig


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
    fig.add_trace(
        go.Scatter(
            x=[x], y=[y],
            mode="markers+text",
            marker=dict(symbol="diamond", size=12, color=color,
                        line=dict(color=color, width=2)),
            text=[name],
            textposition="top center",
            name=name,
            hovertemplate=f"{name} ({x:.4f}, {y:.4f})<extra></extra>",
            zorder=8,
        )
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
    fig.add_trace(
        go.Scatter(
            x=[x], y=[y],
            mode="markers+text",
            marker=dict(symbol="cross", size=14, color=color,
                        line=dict(color=color, width=2)),
            text=[name],
            textposition="top center",
            name=name,
            hovertemplate=f"{name} ({x:.4f}, {y:.4f})<extra></extra>",
            zorder=8,
        )
    )
    return fig
