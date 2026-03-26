# test_plots.R — C10/C13: Unit tests for spatial voting visualization helpers

VOTERS <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6, -0.4, 0.8, 0.5, -0.7)
ALTS   <- c( 0.0,  0.0, 0.6, 0.4, -0.5, 0.3)
SQ     <- c(0.0, 0.0)

.competition_trace_2d <- function() {
  competition_run(
    competitor_positions = c(0.0, 0.0, 2.0, 0.0),
    strategy_kinds = c("sticker", "sticker"),
    voter_ideals = c(-0.5, 0.0, 0.0, 0.2, 1.8, 0.1),
    dist_config = make_dist_config(n_dims = 2L),
    engine_config = make_competition_engine_config(
      seat_count = 1L,
      seat_rule = "plurality_top_k",
      max_rounds = 2L,
      step_config = make_competition_step_config(
        kind = "fixed",
        fixed_step_size = 0.5
      )
    )
  )
}

# ---------------------------------------------------------------------------
# plot_spatial_voting
# ---------------------------------------------------------------------------

test_that("plot_spatial_voting returns spatial_voting_canvas htmlwidget", {
  fig <- plot_spatial_voting(VOTERS, ALTS)
  expect_s3_class(fig, "spatial_voting_canvas")
  expect_s3_class(fig, "htmlwidget")
  expect_true("voters_x" %in% names(fig$x))
  expect_equal(length(fig$x$voters_x), 5L)
  expect_true(is.list(fig$x$layers))
})

test_that("plot_spatial_voting with sq sets sq in payload", {
  fig <- plot_spatial_voting(VOTERS, ALTS, sq = SQ)
  expect_equal(fig$x$sq, as.numeric(SQ))
})

test_that("plot_spatial_voting accepts custom labels", {
  fig <- plot_spatial_voting(
    VOTERS, ALTS,
    voter_names = c("Alice", "Bob", "Carol", "Dan", "Eve"),
    alt_names   = c("Origin", "Alt A", "Alt B"),
    dim_names   = c("Economic", "Social"),
    title       = "Test Legislature"
  )
  expect_equal(fig$x$voter_names[1L], "Alice")
  expect_equal(fig$x$alternative_names, c("Origin", "Alt A", "Alt B"))
})

test_that("plot_spatial_voting accepts custom dimensions", {
  fig <- plot_spatial_voting(VOTERS[1:4], ALTS[1:2], width = 800L, height = 500L)
  expect_equal(fig$width, 800L)
  expect_equal(fig$height, 500L)
})

test_that("plot_spatial_voting accepts explicit xlim and ylim", {
  fig <- plot_spatial_voting(VOTERS, ALTS, sq = SQ,
                             xlim = c(-2, 2), ylim = c(-2, 2))
  expect_equal(fig$x$xlim, c(-2, 2))
  expect_equal(fig$x$ylim, c(-2, 2))
})

test_that("plot_spatial_voting defaults include origin in ranges", {
  fig <- plot_spatial_voting(VOTERS, ALTS, sq = SQ)
  expect_lte(fig$x$xlim[1L], 0)
  expect_gte(fig$x$xlim[2L], 0)
  expect_lte(fig$x$ylim[1L], 0)
  expect_gte(fig$x$ylim[2L], 0)
})

test_that("plot_spatial_voting show_labels flag is stored", {
  fig <- plot_spatial_voting(VOTERS, ALTS, sq = SQ, show_labels = TRUE)
  expect_true(isTRUE(fig$x$show_labels))
})

test_that("plot_spatial_voting layer_toggles is stored", {
  fig_on  <- plot_spatial_voting(VOTERS, ALTS, sq = SQ)
  fig_off <- plot_spatial_voting(VOTERS, ALTS, sq = SQ, layer_toggles = FALSE)
  expect_true(isTRUE(fig_on$x$layer_toggles))
  expect_identical(fig_off$x$layer_toggles, FALSE)
})

test_that("plot_spatial_voting with no alternatives works", {
  fig <- plot_spatial_voting(VOTERS, sq = SQ)
  expect_equal(length(fig$x$alternatives), 0L)
})

# ---------------------------------------------------------------------------
# animate_competition_canvas
# ---------------------------------------------------------------------------

test_that("animate_competition_canvas returns an htmlwidget with expected payload", {
  skip_without_lib()
  trace <- .competition_trace_2d()
  w <- animate_competition_canvas(trace, voters = VOTERS)
  expect_s3_class(w, "competition_canvas")
  expect_s3_class(w, "htmlwidget")
  expect_true("x" %in% names(w))
  expect_equal(names(w$x), c(
    "voters_x", "voters_y", "voter_color", "xlim", "ylim", "dim_names",
    "rounds", "positions", "competitor_names", "competitor_colors",
    "vote_shares", "trail", "trail_length", "title", "seat_holder_indices"
  ))
  d <- trace$dims()
  expect_length(w$x$rounds, d$n_rounds + 1L)
  expect_length(w$x$positions, d$n_rounds + 1L)
  expect_length(w$x$competitor_names, d$n_competitors)
  expect_identical(w$x$trail, "fade")
})

test_that("animate_competition_canvas accepts trail none and full", {
  skip_without_lib()
  trace <- .competition_trace_2d()
  w_none <- animate_competition_canvas(trace, voters = VOTERS, trail = "none")
  w_full <- animate_competition_canvas(trace, voters = VOTERS, trail = "full")
  expect_identical(w_none$x$trail, "none")
  expect_identical(w_full$x$trail, "full")
})

test_that("animate_competition_canvas rejects non-CompetitionTrace", {
  expect_error(
    animate_competition_canvas(list(a = 1)),
    "Expected a CompetitionTrace"
  )
})

test_that("strategy_kinds returns correct strategies from trace", {
  skip_without_lib()
  trace <- .competition_trace_2d()
  kinds <- trace$strategy_kinds()
  expect_equal(kinds, c("sticker", "sticker"))
})

test_that("animate_competition_canvas auto-generates names from strategies", {
  skip_without_lib()
  trace <- .competition_trace_2d()
  w <- animate_competition_canvas(trace, voters = VOTERS)
  expect_equal(w$x$competitor_names, list("Sticker A", "Sticker B"))
})

test_that("animate_competition_canvas annotates user-supplied names with strategy", {
  skip_without_lib()
  trace <- .competition_trace_2d()
  w <- animate_competition_canvas(
    trace, voters = VOTERS,
    competitor_names = c("Alice", "Bob")
  )
  expect_equal(w$x$competitor_names, list("Alice (Sticker)", "Bob (Sticker)"))
})

test_that("layer stack keys exist after convex hull and winset", {
  skip_without_lib()
  hull <- convex_hull_2d(VOTERS)
  ws <- winset_2d(SQ, VOTERS)
  fig <- plot_spatial_voting(VOTERS, sq = SQ)
  fig <- layer_convex_hull(fig, hull)
  fig <- layer_winset(fig, ws)
  expect_true("convex_hull_xy" %in% names(fig$x$layers))
  expect_true("winset_paths" %in% names(fig$x$layers))
})

# ---------------------------------------------------------------------------
# layer_yolk
# ---------------------------------------------------------------------------

test_that("layer_yolk updates payload", {
  fig  <- plot_spatial_voting(VOTERS[1:6], c(0.0, 0.0))
  fig2 <- layer_yolk(fig, center_x = 0.1, center_y = 0.05, radius = 0.3)
  expect_equal(fig2$x$layers$yolk$r, 0.3, tolerance = 1e-8)
})

test_that("layer_yolk with custom fill and line colors works", {
  fig  <- plot_spatial_voting(c(-1, -1, 1, 1), c(0, 0))
  fig2 <- layer_yolk(fig, 0, 0, 0.5,
                     fill_color = "rgba(0,0,255,0.2)",
                     line_color = "rgba(0,0,255,0.8)",
                     name = "Approx. Yolk")
  expect_equal(fig2$x$layers$yolk$fill, "rgba(0,0,255,0.2)")
})

# ---------------------------------------------------------------------------
# layer_convex_hull
# ---------------------------------------------------------------------------

test_that("layer_convex_hull adds convex_hull_xy", {
  hull <- matrix(c(0, 0, 1, 0, 1, 1, 0, 1), ncol = 2L,
                 dimnames = list(NULL, c("x", "y")))
  fig  <- plot_spatial_voting(c(-1, -1, 1, -1, 1, 1, -1, 1), c(0.0, 0.0))
  fig2 <- layer_convex_hull(fig, hull)
  expect_true("xy" %in% names(fig2$x$layers$convex_hull_xy))
})

test_that("layer_convex_hull handles empty hull unchanged", {
  hull <- matrix(numeric(0L), ncol = 2L, dimnames = list(NULL, c("x", "y")))
  fig  <- plot_spatial_voting(c(-1, -1, 1, 1), c(0, 0))
  fig2 <- layer_convex_hull(fig, hull)
  expect_false("convex_hull_xy" %in% names(fig2$x$layers))
})

test_that("layer_convex_hull with computed hull", {
  skip_without_lib()
  hull <- convex_hull_2d(VOTERS)
  fig  <- plot_spatial_voting(VOTERS, c(0.0, 0.0))
  fig2 <- layer_convex_hull(fig, hull)
  expect_true("convex_hull_xy" %in% names(fig2$x$layers))
})

# ---------------------------------------------------------------------------
# layer_uncovered_set
# ---------------------------------------------------------------------------

test_that("layer_uncovered_set adds uncovered_xy", {
  bnd <- matrix(c(-1, 0, 0, 1, 1, 0, 0, -1), ncol = 2L,
                dimnames = list(NULL, c("x", "y")))
  fig  <- plot_spatial_voting(c(-1, -1, 1, -1, 1, 1), c(0.0, 0.0))
  fig2 <- layer_uncovered_set(fig, bnd)
  expect_true("uncovered_xy" %in% names(fig2$x$layers))
})

test_that("layer_uncovered_set handles empty boundary unchanged", {
  bnd  <- matrix(numeric(0L), ncol = 2L, dimnames = list(NULL, c("x", "y")))
  fig  <- plot_spatial_voting(c(-1, -1, 1, 1), c(0, 0))
  fig2 <- layer_uncovered_set(fig, bnd)
  expect_false("uncovered_xy" %in% names(fig2$x$layers))
})

test_that("layer_uncovered_set with computed boundary", {
  skip_without_lib()
  bnd  <- uncovered_set_boundary_2d(VOTERS, grid_resolution = 8L)
  fig  <- plot_spatial_voting(VOTERS, c(0.0, 0.0))
  fig2 <- layer_uncovered_set(fig, bnd)
  expect_true("uncovered_xy" %in% names(fig2$x$layers))
})

test_that("layer_uncovered_set auto-computes from voters", {
  skip_without_lib()
  fig  <- plot_spatial_voting(VOTERS, c(0.0, 0.0))
  fig2 <- layer_uncovered_set(fig, voters = VOTERS, grid_resolution = 8L)
  expect_true("uncovered_xy" %in% names(fig2$x$layers))
})

test_that("layer_uncovered_set errors without boundary_xy or voters", {
  fig <- plot_spatial_voting(VOTERS, c(0.0, 0.0))
  expect_error(layer_uncovered_set(fig), "boundary_xy")
})

# ---------------------------------------------------------------------------
# layer_winset
# ---------------------------------------------------------------------------

test_that("layer_winset with non-empty winset adds winset_paths", {
  skip_without_lib()
  ws   <- winset_2d(SQ, VOTERS)
  fig  <- plot_spatial_voting(VOTERS, c(0.0, 0.0), sq = SQ)
  fig2 <- layer_winset(fig, ws)
  expect_true("winset_paths" %in% names(fig2$x$layers))
})

test_that("layer_winset with empty winset returns figure unchanged", {
  skip_without_lib()
  voters <- c(0.0, 0.0, 0.0, 0.0, 0.0, 0.0)
  ws   <- winset_2d(c(0.0, 0.0), voters)
  fig  <- plot_spatial_voting(voters, c(0.0, 0.0))
  fig2 <- layer_winset(fig, ws)
  expect_false("winset_paths" %in% names(fig2$x$layers))
})

test_that("layer_winset auto-computes from voters and sq", {
  skip_without_lib()
  fig  <- plot_spatial_voting(VOTERS, sq = SQ)
  fig2 <- layer_winset(fig, voters = VOTERS, sq = SQ)
  expect_true("winset_paths" %in% names(fig2$x$layers))
})

test_that("layer_winset errors without winset or voters+sq", {
  fig <- plot_spatial_voting(VOTERS, c(0.0, 0.0))
  expect_error(layer_winset(fig), "voters")
})

test_that("layer_winset auto-computes with Manhattan dist_config", {
  skip_without_lib()
  fig  <- plot_spatial_voting(VOTERS, sq = SQ)
  dc   <- make_dist_config("manhattan")
  fig2 <- layer_winset(fig, voters = VOTERS, sq = SQ, dist_config = dc)
  expect_true("winset_paths" %in% names(fig2$x$layers))
})

test_that("layer_winset auto-computes with Chebyshev dist_config", {
  skip_without_lib()
  fig  <- plot_spatial_voting(VOTERS, sq = SQ)
  dc   <- make_dist_config("chebyshev")
  fig2 <- layer_winset(fig, voters = VOTERS, sq = SQ, dist_config = dc)
  # Winset may be empty for Chebyshev on this fixture; must not error.
  expect_s3_class(fig2, "spatial_voting_canvas")
})

test_that("layer_winset auto-computes with Minkowski p=3 dist_config", {
  skip_without_lib()
  fig  <- plot_spatial_voting(VOTERS, sq = SQ)
  dc   <- make_dist_config("minkowski", order_p = 3.0)
  fig2 <- layer_winset(fig, voters = VOTERS, sq = SQ, dist_config = dc)
  expect_true("winset_paths" %in% names(fig2$x$layers))
})

# ---------------------------------------------------------------------------
# layer_ic
# ---------------------------------------------------------------------------

test_that("layer_ic adds ic_curves", {
  fig  <- plot_spatial_voting(VOTERS, sq = SQ)
  fig2 <- layer_ic(fig, VOTERS, SQ)
  expect_length(fig2$x$layers$ic_curves, 5L)
})

test_that("layer_ic adds one curve per voter", {
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
  fig  <- plot_spatial_voting(voters, sq = SQ)
  fig2 <- layer_ic(fig, voters, SQ)
  expect_length(fig2$x$layers$ic_curves, 3L)
})

test_that("layer_ic with color_by_voter = TRUE works", {
  fig  <- plot_spatial_voting(VOTERS, sq = SQ)
  fig2 <- layer_ic(fig, VOTERS, SQ, color_by_voter = TRUE)
  expect_length(fig2$x$layers$ic_curves, 5L)
})

test_that("layer_ic with fill_color adds fill on curves", {
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
  fig  <- plot_spatial_voting(voters, sq = SQ)
  fig2 <- layer_ic(fig, voters, SQ, fill_color = "rgba(150,150,200,0.08)")
  expect_true(all(vapply(fig2$x$layers$ic_curves, function(z) !is.null(z$fill), logical(1L))))
})

test_that("layer_ic with custom voter_names works", {
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
  fig  <- plot_spatial_voting(voters, sq = SQ)
  fig2 <- layer_ic(fig, voters, SQ, voter_names = c("Alice", "Bob", "Carol"))
  expect_length(fig2$x$layers$ic_curves, 3L)
})

test_that("layer_ic non-Euclidean Manhattan adds one curve per voter", {
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
  fig  <- plot_spatial_voting(voters, sq = SQ)
  dc   <- make_dist_config("manhattan")
  fig2 <- layer_ic(fig, voters, SQ, dist_config = dc)
  expect_length(fig2$x$layers$ic_curves, 3L)
})

test_that("layer_ic non-Euclidean Chebyshev works", {
  voters <- c(-1.0, -0.5, 0.0, 0.0)
  fig  <- plot_spatial_voting(voters, sq = SQ)
  dc   <- make_dist_config("chebyshev")
  fig2 <- layer_ic(fig, voters, SQ, dist_config = dc)
  expect_length(fig2$x$layers$ic_curves, 2L)
})

# ---------------------------------------------------------------------------
# layer_preferred_regions
# ---------------------------------------------------------------------------

test_that("layer_preferred_regions adds preferred_regions", {
  fig  <- plot_spatial_voting(VOTERS, sq = SQ)
  fig2 <- layer_preferred_regions(fig, VOTERS, SQ)
  expect_length(fig2$x$layers$preferred_regions, 5L)
})

test_that("layer_preferred_regions adds one region per voter", {
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
  fig  <- plot_spatial_voting(voters, sq = SQ)
  fig2 <- layer_preferred_regions(fig, voters, SQ)
  expect_length(fig2$x$layers$preferred_regions, 3L)
})

test_that("layer_preferred_regions with color_by_voter = TRUE works", {
  fig  <- plot_spatial_voting(VOTERS, sq = SQ)
  fig2 <- layer_preferred_regions(fig, VOTERS, SQ, color_by_voter = TRUE)
  expect_length(fig2$x$layers$preferred_regions, 5L)
})

test_that("layer_preferred_regions non-Euclidean Manhattan", {
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
  fig  <- plot_spatial_voting(voters, sq = SQ)
  dc   <- make_dist_config("manhattan")
  fig2 <- layer_preferred_regions(fig, voters, SQ, dist_config = dc)
  expect_length(fig2$x$layers$preferred_regions, 3L)
})

test_that("layer_preferred_regions non-Euclidean Chebyshev works", {
  voters <- c(-1.0, -0.5, 0.0, 0.0)
  fig  <- plot_spatial_voting(voters, sq = SQ)
  dc   <- make_dist_config("chebyshev")
  fig2 <- layer_preferred_regions(fig, voters, SQ, dist_config = dc)
  expect_length(fig2$x$layers$preferred_regions, 2L)
})

# ---------------------------------------------------------------------------
# save_plot
# ---------------------------------------------------------------------------

test_that("save_plot saves an HTML file", {
  skip_without_lib()
  fig  <- plot_spatial_voting(VOTERS, sq = SQ)
  tmp  <- tempfile(fileext = ".html")
  on.exit(unlink(tmp))
  result <- save_plot(fig, tmp)
  expect_equal(result, tmp)
  expect_true(file.exists(tmp))
  expect_gt(file.size(tmp), 0L)
  txt <- paste(readLines(tmp, warn = FALSE), collapse = "\n")
  expect_true(grepl("spatial_voting_canvas", txt))
})

test_that("save_plot errors on PNG export", {
  fig <- plot_spatial_voting(VOTERS, sq = SQ)
  expect_error(save_plot(fig, tempfile(fileext = ".png")), "PNG/SVG")
})

test_that("save_plot errors on unsupported extension", {
  fig <- plot_spatial_voting(VOTERS, sq = SQ)
  expect_error(save_plot(fig, "output.xyz"), "unsupported file extension")
})

# ---------------------------------------------------------------------------
# Full composition
# ---------------------------------------------------------------------------

test_that("all layers can be composed without error", {
  skip_without_lib()
  ws   <- winset_2d(SQ, VOTERS)
  hull <- convex_hull_2d(VOTERS)
  bnd  <- uncovered_set_boundary_2d(VOTERS, grid_resolution = 8L)

  fig <- plot_spatial_voting(VOTERS, ALTS, sq = SQ, title = "Composition test")
  fig <- layer_ic(fig, VOTERS, SQ)
  fig <- layer_preferred_regions(fig, VOTERS, SQ)
  fig <- layer_convex_hull(fig, hull)
  fig <- layer_uncovered_set(fig, bnd)
  fig <- layer_yolk(fig, center_x = 0.1, center_y = 0.05, radius = 0.25)
  fig <- layer_winset(fig, ws)
  expect_true(length(fig$x$layers$ic_curves) > 0L)
  expect_true(length(fig$x$layers$winset_paths) > 0L)
})

# ---------------------------------------------------------------------------
# layer_centroid
# ---------------------------------------------------------------------------

test_that("layer_centroid sets centroid in layers", {
  skip_without_lib()
  fig  <- plot_spatial_voting(VOTERS, sq = SQ)
  fig2 <- layer_centroid(fig, VOTERS)
  expect_length(fig2$x$layers$centroid, 2L)
})

test_that("layer_centroid position matches mean", {
  skip_without_lib()
  voters <- c(-1.0, 0.0, 1.0, 0.0, 0.0, 2.0)
  fig <- plot_spatial_voting(voters)
  fig <- layer_centroid(fig, voters)
  expect_equal(fig$x$layers$centroid[1L], 0.0, tolerance = 1e-10)
  expect_equal(fig$x$layers$centroid[2L], 2.0 / 3.0, tolerance = 1e-10)
})

# ---------------------------------------------------------------------------
# layer_marginal_median
# ---------------------------------------------------------------------------

test_that("layer_marginal_median sets marginal_median in layers", {
  skip_without_lib()
  fig  <- plot_spatial_voting(VOTERS, sq = SQ)
  fig2 <- layer_marginal_median(fig, VOTERS)
  expect_length(fig2$x$layers$marginal_median, 2L)
})

test_that("layer_marginal_median position matches coordinate-wise median", {
  skip_without_lib()
  voters <- c(-1.0, 0.0, 0.0, 0.0, 1.0, 0.0)
  fig <- plot_spatial_voting(voters)
  fig <- layer_marginal_median(fig, voters)
  expect_equal(fig$x$layers$marginal_median[1L], 0.0, tolerance = 1e-10)
  expect_equal(fig$x$layers$marginal_median[2L], 0.0, tolerance = 1e-10)
})
