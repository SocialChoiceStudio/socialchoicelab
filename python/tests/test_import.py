"""Smoke tests for the B4 Python binding infrastructure.

Covers:
- Package import and version string (B4.1)
- Exception hierarchy (B4.2)
- StreamManager: create, draw, context manager (B4.3)
- Winset: factory, queries, boolean ops, context manager (B4.4)
- Profile: all factories, dims(), get_ranking(), export() (B4.5)
"""

import numpy as np
import pytest

import socialchoicelab as scl


def test_version_is_string():
    """Package exposes a non-empty __version__ string."""
    assert isinstance(scl.__version__, str)
    assert len(scl.__version__) > 0


def test_exception_hierarchy():
    """SCS exception subclasses are importable."""
    assert issubclass(scl.SCSInvalidArgumentError, scl.SCSError)
    assert issubclass(scl.SCSInternalError, scl.SCSError)
    assert issubclass(scl.SCSBufferTooSmallError, scl.SCSError)
    assert issubclass(scl.SCSOutOfMemoryError, scl.SCSError)


# ---------------------------------------------------------------------------
# DistanceConfig / LossConfig
# ---------------------------------------------------------------------------

def test_make_dist_config_defaults():
    d = scl.make_dist_config()
    assert d.distance_type == "euclidean"
    assert d.salience_weights is None


def test_make_dist_config_invalid():
    with pytest.raises(ValueError, match="distance_type"):
        scl.make_dist_config("bogus")


def test_make_loss_config_defaults():
    lc = scl.make_loss_config()
    assert lc.loss_type == "quadratic"


def test_make_loss_config_invalid():
    with pytest.raises(ValueError, match="loss_type"):
        scl.make_loss_config("bogus")


# ---------------------------------------------------------------------------
# StreamManager (B4.3)
# ---------------------------------------------------------------------------

def test_stream_manager_create_and_draw():
    with scl.StreamManager(42, ["s"]) as sm:
        x = sm.uniform_real("s", 0.0, 1.0)
        assert 0.0 <= x < 1.0
        n = sm.uniform_int("s", 0, 9)
        assert 0 <= n <= 9


def test_stream_manager_reproducibility():
    with scl.StreamManager(42, ["s"]) as sm1:
        x1 = sm1.uniform_real("s", 0.0, 1.0)
    with scl.StreamManager(42, ["s"]) as sm2:
        x2 = sm2.uniform_real("s", 0.0, 1.0)
    assert x1 == x2


def test_stream_manager_reset_all():
    with scl.StreamManager(42, ["s"]) as sm:
        x1 = sm.uniform_real("s", 0.0, 1.0)
        sm.reset_all(42)
        x2 = sm.uniform_real("s", 0.0, 1.0)
    assert x1 == x2


def test_stream_manager_bernoulli():
    with scl.StreamManager(1, ["s"]) as sm:
        v = sm.bernoulli("s", 0.0)
        assert v is False
        v = sm.bernoulli("s", 1.0)
        assert v is True


def test_stream_manager_uniform_choice():
    with scl.StreamManager(5, ["s"]) as sm:
        idx = sm.uniform_choice("s", 3)
        assert 0 <= idx < 3


# ---------------------------------------------------------------------------
# Winset (B4.4)
# ---------------------------------------------------------------------------

# Three voters near (0.5, 0): non-empty winset for SQ at origin.
_IDEALS = [0.0, 0.0,  0.5, 0.0,  0.5, 0.5]


def test_winset_non_empty():
    with scl.winset_2d(0.0, 0.0, _IDEALS) as w:
        assert not w.is_empty()


def test_winset_empty():
    # Symmetric 4-voter configuration — no policy beats the centre.
    sym_ideals = [1.0, 0.0,  -1.0, 0.0,  0.0, 1.0,  0.0, -1.0]
    with scl.winset_2d(0.0, 0.0, sym_ideals) as w:
        assert w.is_empty()


def test_winset_bbox_returns_dict():
    with scl.winset_2d(0.0, 0.0, _IDEALS) as w:
        bb = w.bbox()
        assert set(bb.keys()) == {"min_x", "min_y", "max_x", "max_y"}
        assert bb["max_x"] >= bb["min_x"]


def test_winset_contains():
    with scl.winset_2d(0.0, 0.0, _IDEALS) as w:
        assert w.contains(0.4, 0.1)
        assert not w.contains(10.0, 10.0)


def test_winset_boundary():
    with scl.winset_2d(0.0, 0.0, _IDEALS) as w:
        bnd = w.boundary()
        assert "xy" in bnd
        assert bnd["xy"].ndim == 2
        assert bnd["xy"].shape[1] == 2
        assert len(bnd["path_starts"]) >= 1


def test_winset_clone():
    with scl.winset_2d(0.0, 0.0, _IDEALS) as w:
        with w.clone() as w2:
            assert not w2.is_empty()


def test_winset_union():
    with scl.winset_2d(0.0, 0.0, _IDEALS) as w1:
        with scl.winset_2d(0.5, 0.0, _IDEALS) as w2:
            with w1.union(w2) as u:
                assert not u.is_empty()


def test_winset_intersection():
    with scl.winset_2d(0.0, 0.0, _IDEALS) as w1:
        with scl.winset_2d(0.5, 0.0, _IDEALS) as w2:
            with w1.intersection(w2) as i:
                assert isinstance(i.is_empty(), bool)


def test_weighted_winset_2d():
    ideals = [0.0, 0.0,  1.0, 0.0,  1.0, 1.0]
    weights = [1.0, 2.0, 2.0]
    with scl.weighted_winset_2d(0.0, 0.0, ideals, weights) as w:
        assert isinstance(w.is_empty(), bool)


def test_winset_with_veto():
    with scl.winset_with_veto_2d(0.0, 0.0, _IDEALS) as w:
        assert isinstance(w.is_empty(), bool)


# ---------------------------------------------------------------------------
# Profile (B4.5)
# ---------------------------------------------------------------------------

_ALTS = [0.0, 0.0,  1.0, 0.0,  -1.0, 0.0]
_VTR  = [0.2, 0.1,  -0.5, 0.3]


def test_profile_build_spatial():
    with scl.profile_build_spatial(_ALTS, _VTR) as p:
        nv, na = p.dims()
        assert nv == 2
        assert na == 3


def test_profile_get_ranking_shape():
    with scl.profile_build_spatial(_ALTS, _VTR) as p:
        r = p.get_ranking(0)
        assert r.shape == (3,)
        assert set(r.tolist()) == {0, 1, 2}


def test_profile_get_ranking_out_of_bounds():
    with scl.profile_build_spatial(_ALTS, _VTR) as p:
        with pytest.raises(IndexError):
            p.get_ranking(99)


def test_profile_export_shape():
    with scl.profile_build_spatial(_ALTS, _VTR) as p:
        exp = p.export()
        nv, na = p.dims()
        assert exp.shape == (nv, na)


def test_profile_clone():
    with scl.profile_build_spatial(_ALTS, _VTR) as p:
        with p.clone() as p2:
            assert p2.dims() == p.dims()


def test_profile_from_utility_matrix():
    U = np.array([[3.0, 1.0, 2.0],
                  [1.0, 3.0, 2.0]])
    with scl.profile_from_utility_matrix(U, 2, 3) as p:
        assert p.dims() == (2, 3)
        r0 = p.get_ranking(0)
        assert r0[0] == 0   # highest utility for voter 0 is alt 0


def test_profile_impartial_culture():
    with scl.StreamManager(42, ["ic"]) as sm:
        with scl.profile_impartial_culture(10, 4, sm, "ic") as p:
            nv, na = p.dims()
            assert nv == 10
            assert na == 4


def test_profile_uniform_spatial():
    with scl.StreamManager(42, ["sp"]) as sm:
        with scl.profile_uniform_spatial(20, 3, sm, "sp") as p:
            nv, na = p.dims()
            assert nv == 20
            assert na == 3


def test_profile_gaussian_spatial():
    with scl.StreamManager(42, ["gs"]) as sm:
        with scl.profile_gaussian_spatial(15, 5, sm, "gs") as p:
            nv, na = p.dims()
            assert nv == 15
            assert na == 5
