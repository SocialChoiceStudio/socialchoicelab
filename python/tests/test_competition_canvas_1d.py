# test_competition_canvas_1d.py — Unit tests for 1D animate_competition_canvas.

import json
import re
import warnings

import numpy as np

import socialchoicelab as scl
import socialchoicelab.plots as sclp

# ---------------------------------------------------------------------------
# Fixture helpers
# ---------------------------------------------------------------------------

# 3 1D voters, 2 sticker competitors, 1 seat, 2 rounds.
# Competitor 0 at x=0 wins every round (it is the closest to 2 of 3 voters).
# n_frames = 3 (Round 1, Round 2, Final); 1 seat holder per frame.
#
# competition_run requires voters as a (n, n_dims) array for Python (it cannot
# infer n_dims from a flat 1D array without an explicit parameter).

_VOTERS_1D   = np.array([[-0.5], [0.1], [1.8]])  # shape (3, 1)
_COMPS_1D    = np.array([[ 0.0], [ 2.0]])         # shape (2, 1)
_ENGINE_1D   = scl.CompetitionEngineConfig(
    seat_count=1,
    seat_rule="plurality_top_k",
    max_rounds=2,
    step_config=scl.CompetitionStepConfig(kind="fixed", fixed_step_size=0.5),
)
_DIST_1D     = scl.DistanceConfig(salience_weights=[1.0])


def _trace_1d():
    return scl.competition_run(
        _COMPS_1D,
        ["sticker", "sticker"],
        _VOTERS_1D,
        dist_config=_DIST_1D,
        engine_config=_ENGINE_1D,
    )


def _extract_payload(html: str) -> dict:
    match = re.search(
        r"HTMLWidgets\._init\(.*?\d+,\s*\d+,\s*(\{.*\})\s*\);",
        html,
        re.DOTALL,
    )
    assert match is not None, "Could not find JSON payload in HTML."
    return json.loads(match.group(1))


# ---------------------------------------------------------------------------
# Payload shape tests
# ---------------------------------------------------------------------------


def test_1d_payload_has_n_dims_key():
    with _trace_1d() as trace:
        html = sclp.animate_competition_canvas(trace, voters=_VOTERS_1D)
    payload = _extract_payload(html)
    assert payload["n_dims"] == 1


def test_1d_payload_has_voters_x_not_voters_y():
    with _trace_1d() as trace:
        html = sclp.animate_competition_canvas(trace, voters=_VOTERS_1D)
    payload = _extract_payload(html)
    assert "voters_x" in payload
    assert "voters_y" not in payload
    assert len(payload["voters_x"]) == 3


def test_1d_payload_positions_are_length1_lists():
    with _trace_1d() as trace:
        html = sclp.animate_competition_canvas(trace, voters=_VOTERS_1D)
    payload = _extract_payload(html)
    n_frames = len(payload["positions"])
    n_comp   = len(payload["positions"][0])
    for f in range(n_frames):
        for c in range(n_comp):
            inner = payload["positions"][f][c]
            assert isinstance(inner, list), f"positions[{f}][{c}] is not a list"
            assert len(inner) == 1, f"positions[{f}][{c}] has {len(inner)} elements, expected 1"


def test_1d_payload_no_ylim():
    with _trace_1d() as trace:
        html = sclp.animate_competition_canvas(trace, voters=_VOTERS_1D)
    payload = _extract_payload(html)
    assert "ylim" not in payload


def test_1d_payload_no_winsets():
    with _trace_1d() as trace:
        html = sclp.animate_competition_canvas(
            trace, voters=_VOTERS_1D, compute_winset=False
        )
    payload = _extract_payload(html)
    assert "winsets" not in payload


def test_1d_payload_dim_names_length1():
    with _trace_1d() as trace:
        html = sclp.animate_competition_canvas(
            trace, voters=_VOTERS_1D, dim_names=("Policy",)
        )
    payload = _extract_payload(html)
    assert payload["dim_names"] == ["Policy"]


def test_1d_payload_xlim_covers_all_positions():
    with _trace_1d() as trace:
        html = sclp.animate_competition_canvas(trace, voters=_VOTERS_1D)
    payload = _extract_payload(html)
    xlim = payload["xlim"]
    assert xlim[0] <= min(_VOTERS_1D.min(), _COMPS_1D.min())
    assert xlim[1] >= max(_VOTERS_1D.max(), _COMPS_1D.max())


# ---------------------------------------------------------------------------
# IC tests
# ---------------------------------------------------------------------------


def test_1d_no_ic_by_default():
    with _trace_1d() as trace:
        html = sclp.animate_competition_canvas(trace, voters=_VOTERS_1D)
    payload = _extract_payload(html)
    assert "indifference_curves" not in payload
    assert "ic_competitor_indices" not in payload


def test_1d_ic_shape():
    with _trace_1d() as trace:
        html = sclp.animate_competition_canvas(
            trace, voters=_VOTERS_1D, compute_ic=True
        )
    payload = _extract_payload(html)
    assert "indifference_curves" in payload
    ic      = payload["indifference_curves"]
    n_frames = len(payload["positions"])   # 3
    n_voters = len(payload["voters_x"])    # 3
    assert len(ic) == n_frames
    for frame in ic:
        # one seat holder per frame (competitor 0 always wins)
        assert len(frame) == 1
        voter_curves = frame[0]
        assert len(voter_curves) == n_voters
        for pts in voter_curves:
            # each IC is 0, 1, or 2 floats
            assert isinstance(pts, list)
            assert len(pts) <= 2


def test_1d_ic_boundaries_are_symmetric():
    """For linear loss + unit weight, level set of voter at x=0.5 at utility -0.5
    should span [0.0, 1.0] — symmetric around the ideal point."""
    voters = np.array([[0.5]])   # shape (1, 1) — voter at 0.5
    comp   = np.array([[0.0]])   # shape (1, 1) — seat at 0.0

    trace = scl.competition_run(
        comp,
        ["sticker"],
        voters,
        dist_config=scl.DistanceConfig(salience_weights=[1.0]),
        engine_config=scl.CompetitionEngineConfig(
            seat_count=1,
            seat_rule="plurality_top_k",
            max_rounds=1,
            step_config=scl.CompetitionStepConfig(kind="fixed", fixed_step_size=0.0),
        ),
    )
    with trace:
        html = sclp.animate_competition_canvas(
            trace,
            voters=voters,
            compute_ic=True,
            ic_loss_config=scl.LossConfig(loss_type="linear", sensitivity=1.0),
            ic_dist_config=scl.DistanceConfig(salience_weights=[1.0]),
        )
    payload = _extract_payload(html)
    # First frame, first (only) seat holder, first (only) voter
    pts = payload["indifference_curves"][0][0][0]
    assert len(pts) == 2, f"Expected 2 boundary points, got {pts}"
    a, b = sorted(pts)
    assert abs(a - 0.0) < 1e-9, f"Left boundary {a} != 0.0"
    assert abs(b - 1.0) < 1e-9, f"Right boundary {b} != 1.0"


# ---------------------------------------------------------------------------
# 1D overlay tests
# ---------------------------------------------------------------------------


def test_1d_overlays_centroid_and_median():
    with _trace_1d() as trace:
        html = sclp.animate_competition_canvas(
            trace,
            voters=_VOTERS_1D,
            overlays={"centroid": True, "marginal_median": True},
        )
    payload = _extract_payload(html)
    ovs = payload["overlays_static"]
    assert "centroid" in ovs
    assert "marginal_median" in ovs
    expected_mean   = float(np.mean(_VOTERS_1D))
    expected_median = float(np.median(_VOTERS_1D))
    assert abs(ovs["centroid"]["x"] - expected_mean) < 1e-9
    assert abs(ovs["marginal_median"]["x"] - expected_median) < 1e-9


def test_1d_unsupported_overlay_warns_and_is_absent():
    with _trace_1d() as trace:
        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")
            html = sclp.animate_competition_canvas(
                trace,
                voters=_VOTERS_1D,
                overlays={"pareto_set": True},
            )
    assert any("pareto_set" in str(warning.message) for warning in w)
    payload = _extract_payload(html)
    ovs = payload.get("overlays_static", {})
    assert "pareto_set" not in ovs


# ---------------------------------------------------------------------------
# WinSet 1D tests
# ---------------------------------------------------------------------------


def test_1d_winset_intervals_absent_when_disabled(tmp_path):
    with _trace_1d() as trace:
        html = sclp.animate_competition_canvas(
            trace, voters=_VOTERS_1D, width=900, height=400,
            path=str(tmp_path / "out.html"), compute_winset=False,
        )
    payload = _extract_payload(html)
    assert "winset_intervals_1d" not in payload


def test_1d_winset_intervals_present_and_correct_shape(tmp_path):
    with _trace_1d() as trace:
        html = sclp.animate_competition_canvas(
            trace, voters=_VOTERS_1D, width=900, height=400,
            path=str(tmp_path / "out.html"), compute_winset=True,
        )
    payload = _extract_payload(html)
    ws = payload.get("winset_intervals_1d")
    assert ws is not None, "winset_intervals_1d key missing"
    # 3 frames: Round 1, Round 2, Final
    assert len(ws) == 3
    for frame in ws:
        for entry in frame:
            assert "lo" in entry and "hi" in entry and "competitor_idx" in entry


def test_1d_winset_lo_le_hi_when_non_empty(tmp_path):
    with _trace_1d() as trace:
        html = sclp.animate_competition_canvas(
            trace, voters=_VOTERS_1D, width=900, height=400,
            path=str(tmp_path / "out.html"), compute_winset=True,
        )
    payload = _extract_payload(html)
    ws = payload["winset_intervals_1d"]
    import math
    for frame in ws:
        for entry in frame:
            lo, hi = entry["lo"], entry["hi"]
            if not math.isnan(lo) and not math.isnan(hi):
                assert lo <= hi, f"lo={lo} > hi={hi}"


# ---------------------------------------------------------------------------
# Error / guard tests
# ---------------------------------------------------------------------------
# Note: the C engine only supports 1D and 2D, so a 3D guard test cannot be
# written without a mock — omitted intentionally.
