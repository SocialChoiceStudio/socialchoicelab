# test_plots.py — C11.3: Unit tests for spatial voting visualization helpers.
#
# All tests that call the C library use fixtures that load libscs_api.
# Tests of pure Python plotting logic (layer_yolk, composition with synthetic
# data) can run without the library.

import numpy as np
import pytest

plotly = pytest.importorskip("plotly")
import plotly.graph_objects as go  # noqa: E402

import socialchoicelab as scl  # noqa: E402  (requires libscs_api)
import socialchoicelab.plots as sclp  # noqa: E402

# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------

VOTERS = np.array([-1.0, -0.5,  0.0,  0.0,  0.8,  0.6, -0.4,  0.8,  0.5, -0.7])
ALTS   = np.array([ 0.0,  0.0,  0.6,  0.4, -0.5,  0.3])
SQ     = np.array([ 0.0,  0.0])


# ---------------------------------------------------------------------------
# plot_spatial_voting
# ---------------------------------------------------------------------------


def test_plot_spatial_voting_returns_figure():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS)
    assert isinstance(fig, go.Figure)


def test_plot_spatial_voting_with_sq():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ)
    assert isinstance(fig, go.Figure)


def test_plot_spatial_voting_custom_names():
    fig = sclp.plot_spatial_voting(
        VOTERS, ALTS,
        sq=SQ,
        voter_names=[f"V{i}" for i in range(5)],
        alt_names=["Origin", "Alt A", "Alt B"],
        dim_names=("Economic", "Social"),
        title="Test Legislature",
    )
    assert isinstance(fig, go.Figure)


def test_plot_spatial_voting_custom_dimensions():
    fig = sclp.plot_spatial_voting(VOTERS[:4], ALTS[:2], width=800, height=500)
    assert isinstance(fig, go.Figure)


# ---------------------------------------------------------------------------
# layer_yolk  (no C library needed)
# ---------------------------------------------------------------------------


def test_layer_yolk_returns_figure():
    fig = sclp.plot_spatial_voting(VOTERS[:6], ALTS[:2])
    fig = sclp.layer_yolk(fig, center_x=0.1, center_y=0.05, radius=0.3)
    assert isinstance(fig, go.Figure)


def test_layer_yolk_custom_color():
    fig = sclp.plot_spatial_voting(VOTERS[:4], ALTS[:2])
    fig = sclp.layer_yolk(fig, 0.0, 0.0, 0.5,
                          color="rgba(0,0,255,0.4)", name="Yolk approx.")
    assert isinstance(fig, go.Figure)


# ---------------------------------------------------------------------------
# layer_convex_hull
# ---------------------------------------------------------------------------


def test_layer_convex_hull_returns_figure():
    hull = scl.convex_hull_2d(VOTERS)
    fig  = sclp.plot_spatial_voting(VOTERS, ALTS)
    fig  = sclp.layer_convex_hull(fig, hull)
    assert isinstance(fig, go.Figure)


def test_layer_convex_hull_empty():
    hull = np.empty((0, 2), dtype=np.float64)
    fig  = sclp.plot_spatial_voting(VOTERS, ALTS)
    n_before = len(fig.data)
    fig  = sclp.layer_convex_hull(fig, hull)
    assert isinstance(fig, go.Figure)
    assert len(fig.data) == n_before


# ---------------------------------------------------------------------------
# layer_uncovered_set
# ---------------------------------------------------------------------------


def test_layer_uncovered_set_returns_figure():
    bnd = scl.uncovered_set_boundary_2d(VOTERS, grid_resolution=8)
    fig = sclp.plot_spatial_voting(VOTERS, ALTS)
    fig = sclp.layer_uncovered_set(fig, bnd)
    assert isinstance(fig, go.Figure)


def test_layer_uncovered_set_empty():
    bnd = np.empty((0, 2), dtype=np.float64)
    fig = sclp.plot_spatial_voting(VOTERS, ALTS)
    n_before = len(fig.data)
    fig = sclp.layer_uncovered_set(fig, bnd)
    assert isinstance(fig, go.Figure)
    assert len(fig.data) == n_before


# ---------------------------------------------------------------------------
# layer_winset
# ---------------------------------------------------------------------------


def test_layer_winset_non_empty():
    ws  = scl.winset_2d(SQ[0], SQ[1], VOTERS)
    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ)
    fig = sclp.layer_winset(fig, ws)
    assert isinstance(fig, go.Figure)


def test_layer_winset_empty():
    voters_same = np.zeros(6, dtype=np.float64)
    ws  = scl.winset_2d(0.0, 0.0, voters_same)
    fig = sclp.plot_spatial_voting(voters_same, ALTS[:2])
    n_before = len(fig.data)
    fig = sclp.layer_winset(fig, ws)
    assert isinstance(fig, go.Figure)
    assert len(fig.data) == n_before


# ---------------------------------------------------------------------------
# Full composition
# ---------------------------------------------------------------------------


def test_full_composition():
    ws   = scl.winset_2d(SQ[0], SQ[1], VOTERS)
    hull = scl.convex_hull_2d(VOTERS)
    bnd  = scl.uncovered_set_boundary_2d(VOTERS, grid_resolution=8)

    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ, title="Composition Test")
    fig = sclp.layer_winset(fig, ws)
    fig = sclp.layer_uncovered_set(fig, bnd)
    fig = sclp.layer_convex_hull(fig, hull)
    fig = sclp.layer_yolk(fig, 0.1, 0.05, 0.3)

    assert isinstance(fig, go.Figure)
    assert len(fig.data) >= 5
