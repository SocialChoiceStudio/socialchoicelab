"""TestScript_11_Tovey.py — Tovey Regular Polygon scenario
==========================================================
Scenario: Regular Polygon Model (11 players)
  Space:  0–100 × 0–100
  Rule:   Simple majority (0.5)

Run from the project root:
  source .venv/bin/activate
  export SCS_LIB_PATH=$(pwd)/build
  python python/TestScript_11_Tovey.py

Layers plotted:
  - Voter ideal points + Status Quo   [plot_spatial_voting]
  - Pareto set (convex hull)           [layer_convex_hull]
  - Win-set of the Status Quo          [layer_winset]

NOTE: preferred-to sets not yet implemented (C13.6).
"""

import numpy as np
import socialchoicelab as scl
import socialchoicelab.plots as sclp

# ---------------------------------------------------------------------------
# Scenario data
# ---------------------------------------------------------------------------

scenario = scl.load_scenario("tovey_regular_polygon")

voters = scenario["voters"]
sq = scenario["status_quo"]
voter_names = scenario["voter_names"]

# ---------------------------------------------------------------------------
# Compute
# ---------------------------------------------------------------------------

hull = scl.convex_hull_2d(voters)
ws   = scl.winset_2d(sq[0], sq[1], voters)

print(f"Win-set empty: {ws.is_empty()}")
if not ws.is_empty():
    bbox = ws.bbox()
    print(f"Win-set bbox: [{bbox['min_x']:.2f}, {bbox['max_x']:.2f}]"
          f" x [{bbox['min_y']:.2f}, {bbox['max_y']:.2f}]")

# ---------------------------------------------------------------------------
# Plot
# ---------------------------------------------------------------------------

fig = sclp.plot_spatial_voting(
    voters,
    sq          = sq,
    voter_names = voter_names,
    dim_names   = tuple(scenario["dim_names"]),
    title       = scenario["name"],
)

fig = sclp.layer_convex_hull(fig, hull)
fig = sclp.layer_winset(fig, ws)
fig = sclp.finalize_plot(fig)

fig.show()
