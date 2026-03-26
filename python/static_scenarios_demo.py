"""static_scenarios_demo.py — Static spatial voting plots for canonical scenarios

Scenarios:
  1. Dougherty & Edward (PUCH 2012, Fig 3)    — 5 voters, SQ at (0.5, 0.5)
  2. DPMR (PUCH 2014)                         — 7 voters, SQ at (0.5, 0.5)
  3. Laing-Olmsted-Bear                        — 5 voters, SQ at (100, 100)
  4. Laing-Olmsted Skewed Star                 — 5 voters, SQ at (0, 0)

  Non-Euclidean ICs (all using Dougherty & Edward voters):
  5. Manhattan  (L1  / p=1) — diamond-shaped indifference contours
  6. Chebyshev  (L∞  / p→∞) — square-shaped  indifference contours
  7. Minkowski  (Lp  / p=3) — intermediate squircle contours

Each plot is built in layers. Toggle any layer on or off by
commenting / uncommenting the corresponding line.

Run from the project root:
  export SCS_LIB_PATH=$(pwd)/build
  python python/static_scenarios_demo.py

Writes self-contained canvas HTML under ./tmp/ (HTML5 canvas, no Plotly).
Set OPEN_FIRST_IN_BROWSER = True to open only the first plot in your default browser.
"""

from __future__ import annotations

import webbrowser
from pathlib import Path

import numpy as np

import socialchoicelab as scl
import socialchoicelab.plots as sclp

# ── Common settings ────────────────────────────────────────────────────────────
THEME         = "dark2"   # "dark2" | "set2" | "okabe_ito" | "paired" | "bw"
WIDTH         = 700
HEIGHT        = 650
SHOW_LABELS   = True  # alternative labels on-canvas; voters via legend / hover

# Project-root output dir (created if missing)
OUTPUT_DIR = Path(__file__).resolve().parent.parent / "tmp"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

OPEN_FIRST_IN_BROWSER = True   # open only scenario 1; set False to skip browser


def _save_and_maybe_open(path: Path, *, open_browser: bool) -> None:
    path = path.resolve()
    print(f"Saved: {path}")
    if open_browser:
        webbrowser.open(path.as_uri())

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

out1 = OUTPUT_DIR / "dougherty_edward_2012.html"
sclp.save_plot(fig1, str(out1))
_save_and_maybe_open(out1, open_browser=OPEN_FIRST_IN_BROWSER)

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

out2 = OUTPUT_DIR / "dpmr_2014.html"
sclp.save_plot(fig2, str(out2))
_save_and_maybe_open(out2, open_browser=False)

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

out3 = OUTPUT_DIR / "laing_olmsted_bear.html"
sclp.save_plot(fig3, str(out3))
_save_and_maybe_open(out3, open_browser=False)

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

out4 = OUTPUT_DIR / "laing_olmsted_skewed_star.html"
sclp.save_plot(fig4, str(out4))
_save_and_maybe_open(out4, open_browser=False)

# ===========================================================================
# 5–7. Non-Euclidean indifference contours + winsets
#      All three reuse the Dougherty & Edward voters so the shape change is
#      easy to see.  Compare with plot 1 (Euclidean circles and winset).
# ===========================================================================

# Reuse voters / hull already computed for scenario 1.
# Each plot shows ICs, preferred regions, AND the winset — all under the
# same non-Euclidean metric, so all three layers are self-consistent.

# ===========================================================================
# 5. Manhattan (L1 / p=1) — diamond-shaped ICs and winset
# ===========================================================================
dist_l1 = scl.make_dist_config("manhattan")

fig5 = sclp.plot_spatial_voting(
    voters=voters1,
    sq=sq1,
    voter_names=sc1["voter_names"],
    dim_names=sc1["dim_names"],
    title="Dougherty & Edward — Manhattan (L1) indifference contours",
    show_labels=SHOW_LABELS,
    theme=THEME,
    width=WIDTH,
    height=HEIGHT,
)
fig5 = sclp.layer_convex_hull(fig5, hull1, theme=THEME)
fig5 = sclp.layer_winset(fig5, voters=voters1, sq=sq1,
                          dist_config=dist_l1, theme=THEME)
fig5 = sclp.layer_ic(fig5, voters1, sq=sq1, dist_config=dist_l1,
                     line_width=3, theme=THEME)
fig5 = sclp.layer_preferred_regions(fig5, voters1, sq=sq1,
                                     dist_config=dist_l1, theme=THEME)
fig5 = sclp.layer_centroid(fig5, voters1, theme=THEME)
fig5 = sclp.layer_marginal_median(fig5, voters1, theme=THEME)

out5 = OUTPUT_DIR / "dougherty_edward_manhattan.html"
sclp.save_plot(fig5, str(out5))
_save_and_maybe_open(out5, open_browser=False)

# ===========================================================================
# 6. Chebyshev (L∞ / p→∞) — square-shaped ICs and winset
# ===========================================================================
dist_linf = scl.make_dist_config("chebyshev")

fig6 = sclp.plot_spatial_voting(
    voters=voters1,
    sq=sq1,
    voter_names=sc1["voter_names"],
    dim_names=sc1["dim_names"],
    title="Dougherty & Edward — Chebyshev (L\u221e) indifference contours",
    show_labels=SHOW_LABELS,
    theme=THEME,
    width=WIDTH,
    height=HEIGHT,
)
fig6 = sclp.layer_convex_hull(fig6, hull1, theme=THEME)
fig6 = sclp.layer_winset(fig6, voters=voters1, sq=sq1,
                          dist_config=dist_linf, theme=THEME)
fig6 = sclp.layer_ic(fig6, voters1, sq=sq1, dist_config=dist_linf,
                     line_width=3, theme=THEME)
fig6 = sclp.layer_preferred_regions(fig6, voters1, sq=sq1,
                                     dist_config=dist_linf, theme=THEME)
fig6 = sclp.layer_centroid(fig6, voters1, theme=THEME)
fig6 = sclp.layer_marginal_median(fig6, voters1, theme=THEME)

out6 = OUTPUT_DIR / "dougherty_edward_chebyshev.html"
sclp.save_plot(fig6, str(out6))
_save_and_maybe_open(out6, open_browser=False)

# ===========================================================================
# 7. Minkowski p=3 — intermediate squircle ICs and winset
# ===========================================================================
dist_l3 = scl.make_dist_config("minkowski", order_p=3.0)

fig7 = sclp.plot_spatial_voting(
    voters=voters1,
    sq=sq1,
    voter_names=sc1["voter_names"],
    dim_names=sc1["dim_names"],
    title="Dougherty & Edward — Minkowski (p=3) indifference contours",
    show_labels=SHOW_LABELS,
    theme=THEME,
    width=WIDTH,
    height=HEIGHT,
)
fig7 = sclp.layer_convex_hull(fig7, hull1, theme=THEME)
fig7 = sclp.layer_winset(fig7, voters=voters1, sq=sq1,
                          dist_config=dist_l3, theme=THEME)
fig7 = sclp.layer_ic(fig7, voters1, sq=sq1, dist_config=dist_l3,
                     line_width=3, theme=THEME)
fig7 = sclp.layer_preferred_regions(fig7, voters1, sq=sq1,
                                     dist_config=dist_l3, theme=THEME)
fig7 = sclp.layer_centroid(fig7, voters1, theme=THEME)
fig7 = sclp.layer_marginal_median(fig7, voters1, theme=THEME)

out7 = OUTPUT_DIR / "dougherty_edward_minkowski_p3.html"
sclp.save_plot(fig7, str(out7))
_save_and_maybe_open(out7, open_browser=False)
