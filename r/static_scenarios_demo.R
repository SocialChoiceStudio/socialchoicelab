# static_scenarios_demo.R — Static spatial voting plots for three canonical scenarios
#
# Scenarios:
#   1. Dougherty & Edward (PUCH 2012, Fig 3)    — 5 voters, SQ at (0.5, 0.5)
#   2. DPMR (PUCH 2014)                         — 7 voters, SQ at (0.5, 0.5)
#   3. Laing-Olmsted-Bear                        — 5 voters, SQ at (100, 100)
#   4. Laing-Olmsted Skewed Star                 — 5 voters, SQ at (0, 0)
#
# Each plot is built in layers. Toggle any layer on or off by
# commenting / uncommenting the corresponding line.
#
# HOW TO RUN IN RSTUDIO:
#   Session > Set Working Directory > To Source File Location
#   Then source or step through with Cmd+Enter.
devtools::load_all("r/")

library(socialchoicelab)

# ── Common settings ────────────────────────────────────────────────────────────
THEME  <- "dark2"   # "dark2" | "set2" | "okabe_ito" | "paired" | "bw"
WIDTH  <- 1000L
HEIGHT <- 950L
SHOW_LABELS <- TRUE   # draw voter names on the plot

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

#fig1

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

#fig2

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

#fig3

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

#fig4

# ===========================================================================
# Optional: save any plot to HTML (self-contained, no server needed)
# ===========================================================================
# dir.create("tmp", showWarnings = FALSE)
# save_plot(fig1, "tmp/dougherty_edward_2012.html")
# save_plot(fig2, "tmp/dpmr_2014.html")
# save_plot(fig3, "tmp/laing_olmsted_bear.html")
# save_plot(fig4, "tmp/laing_olmsted_skewed_star.html")



fig4
fig3
fig2
fig1
