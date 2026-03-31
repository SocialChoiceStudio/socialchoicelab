"""canvas_save_demo.py — Generate competition canvases and save to .scsview files.

Uses the same models as:
  competition_1d_demo.py
  competition_2D_demo_2Cand.py
  competition_2D_demo_3Cand.py

Saves all three payloads to tmp/ without displaying anything.
Run canvas_load_demo.py to reload without recomputing.

Run from the project root:
  export SCS_LIB_PATH=$(pwd)/build
  python python/canvas_save_demo.py
"""

from __future__ import annotations

import numpy as np
from pathlib import Path

import socialchoicelab as scl
import socialchoicelab.plots as sclp

# Output paths — written to tmp/ in the project root (visible in the file tree).
# tmp/ is tracked in git but its contents are listed in .gitignore.
# Shared with canvas_load_demo.py and the R pair.
Path("tmp").mkdir(exist_ok=True)
PATH_1D    = "tmp/canvas_1d_demo.scsview"
PATH_2D    = "tmp/canvas_2d_demo.scsview"
PATH_3CAND = "tmp/canvas_3cand_2d_demo.scsview"

# ===========================================================================
# 1D competition — same model as competition_1d_demo.py
# 20 voters, up to 200 rounds, 2 hunter-sticker candidates
# ===========================================================================
rng       = np.random.default_rng(123)
voters_1d = rng.normal(0.0, 0.5, (20, 1))
print(f"1D voters: {voters_1d.shape[0]} ideal points, "
      f"range [{voters_1d.min():.3f}, {voters_1d.max():.3f}]")

sm_1d       = scl.StreamManager(master_seed=42, stream_names=["competition_adaptation_hunter"])
headings_1d = np.array([[1.0], [-1.0]])

with scl.competition_run(
    np.array([[-1.2], [1.2]]),
    ["hunter_sticker", "hunter_sticker"],
    voters_1d,
    competitor_headings=headings_1d,
    dist_config=scl.DistanceConfig(salience_weights=[1.0]),
    engine_config=scl.CompetitionEngineConfig(
        seat_count=1,
        seat_rule="plurality_top_k",
        max_rounds=200,
        step_config=scl.CompetitionStepConfig(kind="fixed", fixed_step_size=0.12),
    ),
    stream_manager=sm_1d,
) as trace_1d:
    n_rounds, n_comp, n_dims = trace_1d.dims()
    print(f"1D rounds: {n_rounds}")
    print("1D final positions:")
    print(trace_1d.final_positions())

    sclp.animate_competition_canvas(
        trace_1d,
        voters=voters_1d,
        competitor_names=["Candidate A", "Candidate B"],
        title="1D Hunter-Sticker Competition",
        trail="fade",
        dim_names=("Policy Dimension",),
        overlays={"centroid": True, "marginal_median": True},
        width=900,
        height=400,
        compute_ic=True,
        compute_winset=True,
        payload_path=PATH_1D,
    )
    print(f"Saved 1D canvas to: {PATH_1D}")

# ===========================================================================
# 2D competition — same model as competition_2D_demo_2Cand.py
# 20 voters, up to 200 rounds, hunter vs hunter
# ===========================================================================
sm_voters_2c = scl.StreamManager(master_seed=123, stream_names=["voters"])
voter_cfg_2c = scl.make_voter_sampler("gaussian", mean=0.0, stddev=0.5)
voters_2d    = scl.draw_voters(20, voter_cfg_2c, sm_voters_2c, "voters").reshape(-1, 2)
print(f"2D (2-cand) voters: {voters_2d.shape[0]} points in 2D")

overlays_2d = {
    "centroid":        scl.centroid_2d(voters_2d.ravel()),
    "marginal_median": scl.marginal_median_2d(voters_2d.ravel()),
    "pareto_set":      scl.convex_hull_2d(voters_2d),
}

sm_2c       = scl.StreamManager(
    master_seed=42,
    stream_names=["competition_adaptation_hunter", "voters", "competition_motion_step_sizes"],
)
headings_2c = np.array([[0.71, 0.71], [-0.71, -0.71]])

with scl.competition_run(
    np.array([[-1.5, -1.2], [1.5, 1.2]]),
    ["hunter", "hunter"],
    voters_2d,
    competitor_headings=headings_2c,
    dist_config=scl.DistanceConfig(salience_weights=[1.0, 1.0]),
    engine_config=scl.CompetitionEngineConfig(
        seat_count=1,
        seat_rule="plurality_top_k",
        max_rounds=200,
        step_config=scl.CompetitionStepConfig(kind="random_uniform", min_step_size=0.0, max_step_size=0.12),
    ),
    stream_manager=sm_2c,
) as trace_2d:
    n_rounds, n_comp, n_dims = trace_2d.dims()
    print(f"2D (2-cand) rounds: {n_rounds}")
    print("2D (2-cand) final positions:")
    print(trace_2d.final_positions())

    sclp.animate_competition_canvas(
        trace_2d,
        voters=voters_2d,
        competitor_names=["Candidate A", "Candidate B"],
        title="Two Candidate Competition",
        trail="fade",
        overlays=overlays_2d,
        width=900,
        height=800,
        compute_ic=True,
        ic_max_curves=15000,
        compute_winset=True,
        compute_voronoi=True,
        payload_path=PATH_2D,
    )
    print(f"Saved 2D (2-cand) canvas to: {PATH_2D}")

# ===========================================================================
# 2D competition — same model as competition_2D_demo_3Cand.py
# 20 voters, up to 200 rounds, hunter vs hunter vs aggregator
# ===========================================================================
sm_voters_3c = scl.StreamManager(master_seed=123, stream_names=["voters"])
voter_cfg_3c = scl.make_voter_sampler("gaussian", mean=0.0, stddev=0.5)
voters_3c    = scl.draw_voters(20, voter_cfg_3c, sm_voters_3c, "voters").reshape(-1, 2)
print(f"2D (3-cand) voters: {voters_3c.shape[0]} points in 2D")

overlays_3c = {
    "centroid":        scl.centroid_2d(voters_3c.ravel()),
    "marginal_median": scl.marginal_median_2d(voters_3c.ravel()),
    "pareto_set":      scl.convex_hull_2d(voters_3c),
}

sm_3c       = scl.StreamManager(
    master_seed=42,
    stream_names=["competition_adaptation_hunter", "voters", "competition_motion_step_sizes"],
)
headings_3c = np.array([[0.71, 0.71], [-0.71, -0.71], [0.0, -1.0]])

with scl.competition_run(
    np.array([[-1.5, -1.2], [1.5, 1.2], [0.0, 1.5]]),
    ["hunter", "hunter", "aggregator"],
    voters_3c,
    competitor_headings=headings_3c,
    dist_config=scl.DistanceConfig(salience_weights=[1.0, 1.0]),
    engine_config=scl.CompetitionEngineConfig(
        seat_count=1,
        seat_rule="plurality_top_k",
        max_rounds=200,
        step_config=scl.CompetitionStepConfig(kind="random_uniform", min_step_size=0.0, max_step_size=0.12),
    ),
    stream_manager=sm_3c,
) as trace_3c:
    n_rounds, n_comp, n_dims = trace_3c.dims()
    print(f"2D (3-cand) rounds: {n_rounds}")
    print("2D (3-cand) final positions:")
    print(trace_3c.final_positions())

    sclp.animate_competition_canvas(
        trace_3c,
        voters=voters_3c,
        competitor_names=["Candidate A", "Candidate B", "Candidate C"],
        title="Three Candidate Competition",
        trail="fade",
        overlays=overlays_3c,
        width=900,
        height=800,
        compute_ic=True,
        ic_max_curves=15000,
        compute_winset=False,
        compute_voronoi=True,
        payload_path=PATH_3CAND,
    )
    print(f"Saved 2D (3-cand) canvas to: {PATH_3CAND}")

print("\nDone. Run canvas_load_demo.py to reload without recomputing.")
