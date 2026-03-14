"""Palette and theme utilities for spatial voting visualization (C13.4).

Provides a small set of curated, colorblind-safe colour palettes drawn from
ColorBrewer and the Okabe-Ito palette, plus a theme system that maps each
layer type to a coordinated colour automatically.

Usage::

    import socialchoicelab.plots as sclp

    # Get 5 coordinated RGBA strings for custom traces
    colors = sclp.scl_palette("dark2", n=5)

    # Find out what colours layer_winset will use
    fill, line = sclp.scl_theme_colors("winset", theme="dark2")

    # Plot everything with a coordinated palette
    fig = sclp.plot_spatial_voting(voters, sq=sq, theme="dark2")
    fig = sclp.layer_winset(fig, ws, theme="dark2")
"""

from __future__ import annotations

__all__ = ["scl_palette", "scl_theme_colors"]

# ---------------------------------------------------------------------------
# Raw hex palettes
# ---------------------------------------------------------------------------

#: Approved qualitative palettes.  All entries are colorblind-safe or
#: near-colorblind-safe.  Source: ColorBrewer 2.0 (Cynthia Brewer) and
#: Okabe & Ito (2008).
_PALETTE_HEX: dict[str, list[str]] = {
    # ColorBrewer Dark2 — 8 colours, robust colorblind safety, high contrast.
    "dark2": [
        "#1B9E77",  # teal
        "#D95F02",  # orange
        "#7570B3",  # purple
        "#E7298A",  # magenta
        "#66A61E",  # lime green
        "#E6AB02",  # gold
        "#A6761D",  # brown
        "#666666",  # grey
    ],
    # ColorBrewer Set2 — 8 colours, softer/pastel version of Dark2.
    "set2": [
        "#66C2A5",  # mint
        "#FC8D62",  # salmon
        "#8DA0CB",  # periwinkle
        "#E78AC3",  # pink
        "#A6D854",  # yellow-green
        "#FFD92F",  # yellow
        "#E5C494",  # tan
        "#B3B3B3",  # light grey
    ],
    # Okabe-Ito — 7 colours, print-safe, distinguishable in B&W.
    "okabe_ito": [
        "#0072B2",  # blue
        "#D55E00",  # vermilion
        "#009E73",  # bluish green
        "#E69F00",  # orange
        "#56B4E9",  # sky blue
        "#CC79A7",  # reddish purple
        "#000000",  # black
    ],
    # ColorBrewer Paired — 12 colours; useful when n > 8.
    "paired": [
        "#A6CEE3",  # light blue
        "#1F78B4",  # blue
        "#B2DF8A",  # light green
        "#33A02C",  # green
        "#FB9A99",  # light red
        "#E31A1C",  # red
        "#FDBF6F",  # light orange
        "#FF7F00",  # orange
        "#CAB2D6",  # light purple
        "#6A3D9A",  # purple
        "#FFFF99",  # light yellow
        "#B15928",  # brown
    ],
}

# ---------------------------------------------------------------------------
# Layer-type metadata
# ---------------------------------------------------------------------------

# Each layer type is assigned a fixed palette slot index (0-based).
# Region layers use slots 0-3; point layers use slots 4-5.
# This assignment is identical across all colour palettes so that switching
# themes keeps the same semantic mapping.
_LAYER_SLOTS: dict[str, int] = {
    "winset":        0,
    "yolk":          1,
    "uncovered_set": 2,
    "convex_hull":   3,
    "voters":        4,
    "alternatives":  5,
}

# (fill_alpha, line_alpha) — opacity increases with z-order depth so that
# higher layers read as more visually prominent.
_LAYER_ALPHAS: dict[str, tuple[float, float]] = {
    "winset":        (0.28, 0.80),
    "yolk":          (0.22, 0.70),
    "uncovered_set": (0.18, 0.65),
    "convex_hull":   (0.12, 0.55),
    "voters":        (0.85, 1.00),
    "alternatives":  (0.90, 1.00),
}

# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

def _hex_to_rgb(h: str) -> tuple[int, int, int]:
    h = h.lstrip("#")
    return int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16)


def _rgba(hex_color: str, alpha: float) -> str:
    r, g, b = _hex_to_rgb(hex_color)
    return f"rgba({r},{g},{b},{alpha:.3g})"


# Green-free candidate colour sequence for 1D competition canvases.
# Voter dots are lime green (#66A61E); overlay centroid is crimson; overlay
# marginal median is indigo-violet.  All three are excluded from this list.
# Blue (#0072B2) is demoted to slot 4 so the most common 2- or 3-candidate
# demos never put a blue candidate next to the indigo-violet median overlay.
_COMPETITION_1D_HEX: list[str] = [
    "#D55E00",  # vermilion
    "#CC79A7",  # reddish purple / mauve
    "#E69F00",  # amber / orange
    "#56B4E9",  # sky blue  (clearly lighter than indigo-violet overlay)
    "#F0E442",  # yellow
    "#000000",  # black
    "#A6761D",  # brown
]


def _candidate_colors_1d(n: int, alpha: float = 0.95) -> list[str]:
    """Return n green-free RGBA colour strings for 1D competition candidates."""
    hex_list = _COMPETITION_1D_HEX
    return [_rgba(hex_list[i % len(hex_list)], alpha) for i in range(n)]


def _best_palette_for_n(n: int) -> str:
    if n <= 7:
        return "okabe_ito"
    if n <= 8:
        return "dark2"
    return "paired"


def _resolve_palette(name: str, n: int) -> list[str]:
    resolved = _best_palette_for_n(n) if name == "auto" else name
    pal = _PALETTE_HEX.get(resolved)
    if pal is None:
        available = ", ".join(sorted(_PALETTE_HEX))
        raise ValueError(
            f"scl_palette: unknown palette '{name}'. "
            f"Available palettes: {available}."
        )
    return pal


def _layer_fill_color(layer_type: str, theme: str) -> str:
    """Return the fill RGBA string for a layer type in a named theme."""
    fill_a, _ = _LAYER_ALPHAS.get(layer_type, (0.18, 0.65))
    if theme == "bw":
        return f"rgba(200,200,200,{fill_a:.3g})"
    pal = _resolve_palette(theme, 8)
    slot = _LAYER_SLOTS.get(layer_type, 0)
    return _rgba(pal[slot % len(pal)], fill_a)


def _layer_line_color(layer_type: str, theme: str) -> str:
    """Return the line RGBA string for a layer type in a named theme."""
    _, line_a = _LAYER_ALPHAS.get(layer_type, (0.18, 0.65))
    if theme == "bw":
        return f"rgba(30,30,30,{line_a:.3g})"
    pal = _resolve_palette(theme, 8)
    slot = _LAYER_SLOTS.get(layer_type, 0)
    return _rgba(pal[slot % len(pal)], line_a)


def _voter_point_color(theme: str) -> str:
    if theme == "bw":
        return "rgba(30,30,30,0.85)"
    pal = _resolve_palette(theme, 8)
    slot = _LAYER_SLOTS["voters"]
    return _rgba(pal[slot % len(pal)], 0.85)


def _alt_point_color(theme: str) -> str:
    if theme == "bw":
        return "rgba(60,60,60,0.90)"
    pal = _resolve_palette(theme, 8)
    slot = _LAYER_SLOTS["alternatives"]
    return _rgba(pal[slot % len(pal)], 0.90)


def _ic_uniform_line(theme: str) -> str:
    """Neutral line colour for indifference curves (uniform / non-voter colour mode)."""
    if theme == "bw":
        return "rgba(80,80,80,0.40)"
    return "rgba(120,120,160,0.40)"


def _preferred_uniform_fill(theme: str) -> str:
    if theme == "bw":
        return "rgba(160,160,160,0.08)"
    return "rgba(120,120,160,0.08)"


def _preferred_uniform_line(theme: str) -> str:
    if theme == "bw":
        return "rgba(80,80,80,0.28)"
    return "rgba(120,120,160,0.28)"


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def scl_palette(name: str = "auto", n: int = 8, alpha: float = 1.0) -> list[str]:
    """Return *n* RGBA colour strings from a named palette.

    A convenience utility for retrieving coordinated colours from the same
    palettes used by the ``layer_*`` plotting functions.  Useful when adding
    custom Plotly traces that should remain visually consistent with the rest
    of the plot.

    Parameters
    ----------
    name:
        Palette name: ``"dark2"`` (ColorBrewer Dark2, 8 colours, colorblind-safe),
        ``"set2"`` (ColorBrewer Set2, 8 colours, softer), ``"okabe_ito"``
        (Okabe-Ito, 7 colours, print-safe), ``"paired"`` (ColorBrewer Paired,
        12 colours), or ``"auto"`` (selects the best palette for *n*:
        Okabe-Ito for n ≤ 7, Dark2 for n ≤ 8, Paired for n > 8).
    n:
        Number of colours to return.  If *n* exceeds the palette size, colours
        are cycled.
    alpha:
        Opacity applied uniformly to all returned colours (0–1).

    Returns
    -------
    list[str]
        RGBA strings (e.g. ``"rgba(27,158,119,1)"``).

    Examples
    --------
    >>> colors = scl_palette("dark2", n=4)
    >>> len(colors)
    4
    >>> colors = scl_palette("auto", n=5, alpha=0.7)
    >>> len(colors)
    5
    """
    pal = _resolve_palette(name, n)
    return [_rgba(pal[i % len(pal)], alpha) for i in range(n)]


def scl_theme_colors(layer_type: str, theme: str = "dark2") -> tuple[str, str]:
    """Return the ``(fill_color, line_color)`` RGBA pair for a layer type in a theme.

    Useful when adding custom Plotly traces that should match the colours
    used by the built-in ``layer_*`` functions.

    Parameters
    ----------
    layer_type:
        One of ``"winset"``, ``"yolk"``, ``"uncovered_set"``,
        ``"convex_hull"``, ``"voters"``, ``"alternatives"``.
    theme:
        Theme name — same options as the ``theme`` parameter of
        :func:`~socialchoicelab.plots.plot_spatial_voting`:
        ``"dark2"`` (default), ``"set2"``, ``"okabe_ito"``, ``"paired"``,
        ``"bw"`` (black-and-white print).

    Returns
    -------
    tuple[str, str]
        ``(fill_rgba, line_rgba)`` strings.

    Raises
    ------
    ValueError
        If *theme* is not a recognised theme name.

    Examples
    --------
    >>> fill, line = scl_theme_colors("winset", theme="dark2")
    >>> fill
    'rgba(27,158,119,0.28)'
    """
    valid_themes = set(_PALETTE_HEX) | {"bw"}
    if theme not in valid_themes:
        raise ValueError(
            f"scl_theme_colors: unknown theme '{theme}'. "
            f"Available: {', '.join(sorted(valid_themes))}."
        )
    return _layer_fill_color(layer_type, theme), _layer_line_color(layer_type, theme)
