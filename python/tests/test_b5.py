"""Smoke tests for the B5 Python binding: function groups.

Covers:
- scs_version()                                    (B5)
- calculate_distance, distance_to_utility,
  normalize_utility                                (B5)
- level_set_1d, level_set_2d, level_set_to_polygon (B5)
- convex_hull_2d                                   (B5)
- majority_prefers_2d, weighted_majority_prefers_2d,
  pairwise_matrix_2d                               (B5)
- copeland_scores_2d, copeland_winner_2d,
  has_condorcet_winner_2d, condorcet_winner_2d,
  core_2d, uncovered_set_2d,
  uncovered_set_boundary_2d                        (B5)
- marginal_median_2d, centroid_2d, geometric_mean_2d (B5 centrality)
- plurality_*, borda_*, antiplurality_*,
  scoring_rule_*, approval_*                       (B5)
- rank_by_scores, pairwise_ranking_from_matrix,
  pareto_set, is_pareto_efficient,
  has_condorcet_winner_profile,
  condorcet_winner_profile,
  is_condorcet_consistent, is_majority_consistent  (B5)
"""

import numpy as np
import pytest

import socialchoicelab as scl
from socialchoicelab._error import SCSInvalidArgumentError

# ---------------------------------------------------------------------------
# Shared fixtures
# ---------------------------------------------------------------------------

# 3 voters in 2D: voter 0 at (-1,-1), voter 1 at (1,-1), voter 2 at (1,1)
# 3 alternatives: A=(0,0), B=(2,0), C=(-2,0)
_VOTERS_XY = [-1.0, -1.0, 1.0, -1.0, 1.0, 1.0]
_ALTS_XY = [0.0, 0.0, 2.0, 0.0, -2.0, 0.0]
_N_ALTS = 3
_N_VOTERS = 3


@pytest.fixture
def spatial_profile():
    """Profile built from a fixed spatial configuration (deterministic)."""
    alt_xy = np.array(_ALTS_XY)
    vtr_xy = np.array(_VOTERS_XY)
    return scl.profile_build_spatial(alt_xy, vtr_xy)


@pytest.fixture
def ic_profile():
    """Impartial-culture profile with 10 voters and 3 alternatives."""
    sm = scl.StreamManager(master_seed=7, stream_names=["ic"])
    return scl.profile_impartial_culture(n_voters=10, n_alts=3, stream_manager=sm, stream_name="ic")


# ---------------------------------------------------------------------------
# B5 — scs_version
# ---------------------------------------------------------------------------


def test_scs_version_keys():
    v = scl.scs_version()
    assert set(v.keys()) == {"major", "minor", "patch"}
    assert all(isinstance(v[k], int) and v[k] >= 0 for k in v)


# ---------------------------------------------------------------------------
# B5 — distance / utility
# ---------------------------------------------------------------------------


def test_calculate_distance_zero():
    d = scl.calculate_distance([0.0, 0.0], [0.0, 0.0])
    assert d == pytest.approx(0.0)


def test_calculate_distance_positive():
    d = scl.calculate_distance([0.0, 0.0], [3.0, 4.0])
    assert d == pytest.approx(5.0)


def test_distance_to_utility_monotone():
    u0 = scl.distance_to_utility(0.0)
    u1 = scl.distance_to_utility(1.0)
    u2 = scl.distance_to_utility(2.0)
    assert u0 >= u1 >= u2, "Utility must be non-increasing in distance."


def test_normalize_utility_at_zero_distance():
    u0 = scl.distance_to_utility(0.0)
    n = scl.normalize_utility(u0, max_distance=5.0)
    assert n == pytest.approx(1.0)


# ---------------------------------------------------------------------------
# B5 — level-set functions
# ---------------------------------------------------------------------------


def test_level_set_1d_returns_array():
    pts = scl.level_set_1d(ideal=0.0, weight=1.0, utility_level=-1.0)
    assert isinstance(pts, np.ndarray)
    assert pts.ndim == 1


def test_level_set_2d_circle():
    ls = scl.level_set_2d(ideal_x=0.0, ideal_y=0.0, utility_level=-1.0)
    assert isinstance(ls, dict)
    assert ls["type"] in {"circle", "ellipse", "superellipse", "polygon"}
    assert "center_x" in ls and "center_y" in ls


def test_level_set_to_polygon_shape():
    ls = scl.level_set_2d(ideal_x=0.0, ideal_y=0.0, utility_level=-1.0)
    poly = scl.level_set_to_polygon(ls, num_samples=16)
    assert isinstance(poly, np.ndarray)
    assert poly.ndim == 2 and poly.shape[1] == 2
    assert len(poly) > 0


# ---------------------------------------------------------------------------
# B5 — convex hull
# ---------------------------------------------------------------------------


def test_convex_hull_2d_square():
    pts = [0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, 0.5, 0.5]  # 5th point interior
    hull = scl.convex_hull_2d(pts)
    assert hull.shape == (4, 2)


# ---------------------------------------------------------------------------
# B5 — majority preference
# ---------------------------------------------------------------------------


def test_majority_prefers_2d_basic():
    # Voters at (-1,-1),(1,-1),(1,1): 2 out of 3 are closer to A=(0,0) than B=(5,0)
    result = scl.majority_prefers_2d(0.0, 0.0, 5.0, 0.0, _VOTERS_XY)
    assert isinstance(result, bool)


def test_weighted_majority_prefers_2d_basic():
    weights = [1.0, 1.0, 1.0]
    result = scl.weighted_majority_prefers_2d(0.0, 0.0, 5.0, 0.0, _VOTERS_XY, weights)
    assert isinstance(result, bool)


def test_pairwise_matrix_2d_shape():
    mat = scl.pairwise_matrix_2d(_ALTS_XY, _VOTERS_XY)
    assert mat.shape == (_N_ALTS, _N_ALTS)
    assert mat.dtype == np.int32
    # Diagonal must be 0 (SCS_PAIRWISE_TIE)
    assert np.all(np.diag(mat) == 0)
    # Antisymmetry: M[i,j] == -M[j,i]
    for i in range(_N_ALTS):
        for j in range(_N_ALTS):
            assert mat[i, j] == -mat[j, i]


# ---------------------------------------------------------------------------
# B5 — Copeland
# ---------------------------------------------------------------------------


def test_copeland_scores_2d_shape():
    scores = scl.copeland_scores_2d(_ALTS_XY, _VOTERS_XY)
    assert scores.shape == (_N_ALTS,)
    assert scores.dtype == np.int32


def test_copeland_winner_2d_in_range():
    w = scl.copeland_winner_2d(_ALTS_XY, _VOTERS_XY)
    assert 0 <= w < _N_ALTS


# ---------------------------------------------------------------------------
# B5 — Condorcet and core (continuous)
# ---------------------------------------------------------------------------


def test_has_condorcet_winner_2d_bool():
    result = scl.has_condorcet_winner_2d(_ALTS_XY, _VOTERS_XY)
    assert isinstance(result, bool)


def test_condorcet_winner_2d_returns_int_or_none():
    result = scl.condorcet_winner_2d(_ALTS_XY, _VOTERS_XY)
    assert result is None or (isinstance(result, int) and 0 <= result < _N_ALTS)


def test_core_2d_returns_dict_or_none():
    result = scl.core_2d(_VOTERS_XY)
    assert result is None or (isinstance(result, dict) and "x" in result and "y" in result)


# ---------------------------------------------------------------------------
# B5 — uncovered set
# ---------------------------------------------------------------------------


def test_uncovered_set_2d_nonempty():
    uc = scl.uncovered_set_2d(_ALTS_XY, _VOTERS_XY)
    assert uc.dtype == np.int32
    assert len(uc) > 0
    assert all(0 <= idx < _N_ALTS for idx in uc)


def test_uncovered_set_boundary_2d_shape():
    pts = scl.uncovered_set_boundary_2d(_VOTERS_XY)
    assert isinstance(pts, np.ndarray)
    assert pts.ndim == 2 and pts.shape[1] == 2


# ---------------------------------------------------------------------------
# B5 — Plurality
# ---------------------------------------------------------------------------


def test_plurality_scores_shape(spatial_profile):
    scores = scl.plurality_scores(spatial_profile)
    _, n_alts = spatial_profile.dims()
    assert scores.shape == (n_alts,)
    assert scores.dtype == np.int32
    assert scores.sum() == spatial_profile.dims()[0]


def test_plurality_all_winners_nonempty(spatial_profile):
    winners = scl.plurality_all_winners(spatial_profile)
    assert winners.dtype == np.int32
    assert len(winners) >= 1


def test_plurality_one_winner_in_range(spatial_profile):
    w = scl.plurality_one_winner(spatial_profile)
    _, n_alts = spatial_profile.dims()
    assert 0 <= w < n_alts


# ---------------------------------------------------------------------------
# B5 — Borda
# ---------------------------------------------------------------------------


def test_borda_scores_shape(spatial_profile):
    scores = scl.borda_scores(spatial_profile)
    _, n_alts = spatial_profile.dims()
    assert scores.shape == (n_alts,)
    assert scores.dtype == np.int32


def test_borda_all_winners_nonempty(spatial_profile):
    winners = scl.borda_all_winners(spatial_profile)
    assert len(winners) >= 1


def test_borda_one_winner_in_range(spatial_profile):
    w = scl.borda_one_winner(spatial_profile)
    _, n_alts = spatial_profile.dims()
    assert 0 <= w < n_alts


def test_borda_ranking_permutation(spatial_profile):
    ranking = scl.borda_ranking(spatial_profile)
    _, n_alts = spatial_profile.dims()
    assert sorted(ranking.tolist()) == list(range(n_alts))


# ---------------------------------------------------------------------------
# B5 — Anti-plurality
# ---------------------------------------------------------------------------


def test_antiplurality_scores_shape(spatial_profile):
    scores = scl.antiplurality_scores(spatial_profile)
    _, n_alts = spatial_profile.dims()
    assert scores.shape == (n_alts,)


def test_antiplurality_one_winner_in_range(spatial_profile):
    w = scl.antiplurality_one_winner(spatial_profile)
    _, n_alts = spatial_profile.dims()
    assert 0 <= w < n_alts


# ---------------------------------------------------------------------------
# B5 — Scoring rule
# ---------------------------------------------------------------------------


def test_scoring_rule_scores_borda_equivalence(spatial_profile):
    """scoring_rule_scores with linear weights must equal Borda scores."""
    _, n_alts = spatial_profile.dims()
    weights = [float(n_alts - 1 - r) for r in range(n_alts)]
    sr_scores = scl.scoring_rule_scores(spatial_profile, weights)
    borda = scl.borda_scores(spatial_profile).astype(np.float64)
    np.testing.assert_array_almost_equal(sr_scores, borda)


def test_scoring_rule_one_winner_in_range(spatial_profile):
    _, n_alts = spatial_profile.dims()
    weights = [float(n_alts - 1 - r) for r in range(n_alts)]
    w = scl.scoring_rule_one_winner(spatial_profile, weights)
    assert 0 <= w < n_alts


# ---------------------------------------------------------------------------
# B5 — Approval voting
# ---------------------------------------------------------------------------


def test_approval_scores_spatial_shape():
    scores = scl.approval_scores_spatial(_ALTS_XY, _VOTERS_XY, threshold=2.0)
    assert scores.shape == (_N_ALTS,)
    assert scores.dtype == np.int32


def test_approval_all_winners_spatial_result():
    winners = scl.approval_all_winners_spatial(_ALTS_XY, _VOTERS_XY, threshold=2.0)
    assert winners.dtype == np.int32
    assert all(0 <= w < _N_ALTS for w in winners)


def test_approval_scores_topk_shape(spatial_profile):
    scores = scl.approval_scores_topk(spatial_profile, k=1)
    _, n_alts = spatial_profile.dims()
    assert scores.shape == (n_alts,)


def test_approval_all_winners_topk_nonempty(spatial_profile):
    _, n_alts = spatial_profile.dims()
    winners = scl.approval_all_winners_topk(spatial_profile, k=1)
    assert len(winners) >= 1


# ---------------------------------------------------------------------------
# B5 — Social rankings
# ---------------------------------------------------------------------------


def test_rank_by_scores_permutation():
    scores = [3.0, 1.0, 2.0]
    ranking = scl.rank_by_scores(scores)
    assert sorted(ranking.tolist()) == [0, 1, 2]
    assert ranking[0] == 0  # alt 0 has highest score


def test_pairwise_ranking_from_matrix_permutation():
    mat = scl.pairwise_matrix_2d(_ALTS_XY, _VOTERS_XY)
    ranking = scl.pairwise_ranking_from_matrix(mat)
    assert sorted(ranking.tolist()) == list(range(_N_ALTS))


# ---------------------------------------------------------------------------
# B5 — Pareto and Condorcet predicates
# ---------------------------------------------------------------------------


def test_pareto_set_nonempty(ic_profile):
    ps = scl.pareto_set(ic_profile)
    assert len(ps) >= 1


def test_is_pareto_efficient_type(ic_profile):
    result = scl.is_pareto_efficient(ic_profile, alt_idx=0)
    assert isinstance(result, bool)


def test_has_condorcet_winner_profile_type(ic_profile):
    result = scl.has_condorcet_winner_profile(ic_profile)
    assert isinstance(result, bool)


def test_condorcet_winner_profile_none_or_int(ic_profile):
    result = scl.condorcet_winner_profile(ic_profile)
    n_alts = ic_profile.dims()[1]
    assert result is None or (isinstance(result, int) and 0 <= result < n_alts)


def test_is_condorcet_consistent_type(ic_profile):
    result = scl.is_condorcet_consistent(ic_profile, alt_idx=0)
    assert isinstance(result, bool)


def test_is_majority_consistent_type(ic_profile):
    result = scl.is_majority_consistent(ic_profile, alt_idx=0)
    assert isinstance(result, bool)


def test_condorcet_majority_consistent_agreement(ic_profile):
    """When a Condorcet winner exists, both predicates must agree on it."""
    w = scl.condorcet_winner_profile(ic_profile)
    if w is not None:
        assert scl.is_condorcet_consistent(ic_profile, w)
        assert scl.is_majority_consistent(ic_profile, w)


# ---------------------------------------------------------------------------
# B5 — Invalid-argument error propagation
# ---------------------------------------------------------------------------


def test_bad_tie_break_raises():
    sm = scl.StreamManager(master_seed=1, stream_names=["x"])
    with pytest.raises(ValueError, match="tie_break"):
        scl.plurality_one_winner(
            scl.profile_impartial_culture(n_voters=4, n_alts=3, stream_manager=sm, stream_name="x"),
            tie_break="invalid",
        )


def test_random_tie_break_without_stream_raises(spatial_profile):
    with pytest.raises(ValueError, match="StreamManager"):
        scl.borda_one_winner(spatial_profile, tie_break="random")


# ---------------------------------------------------------------------------
# Centrality measures
# ---------------------------------------------------------------------------

_CV3 = np.array([-1.0, 0.0,  0.0, 0.0,  1.0, 0.0])   # three voters on x-axis
_GM2 = np.array([1.0, 2.0,  4.0, 8.0])                # two voters; geo-mean (2,4)


def test_marginal_median_2d_returns_tuple():
    result = scl.marginal_median_2d(_CV3)
    assert isinstance(result, tuple)
    assert len(result) == 2


def test_marginal_median_2d_collinear():
    x, y = scl.marginal_median_2d(_CV3)
    assert abs(x - 0.0) < 1e-10
    assert abs(y - 0.0) < 1e-10


def test_marginal_median_2d_even_n_averages_middles():
    # Four x-coords: -2,-1,1,2 → median = 0
    voters = np.array([-2.0, 0.0,  -1.0, 0.0,  1.0, 0.0,  2.0, 0.0])
    x, y = scl.marginal_median_2d(voters)
    assert abs(x - 0.0) < 1e-10


def test_marginal_median_2d_empty_raises():
    with pytest.raises(SCSInvalidArgumentError):
        scl.marginal_median_2d(np.array([]))


def test_centroid_2d_returns_tuple():
    result = scl.centroid_2d(_CV3)
    assert isinstance(result, tuple)
    assert len(result) == 2


def test_centroid_2d_collinear():
    x, y = scl.centroid_2d(_CV3)
    assert abs(x - 0.0) < 1e-10
    assert abs(y - 0.0) < 1e-10


def test_centroid_2d_single_voter():
    x, y = scl.centroid_2d(np.array([3.5, -7.2]))
    assert abs(x - 3.5) < 1e-10
    assert abs(y - (-7.2)) < 1e-10


def test_centroid_2d_empty_raises():
    with pytest.raises(SCSInvalidArgumentError):
        scl.centroid_2d(np.array([]))


def test_geometric_mean_2d_returns_tuple():
    result = scl.geometric_mean_2d(_GM2)
    assert isinstance(result, tuple)
    assert len(result) == 2


def test_geometric_mean_2d_correct_value():
    x, y = scl.geometric_mean_2d(_GM2)
    assert abs(x - 2.0) < 1e-10
    assert abs(y - 4.0) < 1e-10


def test_geometric_mean_2d_negative_coords_raises():
    with pytest.raises(SCSInvalidArgumentError, match="strictly positive"):
        scl.geometric_mean_2d(_CV3)  # _CV3 has x=-1 (non-positive)


def test_geometric_mean_2d_empty_raises():
    with pytest.raises(SCSInvalidArgumentError):
        scl.geometric_mean_2d(np.array([]))
