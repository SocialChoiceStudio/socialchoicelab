"""static_scenarios_demo.py — Static spatial voting plots for three canonical scenarios

Scenarios:
  1. Dougherty & Edward (PUCH 2012, Fig 3)    — 5 voters, SQ at (0.5, 0.5)
  2. DPMR (PUCH 2014)                         — 7 voters, SQ at (0.5, 0.5)
  3. Laing-Olmsted-Bear                        — 5 voters, SQ at (100, 100)
  4. Laing-Olmsted Skewed Star                 — 5 voters, SQ at (0, 0)

Each plot is built in layers. Toggle any layer on or off by
commenting / uncommenting the corresponding line.

Run from the project root:
  export SCS_LIB_PATH=$(pwd)/build
  python python/static_scenarios_demo.py
"""

from __future__ import annotations

import subprocess
from pathlib import Path

import numpy as np

import socialchoicelab as scl
import socialchoicelab.plots as sclp

# ── Common settings ────────────────────────────────────────────────────────────
THEME       = "dark2"   # "dark2" | "set2" | "okabe_ito" | "paired" | "bw"
WIDTH       = 700
HEIGHT      = 650
SHOW_LABELS = True      # draw voter names on the plot

# Output directory for saved HTML files
Path("tmp").mkdir(exist_ok=True)

# ===========================================================================
# 1. Dougherty & Edward (PUCH 2012, Fig 3)
#    5 voters · SQ at (0.5, 0.5)
# ===========================================================================
sc1 = scl.load_scenario("dougherty_edward_p_u_c_h2012_fig3")

# load_scenario returns voters as shape (n, 2); plots API expects flat array
voters1 = sc1["voters"].ravel()
sq1     = np.array([0.5, 0.5])

# Compute geometry ─────────────────────────────────────────────────────────────
hull1 = scl.convex_hull_2d(sc1["voters"])
ws1   = scl.winset_2d(sq1[0], sq1[1], voters1)

# Build plot ───────────────────────────────────────────────────────────────────
fig1 = sclp.plot_spatial_voting(
    voters=voters1,
    sq=sq1,
    voter_names=sc1["voter_names"],
    dim_names=sc1["dim_names"],
    title="Dougherty & Edward (PUCH 2012, Fig 3)",
    show_labels=SHOW_LABELS,
    theme=THEME,
    width=WIDTH,
    height=HEIGHT,
)

# ── Region layers (drawn first, underneath everything else) ───────────────────
# Pareto set / convex hull of voter ideal points
fig1 = sclp.layer_convex_hull(fig1, hull1, theme=THEME)
fig1 = sclp.layer_winset(fig1, ws1, theme=THEME)
fig1 = sclp.layer_ic(fig1, voters1, sq=sq1, line_width=3, theme=THEME)

# Preferred-to regions — requires a status quo
# fig1 = sclp.layer_preferred_regions(fig1, voters1, sq=sq1, theme=THEME)

# ── Point layers ──────────────────────────────────────────────────────────────
fig1 = sclp.layer_centroid(fig1, voters1, theme=THEME)
fig1 = sclp.layer_marginal_median(fig1, voters1, theme=THEME)

out1 = "/tmp/dougherty_edward_2012_py.html"
sclp.save_plot(fig1, out1)
print(f"Saved: {out1}")
subprocess.Popen(["open", out1])

# ===========================================================================
# 2. DPMR (PUCH 2014)
#    7 voters · SQ at (0.5, 0.5)
# ===========================================================================
sc2 = scl.load_scenario("dpmr_puch2014_seven")

voters2 = sc2["voters"].ravel()
sq2     = np.array([0.5, 0.5])

hull2 = scl.convex_hull_2d(sc2["voters"])
ws2   = scl.winset_2d(sq2[0], sq2[1], voters2)

fig2 = sclp.plot_spatial_voting(
    voters=voters2,
    sq=sq2,
    voter_names=sc2["voter_names"],
    dim_names=sc2["dim_names"],
    title="DPMR (PUCH 2014)",
    show_labels=SHOW_LABELS,
    theme=THEME,
    width=WIDTH,
    height=HEIGHT,
)

fig2 = sclp.layer_convex_hull(fig2, hull2, theme=THEME)
fig2 = sclp.layer_winset(fig2, ws2, theme=THEME)
fig2 = sclp.layer_ic(fig2, voters2, sq=sq2, line_width=3, theme=THEME)
# fig2 = sclp.layer_preferred_regions(fig2, voters2, sq=sq2, theme=THEME)
fig2 = sclp.layer_centroid(fig2, voters2, theme=THEME)
fig2 = sclp.layer_marginal_median(fig2, voters2, theme=THEME)

out2 = "/tmp/dpmr_2014_py.html"
sclp.save_plot(fig2, out2)
print(f"Saved: {out2}")
subprocess.Popen(["open", out2])

# ===========================================================================
# 3. Laing-Olmsted-Bear
#    5 voters · status quo at (100, 100) — canonical cycling example
# ===========================================================================
sc3 = scl.load_scenario("laing_olmsted_bear")

voters3 = sc3["voters"].ravel()
sq3     = sc3["status_quo"]   # np.array([100., 100.])

hull3 = scl.convex_hull_2d(sc3["voters"])
ws3   = scl.winset_2d(sq3[0], sq3[1], voters3)

fig3 = sclp.plot_spatial_voting(
    voters=voters3,
    sq=sq3,
    voter_names=sc3["voter_names"],
    dim_names=sc3["dim_names"],
    title="Laing-Olmsted-Bear — Status Quo at (100, 100)",
    show_labels=SHOW_LABELS,
    theme=THEME,
    width=WIDTH,
    height=HEIGHT,
)

# ── Region layers ──────────────────────────────────────────────────────────────
fig3 = sclp.layer_convex_hull(fig3, hull3, theme=THEME)
fig3 = sclp.layer_winset(fig3, ws3, theme=THEME)

# Indifference curves through the SQ (one circle per voter)
fig3 = sclp.layer_ic(fig3, voters3, sq=sq3, line_width=3, theme=THEME)

# Preferred-to regions — filled circles (strictly inside IC = preferred to SQ)
# fig3 = sclp.layer_preferred_regions(fig3, voters3, sq=sq3, theme=THEME)

# ── Point layers ──────────────────────────────────────────────────────────────
fig3 = sclp.layer_centroid(fig3, voters3, theme=THEME)
fig3 = sclp.layer_marginal_median(fig3, voters3, theme=THEME)

out3 = "/tmp/laing_olmsted_bear_py.html"
sclp.save_plot(fig3, out3)
print(f"Saved: {out3}")
subprocess.Popen(["open", out3])

# ===========================================================================
# 4. Laing-Olmsted Skewed Star
#    5 voters · SQ at (0, 0)
# ===========================================================================
sc4 = scl.load_scenario("laing_olmsted_skewed_star")

voters4 = sc4["voters"].ravel()
sq4     = sc4["status_quo"]   # np.array([0., 0.])

hull4 = scl.convex_hull_2d(sc4["voters"])
ws4   = scl.winset_2d(sq4[0], sq4[1], voters4)

fig4 = sclp.plot_spatial_voting(
    voters=voters4,
    sq=sq4,
    voter_names=sc4["voter_names"],
    dim_names=sc4["dim_names"],
    title="Laing-Olmsted Skewed Star — Status Quo at (0, 0)",
    show_labels=SHOW_LABELS,
    theme=THEME,
    width=WIDTH,
    height=HEIGHT,
)

# ── Region layers ──────────────────────────────────────────────────────────────
fig4 = sclp.layer_convex_hull(fig4, hull4, theme=THEME)
fig4 = sclp.layer_winset(fig4, ws4, theme=THEME)

# Indifference curves through the SQ (one circle per voter)
fig4 = sclp.layer_ic(fig4, voters4, sq=sq4, line_width=3, theme=THEME)

# Preferred-to regions — filled circles (strictly inside IC = preferred to SQ)
# fig4 = sclp.layer_preferred_regions(fig4, voters4, sq=sq4, theme=THEME)

# ── Point layers ──────────────────────────────────────────────────────────────
fig4 = sclp.layer_centroid(fig4, voters4, theme=THEME)
fig4 = sclp.layer_marginal_median(fig4, voters4, theme=THEME)

out4 = "/tmp/laing_olmsted_skewed_star_py.html"
sclp.save_plot(fig4, out4)
print(f"Saved: {out4}")
subprocess.Popen(["open", out4])
