# test_competition_canvas_voronoi.py — Unit tests for animate_competition_canvas Voronoi feature.

import json
import re

import numpy as np
import pytest

import socialchoicelab as scl
import socialchoicelab.plots as sclp

# Same fixture as winset tests: 3 2D voters, 2 sticker competitors, 1 seat, 2 rounds.
WS_VOTERS = np.array([[-0.5, 0.0], [0.0, 0.2], [1.8, 0.1]])
_WS_COMPETITORS = np.array([[0.0, 0.0], [2.0, 0.0]])
_WS_ENGINE_CFG = scl.CompetitionEngineConfig(
    seat_count=1,
    seat_rule="plurality_top_k",
    max_rounds=2,
    step_config=scl.CompetitionStepConfig(kind="fixed", fixed_step_size=0.5),
)


def _ws_trace_2d():
    return scl.competition_run(
        _WS_COMPETITORS,
        ["sticker", "sticker"],
        WS_VOTERS,
        dist_config=scl.DistanceConfig(salience_weights=[1.0, 1.0]),
        engine_config=_WS_ENGINE_CFG,
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


def test_compute_voronoi_false_leaves_key_absent():
    with _ws_trace_2d() as trace:
        html = sclp.animate_competition_canvas(trace, voters=WS_VOTERS)
    payload = _extract_payload(html)
    assert "voronoi_cells" not in payload


def test_compute_voronoi_true_produces_correctly_shaped_payload():
    with _ws_trace_2d() as trace:
        n_rounds, n_competitors, _ = trace.dims()
        html = sclp.animate_competition_canvas(
            trace,
            voters=WS_VOTERS,
            compute_voronoi=True,
        )
    payload = _extract_payload(html)

    assert "overlays_frames" in payload
    assert "voronoi_cells" not in payload
    frame_overlays = payload["overlays_frames"]
    n_frames = n_rounds + 1
    assert len(frame_overlays) == n_frames

    for frame_i in range(n_frames):
        canonical_entries = frame_overlays[frame_i]["candidate_regions"]
        assert len(canonical_entries) == n_competitors
        for entry in canonical_entries:
            assert "polygons" in entry
            assert "candidate" in entry
            assert isinstance(entry["polygons"], list)
            assert len(entry["polygons"]) >= 1
            assert 0 <= entry["candidate"] < n_competitors


def test_compute_voronoi_non_euclidean_raises():
    with _ws_trace_2d() as trace:
        with pytest.raises(ValueError, match="Euclidean.*uniform"):
            sclp.animate_competition_canvas(
                trace,
                voters=WS_VOTERS,
                compute_voronoi=True,
                voronoi_dist_config=scl.DistanceConfig(
                    distance_type="manhattan",
                    salience_weights=[1.0, 1.0],
                ),
            )
    with _ws_trace_2d() as trace:
        with pytest.raises(ValueError, match="Euclidean.*uniform"):
            sclp.animate_competition_canvas(
                trace,
                voters=WS_VOTERS,
                compute_voronoi=True,
                voronoi_dist_config=scl.DistanceConfig(
                    salience_weights=[2.0, 1.0],
                ),
            )
