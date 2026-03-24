"""benchmark_ic.py — Baseline IC computation timing.

Replicates the IC loop from animate_competition_canvas using the same
parameters as competition_2D_demo_3Cand.py:
  20 voters x 1 seat x 201 frames = 4 020 ICs
  4 C API calls per IC = 16 080 total wall crossings

Run from the project root:
  export SCS_LIB_PATH=$(pwd)/build
  python python/benchmark_ic.py
"""

from __future__ import annotations

import time

import numpy as np

import socialchoicelab as scl
from socialchoicelab._functions import (
    calculate_distance,
    distance_to_utility,
    ic_polygon_2d,
    level_set_2d,
    level_set_to_polygon,
)
from socialchoicelab._types import DistanceConfig, LossConfig

# ---------------------------------------------------------------------------
# Setup — mirror competition_2D_demo_3Cand.py exactly
# ---------------------------------------------------------------------------

sm_voters = scl.StreamManager(master_seed=123, stream_names=["voters"])
voter_cfg  = scl.make_voter_sampler("gaussian", mean=0.0, stddev=0.5)
voters     = scl.draw_voters(20, voter_cfg, sm_voters, "voters").reshape(-1, 2)
n_voters   = voters.shape[0]
voters_x   = voters[:, 0].tolist()
voters_y   = voters[:, 1].tolist()

sm = scl.StreamManager(
    master_seed=42,
    stream_names=["competition_adaptation_hunter", "voters", "competition_motion_step_sizes"],
)

headings = np.array([[0.71, 0.71], [-0.71, -0.71], [0.0, -1.0]])

with scl.competition_run(
    np.array([[-1.5, -1.2], [1.5, 1.2], [0.0, 1.5]]),
    ["hunter", "hunter", "aggregator"],
    voters,
    competitor_headings=headings,
    dist_config=scl.DistanceConfig(salience_weights=[1.0, 1.0]),
    engine_config=scl.CompetitionEngineConfig(
        seat_count=1,
        seat_rule="plurality_top_k",
        max_rounds=200,
        step_config=scl.CompetitionStepConfig(
            kind="random_uniform", min_step_size=0.0, max_step_size=0.12
        ),
    ),
    stream_manager=sm,
) as trace:
    n_rounds, n_comp, n_dims = trace.dims()
    positions = [trace.round_positions(r) for r in range(n_rounds)]
    positions.append(trace.final_positions())

    seat_idxs_per_frame = []
    for r in range(n_rounds):
        st = trace.round_seat_totals(r)
        seat_idxs_per_frame.append([int(i) for i in np.where(st > 0)[0]])
    final_ss = trace.final_seat_shares()
    seat_idxs_per_frame.append([int(i) for i in np.where(final_ss > 0)[0]])

ic_loss_config = LossConfig(loss_type="linear", sensitivity=1.0)
ic_dist_config = DistanceConfig(salience_weights=[1.0, 1.0])
ic_num_samples = 32

n_frames     = len(positions)
total_ics    = sum(n_voters * len(s) for s in seat_idxs_per_frame)
total_calls  = total_ics * 4

print(f"frames       : {n_frames}")
print(f"voters       : {n_voters}")
print(f"total ICs    : {total_ics}")
print(f"total C API calls (current, 4 per IC): {total_calls}")
print()

# ---------------------------------------------------------------------------
# Benchmark — current 4-call sequence
# ---------------------------------------------------------------------------

t0 = time.perf_counter()

for frame_i, pos_mat in enumerate(positions):
    seat_idxs = seat_idxs_per_frame[frame_i]
    for s in seat_idxs:
        seat_pos = [float(pos_mat[s, 0]), float(pos_mat[s, 1])]
        for v in range(n_voters):
            voter_ideal = [voters_x[v], voters_y[v]]
            dist_val   = calculate_distance(voter_ideal, seat_pos, ic_dist_config)
            util_level = distance_to_utility(dist_val, ic_loss_config)
            ls         = level_set_2d(voters_x[v], voters_y[v], util_level,
                                      ic_loss_config, ic_dist_config)
            poly       = level_set_to_polygon(ls, ic_num_samples)

t1 = time.perf_counter()

elapsed_ms   = (t1 - t0) * 1000
per_ic_us    = (t1 - t0) / total_ics * 1e6
per_call_us  = (t1 - t0) / total_calls * 1e6

print(f"Total elapsed  : {elapsed_ms:.1f} ms")
print(f"Per IC         : {per_ic_us:.2f} µs")
print(f"Per C API call : {per_call_us:.2f} µs")

# ---------------------------------------------------------------------------
# Benchmark — compound scs_ic_polygon_2d (1 C API call per IC)
# ---------------------------------------------------------------------------

print()
print(f"total C API calls (compound, 1 per IC): {total_ics}")

t0 = time.perf_counter()

for frame_i, pos_mat in enumerate(positions):
    seat_idxs = seat_idxs_per_frame[frame_i]
    for s in seat_idxs:
        seat_pos = [float(pos_mat[s, 0]), float(pos_mat[s, 1])]
        for v in range(n_voters):
            poly = ic_polygon_2d(
                voters_x[v], voters_y[v],
                seat_pos[0], seat_pos[1],
                ic_loss_config, ic_dist_config,
                ic_num_samples,
            )

t1 = time.perf_counter()

elapsed_ms_c  = (t1 - t0) * 1000
per_ic_us_c   = (t1 - t0) / total_ics * 1e6
per_call_us_c = per_ic_us_c  # 1 call per IC

print(f"Total elapsed  : {elapsed_ms_c:.1f} ms")
print(f"Per IC         : {per_ic_us_c:.2f} µs")
print(f"Per C API call : {per_call_us_c:.2f} µs")

print()
speedup = elapsed_ms / elapsed_ms_c
print(f"Speedup        : {speedup:.2f}x")
