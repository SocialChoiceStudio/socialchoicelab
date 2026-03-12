# competition_demo.R
#
# HOW TO RUN IN RSTUDIO:
#   Session > Set Working Directory > To Source File Location
#   (picks up r/.Renviron which sets SCS_LIB_PATH / DYLD_LIBRARY_PATH)
#   Then source or step through with Cmd+Enter.
devtools::load_all("r/")

library(socialchoicelab)
library(htmlwidgets)

# 15 voters spread across 2D space
# voters <- c(
#   -1.2, -0.9,
#   -1.0,  0.7,
#   -0.8, -0.4,
#   -0.6,  1.0,
#   -0.4, -0.8,
#   -0.2,  0.3,
#    0.0,  0.0,
#    0.2, -0.6,
#    0.3,  0.9,
#    0.5,  0.2,
#    0.6, -0.9,
#    0.8,  0.5,
#    0.9, -0.3,
#    1.0,  0.8,
#    1.1, -0.7
# )

# Option A: load a pre-defined scenario (uncomment to use)
# doughertyEdward <- load_scenario("dougherty_edward_p_u_c_h2012_fig3")
# voters <- doughertyEdward$voters

# Option B: generate voters from a Gaussian distribution
sm_voters <- stream_manager(123L, c("voters"))
voter_cfg  <- make_voter_sampler("gaussian", mean = 0, sd = 0.5)
voters     <- draw_voters(2000L, voter_cfg, sm_voters, "voters")

# --------------------------------------------------------------------------
# Option A: aggregator — clearly converges toward voter centroid. Good demo.
# Commented out while testing hunter.
# --------------------------------------------------------------------------
# trace <- competition_run(
#   competitor_positions = c(-1.5, -1.2,   1.5, 1.2),
#   strategy_kinds       = c("aggregator", "aggregator"),
#   voter_ideals         = voters,
#   dist_config          = make_dist_config(n_dims = 2L),
#   engine_config        = make_competition_engine_config(
#     seat_count  = 1L,
#     seat_rule   = "plurality_top_k",
#     max_rounds  = 40L,
#     step_config = make_competition_step_config(
#       kind            = "fixed",
#       fixed_step_size = 0.12
#     )
#   )
# )

# --------------------------------------------------------------------------
# Option B: hunter — reinforce-or-reverse (Fowler & Laver 2008). On a
# performance drop the strategy picks a random direction uniformly from the
# backward half-space (not a rigid 180° flip), so convergence is stochastic
# but genuinely exploratory.
# --------------------------------------------------------------------------
sm <- stream_manager(42L, c("competition_adaptation_hunter", "voters"))
headings <- c(0.71, 0.71,  -0.71, -0.71)   # toward voter cluster centre
trace <- competition_run(
  competitor_positions = c(-1.5, -1.2,   1.5, 1.2),
  competitor_headings  = headings,
  strategy_kinds       = c("hunter", "hunter"),
  voter_ideals         = voters,
  dist_config          = make_dist_config(n_dims = 2L),
  engine_config        = make_competition_engine_config(
    seat_count  = 1L,
    seat_rule   = "plurality_top_k",
    max_rounds  = 400L,
    step_config = make_competition_step_config(kind = "fixed", fixed_step_size = 0.12)
  ),
  stream_manager = sm
)
cat("n_rounds:", trace$dims()$n_rounds, "\n")
cat("final positions:\n")
print(trace$final_positions())

# Static geometry overlays: centroid and Pareto Set (convex hull, Euclidean only)
overlays <- list(
  centroid        = centroid_2d(voters),
  marginal_median = marginal_median_2d(voters),
  pareto_set      = convex_hull_2d(voters)
)

# Canvas version
w_canvas <- animate_competition_canvas(
  trace,
  voters           = voters,
  competitor_names = c("Candidate A", "Candidate B"),
  title            = "Two Candidate Competition",
  trail            = "none",
  overlays         = overlays,
  width            = 800L,
  height           = 700L
)


# R Viewer
w_canvas


# Browser
# htmlwidgets::saveWidget(w_canvas, "/tmp/competition_demo_r.html", selfcontained = TRUE)
# browseURL("/tmp/competition_demo_r.html")





# View plotly inline in RStudio Viewer
# w_plotly
