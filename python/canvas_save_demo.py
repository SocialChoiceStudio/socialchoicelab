"""canvas_save_demo.py — Generate competition canvases and save to .scscanvas files.

Creates a 1D and a 2D competition canvas (10 voters, 10 rounds each) and
saves both to /tmp/ as .scscanvas files.  Run canvas_load_demo.py to reload
them without recomputing anything.

Run from the project root:
  export SCS_LIB_PATH=$(pwd)/build
  python python/canvas_save_demo.py
"""

from __future__ import annotations

import numpy as np

import socialchoicelab as scl
import socialchoicelab.plots as sclp

# Shared output paths — also used by canvas_load_demo.py and the R pair.
PATH_1D = "/tmp/canvas_1d_demo.scscanvas"
PATH_2D = "/tmp/canvas_2d_demo.scscanvas"

# ===========================================================================
# 1D competition — 10 voters, 10 rounds, 2 sticker candidates
# ===========================================================================
rng = np.random.default_rng(7)
voters_1d = rng.uniform(-1.0, 1.0, (10, 1))   # shape (n_voters, n_dims)
print(f"1D voters: {voters_1d.shape[0]} ideal points, "
      f"range [{voters_1d.min():.3f}, {voters_1d.max():.3f}]")

with scl.competition_run(
    np.array([[-0.8], [0.8]]),
    ["sticker", "sticker"],
    voters_1d,
    dist_config=scl.DistanceConfig(salience_weights=[1.0]),
    engine_config=scl.CompetitionEngineConfig(
        seat_count=1,
        seat_rule="plurality_top_k",
        max_rounds=10,
        step_config=scl.CompetitionStepConfig(kind="fixed", fixed_step_size=0.1),
    ),
) as trace_1d:
    n_rounds, n_comp, n_dims = trace_1d.dims()
    print(f"1D rounds: {n_rounds}")

    # payload_path writes the .scscanvas file before returning the HTML.
    sclp.animate_competition_canvas(
        trace_1d,
        voters=voters_1d,
        competitor_names=["Candidate A", "Candidate B"],
        title="1D Sticker Competition (saved)",
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
# 2D competition — 10 voters, 10 rounds, 2 hunter candidates
# ===========================================================================
sm_v      = scl.StreamManager(master_seed=99, stream_names=["voters"])
voter_cfg = scl.make_voter_sampler("uniform", lo=-0.75, hi=0.75)
voters_2d = scl.draw_voters(10, voter_cfg, sm_v, "voters").reshape(-1, 2)
print(f"2D voters: {voters_2d.shape[0]} points in 2D")

sm = scl.StreamManager(
    master_seed=42,
    stream_names=["competition_adaptation_hunter"],
)
headings = np.array([[0.71, 0.71], [-0.71, -0.71]])

with scl.competition_run(
    np.array([[-0.8, -0.5], [0.8, 0.5]]),
    ["hunter", "hunter"],
    voters_2d,
    competitor_headings=headings,
    dist_config=scl.DistanceConfig(salience_weights=[1.0, 1.0]),
    engine_config=scl.CompetitionEngineConfig(
        seat_count=1,
        seat_rule="plurality_top_k",
        max_rounds=10,
        step_config=scl.CompetitionStepConfig(kind="fixed", fixed_step_size=0.1),
    ),
    stream_manager=sm,
) as trace_2d:
    n_rounds, n_comp, n_dims = trace_2d.dims()
    print(f"2D rounds: {n_rounds}")

    sclp.animate_competition_canvas(
        trace_2d,
        voters=voters_2d,
        competitor_names=["Candidate A", "Candidate B"],
        title="2D Hunter Competition (saved)",
        trail="fade",
        width=900,
        height=800,
        compute_ic=True,
        compute_winset=True,
        compute_voronoi=True,
        payload_path=PATH_2D,
    )
    print(f"Saved 2D canvas to: {PATH_2D}")

print("\nDone. Run canvas_load_demo.py to reload without recomputing.")
