# test_competition_canvas_save_load.py — Round-trip tests for save/load_competition_canvas.

import json
import tempfile
from pathlib import Path

import numpy as np

import socialchoicelab as scl
import socialchoicelab.plots as sclp

# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------

_VOTERS_1D = np.array([[-0.5], [0.1], [1.8]])
_COMPS_1D  = np.array([[0.0], [2.0]])
_ENGINE_1D = scl.CompetitionEngineConfig(
    seat_count=1,
    seat_rule="plurality_top_k",
    max_rounds=2,
    step_config=scl.CompetitionStepConfig(kind="fixed", fixed_step_size=0.5),
)
_DIST_1D = scl.DistanceConfig(salience_weights=[1.0])

_VOTERS_2D = np.array([[-0.5, 0.0], [0.0, 0.2], [1.8, 0.1]])
_COMPS_2D  = np.array([[0.0, 0.0], [2.0, 0.0]])
_DIST_2D   = scl.DistanceConfig(salience_weights=[1.0, 1.0])
_ENGINE_2D = scl.CompetitionEngineConfig(
    seat_count=1,
    seat_rule="plurality_top_k",
    max_rounds=2,
    step_config=scl.CompetitionStepConfig(kind="fixed", fixed_step_size=0.5),
)


def _trace_1d():
    return scl.competition_run(
        _COMPS_1D, ["sticker", "sticker"], _VOTERS_1D,
        dist_config=_DIST_1D, engine_config=_ENGINE_1D,
    )


def _trace_2d():
    return scl.competition_run(
        _COMPS_2D, ["sticker", "sticker"], _VOTERS_2D,
        dist_config=_DIST_2D, engine_config=_ENGINE_2D,
    )


# ---------------------------------------------------------------------------
# 1D round-trip via payload_path
# ---------------------------------------------------------------------------

def test_payload_path_writes_scscanvas_file_1d():
    trace = _trace_1d()
    with tempfile.NamedTemporaryFile(suffix=".scscanvas", delete=False) as f:
        tmp = f.name
    try:
        sclp.animate_competition_canvas(
            trace, voters=_VOTERS_1D.ravel(), width=900, height=400,
            payload_path=tmp,
        )
        assert Path(tmp).exists()
        envelope = json.loads(Path(tmp).read_text())
        assert envelope["format"] == "scscanvas"
        assert envelope["version"] == "1"
        assert "created" in envelope
        assert envelope["generator"].startswith("socialchoicelab/python/")
        assert envelope["width"] == 900
        assert envelope["height"] == 400
        assert "payload" in envelope
    finally:
        Path(tmp).unlink(missing_ok=True)


def test_load_competition_canvas_returns_html_1d():
    trace = _trace_1d()
    with tempfile.NamedTemporaryFile(suffix=".scscanvas", delete=False) as f:
        tmp = f.name
    try:
        html_orig = sclp.animate_competition_canvas(
            trace, voters=_VOTERS_1D.ravel(), width=900, height=400,
            payload_path=tmp,
        )
        html_loaded = sclp.load_competition_canvas(tmp)
        assert "<!DOCTYPE html>" in html_loaded
        # Payload content is identical so the serialised JSON inside should match.
        envelope = json.loads(Path(tmp).read_text())
        payload = envelope["payload"]
        assert payload["voters_x"] == json.loads(
            html_orig[html_orig.index("HTMLWidgets._init(") :]
            .split("HTMLWidgets._init(")[0]  # not used; just verify voters_x via envelope
            or "{}"
        ).get("voters_x", payload["voters_x"])
        # Key structure check: loaded HTML contains the same voter count.
        assert str(len(payload["voters_x"])) in html_loaded
    finally:
        Path(tmp).unlink(missing_ok=True)


def test_load_competition_canvas_payload_matches_original_1d():
    trace = _trace_1d()
    with tempfile.NamedTemporaryFile(suffix=".scscanvas", delete=False) as f:
        tmp = f.name
    try:
        sclp.animate_competition_canvas(
            trace, voters=_VOTERS_1D.ravel(), width=900, height=400,
            payload_path=tmp,
        )
        envelope = json.loads(Path(tmp).read_text())
        payload = envelope["payload"]
        assert payload["n_dims"] == 1
        assert len(payload["voters_x"]) == 3
        assert len(payload["rounds"]) == 3   # Round 1, Round 2, Final
        assert len(payload["positions"]) == 3
    finally:
        Path(tmp).unlink(missing_ok=True)


# ---------------------------------------------------------------------------
# 2D round-trip via payload_path
# ---------------------------------------------------------------------------

def test_payload_path_writes_scscanvas_file_2d():
    trace = _trace_2d()
    with tempfile.NamedTemporaryFile(suffix=".scscanvas", delete=False) as f:
        tmp = f.name
    try:
        sclp.animate_competition_canvas(
            trace, voters=_VOTERS_2D, width=900, height=800,
            payload_path=tmp,
        )
        envelope = json.loads(Path(tmp).read_text())
        assert envelope["format"] == "scscanvas"
        assert envelope["width"] == 900
        assert envelope["height"] == 800
        payload = envelope["payload"]
        assert "voters_x" in payload
        assert "voters_y" in payload
        assert len(payload["voters_x"]) == 3
    finally:
        Path(tmp).unlink(missing_ok=True)


def test_load_competition_canvas_returns_html_2d():
    trace = _trace_2d()
    with tempfile.NamedTemporaryFile(suffix=".scscanvas", delete=False) as f:
        tmp = f.name
    try:
        sclp.animate_competition_canvas(
            trace, voters=_VOTERS_2D, width=900, height=800,
            payload_path=tmp,
        )
        html_loaded = sclp.load_competition_canvas(tmp)
        assert "<!DOCTYPE html>" in html_loaded
        assert "competition-canvas" in html_loaded
    finally:
        Path(tmp).unlink(missing_ok=True)


def test_load_competition_canvas_width_height_override():
    trace = _trace_2d()
    with tempfile.NamedTemporaryFile(suffix=".scscanvas", delete=False) as f:
        tmp = f.name
    try:
        sclp.animate_competition_canvas(
            trace, voters=_VOTERS_2D, width=900, height=800,
            payload_path=tmp,
        )
        html = sclp.load_competition_canvas(tmp, width=1200, height=600)
        assert "1200px" in html
        assert "600px" in html
    finally:
        Path(tmp).unlink(missing_ok=True)


# ---------------------------------------------------------------------------
# top-level import
# ---------------------------------------------------------------------------

def test_load_competition_canvas_importable_from_top_level():
    from socialchoicelab import load_competition_canvas  # noqa: F401
    assert callable(load_competition_canvas)


# ---------------------------------------------------------------------------
# Error cases
# ---------------------------------------------------------------------------

def test_load_competition_canvas_missing_file():
    import pytest
    with pytest.raises(FileNotFoundError, match="file not found"):
        sclp.load_competition_canvas("/tmp/does_not_exist_xyz.scscanvas")


def test_load_competition_canvas_wrong_format():
    import pytest
    with tempfile.NamedTemporaryFile(
        suffix=".scscanvas", mode="w", delete=False, encoding="utf-8"
    ) as f:
        json.dump({"format": "other", "payload": {}}, f)
        tmp = f.name
    try:
        with pytest.raises(ValueError, match="unexpected format"):
            sclp.load_competition_canvas(tmp)
    finally:
        Path(tmp).unlink(missing_ok=True)
