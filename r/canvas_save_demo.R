# canvas_save_demo.R — Generate competition canvases and save to .scsview files.
#
# Uses the same models as:
#   competition_1d_demo.R
#   competition_2D_demo_2Cand.R
#   competition_2D_demo_3Cand.R
#
# Saves all three payloads to tmp/ without displaying anything.
# Run canvas_load_demo.R to reload without recomputing.
#
# HOW TO RUN IN RSTUDIO:
#   Session > Set Working Directory > To Source File Location
#   (picks up r/.Renviron which sets SCS_LIB_PATH / DYLD_LIBRARY_PATH)
#   Then source or step through with Cmd+Enter.
devtools::load_all("r/")

library(socialchoicelab)

# Output paths — written to tmp/ in the project root (visible in Files pane).
# tmp/ is tracked in git but its contents are listed in .gitignore.
# Shared with canvas_load_demo.R and the Python pair.
dir.create("tmp", showWarnings = FALSE)
PATH_1D     <- "tmp/canvas_1d_demo.scsview"
PATH_2D     <- "tmp/canvas_2d_demo.scsview"
PATH_3CAND  <- "tmp/canvas_3cand_2d_demo.scsview"

# ===========================================================================
# 1D competition — same model as competition_1d_demo.R
# 101 voters, up to 200 rounds, 2 hunter-sticker candidates
# ===========================================================================
set.seed(123L)
voters_1d <- runif(101L, min = -1, max = 1)
cat("1D voters:", length(voters_1d), "ideal points\n")
cat("range:", range(voters_1d), "\n")

sm_1d <- stream_manager(42L, c("competition_adaptation_hunter"))

trace_1d <- competition_run(
  competitor_positions = c(-.8, .7),
  competitor_headings  = c(1.0, -1.0),
  strategy_kinds       = c("hunter_sticker", "hunter_sticker"),
  voter_ideals         = voters_1d,
  dist_config          = make_dist_config(n_dims = 1L),
  engine_config        = make_competition_engine_config(
    seat_count  = 1L,
    seat_rule   = "plurality_top_k",
    max_rounds  = 200L,
    step_config = make_competition_step_config(kind = "fixed", fixed_step_size = 0.06)
  ),
  stream_manager = sm_1d
)
cat("1D rounds:", trace_1d$dims()$n_rounds, "\n")
cat("1D final positions:\n")
print(trace_1d$final_positions())

w_1d <- animate_competition_canvas(
  trace_1d,
  voters           = voters_1d,
  competitor_names = c("Candidate A", "Candidate B"),
  title            = "1D Hunter-Sticker Competition",
  trail            = "fade",
  dim_names        = c("Policy Dimension"),
  overlays         = list(centroid = TRUE, marginal_median = TRUE),
  width            = 400L,
  height           = 400L,
  compute_ic       = TRUE,
  compute_winset   = TRUE
)

save_competition_canvas(w_1d, PATH_1D)
cat("Saved 1D canvas to:", PATH_1D, "\n")
# 
# # ===========================================================================
# # 2D competition — same model as competition_2D_demo_2Cand.R
# # 10 voters, up to 300 rounds, hunter vs aggregator
# # ===========================================================================
# sm_voters_2c <- stream_manager(123L, c("voters"))
# voter_cfg_2c <- make_voter_sampler("uniform", lo = -.75, hi = .75)
# voters_2d    <- draw_voters(30L, voter_cfg_2c, sm_voters_2c, "voters")
# cat("2D (2-cand) voters: 10 points in 2D\n")
# 
# overlays_2d <- list(
#   centroid        = centroid_2d(voters_2d),
#   marginal_median = marginal_median_2d(voters_2d),
#   pareto_set      = convex_hull_2d(voters_2d)
# )
# 
# sm_2c    <- stream_manager(42L, c("competition_adaptation_hunter", "voters", "competition_motion_step_sizes"))
# headings_2c <- c(0.71, 0.71, -0.71, -0.71)
# 
# trace_2d <- competition_run(
#   competitor_positions = c(-1, -1,   1, 1),
#   competitor_headings  = headings_2c,
#   strategy_kinds       = c("hunter", "aggregator"),
#   voter_ideals         = voters_2d,
#   dist_config          = make_dist_config(n_dims = 2L),
#   engine_config        = make_competition_engine_config(
#     seat_count  = 1L,
#     seat_rule   = "plurality_top_k",
#     max_rounds  = 200L,
#     step_config = make_competition_step_config(kind = "random_uniform", min_step_size = 0.0, max_step_size = 0.12)
#   ),
#   stream_manager = sm_2c
# )
# cat("2D (2-cand) rounds:", trace_2d$dims()$n_rounds, "\n")
# cat("2D (2-cand) final positions:\n")
# print(trace_2d$final_positions())
# 
# w_2d <- animate_competition_canvas(
#   trace_2d,
#   voters           = voters_2d,
#   competitor_names = c("Candidate A", "Candidate B"),
#   title            = "Two Candidate Competition",
#   trail            = "none",
#   overlays         = overlays_2d,
#   width            = 950L,
#   height           = 850L,
#   compute_ic       = TRUE,
#   ic_max_curves    = 15000L,
#   compute_winset   = TRUE,
#   compute_voronoi  = TRUE
# )
# 
# save_competition_canvas(w_2d, PATH_2D)
# cat("Saved 2D (2-cand) canvas to:", PATH_2D, "\n")

# ===========================================================================
# 2D competition — same model as competition_2D_demo_3Cand.R
# 200 voters, up to 200 rounds, aggregator vs hunter vs aggregator
# ===========================================================================
sm_voters_3c <- stream_manager(123L, c("voters"))
voter_cfg_3c <- make_voter_sampler("uniform", lo = -.75, hi = .75)
voters_3c    <- draw_voters(200L, voter_cfg_3c, sm_voters_3c, "voters")
cat("2D (3-cand) voters: 200 points in 2D\n")

overlays_3c <- list(
  centroid        = centroid_2d(voters_3c),
  marginal_median = marginal_median_2d(voters_3c),
  pareto_set      = convex_hull_2d(voters_3c)
)

sm_3c       <- stream_manager(42L, c("competition_adaptation_hunter", "voters", "competition_motion_step_sizes"))
headings_3c <- c(0.71, 0.71, -0.71, -0.71, 0.0, -1.0)

trace_3c <- competition_run(
  competitor_positions = c(-1, -1,   1, 1,   -1, 0.5),
  competitor_headings  = headings_3c,
  strategy_kinds       = c("aggregator", "hunter", "aggregator"),
  voter_ideals         = voters_3c,
  dist_config          = make_dist_config(n_dims = 2L),
  engine_config        = make_competition_engine_config(
    seat_count  = 1L,
    seat_rule   = "plurality_top_k",
    max_rounds  = 200L,
    step_config = make_competition_step_config(kind = "random_uniform", min_step_size = 0.0, max_step_size = 0.12)
  ),
  stream_manager = sm_3c
)
cat("2D (3-cand) rounds:", trace_3c$dims()$n_rounds, "\n")
cat("2D (3-cand) final positions:\n")
print(trace_3c$final_positions())

w_3c <- animate_competition_canvas(
  trace_3c,
  voters           = voters_3c,
  competitor_names = c("Candidate A", "Candidate B", "Candidate C"),
  title            = "Three Candidate Competition",
  trail            = "none",
  overlays         = overlays_3c,
  width            = 950L,
  height           = 850L,
  compute_ic       = FALSE,
  ic_max_curves    = 15000L,
  compute_winset   = FALSE,
  compute_voronoi  = TRUE
)

save_competition_canvas(w_3c, PATH_3CAND)
cat("Saved 2D (3-cand) canvas to:", PATH_3CAND, "\n")

cat("\nDone. Run canvas_load_demo.R to reload without recomputing.\n")
