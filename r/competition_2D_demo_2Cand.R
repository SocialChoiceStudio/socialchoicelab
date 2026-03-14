# competition_2D_demo_2Cand.R
#
# HOW TO RUN IN RSTUDIO:
#   Session > Set Working Directory > To Source File Location
#   (picks up r/.Renviron which sets SCS_LIB_PATH / DYLD_LIBRARY_PATH)
#   Then source or step through with Cmd+Enter.
devtools::load_all("r/")

library(socialchoicelab)
library(htmlwidgets)


# Option A: load a pre-defined scenario (uncomment to use)
# doughertyEdward <- load_scenario("dougherty_edward_p_u_c_h2012_fig3")
# voters <- doughertyEdward$voters

# Option B: generate voters from a Gaussian distribution
# IC load: 20 voters × 1 seat × 201 frames = 4020 curves (80% of ic_max_curves = 5000).
sm_voters <- stream_manager(123L, c("voters"))
voter_cfg  <- make_voter_sampler("uniform", lo = -.75, hi = .75)
voters     <- draw_voters(10L, voter_cfg, sm_voters, "voters")

# --------------------------------------------------------------------------
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
sm <- stream_manager(42L, c("competition_adaptation_hunter", "voters", "competition_motion_step_sizes"))
headings <- c(0.71, 0.71,  -0.71, -0.71)   # toward voter cluster centre
trace <- competition_run(
  competitor_positions = c(-1, -1,   1, 1),
  competitor_headings  = headings,
  strategy_kinds       = c("hunter", "aggregator"),
  voter_ideals         = voters,
  dist_config          = make_dist_config(n_dims = 2L),
  engine_config        = make_competition_engine_config(
    seat_count  = 1L,
    seat_rule   = "plurality_top_k",
    max_rounds  = 500L,
    step_config = make_competition_step_config(kind = "random_uniform", min_step_size = 0.0, max_step_size = 0.12)
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

# ── Indifference-curve toggle ─────────────────────────────────────────────────
# Set compute_ic = TRUE to compute and display per-voter ICs through each seat
# holder's position at every frame.  Set to FALSE to omit them entirely (the
# "Indiff. Curves" checkbox will not appear in the player UI).
# Current load: 20 voters × 1 seat × 201 frames = 4020 curves.
COMPUTE_IC <- TRUE
# ─────────────────────────────────────────────────────────────────────────────

# ── WinSet toggle ─────────────────────────────────────────────────────────────
# Set compute_winset = TRUE to compute and display the WinSet boundary for each
# seat holder's position at every frame.  Set to FALSE to omit it entirely (the
# "Win Set" checkbox will not appear in the player UI).
# Current load: 1 seat × 201 frames = 201 WinSet computations.
# COMPUTE_WINSET <- TRUE
COMPUTE_WINSET <- TRUE   # commented out for demo; use Voronoi instead
# ─────────────────────────────────────────────────────────────────────────────

# ── Voronoi toggle ───────────────────────────────────────────────────────────
# Set compute_voronoi = TRUE to compute Euclidean Voronoi (candidate regions)
# per frame and show the "Voronoi" checkbox in the player UI.
COMPUTE_VORONOI <- TRUE
# ─────────────────────────────────────────────────────────────────────────────

# Canvas version
w_canvas <- animate_competition_canvas(
  trace,
  voters           = voters,
  competitor_names = c("Candidate A", "Candidate B"),
  title            = "Two Candidate Competition",
  trail            = "none",
  overlays         = overlays,
  width            = 950L,
  height           = 850L,
  compute_ic       = COMPUTE_IC,
  ic_max_curves    = 15000L,
  compute_winset   = COMPUTE_WINSET,
  compute_voronoi  = COMPUTE_VORONOI
)


# R Viewer
w_canvas


# Browser
# htmlwidgets::saveWidget(w_canvas, "/tmp/competition_demo_r.html", selfcontained = TRUE)
# browseURL("/tmp/competition_demo_r.html")

