# test_plots.py — C10/C13: Unit tests for spatial voting visualization helpers.

import os
import tempfile
from pathlib import Path

import numpy as np
import pytest

import socialchoicelab as scl
import socialchoicelab.plots as sclp

# ---------------------------------------------------------------------------
# Shared data
# ---------------------------------------------------------------------------

VOTERS = np.array([-1.0, -0.5, 0.0, 0.0, 0.8, 0.6, -0.4, 0.8, 0.5, -0.7])
ALTS = np.array([0.0, 0.0, 0.6, 0.4, -0.5, 0.3])
SQ = np.array([0.0, 0.0])


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


def test_plot_spatial_voting_returns_payload():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS)
    assert fig["_scs_spatial_canvas"]
    assert len(fig["voters_x"]) == 5
    assert fig["layers"] == {}


def test_plot_spatial_voting_with_sq():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ)
    assert fig["sq"] == [0.0, 0.0]


def test_plot_spatial_voting_custom_names():
    fig = sclp.plot_spatial_voting(
        VOTERS,
        ALTS,
        sq=SQ,
        voter_names=[f"V{i}" for i in range(5)],
        alt_names=["Origin", "Alt A", "Alt B"],
        dim_names=("Economic", "Social"),
        title="Test Legislature",
    )
    assert fig["voter_names"][0] == "V0"
    assert fig["alternative_names"] == ["Origin", "Alt A", "Alt B"]


def test_plot_spatial_voting_custom_dimensions():
    fig = sclp.plot_spatial_voting(VOTERS[:4], ALTS[:2], width=800, height=500)
    assert fig["_width"] == 800
    assert fig["_height"] == 500


def test_plot_spatial_voting_explicit_xlim_ylim():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ, xlim=[-2, 2], ylim=[-2, 2])
    assert fig["xlim"] == [-2.0, 2.0]
    assert fig["ylim"] == [-2.0, 2.0]


def test_plot_spatial_voting_auto_range_set():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ)
    assert fig["xlim"][0] <= fig["xlim"][1]
    assert fig["ylim"][0] <= fig["ylim"][1]


def test_plot_spatial_voting_defaults_include_origin():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ)
    assert fig["xlim"][0] <= 0 <= fig["xlim"][1]
    assert fig["ylim"][0] <= 0 <= fig["ylim"][1]


def test_plot_spatial_voting_no_alternatives():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    assert fig["alternatives"] == []


def test_plot_spatial_voting_show_labels():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ, show_labels=True)
    assert fig["show_labels"] is True


def test_plot_spatial_voting_layer_toggles_default_and_false():
    fig_on = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ)
    assert fig_on["layer_toggles"] is True
    fig_off = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ, layer_toggles=False)
    assert fig_off["layer_toggles"] is False


# ---------------------------------------------------------------------------
# animate_competition_canvas
# ---------------------------------------------------------------------------


def test_animate_competition_canvas_returns_html_string():
    with _competition_trace_2d() as trace:
        html = sclp.animate_competition_canvas(trace, voters=VOTERS)
        assert isinstance(html, str)
        assert "<!DOCTYPE html>" in html
        assert "competition-canvas" in html


def test_animate_competition_canvas_includes_core_js():
    with _competition_trace_2d() as trace:
        html = sclp.animate_competition_canvas(trace, voters=VOTERS)
        assert "ScsCanvasCore" in html or "scs_canvas_core" in html.lower()


def test_animate_competition_canvas_embeds_json_payload():
    with _competition_trace_2d() as trace:
        html = sclp.animate_competition_canvas(trace, voters=VOTERS, title="Test Title")
        assert "Test Title" in html
        assert "voters_x" in html
        assert "positions" in html
        assert "competitor_colors" in html


def test_animate_competition_canvas_writes_file():
    with _competition_trace_2d() as trace:
        with tempfile.NamedTemporaryFile(suffix=".html", delete=False) as f:
            path = f.name
        try:
            html = sclp.animate_competition_canvas(trace, voters=VOTERS, path=path)
            assert os.path.exists(path)
            assert os.path.getsize(path) > 0
            assert Path(path).read_text(encoding="utf-8") == html
        finally:
            os.unlink(path)


def test_animate_competition_canvas_competitor_names_in_payload():
    with _competition_trace_2d() as trace:
        html = sclp.animate_competition_canvas(
            trace,
            voters=VOTERS,
            competitor_names=["Left Party", "Right Party"],
        )
        assert "Left Party" in html
        assert "Right Party" in html


def test_animate_competition_canvas_trail_options_accepted():
    with _competition_trace_2d() as trace:
        for trail in ("fade", "full", "none"):
            html = sclp.animate_competition_canvas(trace, voters=VOTERS, trail=trail)
            assert isinstance(html, str)


def test_animate_competition_canvas_invalid_trail_raises():
    with _competition_trace_2d() as trace:
        with pytest.raises(ValueError, match="trail"):
            sclp.animate_competition_canvas(trace, trail="bouncy")


def test_animate_competition_canvas_invalid_trail_length_raises():
    with _competition_trace_2d() as trace:
        with pytest.raises(ValueError, match="trail_length"):
            sclp.animate_competition_canvas(trace, trail_length="huge")


def test_animate_competition_canvas_wrong_competitor_names_raises():
    with _competition_trace_2d() as trace:
        with pytest.raises(ValueError, match="competitor_names"):
            sclp.animate_competition_canvas(trace, competitor_names=["Only One Name"])


def test_strategy_kinds_returns_correct_strategies():
    with _competition_trace_2d() as trace:
        kinds = trace.strategy_kinds()
        assert kinds == ["sticker", "sticker"]


def test_animate_competition_canvas_auto_generates_names_from_strategies():
    with _competition_trace_2d() as trace:
        html = sclp.animate_competition_canvas(trace, voters=VOTERS)
        assert "Sticker A" in html
        assert "Sticker B" in html


def test_animate_competition_canvas_annotates_user_names_with_strategy():
    with _competition_trace_2d() as trace:
        html = sclp.animate_competition_canvas(
            trace, voters=VOTERS, competitor_names=["Alice", "Bob"]
        )
        assert "Alice (Sticker)" in html
        assert "Bob (Sticker)" in html


# ---------------------------------------------------------------------------
# layer_yolk
# ---------------------------------------------------------------------------


def test_layer_yolk():
    fig = sclp.plot_spatial_voting(VOTERS[:6], ALTS[:2])
    fig = sclp.layer_yolk(fig, center_x=0.1, center_y=0.05, radius=0.3)
    assert fig["layers"]["yolk"]["r"] == pytest.approx(0.3)


def test_layer_yolk_custom_colors():
    fig = sclp.plot_spatial_voting(VOTERS[:4], ALTS[:2])
    fig = sclp.layer_yolk(
        fig,
        0.0,
        0.0,
        0.5,
        fill_color="rgba(0,0,255,0.2)",
        line_color="rgba(0,0,255,0.8)",
        name="Yolk approx.",
    )
    assert fig["layers"]["yolk"]["fill"] == "rgba(0,0,255,0.2)"


# ---------------------------------------------------------------------------
# layer_convex_hull
# ---------------------------------------------------------------------------


def test_layer_convex_hull():
    hull = scl.convex_hull_2d(VOTERS)
    fig = sclp.plot_spatial_voting(VOTERS, ALTS)
    fig = sclp.layer_convex_hull(fig, hull)
    assert "xy" in fig["layers"]["convex_hull_xy"]


def test_layer_convex_hull_empty():
    hull = np.empty((0, 2), dtype=np.float64)
    fig = sclp.plot_spatial_voting(VOTERS, ALTS)
    fig = sclp.layer_convex_hull(fig, hull)
    assert "convex_hull_xy" not in fig["layers"]


# ---------------------------------------------------------------------------
# layer_uncovered_set
# ---------------------------------------------------------------------------


def test_layer_uncovered_set():
    bnd = scl.uncovered_set_boundary_2d(VOTERS, grid_resolution=8)
    fig = sclp.plot_spatial_voting(VOTERS, ALTS)
    fig = sclp.layer_uncovered_set(fig, bnd)
    assert "uncovered_xy" in fig["layers"]


def test_layer_uncovered_set_empty():
    bnd = np.empty((0, 2), dtype=np.float64)
    fig = sclp.plot_spatial_voting(VOTERS, ALTS)
    fig = sclp.layer_uncovered_set(fig, bnd)
    assert "uncovered_xy" not in fig["layers"]


def test_layer_uncovered_set_auto_compute():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS)
    fig = sclp.layer_uncovered_set(fig, voters=VOTERS, grid_resolution=8)
    assert "uncovered_xy" in fig["layers"]


def test_layer_uncovered_set_error_no_args():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS)
    with pytest.raises(ValueError, match="boundary_xy"):
        sclp.layer_uncovered_set(fig)


# ---------------------------------------------------------------------------
# layer_winset
# ---------------------------------------------------------------------------


def test_layer_winset_non_empty():
    ws = scl.winset_2d(SQ[0], SQ[1], VOTERS)
    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ)
    fig = sclp.layer_winset(fig, ws)
    assert fig["layers"]["winset_paths"]


def test_layer_winset_empty():
    voters_same = np.zeros(6, dtype=np.float64)
    ws = scl.winset_2d(0.0, 0.0, voters_same)
    fig = sclp.plot_spatial_voting(voters_same, ALTS[:2])
    fig = sclp.layer_winset(fig, ws)
    assert "winset_paths" not in fig["layers"]


def test_layer_winset_auto_compute():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ)
    fig = sclp.layer_winset(fig, voters=VOTERS, sq=SQ)
    assert fig["layers"].get("winset_paths") is not None


def test_layer_winset_error_no_args():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ)
    with pytest.raises(ValueError, match="voters"):
        sclp.layer_winset(fig)


def test_layer_winset_auto_compute_manhattan():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ)
    dc = scl.make_dist_config("manhattan")
    fig = sclp.layer_winset(fig, voters=VOTERS, sq=SQ, dist_config=dc)
    assert fig["layers"]["winset_paths"]


def test_layer_winset_auto_compute_chebyshev():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ)
    dc = scl.make_dist_config("chebyshev")
    fig = sclp.layer_winset(fig, voters=VOTERS, sq=SQ, dist_config=dc)
    # Chebyshev winset can be empty for this fixture; layer must not error.
    assert fig["_scs_spatial_canvas"]


def test_layer_winset_auto_compute_minkowski_p3():
    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ)
    dc = scl.make_dist_config("minkowski", order_p=3.0)
    fig = sclp.layer_winset(fig, voters=VOTERS, sq=SQ, dist_config=dc)
    assert fig["layers"]["winset_paths"]


# ---------------------------------------------------------------------------
# layer_ic
# ---------------------------------------------------------------------------


def test_layer_ic():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    fig = sclp.layer_ic(fig, VOTERS, SQ)
    assert len(fig["layers"]["ic_curves"]) == 5


def test_layer_ic_adds_per_voter():
    voters = VOTERS[:6]
    fig = sclp.plot_spatial_voting(voters, sq=SQ)
    fig = sclp.layer_ic(fig, voters, SQ)
    assert len(fig["layers"]["ic_curves"]) == 3


def test_layer_ic_color_by_voter():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    fig = sclp.layer_ic(fig, VOTERS, SQ, color_by_voter=True)
    assert len({c["line"] for c in fig["layers"]["ic_curves"]}) >= 2


def test_layer_ic_with_fill():
    voters = VOTERS[:6]
    fig = sclp.plot_spatial_voting(voters, sq=SQ)
    fig = sclp.layer_ic(fig, voters, SQ, fill_color="rgba(150,150,200,0.08)")
    assert all(c["fill"] is not None for c in fig["layers"]["ic_curves"])


def test_layer_ic_custom_voter_names():
    voters = VOTERS[:6]
    fig = sclp.plot_spatial_voting(voters, sq=SQ)
    fig = sclp.layer_ic(fig, voters, SQ, voter_names=["Alice", "Bob", "Carol"])
    assert len(fig["layers"]["ic_curves"]) == 3


def test_layer_ic_non_euclidean_manhattan():
    voters = VOTERS[:6]
    fig = sclp.plot_spatial_voting(voters, sq=SQ)
    dc = scl.make_dist_config("manhattan")
    fig = sclp.layer_ic(fig, voters, SQ, dist_config=dc)
    assert len(fig["layers"]["ic_curves"]) == 3


def test_layer_ic_non_euclidean_chebyshev():
    voters = VOTERS[:4]
    fig = sclp.plot_spatial_voting(voters, sq=SQ)
    dc = scl.make_dist_config("chebyshev")
    fig = sclp.layer_ic(fig, voters, SQ, dist_config=dc)
    assert len(fig["layers"]["ic_curves"]) == 2


# ---------------------------------------------------------------------------
# layer_preferred_regions
# ---------------------------------------------------------------------------


def test_layer_preferred_regions():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    fig = sclp.layer_preferred_regions(fig, VOTERS, SQ)
    assert len(fig["layers"]["preferred_regions"]) == 5


def test_layer_preferred_regions_adds_per_voter():
    voters = VOTERS[:6]
    fig = sclp.plot_spatial_voting(voters, sq=SQ)
    fig = sclp.layer_preferred_regions(fig, voters, SQ)
    assert len(fig["layers"]["preferred_regions"]) == 3


def test_layer_preferred_regions_color_by_voter():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    fig = sclp.layer_preferred_regions(fig, VOTERS, SQ, color_by_voter=True)
    assert len(fig["layers"]["preferred_regions"]) == 5


def test_layer_preferred_regions_non_euclidean_manhattan():
    voters = VOTERS[:6]
    fig = sclp.plot_spatial_voting(voters, sq=SQ)
    dc = scl.make_dist_config("manhattan")
    fig = sclp.layer_preferred_regions(fig, voters, SQ, dist_config=dc)
    assert len(fig["layers"]["preferred_regions"]) == 3


def test_layer_preferred_regions_non_euclidean_chebyshev():
    voters = VOTERS[:4]
    fig = sclp.plot_spatial_voting(voters, sq=SQ)
    dc = scl.make_dist_config("chebyshev")
    fig = sclp.layer_preferred_regions(fig, voters, SQ, dist_config=dc)
    assert len(fig["layers"]["preferred_regions"]) == 2


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
        txt = Path(path).read_text(encoding="utf-8")
        assert "spatial-voting-canvas" in txt
    finally:
        os.unlink(path)


def test_save_plot_png_not_implemented():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    with pytest.raises(NotImplementedError, match="PNG"):
        sclp.save_plot(fig, "out.png")


def test_save_plot_unsupported_extension():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    with pytest.raises(ValueError, match="unsupported file extension"):
        sclp.save_plot(fig, "output.xyz")


# ---------------------------------------------------------------------------
# Full composition
# ---------------------------------------------------------------------------


def test_full_composition():
    ws = scl.winset_2d(SQ[0], SQ[1], VOTERS)
    hull = scl.convex_hull_2d(VOTERS)
    bnd = scl.uncovered_set_boundary_2d(VOTERS, grid_resolution=8)

    fig = sclp.plot_spatial_voting(VOTERS, ALTS, sq=SQ, title="Composition Test")
    fig = sclp.layer_ic(fig, VOTERS, SQ)
    fig = sclp.layer_preferred_regions(fig, VOTERS, SQ)
    fig = sclp.layer_convex_hull(fig, hull)
    fig = sclp.layer_uncovered_set(fig, bnd)
    fig = sclp.layer_yolk(fig, 0.1, 0.05, 0.3)
    fig = sclp.layer_winset(fig, ws)
    assert fig["layers"]["ic_curves"]
    assert fig["layers"]["preferred_regions"]
    assert fig["layers"]["convex_hull_xy"]
    assert fig["layers"]["uncovered_xy"]
    assert fig["layers"]["yolk"]
    assert fig["layers"]["winset_paths"]


# ---------------------------------------------------------------------------
# layer_centroid
# ---------------------------------------------------------------------------


def test_layer_centroid():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    fig2 = sclp.layer_centroid(fig, VOTERS)
    assert len(fig2["layers"]["centroid"]) == 2


def test_layer_centroid_position_matches_mean():
    voters = np.array([-1.0, 0.0, 1.0, 0.0, 0.0, 2.0])
    fig = sclp.plot_spatial_voting(voters)
    fig = sclp.layer_centroid(fig, voters)
    cx, cy = fig["layers"]["centroid"]
    assert abs(cx - 0.0) < 1e-10
    assert abs(cy - (2.0 / 3.0)) < 1e-10


# ---------------------------------------------------------------------------
# layer_marginal_median
# ---------------------------------------------------------------------------


def test_layer_marginal_median():
    fig = sclp.plot_spatial_voting(VOTERS, sq=SQ)
    fig2 = sclp.layer_marginal_median(fig, VOTERS)
    assert len(fig2["layers"]["marginal_median"]) == 2


def test_layer_marginal_median_position_matches_median():
    voters = np.array([-1.0, 0.0, 0.0, 0.0, 1.0, 0.0])
    fig = sclp.plot_spatial_voting(voters)
    fig = sclp.layer_marginal_median(fig, voters)
    mx, my = fig["layers"]["marginal_median"]
    assert abs(mx - 0.0) < 1e-10
    assert abs(my - 0.0) < 1e-10
