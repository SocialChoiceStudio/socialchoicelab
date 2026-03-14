# test_competition_canvas_winset.py — Unit tests for animate_competition_canvas WinSet feature.

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


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------


def test_compute_winset_false_leaves_key_absent():
    with _ws_trace_2d() as trace:
        html = sclp.animate_competition_canvas(trace, voters=WS_VOTERS)
    payload = _extract_payload(html)
    assert "winsets" not in payload
    assert "winset_competitor_indices" not in payload


def test_compute_winset_true_produces_correctly_shaped_payload():
    with _ws_trace_2d() as trace:
        n_rounds, n_competitors, _ = trace.dims()
        html = sclp.animate_competition_canvas(
            trace,
            voters=WS_VOTERS,
            compute_winset=True,
        )
    payload = _extract_payload(html)

    assert "winsets" in payload
    assert "winset_competitor_indices" in payload

    n_frames = n_rounds + 1  # 3
    ws = payload["winsets"]
    ws_idx = payload["winset_competitor_indices"]

    assert len(ws) == n_frames
    assert len(ws_idx) == n_frames

    for f in range(n_frames):
        frame_ws = ws[f]
        frame_idx = ws_idx[f]
        assert len(frame_ws) >= 1
        assert len(frame_ws) == len(frame_idx)


def test_compute_winset_without_voters_raises():
    with _ws_trace_2d() as trace:
        with pytest.raises(ValueError, match="voters"):
            sclp.animate_competition_canvas(trace, compute_winset=True)


def test_compute_winset_max_exceeded_raises():
    with _ws_trace_2d() as trace:
        # 1 seat × 3 frames = 3 > winset_max = 1.
        with pytest.raises(ValueError, match="winset_max"):
            sclp.animate_competition_canvas(
                trace,
                voters=WS_VOTERS,
                compute_winset=True,
                winset_max=1,
            )


def test_non_empty_winset_entry_has_required_keys():
    with _ws_trace_2d() as trace:
        n_rounds, n_competitors, _ = trace.dims()
        html = sclp.animate_competition_canvas(
            trace,
            voters=WS_VOTERS,
            compute_winset=True,
        )
    payload = _extract_payload(html)
    ws = payload["winsets"]

    # Find the first non-null entry.
    found = False
    for frame_ws in ws:
        for entry in frame_ws:
            if entry is not None:
                assert "paths" in entry
                assert "is_hole" in entry
                assert "competitor_idx" in entry

                # paths is a non-empty list; each path is flat [x,y,...].
                assert len(entry["paths"]) >= 1
                first_path = entry["paths"][0]
                assert isinstance(first_path, list)
                assert len(first_path) >= 4
                assert all(isinstance(v, float) for v in first_path)

                # is_hole length matches paths length.
                assert len(entry["is_hole"]) == len(entry["paths"])

                # competitor_idx is 0-based.
                assert 0 <= entry["competitor_idx"] < n_competitors

                found = True
                break
        if found:
            break

    assert found, "No non-null winset entry found in payload."


def test_winset_num_samples_affects_boundary_vertex_count():
    with _ws_trace_2d() as trace:
        html16 = sclp.animate_competition_canvas(
            trace, voters=WS_VOTERS, compute_winset=True, winset_num_samples=16
        )
        html64 = sclp.animate_competition_canvas(
            trace, voters=WS_VOTERS, compute_winset=True, winset_num_samples=64
        )

    def _first_path_len(ws_payload):
        for frame_ws in ws_payload:
            for entry in frame_ws:
                if entry is not None and entry["paths"]:
                    return len(entry["paths"][0])
        return None

    ws16 = _extract_payload(html16)["winsets"]
    ws64 = _extract_payload(html64)["winsets"]
    len16 = _first_path_len(ws16)
    len64 = _first_path_len(ws64)
    assert len16 is not None, "No non-null winset path found at num_samples=16."
    assert len64 is not None, "No non-null winset path found at num_samples=64."
    # Higher num_samples should produce more vertices (or at least not fewer).
    assert len64 >= len16
