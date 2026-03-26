# static_scenarios_demo.R — Static spatial voting plots for canonical scenarios
#
# Scenarios:
#   1. Dougherty & Edward (PUCH 2012, Fig 3)    — 5 voters, SQ at (0.5, 0.5)
#   2. DPMR (PUCH 2014)                         — 7 voters, SQ at (0.5, 0.5)
#   3. Laing-Olmsted-Bear                        — 5 voters, SQ at (100, 100)
#   4. Laing-Olmsted Skewed Star                 — 5 voters, SQ at (0, 0)
#
#   Non-Euclidean ICs (all using Dougherty & Edward voters):
#   5. Manhattan  (L1  / p=1) — diamond-shaped indifference contours
#   6. Chebyshev  (L∞  / p→∞) — square-shaped  indifference contours
#   7. Minkowski  (Lp  / p=3) — intermediate squircle contours
#
# Each plot is built in layers. Toggle any layer on or off by
# commenting / uncommenting the corresponding line.
#
# HOW TO RUN
#   Option A — RStudio: Session > Set Working Directory > To Source File Location
#              (must be the package root folder `.../socialchoicelab/r/` that contains DESCRIPTION).
#              Then source this file or run line-by-line.
#   Option B — From repo root:  setwd("r"); source("static_scenarios_demo.R")
#
# Plots are htmlwidgets (HTML5 canvas): use print(fig) in the console / Viewer, or set
# SAVE_HTML / OPEN_FIRST_IN_BROWSER below and run the block at the bottom.
if (!requireNamespace("devtools", quietly = TRUE)) {
  stop("static_scenarios_demo.R needs devtools: install.packages(\"devtools\")")
}
devtools::load_all(".")

# ── Common settings ────────────────────────────────────────────────────────────
THEME  <- "dark2"   # "dark2" | "set2" | "okabe_ito" | "paired" | "bw"
WIDTH  <- 1000L
HEIGHT <- 950L
SHOW_LABELS <- TRUE   # canvas: alternative labels on-plot when TRUE; voters via legend / hover

# Save self-contained HTML under r/tmp/ and optionally open the first scenario in the browser
SAVE_HTML           <- TRUE
OPEN_FIRST_IN_BROWSER <- interactive()

# ===========================================================================
# 1. Dougherty & Edward (PUCH 2012, Fig 3)
#    5 voters · SQ at (0.5, 0.5)
# ===========================================================================
sc1 <- load_scenario("dougherty_edward_p_u_c_h2012_fig3")

# Compute geometry ─────────────────────────────────────────────────────────────
hull1 <- convex_hull_2d(sc1$voters)
# ws1 — WinSet requires a status quo; uncomment if you define one below
sq1 <- c(0.5, 0.5)
ws1 <- winset_2d(sq1, sc1$voters)

# Build plot ───────────────────────────────────────────────────────────────────
fig1 <- plot_spatial_voting(
  voters      = sc1$voters,
  sq          = sq1,
  voter_names = sc1$voter_names,
  dim_names   = sc1$dim_names,
  title       = "Dougherty & Edward (PUCH 2012)",
  show_labels = SHOW_LABELS,
  theme       = THEME,
  width       = WIDTH,
  height      = HEIGHT
)

# ── Region layers (drawn first, underneath everything else) ───────────────────
# Pareto set / convex hull of voter ideal points
fig1 <- layer_convex_hull(fig1, hull1, theme = THEME)

fig1 <- layer_winset(fig1, ws1, theme = THEME)
fig1 <- layer_ic(fig1, sc1$voters, sq = sq1, line_width = 3L, theme = THEME)

# Preferred-to regions — requires a status quo
# fig1 <- layer_preferred_regions(fig1, sc1$voters, sq = sq1, theme = THEME)

# ── Point layers ──────────────────────────────────────────────────────────────
fig1 <- layer_centroid(fig1, sc1$voters, theme = THEME)
fig1 <- layer_marginal_median(fig1, sc1$voters, theme = THEME)

fig1

# ===========================================================================
# 2. DPMR (PUCH 2014)
#    7 voters · SQ at (0.5, 0.5)
# ===========================================================================
sc2 <- load_scenario("dpmr_puch2014_seven")

hull2 <- convex_hull_2d(sc2$voters)
sq2   <- c(0.5, 0.5)
ws2   <- winset_2d(sq2, sc2$voters)

fig2 <- plot_spatial_voting(
  voters      = sc2$voters,
  sq          = sq2,
  voter_names = sc2$voter_names,
  dim_names   = sc2$dim_names,
  title       = "DPMR (PUCH 2014)",
  show_labels = SHOW_LABELS,
  theme       = THEME,
  width       = WIDTH,
  height      = HEIGHT
)

fig2 <- layer_convex_hull(fig2, hull2, theme = THEME)
fig2 <- layer_winset(fig2, ws2, theme = THEME)
fig2 <- layer_ic(fig2, sc2$voters, sq = sq2, line_width = 3L, theme = THEME)
# fig2 <- layer_preferred_regions(fig2, sc2$voters, sq = sq2, theme = THEME)
fig2 <- layer_centroid(fig2, sc2$voters, theme = THEME)
fig2 <- layer_marginal_median(fig2, sc2$voters, theme = THEME)

fig2

# ===========================================================================
# 3. Laing-Olmsted-Bear
#    5 voters · status quo at (100, 100) — canonical cycling example
# ===========================================================================
sc3 <- load_scenario("laing_olmsted_bear")
sq3 <- sc3$status_quo   # c(100, 100)

hull3 <- convex_hull_2d(sc3$voters)
ws3   <- winset_2d(sq3, sc3$voters)

fig3 <- plot_spatial_voting(
  voters      = sc3$voters,
  sq          = sq3,
  voter_names = sc3$voter_names,
  dim_names   = sc3$dim_names,
  title       = "Laing-Olmsted- Bear (1978)",
  show_labels = SHOW_LABELS,
  theme       = THEME,
  width       = WIDTH,
  height      = HEIGHT
)

# ── Region layers ──────────────────────────────────────────────────────────────
fig3 <- layer_convex_hull(fig3, hull3, theme = THEME)
fig3 <- layer_winset(fig3, ws3, theme = THEME)

# Indifference curves through the SQ (one circle per voter)
fig3 <- layer_ic(fig3, sc3$voters, sq = sq3, line_width = 3L, theme = THEME)

# Preferred-to regions — filled circles (strictly inside IC = preferred to SQ)
# fig3 <- layer_preferred_regions(fig3, sc3$voters, sq = sq3, theme = THEME)

# ── Point layers ──────────────────────────────────────────────────────────────
fig3 <- layer_centroid(fig3, sc3$voters, theme = THEME)
fig3 <- layer_marginal_median(fig3, sc3$voters, theme = THEME)

fig3

# ===========================================================================
# 4. Laing-Olmsted Skewed Star
#    5 voters · SQ at (0, 0)
# ===========================================================================
sc4 <- load_scenario("laing_olmsted_skewed_star")
sq4 <- sc4$status_quo   # c(0, 0)

hull4 <- convex_hull_2d(sc4$voters)
ws4   <- winset_2d(sq4, sc4$voters)

fig4 <- plot_spatial_voting(
  voters      = sc4$voters,
  sq          = sq4,
  voter_names = sc4$voter_names,
  dim_names   = sc4$dim_names,
  title       = "Laing-Olmsted Skewed Star (1978)",
  show_labels = SHOW_LABELS,
  theme       = THEME,
  width       = WIDTH,
  height      = HEIGHT
)

# ── Region layers ──────────────────────────────────────────────────────────────
fig4 <- layer_convex_hull(fig4, hull4, theme = THEME)
fig4 <- layer_winset(fig4, ws4, theme = THEME)

# Indifference curves through the SQ (one circle per voter)
fig4 <- layer_ic(fig4, sc4$voters, sq = sq4, line_width = 3L, theme = THEME)

# Preferred-to regions — filled circles (strictly inside IC = preferred to SQ)
# fig4 <- layer_preferred_regions(fig4, sc4$voters, sq = sq4, theme = THEME)

# ── Point layers ──────────────────────────────────────────────────────────────
fig4 <- layer_centroid(fig4, sc4$voters, theme = THEME)
fig4 <- layer_marginal_median(fig4, sc4$voters, theme = THEME)

fig4

# ===========================================================================
# 5–7. Non-Euclidean indifference contours
#      All three reuse the Dougherty & Edward voters so the shape change is
#      easy to see.  Compare with fig1 (Euclidean circles).
# ===========================================================================

# Reuse sc1 / hull1 / sq1 already computed above.
# Each plot shows ICs, preferred regions, AND the winset — all under the
# same non-Euclidean metric, so all three layers are self-consistent.




# ===========================================================================
# 5. Manhattan (L1 / p=1) — diamond-shaped ICs and winset
# ===========================================================================
dist_l1 <- make_dist_config("manhattan")

fig5 <- plot_spatial_voting(
  voters      = sc1$voters,
  sq          = sq1,
  voter_names = sc1$voter_names,
  dim_names   = sc1$dim_names,
  title       = "Dougherty & Edward — Manhattan (L1) indifference contours",
  show_labels = SHOW_LABELS,
  theme       = THEME,
  width       = WIDTH,
  height      = HEIGHT
)
fig5 <- layer_convex_hull(fig5, hull1, theme = THEME)
fig5 <- layer_winset(fig5, voters = sc1$voters, sq = sq1,
                     dist_config = dist_l1, theme = THEME)
fig5 <- layer_ic(fig5, sc1$voters, sq = sq1, dist_config = dist_l1,
                 line_width = 3L, theme = THEME)
fig5 <- layer_preferred_regions(fig5, sc1$voters, sq = sq1,
                                dist_config = dist_l1, theme = THEME)
fig5 <- layer_centroid(fig5, sc1$voters, theme = THEME)
fig5 <- layer_marginal_median(fig5, sc1$voters, theme = THEME)



# ===========================================================================
# 6. Chebyshev (L∞ / p→∞) — square-shaped ICs and winset
# ===========================================================================
dist_linf <- make_dist_config("chebyshev")

fig6 <- plot_spatial_voting(
  voters      = sc1$voters,
  sq          = sq1,
  voter_names = sc1$voter_names,
  dim_names   = sc1$dim_names,
  title       = "Dougherty & Edward — Chebyshev (L\u221e) indifference contours",
  show_labels = SHOW_LABELS,
  theme       = THEME,
  width       = WIDTH,
  height      = HEIGHT
)
fig6 <- layer_convex_hull(fig6, hull1, theme = THEME)
fig6 <- layer_winset(fig6, voters = sc1$voters, sq = sq1,
                     dist_config = dist_linf, theme = THEME)
fig6 <- layer_ic(fig6, sc1$voters, sq = sq1, dist_config = dist_linf,
                 line_width = 3L, theme = THEME)
fig6 <- layer_preferred_regions(fig6, sc1$voters, sq = sq1,
                                dist_config = dist_linf, theme = THEME)
fig6 <- layer_centroid(fig6, sc1$voters, theme = THEME)
fig6 <- layer_marginal_median(fig6, sc1$voters, theme = THEME)


# ===========================================================================
# 7. Minkowski p=3 — intermediate squircle ICs and winset
# ===========================================================================
dist_l3 <- make_dist_config("minkowski", order_p = 3.0)

fig7 <- plot_spatial_voting(
  voters      = sc1$voters,
  sq          = sq1,
  voter_names = sc1$voter_names,
  dim_names   = sc1$dim_names,
  title       = "Dougherty & Edward — Minkowski (p=3) indifference contours",
  show_labels = SHOW_LABELS,
  theme       = THEME,
  width       = WIDTH,
  height      = HEIGHT
)
fig7 <- layer_convex_hull(fig7, hull1, theme = THEME)
fig7 <- layer_winset(fig7, voters = sc1$voters, sq = sq1,
                     dist_config = dist_l3, theme = THEME)
fig7 <- layer_ic(fig7, sc1$voters, sq = sq1, dist_config = dist_l3,
                 line_width = 3L, theme = THEME)
fig7 <- layer_preferred_regions(fig7, sc1$voters, sq = sq1,
                                dist_config = dist_l3, theme = THEME)
fig7 <- layer_centroid(fig7, sc1$voters, theme = THEME)
fig7 <- layer_marginal_median(fig7, sc1$voters, theme = THEME)



# ===========================================================================
# Display in RStudio Viewer / console (last line wins if you source the whole file)
# ===========================================================================
fig1

# ===========================================================================
# Optional: save HTML and open in your default browser (no RStudio needed)
# ===========================================================================
if (isTRUE(SAVE_HTML)) {
  tmp_dir <- file.path(getwd(), "tmp")
  dir.create(tmp_dir, showWarnings = FALSE, recursive = TRUE)
  save_plot(fig1, file.path(tmp_dir, "dougherty_edward_2012.html"))
  save_plot(fig2, file.path(tmp_dir, "dpmr_2014.html"))
  save_plot(fig3, file.path(tmp_dir, "laing_olmsted_bear.html"))
  save_plot(fig4, file.path(tmp_dir, "laing_olmsted_skewed_star.html"))
  save_plot(fig5, file.path(tmp_dir, "dougherty_edward_manhattan.html"))
  save_plot(fig6, file.path(tmp_dir, "dougherty_edward_chebyshev.html"))
  save_plot(fig7, file.path(tmp_dir, "dougherty_edward_minkowski_p3.html"))
  message("Saved HTML under: ", normalizePath(tmp_dir, winslash = "/"))
  if (isTRUE(OPEN_FIRST_IN_BROWSER)) {
    p1 <- normalizePath(file.path(tmp_dir, "dougherty_edward_2012.html"))
    utils::browseURL(paste0("file://", p1))
  }
}
