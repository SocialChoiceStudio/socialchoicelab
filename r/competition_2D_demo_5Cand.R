# competition_2D_demo_5Cand.R
#
# HOW TO RUN IN RSTUDIO:
#   Session > Set Working Directory > To Source File Location
#   (picks up r/.Renviron which sets SCS_LIB_PATH / DYLD_LIBRARY_PATH)
#   Then source or step through with Cmd+Enter.
devtools::load_all("r/")

library(socialchoicelab)
library(htmlwidgets)

# Voters — Gaussian cloud (parallel to Python demo)
# IC load: 20 voters × 1 seat × 201 frames = 4020 curves (80% of ic_max_curves = 5000).
sm_voters <- stream_manager(123L, c("voters"))
voter_cfg  <- make_voter_sampler("gaussian", mean = 0.0, sd = 0.5)
voters     <- draw_voters(20L, voter_cfg, sm_voters, "voters")

# --------------------------------------------------------------------------
# Five-candidate race: sticker, hunter, aggregator, hunter, predator
# --------------------------------------------------------------------------
sm <- stream_manager(42L, c("competition_adaptation_hunter", "voters", "competition_motion_step_sizes"))
headings <- c(0.0, 0.0,   0.71, -0.71,   -1.0, 0.0,   0.71, 0.71,   0.0, -1.0)
trace <- competition_run(
  competitor_positions = c(-1.2, -1.0,   1.2, -1.0,   0.0, 1.3,   -1.1, 0.8,   1.1, 0.8),
  competitor_headings  = headings,
  strategy_kinds       = c("sticker", "hunter", "aggregator", "hunter", "predator"),
  voter_ideals         = voters,
  dist_config          = make_dist_config(n_dims = 2L),
  engine_config        = make_competition_engine_config(
    seat_count  = 1L,
    seat_rule   = "plurality_top_k",
    max_rounds  = 200L,
    step_config = make_competition_step_config(kind = "random_uniform", min_step_size = 0.0, max_step_size = 0.12)
  ),
  stream_manager = sm
)
cat("n_rounds:", trace$dims()$n_rounds, "\n")
cat("final positions:\n")
print(trace$final_positions())

# Static geometry overlays: centroid, marginal median, Pareto Set (convex hull, Euclidean only)
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
COMPUTE_WINSET <- FALSE   # commented out for demo; use Voronoi instead
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
  competitor_names = c("Candidate A", "Candidate B", "Candidate C", "Candidate D", "Candidate E"),
  title            = "Five Candidate Competition",
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
# htmlwidgets::saveWidget(w_canvas, "/tmp/competition_2D_demo_5Cand_r.html", selfcontained = TRUE)
# browseURL("/tmp/competition_2D_demo_5Cand_r.html")
