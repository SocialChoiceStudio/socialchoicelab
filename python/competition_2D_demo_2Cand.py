"""competition_2D_demo_2Cand.py — Two-candidate Hunter competition demo
=======================================================================

Parallel to r/competition_2D_demo_2Cand.R. Generates voters from a Gaussian
distribution, runs a hunter-vs-aggregator race, and opens the canvas animation.

Run from the project root:
  export SCS_LIB_PATH=$(pwd)/build
  python python/competition_2D_demo_2Cand.py
"""

from __future__ import annotations

import subprocess

import numpy as np

import socialchoicelab as scl
import socialchoicelab.plots as sclp

# ---------------------------------------------------------------------------
# Voters — Gaussian cloud (parallel to R demo)
# ---------------------------------------------------------------------------

# IC load: 20 voters × 1 seat × 201 frames = 4020 curves (80% of ic_max_curves = 5000).
sm_voters = scl.StreamManager(master_seed=123, stream_names=["voters"])
voter_cfg  = scl.make_voter_sampler("gaussian", mean=0.0, stddev=0.5)
voters     = scl.draw_voters(20, voter_cfg, sm_voters, "voters").reshape(-1, 2)

print(f"voters: {voters.shape[0]} points in {voters.shape[1]}D")

# ---------------------------------------------------------------------------
# Competition — hunter vs hunter
# ---------------------------------------------------------------------------

sm = scl.StreamManager(
    master_seed=42,
    stream_names=["competition_adaptation_hunter", "voters", "competition_motion_step_sizes"],
)

headings = np.array([[0.71, 0.71], [-0.71, -0.71]])

with scl.competition_run(
    np.array([[-1.5, -1.2], [1.5, 1.2]]),
    ["hunter", "hunter"],
    voters,
    competitor_headings=headings,
    dist_config=scl.DistanceConfig(salience_weights=[1.0, 1.0]),
    engine_config=scl.CompetitionEngineConfig(
        seat_count=1,
        seat_rule="plurality_top_k",
        max_rounds=200,
        step_config=scl.CompetitionStepConfig(kind="random_uniform", min_step_size=0.0, max_step_size=0.12),
    ),
    stream_manager=sm,
) as trace:
    n_rounds, n_comp, n_dims = trace.dims()
    print(f"n_rounds={n_rounds}, n_competitors={n_comp}, n_dims={n_dims}")
    print("final positions:")
    print(trace.final_positions())

    # Static geometry overlays: centroid, marginal median, Pareto Set (convex hull, Euclidean only)
    overlays = {
        "centroid":        scl.centroid_2d(voters.ravel()),
        "marginal_median": scl.marginal_median_2d(voters.ravel()),
        "pareto_set":      scl.convex_hull_2d(voters),
    }

    # ── Indifference-curve toggle ─────────────────────────────────────────────
    # Set COMPUTE_IC = True to compute and display per-voter ICs through each
    # seat holder's position at every frame.  Set to False to omit them entirely
    # (the "Indiff. Curves" checkbox will not appear in the player UI).
    # Current load: 20 voters × 1 seat × 201 frames = 4020 curves.
    COMPUTE_IC = True
    # ─────────────────────────────────────────────────────────────────────────

    # ── WinSet toggle ─────────────────────────────────────────────────────────
    # Set COMPUTE_WINSET = True to compute and display the WinSet boundary for
    # each seat holder's position at every frame.  Set to False to omit it
    # entirely (the "Win Set" checkbox will not appear in the player UI).
    # Current load: 1 seat × 201 frames = 201 WinSet computations.
    # COMPUTE_WINSET = True
    COMPUTE_WINSET = False  # commented out for demo; use Voronoi instead
    # ─────────────────────────────────────────────────────────────────────────

    # ── Voronoi toggle ────────────────────────────────────────────────────────
    # Set COMPUTE_VORONOI = True to compute Euclidean Voronoi (candidate
    # regions) per frame and show the "Voronoi" checkbox in the player UI.
    COMPUTE_VORONOI = True
    # ─────────────────────────────────────────────────────────────────────────

    canvas_path = "/tmp/competition_2D_demo_2Cand_py.html"
    sclp.animate_competition_canvas(
        trace,
        voters=voters,
        competitor_names=["Candidate A", "Candidate B"],
        title="Two Candidate Competition",
        trail="fade",
        overlays=overlays,
        width=900,
        height=800,
        path=canvas_path,
        compute_ic=COMPUTE_IC,
        ic_max_curves=15000,
        compute_winset=COMPUTE_WINSET,
        compute_voronoi=COMPUTE_VORONOI,
    )
    print(f"Canvas saved to {canvas_path}")
    subprocess.Popen(["open", canvas_path])
