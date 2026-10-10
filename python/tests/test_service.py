"""test_service.py — Tests for the nontechnical-user-release application service layer."""

import json

import pytest

import socialchoicelab as scl


def test_scenario_mode_returns_json_serializable_result():
    result = scl.run_spatial_voting_mvp({
        "scenario_name": "laing_olmsted_bear",
    })
    assert set(result.keys()) == {"canvas", "summary"}
    # The whole result must be JSON round-trippable: this is the
    # contract a hosted web front end relies on.
    json.dumps(result)
    assert result["summary"]["n_voters"] == 5
    assert result["summary"]["decision_rule"]["kind"] == "majority"
    assert result["canvas"]["_scs_spatial_canvas"] is True
    assert "winset_paths" in result["canvas"]["layers"]


def test_manual_mode_computes_winset_without_a_bundled_scenario():
    result = scl.run_spatial_voting_mvp({
        "manual_voters": [[0.0, 0.0], [2.0, 0.0], [1.0, 2.0]],
        "manual_status_quo": [0.0, 0.0],
        "decision_rule": {"kind": "majority"},
    })
    assert result["summary"]["n_voters"] == 3
    assert result["summary"]["source"] == "Manual entry"
    json.dumps(result)


def test_supermajority_decision_rule_reports_k_and_fraction():
    result = scl.run_spatial_voting_mvp({
        "scenario_name": "laing_olmsted_bear",
        "decision_rule": {"kind": "supermajority", "fraction": 0.8},
    })
    rule = result["summary"]["decision_rule"]
    assert rule["kind"] == "supermajority"
    assert rule["fraction"] == 0.8
    assert rule["k"] == 4  # ceil(0.8 * 5)


def test_overlays_add_expected_canvas_layers():
    result = scl.run_spatial_voting_mvp({
        "scenario_name": "laing_olmsted_bear",
        "overlays": {
            "pareto": True,
            "indifference_curves": True,
            "preferred_regions": True,
        },
    })
    layers = result["canvas"]["layers"]
    assert "convex_hull_xy" in layers
    assert "ic_curves" in layers
    assert "preferred_regions" in layers


def test_rejects_both_scenario_and_manual_voters():
    with pytest.raises(ValueError, match="not both"):
        scl.run_spatial_voting_mvp({
            "scenario_name": "laing_olmsted_bear",
            "manual_voters": [[0.0, 0.0], [1.0, 1.0]],
        })


def test_rejects_neither_scenario_nor_manual_voters():
    with pytest.raises(ValueError, match="neither"):
        scl.run_spatial_voting_mvp({})


def test_rejects_unknown_scenario_name_with_available_list():
    with pytest.raises(ValueError, match="unknown scenario"):
        scl.run_spatial_voting_mvp({"scenario_name": "not_a_real_scenario"})


def test_manual_voters_require_status_quo():
    with pytest.raises(ValueError, match="manual_status_quo"):
        scl.run_spatial_voting_mvp({
            "manual_voters": [[0.0, 0.0], [1.0, 1.0]],
        })


def test_manual_voters_require_at_least_two_voters():
    with pytest.raises(ValueError, match="at least"):
        scl.run_spatial_voting_mvp({
            "manual_voters": [[0.0, 0.0]],
            "manual_status_quo": [0.0, 0.0],
        })


def test_rejects_supermajority_fraction_out_of_range():
    with pytest.raises(ValueError, match="fraction"):
        scl.run_spatial_voting_mvp({
            "scenario_name": "laing_olmsted_bear",
            "decision_rule": {"kind": "supermajority", "fraction": 0.3},
        })


def test_rejects_unknown_overlay_key():
    with pytest.raises(ValueError, match="unknown overlay"):
        scl.run_spatial_voting_mvp({
            "scenario_name": "laing_olmsted_bear",
            "overlays": {"yolk": True},
        })


def test_rejects_non_dict_request():
    with pytest.raises(ValueError, match="dict"):
        scl.run_spatial_voting_mvp("not a dict")
