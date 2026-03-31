# test_competition_canvas_ic.py — Unit tests for animate_competition_canvas IC feature.

import json
import re

import numpy as np
import pytest

import socialchoicelab as scl
import socialchoicelab.plots as sclp

# ---------------------------------------------------------------------------
# Fixture helpers
# ---------------------------------------------------------------------------

# 3 2D voters, 2 sticker competitors, 1 seat, 2 rounds.
# Competitor 0 at (0,0) wins every round (closer to 2 of 3 voters).
# n_frames = 3 (Round 1, Round 2, Final), 1 seat holder per frame.

IC_VOTERS = np.array([[-0.5, 0.0], [0.0, 0.2], [1.8, 0.1]])
_IC_COMPETITORS = np.array([[0.0, 0.0], [2.0, 0.0]])
_IC_ENGINE_CFG = scl.CompetitionEngineConfig(
    seat_count=1,
    seat_rule="plurality_top_k",
    max_rounds=2,
    step_config=scl.CompetitionStepConfig(kind="fixed", fixed_step_size=0.5),
)


def _ic_trace_2d():
    return scl.competition_run(
        _IC_COMPETITORS,
        ["sticker", "sticker"],
        IC_VOTERS,
        dist_config=scl.DistanceConfig(salience_weights=[1.0, 1.0]),
        engine_config=_IC_ENGINE_CFG,
    )


def _extract_payload(html: str) -> dict:
    """Parse the JSON payload embedded in the self-contained HTML widget."""
    match = re.search(
        r"HTMLWidgets\._init\(.*?\d+,\s*\d+,\s*(\{.*\})\s*\);",
        html,
        re.DOTALL,
    )
    assert match is not None, "Could not find JSON payload in HTML."
    return json.loads(match.group(1))


def _first_ic_ring(payload: dict) -> list[float]:
    ring = (
        payload["overlays_frames"][0]["indifference_curves"][0]["curves"][0]["ring"]
    )
    return ring


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------


def test_compute_ic_false_leaves_key_absent():
    with _ic_trace_2d() as trace:
        html = sclp.animate_competition_canvas(trace, voters=IC_VOTERS)
    payload = _extract_payload(html)
    assert "indifference_curves" not in payload
    assert "ic_competitor_indices" not in payload


def test_compute_ic_true_produces_correctly_shaped_payload():
    with _ic_trace_2d() as trace:
        n_rounds, n_competitors, _ = trace.dims()
        html = sclp.animate_competition_canvas(
            trace,
            voters=IC_VOTERS,
            compute_ic=True,
            ic_num_samples=16,
        )
    payload = _extract_payload(html)

    assert "overlays_frames" in payload
    assert "indifference_curves" not in payload
    assert "ic_competitor_indices" not in payload

    n_frames  = n_rounds + 1   # 3
    n_voters  = len(IC_VOTERS)  # 3
    frame_overlays = payload["overlays_frames"]

    assert len(frame_overlays) == n_frames

    for f in range(n_frames):
        canonical_entries = frame_overlays[f]["indifference_curves"]

        assert len(canonical_entries) >= 1

        for s_i, entry in enumerate(canonical_entries):
            voter_curves = entry["curves"]
            assert len(voter_curves) == n_voters
            assert 0 <= entry["candidate"] < n_competitors

            for curve in voter_curves:
                ring = curve["ring"]
                assert len(ring) == 16
                assert all(len(pt) == 2 for pt in ring)
                assert all(isinstance(coord, float) for pt in ring for coord in pt)


def test_compute_ic_max_curves_exceeded_raises():
    with _ic_trace_2d() as trace:
        # 3 voters × 1 seat × 3 frames = 9 > ic_max_curves = 5.
        with pytest.raises(ValueError, match="ic_max_curves"):
            sclp.animate_competition_canvas(
                trace,
                voters=IC_VOTERS,
                compute_ic=True,
                ic_max_curves=5,
            )


def test_compute_ic_without_voters_raises():
    with _ic_trace_2d() as trace:
        with pytest.raises(ValueError, match="voters"):
            sclp.animate_competition_canvas(trace, compute_ic=True)


def test_compute_ic_num_samples_controls_vertex_count():
    with _ic_trace_2d() as trace:
        html8  = sclp.animate_competition_canvas(
            trace, voters=IC_VOTERS, compute_ic=True, ic_num_samples=8
        )
        html32 = sclp.animate_competition_canvas(
            trace, voters=IC_VOTERS, compute_ic=True, ic_num_samples=32
        )
    p8  = _first_ic_ring(_extract_payload(html8))
    p32 = _first_ic_ring(_extract_payload(html32))
    assert len(p8)  == 8
    assert len(p32) == 32


def test_euclidean_ic_polygon_vertices_are_equidistant_from_voter():
    """Geometric invariant: under Euclidean distance every vertex of an IC
    polygon must be exactly as far from the voter's ideal as the seat holder.
    This is what makes the polygon a true circle.  Combined with equal-aspect
    rendering in the JS canvas, it guarantees circles appear on screen.
    If this test breaks, the binding layer is computing incorrect polygons."""
    with _ic_trace_2d() as trace:
        html = sclp.animate_competition_canvas(
            trace, voters=IC_VOTERS, compute_ic=True, ic_num_samples=64
        )
    payload = _extract_payload(html)
    ic = payload["overlays_frames"]
    for frame in ic:
        for seat_entry in frame["indifference_curves"]:
            for v_i, curve in enumerate(seat_entry["curves"]):
                ring = curve["ring"]
                xs = np.array([pt[0] for pt in ring])
                ys = np.array([pt[1] for pt in ring])
                voter_x = float(IC_VOTERS[v_i, 0])
                voter_y = float(IC_VOTERS[v_i, 1])
                dists = np.sqrt((xs - voter_x) ** 2 + (ys - voter_y) ** 2)
                assert np.allclose(dists, dists[0], atol=1e-10), (
                    f"Voter {v_i} IC polygon is not equidistant: "
                    f"range {dists.max() - dists.min():.2e}"
                )


def test_compute_ic_manhattan_produces_4_vertex_polygon():
    """Manhattan distance IC is a rotated square — exactly 4 vertices."""
    manh_dist = scl.DistanceConfig(
        distance_type="manhattan", salience_weights=[1.0, 1.0]
    )
    with scl.competition_run(
        _IC_COMPETITORS,
        ["sticker", "sticker"],
        IC_VOTERS,
        dist_config=manh_dist,
        engine_config=_IC_ENGINE_CFG,
    ) as trace:
        html = sclp.animate_competition_canvas(
            trace,
            voters=IC_VOTERS,
            compute_ic=True,
            ic_dist_config=manh_dist,
            ic_num_samples=32,
        )
    ring = _first_ic_ring(_extract_payload(html))
    # Polygon type returns 4 exact vertices regardless of ic_num_samples.
    assert len(ring) == 4
