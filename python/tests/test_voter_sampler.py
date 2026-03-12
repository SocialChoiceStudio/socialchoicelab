import numpy as np
import pytest

import socialchoicelab as scl
from socialchoicelab import VoterSamplerConfig, draw_voters, make_voter_sampler


# ==========================================================================
# make_voter_sampler
# ==========================================================================


def test_make_voter_sampler_gaussian_config():
    s = make_voter_sampler("gaussian", mean=0.5, stddev=0.3)
    assert isinstance(s, VoterSamplerConfig)
    assert s.kind == 1
    assert s.param1 == pytest.approx(0.5)
    assert s.param2 == pytest.approx(0.3)


def test_make_voter_sampler_uniform_config():
    s = make_voter_sampler("uniform", lo=-2.0, hi=2.0)
    assert isinstance(s, VoterSamplerConfig)
    assert s.kind == 0
    assert s.param1 == pytest.approx(-2.0)
    assert s.param2 == pytest.approx(2.0)


def test_make_voter_sampler_unknown_kind_raises():
    with pytest.raises(ValueError, match="unknown kind"):
        make_voter_sampler("dirichlet")


# ==========================================================================
# draw_voters
# ==========================================================================


def test_draw_voters_gaussian_length():
    sm = scl.StreamManager(master_seed=42, stream_names=["vs"])
    s = make_voter_sampler("gaussian", mean=0.0, stddev=0.4)
    v = draw_voters(50, s, sm, "vs")
    assert isinstance(v, np.ndarray)
    assert v.dtype == np.float64
    assert len(v) == 100
    assert np.all(np.isfinite(v))


def test_draw_voters_uniform_in_range():
    sm = scl.StreamManager(master_seed=7, stream_names=["vs"])
    s = make_voter_sampler("uniform", lo=-1.0, hi=1.0)
    v = draw_voters(200, s, sm, "vs")
    assert len(v) == 400
    assert np.all(v >= -1.0)
    assert np.all(v <= 1.0)


def test_draw_voters_reproducible():
    s = make_voter_sampler("gaussian", mean=0.0, stddev=1.0)
    sm1 = scl.StreamManager(master_seed=99, stream_names=["vs"])
    sm2 = scl.StreamManager(master_seed=99, stream_names=["vs"])
    np.testing.assert_array_equal(
        draw_voters(30, s, sm1, "vs"),
        draw_voters(30, s, sm2, "vs"),
    )


def test_draw_voters_usable_as_voter_ideals():
    """Voters from draw_voters should feed directly into competition_run."""
    sm = scl.StreamManager(
        master_seed=1, stream_names=["vs", "competition_adaptation_hunter"]
    )
    s = make_voter_sampler("gaussian", mean=0.0, stddev=0.4)
    voters = draw_voters(30, s, sm, "vs")
    # Reshape to (n_voters, n_dims) for competition_run
    voter_matrix = voters.reshape(-1, 2)

    competitors = np.array([[-0.5, -0.5], [0.5, 0.5]])
    headings = np.array([0.71, 0.71, -0.71, -0.71])
    cfg = scl.CompetitionEngineConfig(
        seat_count=1,
        seat_rule="plurality_top_k",
        max_rounds=5,
        step_config=scl.CompetitionStepConfig(kind="fixed", fixed_step_size=0.1),
    )
    with scl.competition_run(
        competitors,
        ["hunter", "hunter"],
        voter_matrix,
        dist_config=scl.DistanceConfig(salience_weights=[1.0, 1.0]),
        engine_config=cfg,
        stream_manager=sm,
    ) as trace:
        n_rounds, n_competitors, n_dims = trace.dims()
        assert n_competitors == 2
        assert n_rounds >= 1
        assert n_dims == 2


def test_draw_voters_wrong_sampler_type_raises():
    sm = scl.StreamManager(master_seed=1, stream_names=["vs"])
    with pytest.raises(TypeError, match="VoterSamplerConfig"):
        draw_voters(10, {"kind": 0, "param1": -1.0, "param2": 1.0}, sm, "vs")
