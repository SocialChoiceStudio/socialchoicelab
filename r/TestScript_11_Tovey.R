# TestScript_11_Tovey.R — Tovey Regular Polygon scenario
# =========================================================
# Scenario: Regular Polygon Model (11 players)
#   Space:  0–100 × 0–100
#   Rule:   Simple majority (0.5)
#
# Run in RStudio (r/ as working directory):
#   devtools::load_all()
#   source("TestScript_11_Tovey.R")
#
# Layers plotted:
#   - Voter ideal points + Status Quo   [plot_spatial_voting]
#   - Pareto set (convex hull)           [layer_convex_hull]
#   - Win-set of the Status Quo          [layer_winset]
#
# NOTE: preferred-to sets not yet implemented (C13.6).

# After any change to the R package source, run at the console:
devtools::document()   # regenerate NAMESPACE + man pages
devtools::install()    # reinstall the package

library(socialchoicelab)
library(plotly)
library(listviewer)
# ---------------------------------------------------------------------------
# Scenario data
# ---------------------------------------------------------------------------

scenario <- load_scenario("tovey_regular_polygon")

voters <- scenario$voters
sq     <- scenario$status_quo
voter_names <- scenario$voter_names

# ---------------------------------------------------------------------------
# Compute
# ---------------------------------------------------------------------------

hull <- convex_hull_2d(voters)
ws   <- winset_2d(sq, voters)

cat("Win-set empty:", ws$is_empty(), "\n")
if (!ws$is_empty()) {
  bbox <- ws$bbox()
  cat(sprintf("Win-set bbox: [%.2f, %.2f] x [%.2f, %.2f]\n",
              bbox$min_x, bbox$max_x, bbox$min_y, bbox$max_y))
}

# ---------------------------------------------------------------------------
# Plot
# ---------------------------------------------------------------------------

fig <- plot_spatial_voting(
  voters,
  sq          = sq,
  voter_names = voter_names,
  dim_names   = scenario$dim_names,
  title       = scenario$name
)

fig <- layer_convex_hull(fig, hull)
fig <- layer_winset(fig, ws)
fig <- finalize_plot(fig)

print(fig)

print(fig)

cat(plotly_json(fig, pretty = TRUE, jsonedit = FALSE))
