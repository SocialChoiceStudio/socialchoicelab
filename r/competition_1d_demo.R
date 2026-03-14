# competition_1d_demo.R — One-dimensional candidate competition demo
#
# HOW TO RUN IN RSTUDIO:
#   Session > Set Working Directory > To Source File Location
#   (picks up r/.Renviron which sets SCS_LIB_PATH / DYLD_LIBRARY_PATH)
#   Then source or step through with Cmd+Enter.
devtools::load_all("r/")

library(socialchoicelab)
library(htmlwidgets)

# ---------------------------------------------------------------------------
# Voters — 1D Gaussian cloud generated directly with R's RNG.
# draw_voters() only supports 2D, so we use rnorm() here.
# IC load: 20 voters × 1 seat × 201 frames = 4020 intervals.
# ---------------------------------------------------------------------------
set.seed(123L)
voters_1d <- runif(101L, min = -1, max = 1)   # flat vector of 20 ideal points

cat("voters (1D):", length(voters_1d), "ideal points\n")
cat("range:", range(voters_1d), "\n")

# ---------------------------------------------------------------------------
# Competition — hunter-sticker vs hunter-sticker on the real line.
# Each candidate hunts for votes until it wins a seat, then freezes.
# ---------------------------------------------------------------------------
sm <- stream_manager(42L, c("competition_adaptation_hunter"))

trace <- competition_run(
  competitor_positions = c(-.8, .7),    # 1D: two scalars
  competitor_headings  = c(1.0, -1.0),    # pointing toward centre
  strategy_kinds       = c("hunter_sticker", "hunter_sticker"),
  voter_ideals         = voters_1d,
  dist_config          = make_dist_config(n_dims = 1L),
  engine_config        = make_competition_engine_config(
    seat_count  = 1L,
    seat_rule   = "plurality_top_k",
    max_rounds  = 200L,
    step_config = make_competition_step_config(kind = "fixed", fixed_step_size = 0.06)
  ),
  stream_manager = sm
)
cat("n_rounds:", trace$dims()$n_rounds, "\n")
cat("final positions:\n")
print(trace$final_positions())

# ---------------------------------------------------------------------------
# Overlays — 1D centroid and marginal median (computed inline from voters)
# For 1D traces, pass any named list to select which overlays to show.
# The values are ignored; the function computes mean/median internally.
# Note: pareto_set and geometric_mean are not supported for 1D traces.
# ---------------------------------------------------------------------------

# ── Overlay toggles ───────────────────────────────────────────────────────────
# COMPUTE_IC    — per-voter indifference curves (2 vertical lines per voter).
#                 Load: 101 voters × 1 seat × 201 frames = ~20 k intervals.
# COMPUTE_WINSET — 1D WinSet interval (lo, hi) for each seat holder per frame.
#                 Load: 1 seat × 201 frames = 201 intervals.
COMPUTE_IC     <- TRUE
COMPUTE_WINSET <- TRUE
# ─────────────────────────────────────────────────────────────────────────────

w_canvas <- animate_competition_canvas(
  trace,
  voters           = voters_1d,
  competitor_names = c("Candidate A", "Candidate B"),
  title            = "1D Hunter-Sticker Competition",
  trail            = "fade",
  dim_names        = c("Policy Dimension"),
  overlays         = list(centroid = TRUE, marginal_median = TRUE),
  width            = 400L,
  height           = 400L,
  compute_ic       = COMPUTE_IC,
  compute_winset   = COMPUTE_WINSET
)

# R Viewer
w_canvas

# Browser
# htmlwidgets::saveWidget(w_canvas, "/tmp/competition_1d_demo_r.html", selfcontained = TRUE)
# browseURL("/tmp/competition_1d_demo_r.html")
