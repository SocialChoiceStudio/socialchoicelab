"""test_app.py — Smoke tests for the hosted Shiny for Python MVP app.

These tests import and exercise app.py's pure-logic pieces (request
building, scenario filtering) without starting a real server or browser,
so they run in CI. They do not replace the manual/browser verification
documented in docs/architecture/hosted_mvp_design.md: Shiny's reactive
rendering, the iframe-embedded canvas, and the download endpoint are
only exercised there.
"""

import os
import sys

import pytest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import app as mvp_app  # noqa: E402


def test_app_module_builds_a_shiny_app():
    assert mvp_app.app is not None


def test_scenarios_with_status_quo_excludes_scenarios_without_one():
    import socialchoicelab as scl

    usable = mvp_app._scenarios_with_status_quo()
    assert len(usable) > 0
    assert len(usable) < len(scl.list_scenarios())
    for name in usable:
        assert scl.load_scenario(name)["status_quo"] is not None
    assert "laing_olmsted_bear" in usable


class _FakeInput:
    """Minimal stand-in for Shiny's `input` reactive object in tests."""

    def __init__(self, **values):
        self._values = values

    def __getattr__(self, name):
        if name not in self._values:
            raise AttributeError(name)
        return lambda: self._values[name]


def test_build_request_scenario_mode():
    inp = _FakeInput(
        rule_kind="majority",
        overlays=("pareto",),
        input_mode="scenario",
        scenario_name="laing_olmsted_bear",
    )
    request = mvp_app._build_request(inp)
    assert request["scenario_name"] == "laing_olmsted_bear"
    assert request["decision_rule"] == {"kind": "majority"}
    assert request["overlays"]["pareto"] is True
    assert request["overlays"]["indifference_curves"] is False


def test_build_request_manual_mode_parses_voters_and_status_quo():
    inp = _FakeInput(
        rule_kind="supermajority",
        supermajority_fraction=0.75,
        overlays=(),
        input_mode="manual",
        manual_voters_text="0,0\n2,0\n1,2",
        manual_sq_text="1,0.5",
    )
    request = mvp_app._build_request(inp)
    assert request["manual_voters"] == [[0.0, 0.0], [2.0, 0.0], [1.0, 2.0]]
    assert request["manual_status_quo"] == [1.0, 0.5]
    assert request["decision_rule"] == {"kind": "supermajority", "fraction": 0.75}


def test_build_request_manual_mode_rejects_malformed_voter_line():
    inp = _FakeInput(
        rule_kind="majority",
        overlays=(),
        input_mode="manual",
        manual_voters_text="0,0\nabc,2",
        manual_sq_text="1,0.5",
    )
    with pytest.raises(ValueError, match="not a number"):
        mvp_app._build_request(inp)


def test_build_request_manual_mode_rejects_malformed_status_quo():
    inp = _FakeInput(
        rule_kind="majority",
        overlays=(),
        input_mode="manual",
        manual_voters_text="0,0\n2,0",
        manual_sq_text="1",
    )
    with pytest.raises(ValueError, match="Status quo"):
        mvp_app._build_request(inp)
