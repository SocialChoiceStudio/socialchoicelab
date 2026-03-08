# test_palette.py — C13.4: Unit tests for palette and theme utilities.

import re

import numpy as np
import pytest

import socialchoicelab as scl
import socialchoicelab.plots as sclp
from socialchoicelab.palette import scl_palette, scl_theme_colors

plotly = pytest.importorskip("plotly")
import plotly.graph_objects as go  # noqa: E402

VOTERS = np.array([-1.0, -0.5,  0.0,  0.0,  0.8,  0.6, -0.4,  0.8,  0.5, -0.7])
SQ     = np.array([ 0.0,  0.0])

_RGBA_RE = re.compile(r"^rgba\(\d+,\d+,\d+,[0-9.]+\)$")

# ---------------------------------------------------------------------------
# scl_palette
# ---------------------------------------------------------------------------


def test_palette_returns_correct_count():
    colors = scl_palette("dark2", n=4)
    assert len(colors) == 4
    assert all(_RGBA_RE.match(c) for c in colors)


def test_palette_auto_okabe_ito_for_small_n():
    assert scl_palette("auto", n=5) == scl_palette("okabe_ito", n=5)


def test_palette_auto_dark2_for_n8():
    assert scl_palette("auto", n=8) == scl_palette("dark2", n=8)


def test_palette_auto_paired_for_large_n():
    assert scl_palette("auto", n=10) == scl_palette("paired", n=10)


def test_palette_cycles_beyond_palette_size():
    colors = scl_palette("okabe_ito", n=10)  # only 7 colours
    assert len(colors) == 10
    assert colors[0] != colors[1]   # different within cycle
    assert colors[0] == colors[7]   # cycles back


def test_palette_respects_alpha():
    colors = scl_palette("dark2", n=3, alpha=0.5)
    assert all(",0.5)" in c for c in colors)


def test_palette_alpha_one_omits_trailing_zero():
    colors = scl_palette("dark2", n=2, alpha=1.0)
    assert all(_RGBA_RE.match(c) for c in colors)


def test_palette_all_named_palettes():
    for name in ("dark2", "set2", "okabe_ito", "paired"):
        colors = scl_palette(name, n=4)
        assert len(colors) == 4, f"failed for {name}"


def test_palette_unknown_raises():
    with pytest.raises(ValueError, match="unknown palette"):
        scl_palette("neon", n=3)


# ---------------------------------------------------------------------------
# scl_theme_colors
# ---------------------------------------------------------------------------


def test_theme_colors_returns_tuple_of_two_rgba():
    fill, line = scl_theme_colors("winset", theme="dark2")
    assert _RGBA_RE.match(fill)
    assert _RGBA_RE.match(line)


def test_theme_colors_fill_alpha_less_than_line_alpha():
    fill, line = scl_theme_colors("winset", theme="dark2")
    fill_alpha = float(fill.split(",")[-1].rstrip(")"))
    line_alpha = float(line.split(",")[-1].rstrip(")"))
    assert fill_alpha < line_alpha


def test_theme_colors_bw_returns_grays():
    fill, line = scl_theme_colors("yolk", theme="bw")
    assert fill.startswith("rgba(200,200,200")
    assert line.startswith("rgba(30,30,30")


def test_theme_colors_all_layer_types():
    for lt in ("winset", "yolk", "uncovered_set", "convex_hull", "voters", "alternatives"):
        fill, line = scl_theme_colors(lt, theme="dark2")
        assert _RGBA_RE.match(fill), f"fill failed for {lt}"
        assert _RGBA_RE.match(line), f"line failed for {lt}"


def test_theme_colors_all_themes():
    for theme in ("dark2", "set2", "okabe_ito", "paired", "bw"):
        fill, line = scl_theme_colors("winset", theme=theme)
        assert _RGBA_RE.match(fill), f"fill failed for {theme}"


def test_theme_colors_unknown_theme_raises():
    with pytest.raises(ValueError, match="unknown theme"):
        scl_theme_colors("winset", theme="neon")


def test_different_layer_types_get_different_colors():
    fills = [scl_theme_colors(lt, "dark2")[0]
             for lt in ("winset", "yolk", "uncovered_set", "convex_hull")]
    assert len(set(fills)) == 4, "layer types should have distinct colours"


# ---------------------------------------------------------------------------
# Re-exports via sclp
# ---------------------------------------------------------------------------


def test_scl_palette_accessible_via_plots_module():
    colors = sclp.scl_palette("dark2", n=3)
    assert len(colors) == 3


def test_scl_theme_colors_accessible_via_plots_module():
    fill, line = sclp.scl_theme_colors("winset")
    assert _RGBA_RE.match(fill)


# ---------------------------------------------------------------------------
# Integration: plots use theme colours
# ---------------------------------------------------------------------------


def test_plot_spatial_voting_dark2_theme():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ, theme="dark2")
    assert isinstance(fig, go.Figure)


def test_plot_spatial_voting_bw_theme():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ, theme="bw")
    assert isinstance(fig, go.Figure)


def test_plot_spatial_voting_set2_theme():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ, theme="set2")
    assert isinstance(fig, go.Figure)


def test_layer_winset_explicit_fill_overrides_theme():
    ws  = scl.winset_2d(SQ[0], SQ[1], VOTERS)
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    fig = sclp.layer_winset(fig, ws, fill_color="rgba(255,0,0,0.2)")
    assert isinstance(fig, go.Figure)


def test_layer_yolk_explicit_colors_override_theme():
    fig = sclp.plot_spatial_voting(VOTERS[:6])
    fig = sclp.layer_yolk(fig, 0.0, 0.0, 0.5,
                          fill_color="rgba(0,0,255,0.2)",
                          line_color="rgba(0,0,255,0.8)")
    assert isinstance(fig, go.Figure)


def test_layer_ic_color_by_voter_with_palette():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    fig = sclp.layer_ic(fig, VOTERS, SQ, color_by_voter=True, palette="dark2")
    assert isinstance(fig, go.Figure)
    assert len(fig.data) == 2 + 5  # voters + SQ traces + 5 IC traces


def test_layer_ic_color_by_voter_auto_uses_theme():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ, theme="set2")
    fig = sclp.layer_ic(fig, VOTERS, SQ, color_by_voter=True, theme="set2")
    assert isinstance(fig, go.Figure)


def test_layer_preferred_regions_color_by_voter_with_palette():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    fig = sclp.layer_preferred_regions(fig, VOTERS, SQ,
                                       color_by_voter=True, palette="okabe_ito")
    assert isinstance(fig, go.Figure)


def test_full_composition_with_bw_theme():
    ws   = scl.winset_2d(SQ[0], SQ[1], VOTERS)
    hull = scl.convex_hull_2d(VOTERS)
    bnd  = scl.uncovered_set_boundary_2d(VOTERS, grid_resolution=8)

    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ, theme="bw")
    fig = sclp.layer_convex_hull(fig, hull, theme="bw")
    fig = sclp.layer_uncovered_set(fig, bnd, theme="bw")
    fig = sclp.layer_yolk(fig, 0.1, 0.05, 0.3, theme="bw")
    fig = sclp.layer_winset(fig, ws, theme="bw")
    assert isinstance(fig, go.Figure)
