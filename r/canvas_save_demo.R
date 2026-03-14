# canvas_save_demo.R — Generate competition canvases and save to .scscanvas files.
#
# Creates a 1D and a 2D competition canvas (10 voters, 10 rounds each) and
# saves both to /tmp/ as .scscanvas files. Run canvas_load_demo.R to reload
# them without recomputing anything.
#
# HOW TO RUN IN RSTUDIO:
#   Session > Set Working Directory > To Source File Location
#   (picks up r/.Renviron which sets SCS_LIB_PATH / DYLD_LIBRARY_PATH)
#   Then source or step through with Cmd+Enter.
devtools::load_all("r/")

library(socialchoicelab)
library(htmlwidgets)

# Shared output paths — also used by canvas_load_demo.R and the Python pair.
PATH_1D <- "/tmp/canvas_1d_demo.scscanvas"
PATH_2D <- "/tmp/canvas_2d_demo.scscanvas"

# ===========================================================================
# 1D competition — 10 voters, 10 rounds, 2 sticker candidates
# ===========================================================================
set.seed(7L)
voters_1d <- runif(10L, min = -1, max = 1)
cat("1D voters:", round(voters_1d, 3), "\n")

trace_1d <- competition_run(
  competitor_positions = c(-0.8, 0.8),
  strategy_kinds       = c("sticker", "sticker"),
  voter_ideals         = voters_1d,
  dist_config          = make_dist_config(n_dims = 1L),
  engine_config        = make_competition_engine_config(
    seat_count  = 1L,
    seat_rule   = "plurality_top_k",
    max_rounds  = 10L,
    step_config = make_competition_step_config(kind = "fixed", fixed_step_size = 0.1)
  )
)
cat("1D rounds:", trace_1d$dims()$n_rounds, "\n")

w_1d <- animate_competition_canvas(
  trace_1d,
  voters           = voters_1d,
  competitor_names = c("Candidate A", "Candidate B"),
  title            = "1D Sticker Competition (saved)",
  trail            = "fade",
  dim_names        = c("Policy Dimension"),
  overlays         = list(centroid = TRUE, marginal_median = TRUE),
  width            = 900L,
  height           = 400L,
  compute_ic       = TRUE,
  compute_winset   = TRUE
)

# Preview in RStudio Viewer
w_1d

# Save payload to .scscanvas — reload later with load_competition_canvas().
save_competition_canvas(w_1d, PATH_1D)
cat("Saved 1D canvas to:", PATH_1D, "\n")

# ===========================================================================
# 2D competition — 10 voters, 10 rounds, 2 hunter candidates
# ===========================================================================
sm_v      <- stream_manager(99L, c("voters"))
voter_cfg <- make_voter_sampler("uniform", lo = -0.75, hi = 0.75)
voters_2d <- draw_voters(10L, voter_cfg, sm_v, "voters")
cat("2D voters: 10 points in 2D\n")

sm <- stream_manager(42L, c("competition_adaptation_hunter"))
trace_2d <- competition_run(
  competitor_positions = c(-0.8, -0.5,   0.8, 0.5),
  competitor_headings  = c( 0.71,  0.71, -0.71, -0.71),
  strategy_kinds       = c("hunter", "hunter"),
  voter_ideals         = voters_2d,
  dist_config          = make_dist_config(n_dims = 2L),
  engine_config        = make_competition_engine_config(
    seat_count  = 1L,
    seat_rule   = "plurality_top_k",
    max_rounds  = 10L,
    step_config = make_competition_step_config(kind = "fixed", fixed_step_size = 0.1)
  ),
  stream_manager = sm
)
cat("2D rounds:", trace_2d$dims()$n_rounds, "\n")

w_2d <- animate_competition_canvas(
  trace_2d,
  voters           = voters_2d,
  competitor_names = c("Candidate A", "Candidate B"),
  title            = "2D Hunter Competition (saved)",
  trail            = "fade",
  width            = 900L,
  height           = 800L,
  compute_ic       = TRUE,
  compute_winset   = TRUE,
  compute_voronoi  = TRUE
)

# Preview in RStudio Viewer
w_2d

# Save payload to .scscanvas
save_competition_canvas(w_2d, PATH_2D)
cat("Saved 2D canvas to:", PATH_2D, "\n")

cat("\nDone. Run canvas_load_demo.R to reload without recomputing.\n")
