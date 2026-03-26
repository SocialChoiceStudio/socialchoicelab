"""devScript_MAIN.py — Comprehensive walkthrough of socialchoicelab Python package
================================================================================
This script demonstrates user-facing functions across all major categories.
Run with the project .venv:

    .venv/bin/python python/devScript_MAIN.py

Or activate the venv first:

    source .venv/bin/activate
    python python/devScript_MAIN.py

Scenario: Laing-Olmsted-Bear (A_Group_05)
  Space:  0–200 × 0–200
  Rule:   Simple majority (0.5)

The SCS_LIB_PATH environment variable must point to the compiled library:

    export SCS_LIB_PATH=/path/to/socialchoicelab/build
"""

import os
import tempfile
import webbrowser
from pathlib import Path

import numpy as np
import socialchoicelab as scl

# ===========================================================================
# 1. SETUP: Voter and alternative data in 2D policy space
# ===========================================================================

print("\n=== 1. SETUP: Voter and Alternative Data (Laing-Olmsted-Bear) ===")

# Load the built-in scenario
scenario = scl.load_scenario("laing_olmsted_bear")

voters = scenario["voters"]
sq = scenario["status_quo"]
alts = sq  # No other alternatives in this scenario

print(f"Scenario: {scenario['name']}")
print("Players (flat [x0, y0, x1, y1, ...]):", voters)
print("Status quo:", sq)

# ===========================================================================
# 2. DISTANCE & UTILITY FUNCTIONS
# ===========================================================================

print("\n=== 2. DISTANCE & UTILITY ===")

d_euclidean = scl.calculate_distance([0.0, 0.0], [3.0, 4.0])
d_manhattan = scl.calculate_distance([0.0, 0.0], [3.0, 4.0],
                                     scl.make_dist_config("manhattan"))

print("Distance from origin to (3, 4):")
print(f"  Euclidean: {d_euclidean}")
print(f"  Manhattan: {d_manhattan}")

utility_at_2 = scl.distance_to_utility(2.0)
utility_at_5 = scl.distance_to_utility(5.0)

print("Utility (linear loss):")
print(f"  At distance 2: {utility_at_2}")
print(f"  At distance 5: {utility_at_5}")

utility_norm = scl.normalize_utility(utility_at_2, max_distance=5.0)
print(f"Normalized utility (max_distance=5): {utility_norm}")

# ===========================================================================
# 3. MAJORITY PREFERENCE & PAIRWISE COMPARISONS
# ===========================================================================

print("\n=== 3. MAJORITY PREFERENCE ===")

a_beats_sq = scl.majority_prefers_2d(110.0, 110.0, sq[0], sq[1], voters)
b_beats_sq = scl.majority_prefers_2d( 60.0,  60.0, sq[0], sq[1], voters)

print("Majority preference (5 voters):")
print(f"  (110, 110) beats SQ: {a_beats_sq}")
print(f"  ( 60,  60) beats SQ: {b_beats_sq}")

pairwise_mat = scl.pairwise_matrix_2d(alts, voters)
print("\nPairwise matrix (1=WIN, 0=TIE, -1=LOSS):")
print(pairwise_mat)

# ===========================================================================
# 4. GEOMETRY: COPELAND, CONDORCET, UNCOVERED SET
# ===========================================================================

print("\n=== 4. GEOMETRY: COPELAND & CONDORCET ===")

copeland = scl.copeland_scores_2d(alts, voters)
copeland_winner = scl.copeland_winner_2d(alts, voters)
print(f"Copeland scores: {copeland}")
print(f"Copeland winner (0-based): {copeland_winner}")

has_cw = scl.has_condorcet_winner_2d(alts, voters)
cw = scl.condorcet_winner_2d(alts, voters)
print(f"Has Condorcet winner: {has_cw}")
print(f"Condorcet winner (0-based): {cw}")

uncovered = scl.uncovered_set_2d(alts, voters)
print(f"Uncovered set (0-based indices): {uncovered}")

# ===========================================================================
# 5. WINSET: MAJORITY WINSET OF STATUS QUO
# ===========================================================================

print("\n=== 5. WINSET ===")

ws = scl.winset_2d(sq[0], sq[1], voters)

print(f"Winset of SQ at {sq}:")
print(f"  Empty: {ws.is_empty()}")

if not ws.is_empty():
    bbox = ws.bbox()
    print(f"  Bounding box: [{bbox['min_x']:.1f}, {bbox['max_x']:.1f}] "
          f"× [{bbox['min_y']:.1f}, {bbox['max_y']:.1f}]")
    print(f"  Contains (110, 80):   {ws.contains(110.0, 80.0)}")
    print(f"  Contains (150, 150):  {ws.contains(150.0, 150.0)}")

# ===========================================================================
# 6. ORDINAL PROFILES & VOTING RULES
# ===========================================================================

print("\n=== 6. ORDINAL PROFILES & VOTING RULES ===")

profile = scl.profile_build_spatial(alts, voters)
dims = profile.dims()
print(f"Profile dimensions: {dims[0]} voters, {dims[1]} alternatives")

print("\nRankings (0-based indices, rank 0 = best):")
print(profile.export())

# Plurality
plurality = scl.plurality_scores(profile)
plurality_winner = scl.plurality_one_winner(profile)
print(f"\nPlurality scores: {plurality}")
print(f"Plurality winner (0-based): {plurality_winner}")

# Borda
borda = scl.borda_scores(profile)
borda_winner = scl.borda_one_winner(profile)
borda_ranking = scl.borda_ranking(profile)
print(f"\nBorda scores: {borda}")
print(f"Borda winner (0-based): {borda_winner}")
print(f"Borda ranking (best first): {borda_ranking}")

# Anti-plurality
antiplurality = scl.antiplurality_scores(profile)
antiplurality_winner = scl.antiplurality_one_winner(profile)
print(f"\nAnti-plurality scores: {antiplurality}")
print(f"Anti-plurality winner (0-based): {antiplurality_winner}")

# Approval voting (top-k), capped at n_alts
k_approval = min(2, dims[1])
approval_topk = scl.approval_scores_topk(profile, k=k_approval)
approval_topk_winners = scl.approval_all_winners_topk(profile, k=k_approval)
print(f"\nApproval voting (top-{k_approval}):")
print(f"  Scores:  {approval_topk}")
print(f"  Winners (0-based): {approval_topk_winners}")

# ===========================================================================
# 7. SOCIAL RANKINGS & PROPERTIES
# ===========================================================================

print("\n=== 7. SOCIAL RANKINGS & PROPERTIES ===")

scores = [1.5, 0.8, 2.1]
ranking = scl.rank_by_scores(scores)
print(f"Rank by scores {scores}:")
print(f"  Best-to-worst (0-based): {ranking}")

pareto = scl.pareto_set(profile)
print(f"\nPareto set (0-based): {pareto}")

for i in range(dims[1]):
    is_p = scl.is_pareto_efficient(profile, i)
    print(f"  Alt {i} is Pareto-efficient: {is_p}")

has_cw_prof = scl.has_condorcet_winner_profile(profile)
cw_prof = scl.condorcet_winner_profile(profile)
print(f"\nProfile has Condorcet winner: {has_cw_prof}")
print(f"Condorcet winner (profile, 0-based): {cw_prof}")

if cw_prof is not None:
    is_maj = scl.is_majority_consistent(profile, cw_prof)
    is_con = scl.is_condorcet_consistent(profile, cw_prof)
    print(f"  Is majority-consistent:  {is_maj}")
    print(f"  Is Condorcet-consistent: {is_con}")

# ===========================================================================
# 8. STREAM MANAGER & REPRODUCIBILITY
# ===========================================================================

print("\n=== 8. STREAM MANAGER & REPRODUCIBILITY ===")

sm = scl.StreamManager(master_seed=42, stream_names=["draws", "ties"])

draw1 = sm.uniform_real("draws", 0.0, 1.0)
draw2 = sm.uniform_real("draws", 0.0, 1.0)
print(f"Draws from stream 'draws':")
print(f"  Draw 1: {draw1}")
print(f"  Draw 2: {draw2}")

sm.reset_all(42)
draw1_again = sm.uniform_real("draws", 0.0, 1.0)
draw2_again = sm.uniform_real("draws", 0.0, 1.0)
print(f"After reset (master_seed=42):")
print(f"  Draw 1: {draw1_again}  (equal: {draw1 == draw1_again})")
print(f"  Draw 2: {draw2_again}  (equal: {draw2 == draw2_again})")

sm_ic = scl.StreamManager(master_seed=99, stream_names=["ic"])
profile_ic = scl.profile_impartial_culture(
    n_voters=10, n_alts=4, stream_manager=sm_ic, stream_name="ic"
)
ic_dims = profile_ic.dims()
print(f"\nImpartial culture profile (10 voters, 4 alts):")
print(f"  Dims: {ic_dims[0]} × {ic_dims[1]}")
print(f"  Voter 0 ranking: {profile_ic.get_ranking(0)}")

# ===========================================================================
# 9. CONFIGURATION FUNCTIONS
# ===========================================================================

print("\n=== 9. CONFIGURATION ===")

dist_euc      = scl.make_dist_config("euclidean", salience_weights=[1.0, 1.0])
dist_l1       = scl.make_dist_config("manhattan")
dist_weighted = scl.make_dist_config("euclidean", salience_weights=[2.0, 0.5])

print("Distance configs:")
print(f"  Euclidean (equal weights): {dist_euc}")
print(f"  Manhattan:                 {dist_l1}")
print(f"  Weighted euclidean:        {dist_weighted}")

loss_linear   = scl.make_loss_config("linear")
loss_quad     = scl.make_loss_config("quadratic")
loss_gaussian = scl.make_loss_config("gaussian", max_loss=2.0, steepness=0.5)

print("\nLoss configs:")
print(f"  Linear:    {loss_linear}")
print(f"  Quadratic: {loss_quad}")
print(f"  Gaussian (max_loss=2, steepness=0.5): {loss_gaussian}")

# ===========================================================================
# 10. API VERSION
# ===========================================================================

print("\n=== 10. API VERSION ===")

version = scl.scs_version()
print(f"C API version: {version['major']}.{version['minor']}.{version['patch']}")

# ===========================================================================
# 11. VISUALIZATION (canvas HTML — no plotly)
# ===========================================================================

print("\n=== 11. VISUALIZATION ===")

import socialchoicelab.plots as sclp

hull = scl.convex_hull_2d(voters)
bnd  = scl.uncovered_set_boundary_2d(voters, grid_resolution=10)

# Base plot: players + status quo only
fig = sclp.plot_spatial_voting(
    voters,
    sq          = sq,
    voter_names = scenario["voter_names"],
    dim_names   = tuple(scenario["dim_names"]),
    title       = scenario["name"],
)

fig = sclp.layer_winset(fig, ws)
fig = sclp.layer_convex_hull(fig, hull)
fig = sclp.layer_uncovered_set(fig, bnd)

_fd, _p = tempfile.mkstemp(suffix=".html")
os.close(_fd)
_tmp = Path(_p)
sclp.save_plot(fig, str(_tmp))
webbrowser.open(_tmp.as_uri())
print("Plot opened in browser. Hover over points for details.")

# ===========================================================================
# END OF SCRIPT
# ===========================================================================

print("\n=== END ===")
print("All examples completed successfully!")
print("For more detail, see: help(scl.function_name) or the API docs.")
