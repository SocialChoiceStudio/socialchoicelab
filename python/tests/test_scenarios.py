"""tests/test_scenarios.py — Tests for scenario loading."""

import pytest
import numpy as np
import socialchoicelab as scl


def test_list_scenarios_returns_list():
    """list_scenarios() returns a non-empty sorted list of strings."""
    scenarios = scl.list_scenarios()
    assert isinstance(scenarios, list)
    assert len(scenarios) > 0
    assert all(isinstance(s, str) for s in scenarios)
    assert scenarios == sorted(scenarios)


def test_list_scenarios_includes_builtins():
    """Built-in scenarios are included."""
    scenarios = scl.list_scenarios()
    assert "laing_olmsted_bear" in scenarios
    assert "tovey_regular_polygon" in scenarios


def test_load_scenario_returns_dict():
    """load_scenario() returns a dict with expected keys."""
    sc = scl.load_scenario("laing_olmsted_bear")
    assert isinstance(sc, dict)
    assert "name" in sc
    assert "description" in sc
    assert "source" in sc
    assert "n_voters" in sc
    assert "n_dimensions" in sc
    assert "space" in sc
    assert "decision_rule" in sc
    assert "dim_names" in sc
    assert "voters" in sc
    assert "voter_names" in sc
    assert "status_quo" in sc


def test_load_scenario_types():
    """Returned values have correct types."""
    sc = scl.load_scenario("laing_olmsted_bear")
    assert isinstance(sc["name"], str)
    assert isinstance(sc["description"], str)
    assert isinstance(sc["source"], str)
    assert isinstance(sc["n_voters"], int)
    assert isinstance(sc["n_dimensions"], int)
    assert isinstance(sc["decision_rule"], float)
    assert isinstance(sc["dim_names"], list)
    assert isinstance(sc["voters"], np.ndarray)
    assert isinstance(sc["voter_names"], list)
    assert isinstance(sc["status_quo"], np.ndarray)


def test_load_scenario_laing_olmsted_bear_dimensions():
    """laing_olmsted_bear scenario has correct dimensions."""
    sc = scl.load_scenario("laing_olmsted_bear")
    assert sc["n_voters"] == 5
    assert sc["n_dimensions"] == 2
    assert len(sc["voters"]) == 10  # 5 voters * 2 dims
    assert len(sc["voter_names"]) == 5
    assert len(sc["status_quo"]) == 2
    assert len(sc["dim_names"]) == 2


def test_load_scenario_laing_olmsted_bear_data():
    """laing_olmsted_bear scenario has correct data values."""
    sc = scl.load_scenario("laing_olmsted_bear")
    # First voter: [20.8, 79.6]
    assert np.allclose(sc["voters"][:2], [20.8, 79.6])
    # Status quo: [100.0, 100.0]
    assert np.allclose(sc["status_quo"], [100.0, 100.0])
    # Decision rule: 0.5
    assert sc["decision_rule"] == 0.5


def test_load_scenario_tovey_regular_polygon_dimensions():
    """tovey_regular_polygon scenario has correct dimensions."""
    sc = scl.load_scenario("tovey_regular_polygon")
    assert sc["n_voters"] == 11
    assert sc["n_dimensions"] == 2
    assert len(sc["voters"]) == 22  # 11 voters * 2 dims
    assert len(sc["voter_names"]) == 11


def test_load_scenario_unknown_raises_error():
    """load_scenario() raises ValueError for unknown scenario."""
    with pytest.raises(ValueError, match="Unknown scenario"):
        scl.load_scenario("nonexistent_scenario")


def test_load_scenario_space_ranges():
    """Scenario space contains x_range and y_range."""
    sc = scl.load_scenario("laing_olmsted_bear")
    assert isinstance(sc["space"], dict)
    assert "x_range" in sc["space"]
    assert "y_range" in sc["space"]
    assert sc["space"]["x_range"] == [0, 200]
    assert sc["space"]["y_range"] == [0, 200]


def test_load_scenario_voter_names():
    """voter_names are correctly loaded."""
    sc = scl.load_scenario("laing_olmsted_bear")
    assert sc["voter_names"] == ["P1", "P2", "P3", "P4", "P5"]

    sc2 = scl.load_scenario("tovey_regular_polygon")
    assert sc2["voter_names"] == [f"XX{i}" for i in range(11)]
