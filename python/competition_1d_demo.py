"""competition_1d_demo.py — One-dimensional candidate competition demo
======================================================================

Parallel to r/competition_1d_demo.R. Generates voters from a 1D Gaussian
distribution, runs a hunter-sticker-vs-hunter-sticker race on the real line,
and opens the canvas animation. Each candidate hunts for votes until it wins
a seat, then freezes in place.

Run from the project root:
  export DYLD_LIBRARY_PATH=$(pwd)/build
  python python/competition_1d_demo.py
"""

from __future__ import annotations

import subprocess

import numpy as np

import socialchoicelab as scl
import socialchoicelab.plots as sclp

# ---------------------------------------------------------------------------
# Voters — 1D Gaussian cloud generated directly.
# draw_voters() only supports 2D, so we use numpy's RNG here.
# IC load: 20 voters × 1 seat × 201 frames = 4020 intervals.
# ---------------------------------------------------------------------------
rng       = np.random.default_rng(123)
voters_1d = rng.normal(0.0, 0.5, (20, 1))  # shape (20, 1) — required by competition_run

print(f"voters (1D): {voters_1d.shape[0]} ideal points")
print(f"range: [{voters_1d.min():.3f}, {voters_1d.max():.3f}]")

# ---------------------------------------------------------------------------
# Competition — hunter-sticker vs hunter-sticker on the real line.
# Each candidate hunts until it wins a seat, then freezes.
# ---------------------------------------------------------------------------
sm = scl.StreamManager(
    master_seed=42,
    stream_names=["competition_adaptation_hunter"],
)

headings = np.array([[1.0], [-1.0]])  # pointing toward centre

with scl.competition_run(
    np.array([[-1.2], [1.2]]),          # 2 competitors, 1 dimension
    ["hunter_sticker", "hunter_sticker"],
    voters_1d,
    competitor_headings=headings,
    dist_config=scl.DistanceConfig(salience_weights=[1.0]),
    engine_config=scl.CompetitionEngineConfig(
        seat_count=1,
        seat_rule="plurality_top_k",
        max_rounds=200,
        step_config=scl.CompetitionStepConfig(kind="fixed", fixed_step_size=0.12),
    ),
    stream_manager=sm,
) as trace:
    n_rounds, n_comp, n_dims = trace.dims()
    print(f"n_rounds={n_rounds}, n_competitors={n_comp}, n_dims={n_dims}")
    print("final positions:")
    print(trace.final_positions())

    # ---------------------------------------------------------------------------
    # Overlays — 1D centroid and marginal median (computed inline from voters)
    # For 1D traces pass any dict to select which overlays to show; values are
    # ignored and the function computes mean/median internally from voters.
    # Note: pareto_set and geometric_mean are not supported for 1D traces.
    # ---------------------------------------------------------------------------

    # ── Overlay toggles ───────────────────────────────────────────────────────
    # COMPUTE_IC     — per-voter indifference curves (2 vertical lines/voter).
    #                  Load: 20 voters × 1 seat × 201 frames = ~4 k intervals.
    # COMPUTE_WINSET — 1D WinSet interval (lo, hi) for each seat holder/frame.
    #                  Load: 1 seat × 201 frames = 201 intervals.
    COMPUTE_IC     = True
    COMPUTE_WINSET = True
    # ─────────────────────────────────────────────────────────────────────────

    canvas_path = "/tmp/competition_1d_demo_py.html"
    sclp.animate_competition_canvas(
        trace,
        voters=voters_1d,
        competitor_names=["Candidate A", "Candidate B"],
        title="1D Hunter-Sticker Competition",
        trail="fade",
        dim_names=("Policy Dimension",),
        overlays={"centroid": True, "marginal_median": True},
        width=900,
        height=400,
        path=canvas_path,
        compute_ic=COMPUTE_IC,
        compute_winset=COMPUTE_WINSET,
    )
    print(f"Canvas saved to {canvas_path}")
    subprocess.Popen(["open", canvas_path])
