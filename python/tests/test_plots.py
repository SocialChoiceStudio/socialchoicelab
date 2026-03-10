# test_plots.py — C10/C13: Unit tests for spatial voting visualization helpers.

import os
import tempfile

import numpy as np
import pytest

plotly = pytest.importorskip("plotly")
import plotly.graph_objects as go  # noqa: E402

import socialchoicelab as scl  # noqa: E402
import socialchoicelab.plots as sclp  # noqa: E402

# ---------------------------------------------------------------------------
# Shared data
# ---------------------------------------------------------------------------

VOTERS = np.array([-1.0, -0.5,  0.0,  0.0,  0.8,  0.6, -0.4,  0.8,  0.5, -0.7])
ALTS   = np.array([ 0.0,  0.0,  0.6,  0.4, -0.5,  0.3])
SQ     = np.array([ 0.0,  0.0])


def _competition_trace_2d():
    competitors = np.array([[0.0, 0.0], [2.0, 0.0]])
    voters = np.array([[-0.5, 0.0], [0.0, 0.2], [1.8, 0.1]])
    cfg = scl.CompetitionEngineConfig(
        seat_count=1,
        seat_rule="plurality_top_k",
        max_rounds=2,
        step_config=scl.CompetitionStepConfig(kind="fixed", fixed_step_size=0.5),
    )
    return scl.competition_run(
        competitors,
        ["sticker", "sticker"],
        voters,
        dist_config=scl.DistanceConfig(salience_weights=[1.0, 1.0]),
        engine_config=cfg,
    )

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


def test_plot_spatial_voting_explicit_xlim_ylim():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ, xlim=[-2, 2], ylim=[-2, 2])
    assert isinstance(fig, go.Figure)
    assert tuple(fig.layout.xaxis.range) == (-2, 2)
    assert tuple(fig.layout.yaxis.range) == (-2, 2)


def test_plot_spatial_voting_auto_range_set():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ)
    assert isinstance(fig, go.Figure)
    assert fig.layout.xaxis.range is not None


def test_plot_spatial_voting_defaults_include_origin_and_center_title():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ)
    assert fig.layout.title.x == 0.5
    assert fig.layout.plot_bgcolor == "white"
    assert fig.layout.xaxis.range[0] <= 0 <= fig.layout.xaxis.range[1]
    assert fig.layout.yaxis.range[0] <= 0 <= fig.layout.yaxis.range[1]


def test_plot_spatial_voting_does_not_label_sq_by_default():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    sq_trace = next(trace for trace in fig.data if trace.name == "Status Quo")
    assert sq_trace.mode == "markers"


def test_plot_spatial_voting_no_alternatives():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    assert isinstance(fig, go.Figure)


def test_plot_spatial_voting_show_labels():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ, show_labels=True)
    assert isinstance(fig, go.Figure)


def test_plot_competition_trajectories_returns_figure():
    with _competition_trace_2d() as trace:
        fig = sclp.plot_competition_trajectories(trace, voters=VOTERS)
        assert isinstance(fig, go.Figure)
        assert len(fig.data) >= 3  # voter layer + competitor paths


def test_animate_competition_trajectories_returns_figure_with_frames():
    with _competition_trace_2d() as trace:
        fig = sclp.animate_competition_trajectories(trace, voters=VOTERS)
        assert isinstance(fig, go.Figure)
        assert len(fig.frames) >= 2
        assert fig.layout.margin.b == 220
        assert fig.layout.sliders[0].y < 0
        assert fig.layout.updatemenus[0].y < 0
        assert fig.layout.sliders[0].currentvalue.visible is False
        assert "select2d" in fig.layout.modebar.remove
        overlay_traces = [t for t in fig.data if getattr(t, "meta", {}).get("scl_role") == "overlay"]
        assert overlay_traces


def test_animate_competition_trajectories_trail_none_markers_only():
    """With trail='none' all overlay traces must be markers."""
    with _competition_trace_2d() as trace:
        fig = sclp.animate_competition_trajectories(trace, voters=VOTERS, trail="none")
        overlay_modes = [t.mode for t in fig.data if getattr(t, "meta", {}).get("scl_role") == "overlay"]
        assert overlay_modes
        assert all(mode == "markers" for mode in overlay_modes)


def test_animate_competition_trajectories_fade_trail_produces_frames():
    with _competition_trace_2d() as trace:
        fig = sclp.animate_competition_trajectories(trace, voters=VOTERS, trail="fade")
        assert isinstance(fig, go.Figure)
        assert len(fig.frames) >= 2


def test_animate_competition_trajectories_fade_trail_length_integer_respected():
    """With trail_length=2 (integer), no frame should contain more than 2 non-empty
    segments per competitor."""
    with _competition_trace_2d() as trace:
        n_rounds, n_competitors, _ = trace.dims()
        fig = sclp.animate_competition_trajectories(
            trace, voters=VOTERS, trail="fade", trail_length=2
        )
        for frame in fig.frames:
            non_empty = [
                t for t in frame.data
                if t.mode == "lines" and t.x is not None and len(t.x) > 0
            ]
            assert len(non_empty) <= n_competitors * 2


@pytest.mark.parametrize("shorthand,divisor", [
    ("short", 3),
    ("medium", 2),
    ("long", 4),  # 3/4 rounds, so max_segments // 4 * 3
])
def test_animate_competition_trajectories_fade_trail_length_shorthand(shorthand, divisor):
    """String shorthands resolve relative to total rounds and cap segments correctly."""
    with _competition_trace_2d() as trace:
        n_rounds, n_competitors, _ = trace.dims()
        n_segments_max = n_rounds  # n_rounds + 1 frames → n_rounds segments
        if shorthand == "long":
            expected_max = max(1, (n_segments_max * 3) // 4)
        else:
            expected_max = max(1, n_segments_max // divisor)
        fig = sclp.animate_competition_trajectories(
            trace, voters=VOTERS, trail="fade", trail_length=shorthand
        )
        for frame in fig.frames:
            non_empty = [
                t for t in frame.data
                if t.mode == "lines" and t.x is not None and len(t.x) > 0
            ]
            assert len(non_empty) <= n_competitors * expected_max


def test_animate_competition_trajectories_trail_length_invalid_string_raises():
    with _competition_trace_2d() as trace:
        with pytest.raises(ValueError, match="trail_length"):
            sclp.animate_competition_trajectories(trace, trail="fade", trail_length="huge")


def test_animate_competition_trajectories_trail_length_zero_raises():
    with _competition_trace_2d() as trace:
        with pytest.raises(ValueError, match="trail_length"):
            sclp.animate_competition_trajectories(trace, trail="fade", trail_length=0)


# ---------------------------------------------------------------------------
# layer_yolk  (no C library needed)
# ---------------------------------------------------------------------------


def test_layer_yolk_returns_figure():
    fig = sclp.plot_spatial_voting(VOTERS[:6], ALTS[:2])
    fig = sclp.layer_yolk(fig, center_x=0.1, center_y=0.05, radius=0.3)
    assert isinstance(fig, go.Figure)


def test_layer_yolk_custom_colors():
    fig = sclp.plot_spatial_voting(VOTERS[:4], ALTS[:2])
    fig = sclp.layer_yolk(fig, 0.0, 0.0, 0.5,
                          fill_color="rgba(0,0,255,0.2)",
                          line_color="rgba(0,0,255,0.8)",
                          name="Yolk approx.")
    assert isinstance(fig, go.Figure)


# ---------------------------------------------------------------------------
# layer_convex_hull
# ---------------------------------------------------------------------------


def test_layer_convex_hull_returns_figure():
    hull = scl.convex_hull_2d(VOTERS)
    fig  = sclp.plot_spatial_voting(VOTERS, ALTS)
    fig  = sclp.layer_convex_hull(fig, hull)
    assert isinstance(fig, go.Figure)


def test_background_regions_stay_below_points_without_zorder_dependency():
    hull = scl.convex_hull_2d(VOTERS)
    ws = scl.winset_2d(SQ[0], SQ[1], VOTERS)
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    fig = sclp.layer_convex_hull(fig, hull)
    fig = sclp.layer_winset(fig, ws)
    names = [trace.name for trace in fig.data]
    assert names[:2] == ["Convex Hull", "Winset"]
    assert names[-2:] == ["Voters", "Status Quo"]


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


def test_layer_uncovered_set_auto_compute():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS)
    fig = sclp.layer_uncovered_set(fig, voters=VOTERS, grid_resolution=8)
    assert isinstance(fig, go.Figure)


def test_layer_uncovered_set_error_no_args():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS)
    with pytest.raises(ValueError, match="boundary_xy"):
        sclp.layer_uncovered_set(fig)


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
    assert len(fig.data) == n_before + 1
    empty_traces = [t for t in fig.data if "\u2205" in (t.name or "")]
    assert len(empty_traces) == 1


def test_layer_winset_auto_compute():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ)
    fig = sclp.layer_winset(fig, voters=VOTERS, sq=SQ)
    assert isinstance(fig, go.Figure)


def test_layer_winset_error_no_args():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ)
    with pytest.raises(ValueError, match="voters"):
        sclp.layer_winset(fig)


# ---------------------------------------------------------------------------
# layer_ic
# ---------------------------------------------------------------------------


def test_layer_ic_returns_figure():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    fig = sclp.layer_ic(fig, VOTERS, SQ)
    assert isinstance(fig, go.Figure)


def test_layer_ic_adds_traces_per_voter():
    voters = VOTERS[:6]  # 3 voters
    fig = sclp.plot_spatial_voting(voters, sq=SQ)
    n_before = len(fig.data)
    fig = sclp.layer_ic(fig, voters, SQ)
    assert isinstance(fig, go.Figure)
    assert len(fig.data) == n_before + 3  # one circle per voter


def test_layer_ic_color_by_voter():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    fig = sclp.layer_ic(fig, VOTERS, SQ, color_by_voter=True)
    assert isinstance(fig, go.Figure)


def test_layer_ic_with_fill():
    voters = VOTERS[:6]
    fig = sclp.plot_spatial_voting(voters, sq=SQ)
    fig = sclp.layer_ic(fig, voters, SQ, fill_color="rgba(150,150,200,0.08)")
    assert isinstance(fig, go.Figure)


def test_layer_ic_custom_voter_names():
    voters = VOTERS[:6]
    fig = sclp.plot_spatial_voting(voters, sq=SQ)
    fig = sclp.layer_ic(fig, voters, SQ, voter_names=["Alice", "Bob", "Carol"])
    assert isinstance(fig, go.Figure)


# ---------------------------------------------------------------------------
# layer_preferred_regions
# ---------------------------------------------------------------------------


def test_layer_preferred_regions_returns_figure():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    fig = sclp.layer_preferred_regions(fig, VOTERS, SQ)
    assert isinstance(fig, go.Figure)


def test_layer_preferred_regions_adds_traces():
    voters = VOTERS[:6]  # 3 voters
    fig = sclp.plot_spatial_voting(voters, sq=SQ)
    n_before = len(fig.data)
    fig = sclp.layer_preferred_regions(fig, voters, SQ)
    assert isinstance(fig, go.Figure)
    assert len(fig.data) == n_before + 3


def test_layer_preferred_regions_color_by_voter():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    fig = sclp.layer_preferred_regions(fig, VOTERS, SQ, color_by_voter=True)
    assert isinstance(fig, go.Figure)


# ---------------------------------------------------------------------------
# save_plot
# ---------------------------------------------------------------------------


def test_save_plot_html():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    with tempfile.NamedTemporaryFile(suffix=".html", delete=False) as f:
        path = f.name
    try:
        result = sclp.save_plot(fig, path)
        assert result == path
        assert os.path.exists(path)
        assert os.path.getsize(path) > 0
    finally:
        os.unlink(path)


def test_save_plot_unsupported_extension():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    with pytest.raises(ValueError, match="unsupported file extension"):
        sclp.save_plot(fig, "output.xyz")


# ---------------------------------------------------------------------------
# Full composition
# ---------------------------------------------------------------------------


def test_full_composition():
    ws   = scl.winset_2d(SQ[0], SQ[1], VOTERS)
    hull = scl.convex_hull_2d(VOTERS)
    bnd  = scl.uncovered_set_boundary_2d(VOTERS, grid_resolution=8)

    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ, title="Composition Test")
    fig = sclp.layer_ic(fig, VOTERS, SQ)
    fig = sclp.layer_preferred_regions(fig, VOTERS, SQ)
    fig = sclp.layer_convex_hull(fig, hull)
    fig = sclp.layer_uncovered_set(fig, bnd)
    fig = sclp.layer_yolk(fig, 0.1, 0.05, 0.3)
    fig = sclp.layer_winset(fig, ws)
    fig = sclp.finalize_plot(fig)

    assert isinstance(fig, go.Figure)
    assert len(fig.data) >= 7


# ---------------------------------------------------------------------------
# layer_centroid
# ---------------------------------------------------------------------------


def test_layer_centroid_returns_figure():
    fig  = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    fig2 = sclp.layer_centroid(fig, VOTERS)
    assert isinstance(fig2, go.Figure)


def test_layer_centroid_adds_one_trace():
    fig  = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    n_before = len(fig.data)
    fig2 = sclp.layer_centroid(fig, VOTERS)
    assert len(fig2.data) == n_before + 1


def test_layer_centroid_marker_is_diamond():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    fig = sclp.layer_centroid(fig, VOTERS)
    assert fig.data[-1].marker.symbol == "diamond"


def test_layer_centroid_position_matches_mean():
    voters = np.array([-1.0, 0.0,  1.0, 0.0,  0.0, 2.0])
    fig = sclp.plot_spatial_voting(voters)
    fig = sclp.layer_centroid(fig, voters)
    trace = fig.data[-1]
    assert abs(trace.x[0] - 0.0) < 1e-10
    assert abs(trace.y[0] - (2.0 / 3.0)) < 1e-10


# ---------------------------------------------------------------------------
# layer_marginal_median
# ---------------------------------------------------------------------------


def test_layer_marginal_median_returns_figure():
    fig  = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    fig2 = sclp.layer_marginal_median(fig, VOTERS)
    assert isinstance(fig2, go.Figure)


def test_layer_marginal_median_adds_one_trace():
    fig  = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    n_before = len(fig.data)
    fig2 = sclp.layer_marginal_median(fig, VOTERS)
    assert len(fig2.data) == n_before + 1


def test_layer_marginal_median_marker_is_cross():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    fig = sclp.layer_marginal_median(fig, VOTERS)
    assert fig.data[-1].marker.symbol == "cross"


def test_layer_marginal_median_position_matches_median():
    # Three voters at x=-1,0,1 and y=0 → median (0,0)
    voters = np.array([-1.0, 0.0,  0.0, 0.0,  1.0, 0.0])
    fig = sclp.plot_spatial_voting(voters)
    fig = sclp.layer_marginal_median(fig, voters)
    trace = fig.data[-1]
    assert abs(trace.x[0] - 0.0) < 1e-10
    assert abs(trace.y[0] - 0.0) < 1e-10
