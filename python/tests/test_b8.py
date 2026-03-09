"""B8.2 extended Python tests.

Covers:
- Context manager (with-statement) cleanup for StreamManager, Winset, Profile.
- Reproducibility: same master_seed → same results across independent instances.
- Error-path tests: wrong types, wrong dimensions, out-of-range values.
- numpy array shape and dtype guarantees.
"""

from __future__ import annotations

import numpy as np
import pytest

import socialchoicelab as scl
from socialchoicelab._loader import _ffi

# ---------------------------------------------------------------------------
# Shared data
# ---------------------------------------------------------------------------

_VOTERS = [-1.0, -1.0, 1.0, -1.0, 1.0, 1.0]
_ALTS   = [ 0.0,  0.0, 2.0,  0.0, -2.0, 0.0]


def _make_profile():
    return scl.profile_build_spatial(_ALTS, _VOTERS)


def _make_sm(seed=42, streams=("s",)):
    return scl.StreamManager(master_seed=seed, stream_names=list(streams))


# ---------------------------------------------------------------------------
# Context managers (deterministic cleanup)
# ---------------------------------------------------------------------------

class TestContextManagers:
    def test_stream_manager_context_exits_cleanly(self):
        with scl.StreamManager(master_seed=1, stream_names=["x"]) as sm:
            val = sm.uniform_real("x")
        assert isinstance(val, float)

    def test_winset_context_exits_cleanly(self):
        with scl.winset_2d(0.1, 0.1, _VOTERS) as ws:
            empty = ws.is_empty()
        assert isinstance(empty, bool)

    def test_profile_context_exits_cleanly(self):
        with scl.profile_build_spatial(_ALTS, _VOTERS) as prof:
            dims = prof.dims()
        assert dims == (3, 3)

    def test_winset_methods_unavailable_after_exit(self):
        """After __exit__, the internal pointer is NULL so calls must fail."""
        ws = scl.winset_2d(0.1, 0.1, _VOTERS)
        ws.__exit__(None, None, None)
        with pytest.raises(Exception):
            ws.is_empty()  # must raise (NULL ptr dereference via cffi error)

    def test_stream_manager_methods_unavailable_after_exit(self):
        sm = _make_sm()
        sm.__exit__(None, None, None)
        with pytest.raises(Exception):
            sm.uniform_real("s")


# ---------------------------------------------------------------------------
# Reproducibility
# ---------------------------------------------------------------------------

class TestReproducibility:
    def test_same_seed_same_profile_impartial_culture(self):
        sm1 = _make_sm(7, ("r",))
        sm2 = _make_sm(7, ("r",))
        p1 = scl.profile_impartial_culture(20, 4, sm1, "r")
        p2 = scl.profile_impartial_culture(20, 4, sm2, "r")
        np.testing.assert_array_equal(p1.export(), p2.export())

    def test_same_seed_same_spatial_profile(self):
        sm1 = _make_sm(99, ("s",))
        sm2 = _make_sm(99, ("s",))
        p1 = scl.profile_uniform_spatial(15, 3, stream_manager=sm1, stream_name="s")
        p2 = scl.profile_uniform_spatial(15, 3, stream_manager=sm2, stream_name="s")
        np.testing.assert_array_equal(p1.export(), p2.export())

    def test_reset_all_restores_stream(self):
        sm = _make_sm(5, ("r",))
        v1 = sm.uniform_real("r")
        sm.reset_all(5)
        v2 = sm.uniform_real("r")
        assert v1 == pytest.approx(v2)

    def test_different_seeds_different_profiles(self):
        sm1 = _make_sm(1, ("s",))
        sm2 = _make_sm(2, ("s",))
        p1 = scl.profile_impartial_culture(10, 4, sm1, "s")
        p2 = scl.profile_impartial_culture(10, 4, sm2, "s")
        # With overwhelming probability two independent seeds differ.
        assert not np.array_equal(p1.export(), p2.export())


# ---------------------------------------------------------------------------
# NumPy shape and dtype guarantees
# ---------------------------------------------------------------------------

class TestNumpyShapes:
    def test_profile_export_shape_and_dtype(self):
        prof = _make_profile()
        mat = prof.export()
        assert mat.shape == (3, 3)
        assert mat.dtype == np.int32

    def test_get_ranking_shape_and_dtype(self):
        prof = _make_profile()
        r = prof.get_ranking(0)
        assert r.shape == (3,)
        assert r.dtype == np.int32

    def test_winset_boundary_xy_shape(self):
        ws = scl.winset_2d(0.1, 0.1, _VOTERS)
        bnd = ws.boundary()
        if bnd is not None:
            assert bnd["xy"].ndim == 2
            assert bnd["xy"].shape[1] == 2
            assert bnd["xy"].dtype == np.float64

    def test_borda_scores_dtype(self):
        prof = _make_profile()
        scores = scl.borda_scores(prof)
        assert scores.dtype == np.int32

    def test_pairwise_matrix_dtype(self):
        mat = scl.pairwise_matrix_2d(_ALTS, _VOTERS)
        assert mat.dtype == np.int32
        assert mat.shape == (3, 3)

    def test_uncovered_set_2d_dtype(self):
        uc = scl.uncovered_set_2d(_ALTS, _VOTERS)
        assert uc.dtype == np.int32

    def test_pareto_set_dtype(self):
        ps = scl.pareto_set(_make_profile())
        assert ps.dtype == np.int32

    def test_borda_ranking_is_permutation(self):
        ranking = scl.borda_ranking(_make_profile())
        assert sorted(ranking.tolist()) == list(range(3))

    def test_rank_by_scores_dtype(self):
        ranking = scl.rank_by_scores([3.0, 1.0, 2.0])
        assert ranking.dtype == np.int32
        assert ranking[0] == 0  # highest score first


# ---------------------------------------------------------------------------
# Error paths
# ---------------------------------------------------------------------------

class TestErrorPaths:
    def test_calculate_distance_mismatched_dims_raises(self):
        with pytest.raises(Exception):
            scl.calculate_distance([0.0, 0.0], [1.0, 2.0, 3.0])

    def test_winset_zero_voters_raises(self):
        """Empty voter list should raise."""
        with pytest.raises(Exception):
            scl.winset_2d(0.0, 0.0, [])

    def test_profile_impartial_culture_zero_voters_raises(self):
        sm = _make_sm()
        with pytest.raises(Exception):
            scl.profile_impartial_culture(0, 3, sm, "s")

    def test_profile_impartial_culture_zero_alts_raises(self):
        sm = _make_sm()
        with pytest.raises(Exception):
            scl.profile_impartial_culture(5, 0, sm, "s")

    def test_get_ranking_out_of_range_raises(self):
        prof = _make_profile()
        with pytest.raises(Exception):
            prof.get_ranking(99)

    def test_is_pareto_efficient_out_of_range_raises(self):
        prof = _make_profile()
        with pytest.raises(Exception):
            scl.is_pareto_efficient(prof, 99)

    def test_make_dist_config_unknown_type_raises(self):
        with pytest.raises(Exception):
            scl.make_dist_config(distance_type="cosine")

    def test_make_loss_config_unknown_type_raises(self):
        with pytest.raises(Exception):
            scl.make_loss_config(loss_type="sigmoid")

    def test_scoring_rule_scores_wrong_weight_length_raises(self):
        prof = _make_profile()
        # n_alts=3 but only 2 weights provided.
        with pytest.raises(Exception):
            scl.scoring_rule_scores(prof, [2.0, 1.0])

    def test_random_tiebreak_without_stream_raises(self):
        prof = _make_profile()
        with pytest.raises(ValueError, match="StreamManager"):
            scl.plurality_one_winner(prof, tie_break="random")

    def test_invalid_tiebreak_raises(self):
        prof = _make_profile()
        with pytest.raises(ValueError, match="tie_break"):
            scl.plurality_one_winner(prof, tie_break="median")

    def test_approval_topk_k_zero_raises(self):
        prof = _make_profile()
        with pytest.raises(Exception):
            scl.approval_scores_topk(prof, k=0)

    def test_stream_manager_unregistered_stream_raises(self):
        sm = _make_sm()
        with pytest.raises(Exception):
            sm.uniform_real("nonexistent_stream")


# ---------------------------------------------------------------------------
# StreamManager multi-name registration (regression for CFFI lifetime bug)
# ---------------------------------------------------------------------------

class TestStreamManagerMultiRegister:
    """Regression tests for the char[] lifetime bug in StreamManager.register.

    The underlying CFFI call (scs_register_streams) receives a const char*[]
    whose pointees were previously created as an anonymous inline list and
    could be GC'd before the C function read them. All tests here confirm that
    every registered name is actually usable after registration, not just that
    the registration call returned without error.
    """

    NAMES = [
        "competition_adaptation_hunter",
        "competition_motion_step_sizes",
        "web_voter_generation",
    ]

    def test_multi_register_via_init_all_names_usable(self):
        """Names passed to __init__ stream_names must all be usable."""
        sm = scl.StreamManager(master_seed=17, stream_names=self.NAMES)
        for name in self.NAMES:
            val = sm.uniform_real(name)
            assert isinstance(val, float), f"stream '{name}' not usable after init registration"

    def test_multi_register_via_register_all_names_usable(self):
        """Names passed to register([...]) in one call must all be usable."""
        sm = scl.StreamManager(master_seed=17)
        sm.register(self.NAMES)
        for name in self.NAMES:
            val = sm.uniform_real(name)
            assert isinstance(val, float), f"stream '{name}' not usable after register() call"

    def test_multi_register_reproducible(self):
        """Two StreamManagers with the same seed and the same multi-name registration
        must produce identical draws from each stream."""
        sm1 = scl.StreamManager(master_seed=42, stream_names=self.NAMES)
        sm2 = scl.StreamManager(master_seed=42, stream_names=self.NAMES)
        for name in self.NAMES:
            assert sm1.uniform_real(name) == pytest.approx(sm2.uniform_real(name))

    def test_multi_register_in_competition(self):
        """Registering the three competition streams and then running a
        competition must succeed end-to-end (the originally reported failure
        path in the website backend)."""
        competitors = np.array([[-0.3], [0.4]])
        voters = np.array([[-0.5], [-0.3], [0.1], [0.4], [0.6]])
        sm = scl.StreamManager(master_seed=17, stream_names=self.NAMES)
        cfg = scl.CompetitionEngineConfig(
            seat_count=1,
            seat_rule="plurality_top_k",
            max_rounds=5,
            step_config=scl.CompetitionStepConfig(kind="fixed", fixed_step_size=0.1),
        )
        with scl.competition_run(
            competitors,
            ["sticker", "sticker"],
            voters,
            dist_config=scl.DistanceConfig(salience_weights=[1.0]),
            engine_config=cfg,
            stream_manager=sm,
        ) as trace:
            assert trace.dims()[1] == 2  # 2 competitors


# ---------------------------------------------------------------------------
# 0-based index contract (Python vs R: R is 1-based, Python is 0-based)
# ---------------------------------------------------------------------------

class TestZeroBasedIndices:
    def test_profile_get_ranking_is_zero_based(self):
        prof = _make_profile()
        r = prof.get_ranking(0)  # voter 0 (0-based)
        # All indices must be in [0, n_alts-1].
        assert all(0 <= x < 3 for x in r)

    def test_plurality_winner_is_zero_based(self):
        prof = _make_profile()
        w = scl.plurality_one_winner(prof)
        assert 0 <= w < 3

    def test_pareto_set_is_zero_based(self):
        prof = _make_profile()
        ps = scl.pareto_set(prof)
        assert all(0 <= x < 3 for x in ps)

    def test_condorcet_winner_is_zero_based_or_none(self):
        prof = _make_profile()
        w = scl.condorcet_winner_profile(prof)
        if w is not None:
            assert 0 <= w < 3

    def test_copeland_winner_is_zero_based(self):
        w = scl.copeland_winner_2d(_ALTS, _VOTERS)
        assert 0 <= w < 3
