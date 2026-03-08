# test_plots.R — C10/C13: Unit tests for spatial voting visualization helpers

VOTERS <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6, -0.4, 0.8, 0.5, -0.7)
ALTS   <- c( 0.0,  0.0, 0.6, 0.4, -0.5, 0.3)
SQ     <- c(0.0, 0.0)

# ---------------------------------------------------------------------------
# plot_spatial_voting
# ---------------------------------------------------------------------------

test_that("plot_spatial_voting returns a plotly object", {
  fig <- plot_spatial_voting(VOTERS, ALTS)
  expect_s3_class(fig, "plotly")
})

test_that("plot_spatial_voting with sq returns a plotly object", {
  fig <- plot_spatial_voting(VOTERS, ALTS, sq = SQ)
  expect_s3_class(fig, "plotly")
})

test_that("plot_spatial_voting accepts custom labels", {
  fig <- plot_spatial_voting(
    VOTERS, ALTS,
    voter_names = c("Alice", "Bob", "Carol", "Dan", "Eve"),
    alt_names   = c("Origin", "Alt A", "Alt B"),
    dim_names   = c("Economic", "Social"),
    title       = "Test Legislature"
  )
  expect_s3_class(fig, "plotly")
})

test_that("plot_spatial_voting accepts custom dimensions", {
  fig <- plot_spatial_voting(VOTERS[1:4], ALTS[1:2], width = 800L, height = 500L)
  expect_s3_class(fig, "plotly")
})

test_that("plot_spatial_voting accepts explicit xlim and ylim", {
  fig <- plot_spatial_voting(VOTERS, ALTS, sq = SQ,
                             xlim = c(-2, 2), ylim = c(-2, 2))
  expect_s3_class(fig, "plotly")
})

test_that("plot_spatial_voting show_labels flag works", {
  fig <- plot_spatial_voting(VOTERS, ALTS, sq = SQ, show_labels = TRUE)
  expect_s3_class(fig, "plotly")
})

test_that("plot_spatial_voting with no alternatives works", {
  fig <- plot_spatial_voting(VOTERS, sq = SQ)
  expect_s3_class(fig, "plotly")
})

# ---------------------------------------------------------------------------
# layer_yolk
# ---------------------------------------------------------------------------

test_that("layer_yolk returns a plotly object", {
  fig  <- plot_spatial_voting(VOTERS[1:6], c(0.0, 0.0))
  fig2 <- layer_yolk(fig, center_x = 0.1, center_y = 0.05, radius = 0.3)
  expect_s3_class(fig2, "plotly")
})

test_that("layer_yolk with custom fill and line colors works", {
  fig  <- plot_spatial_voting(c(-1, -1, 1, 1), c(0, 0))
  fig2 <- layer_yolk(fig, 0, 0, 0.5,
                     fill_color = "rgba(0,0,255,0.2)",
                     line_color = "rgba(0,0,255,0.8)",
                     name = "Approx. Yolk")
  expect_s3_class(fig2, "plotly")
})

# ---------------------------------------------------------------------------
# layer_convex_hull
# ---------------------------------------------------------------------------

test_that("layer_convex_hull returns a plotly object", {
  hull <- matrix(c(0, 0, 1, 0, 1, 1, 0, 1), ncol = 2L,
                 dimnames = list(NULL, c("x", "y")))
  fig  <- plot_spatial_voting(c(-1, -1, 1, -1, 1, 1, -1, 1), c(0.0, 0.0))
  fig2 <- layer_convex_hull(fig, hull)
  expect_s3_class(fig2, "plotly")
})

test_that("layer_convex_hull handles empty hull unchanged", {
  hull <- matrix(numeric(0L), ncol = 2L, dimnames = list(NULL, c("x", "y")))
  fig  <- plot_spatial_voting(c(-1, -1, 1, 1), c(0, 0))
  fig2 <- layer_convex_hull(fig, hull)
  expect_s3_class(fig2, "plotly")
  expect_identical(length(fig$x$data), length(fig2$x$data))
})

test_that("layer_convex_hull with computed hull", {
  skip_without_lib()
  hull <- convex_hull_2d(VOTERS)
  fig  <- plot_spatial_voting(VOTERS, c(0.0, 0.0))
  fig2 <- layer_convex_hull(fig, hull)
  expect_s3_class(fig2, "plotly")
})

# ---------------------------------------------------------------------------
# layer_uncovered_set
# ---------------------------------------------------------------------------

test_that("layer_uncovered_set returns a plotly object", {
  bnd <- matrix(c(-1, 0, 0, 1, 1, 0, 0, -1), ncol = 2L,
                dimnames = list(NULL, c("x", "y")))
  fig  <- plot_spatial_voting(c(-1, -1, 1, -1, 1, 1), c(0.0, 0.0))
  fig2 <- layer_uncovered_set(fig, bnd)
  expect_s3_class(fig2, "plotly")
})

test_that("layer_uncovered_set handles empty boundary unchanged", {
  bnd  <- matrix(numeric(0L), ncol = 2L, dimnames = list(NULL, c("x", "y")))
  fig  <- plot_spatial_voting(c(-1, -1, 1, 1), c(0, 0))
  fig2 <- layer_uncovered_set(fig, bnd)
  expect_s3_class(fig2, "plotly")
  expect_identical(length(fig$x$data), length(fig2$x$data))
})

test_that("layer_uncovered_set with computed boundary", {
  skip_without_lib()
  bnd  <- uncovered_set_boundary_2d(VOTERS, grid_resolution = 8L)
  fig  <- plot_spatial_voting(VOTERS, c(0.0, 0.0))
  fig2 <- layer_uncovered_set(fig, bnd)
  expect_s3_class(fig2, "plotly")
})

test_that("layer_uncovered_set auto-computes from voters", {
  skip_without_lib()
  fig  <- plot_spatial_voting(VOTERS, c(0.0, 0.0))
  fig2 <- layer_uncovered_set(fig, voters = VOTERS, grid_resolution = 8L)
  expect_s3_class(fig2, "plotly")
})

test_that("layer_uncovered_set errors without boundary_xy or voters", {
  fig <- plot_spatial_voting(VOTERS, c(0.0, 0.0))
  expect_error(layer_uncovered_set(fig), "boundary_xy")
})

# ---------------------------------------------------------------------------
# layer_winset
# ---------------------------------------------------------------------------

test_that("layer_winset with non-empty winset returns a plotly object", {
  skip_without_lib()
  ws   <- winset_2d(SQ, VOTERS)
  fig  <- plot_spatial_voting(VOTERS, c(0.0, 0.0), sq = SQ)
  fig2 <- layer_winset(fig, ws)
  expect_s3_class(fig2, "plotly")
})

test_that("layer_winset with empty winset returns figure unchanged", {
  skip_without_lib()
  voters <- c(0.0, 0.0, 0.0, 0.0, 0.0, 0.0)
  ws   <- winset_2d(c(0.0, 0.0), voters)
  fig  <- plot_spatial_voting(voters, c(0.0, 0.0))
  fig2 <- layer_winset(fig, ws)
  expect_s3_class(fig2, "plotly")
})

test_that("layer_winset auto-computes from voters and sq", {
  skip_without_lib()
  fig  <- plot_spatial_voting(VOTERS, sq = SQ)
  fig2 <- layer_winset(fig, voters = VOTERS, sq = SQ)
  expect_s3_class(fig2, "plotly")
})

test_that("layer_winset errors without winset or voters+sq", {
  fig <- plot_spatial_voting(VOTERS, c(0.0, 0.0))
  expect_error(layer_winset(fig), "voters")
})

# ---------------------------------------------------------------------------
# layer_ic
# ---------------------------------------------------------------------------

test_that("layer_ic returns a plotly object", {
  fig  <- plot_spatial_voting(VOTERS, sq = SQ)
  fig2 <- layer_ic(fig, VOTERS, SQ)
  expect_s3_class(fig2, "plotly")
})

test_that("layer_ic adds one trace per voter", {
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
  fig  <- plot_spatial_voting(voters, sq = SQ)
  fig2 <- layer_ic(fig, voters, SQ)
  expect_s3_class(fig2, "plotly")
  # 3 voters → 3 circle traces added (one per voter)
  expect_equal(length(fig2$x$attrs), length(fig$x$attrs) + 3L)
})

test_that("layer_ic with color_by_voter = TRUE works", {
  fig  <- plot_spatial_voting(VOTERS, sq = SQ)
  fig2 <- layer_ic(fig, VOTERS, SQ, color_by_voter = TRUE)
  expect_s3_class(fig2, "plotly")
})

test_that("layer_ic with fill_color adds filled circles", {
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
  fig  <- plot_spatial_voting(voters, sq = SQ)
  fig2 <- layer_ic(fig, voters, SQ, fill_color = "rgba(150,150,200,0.08)")
  expect_s3_class(fig2, "plotly")
})

test_that("layer_ic with custom voter_names works", {
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
  fig  <- plot_spatial_voting(voters, sq = SQ)
  fig2 <- layer_ic(fig, voters, SQ, voter_names = c("Alice", "Bob", "Carol"))
  expect_s3_class(fig2, "plotly")
})

# ---------------------------------------------------------------------------
# layer_preferred_regions
# ---------------------------------------------------------------------------

test_that("layer_preferred_regions returns a plotly object", {
  fig  <- plot_spatial_voting(VOTERS, sq = SQ)
  fig2 <- layer_preferred_regions(fig, VOTERS, SQ)
  expect_s3_class(fig2, "plotly")
})

test_that("layer_preferred_regions adds one trace per voter", {
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
  fig  <- plot_spatial_voting(voters, sq = SQ)
  fig2 <- layer_preferred_regions(fig, voters, SQ)
  expect_s3_class(fig2, "plotly")
  expect_equal(length(fig2$x$attrs), length(fig$x$attrs) + 3L)
})

test_that("layer_preferred_regions with color_by_voter = TRUE works", {
  fig  <- plot_spatial_voting(VOTERS, sq = SQ)
  fig2 <- layer_preferred_regions(fig, VOTERS, SQ, color_by_voter = TRUE)
  expect_s3_class(fig2, "plotly")
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
  fig <- finalize_plot(fig)
  expect_s3_class(fig, "plotly")
})

# ---------------------------------------------------------------------------
# layer_centroid
# ---------------------------------------------------------------------------

test_that("layer_centroid returns a plotly object", {
  skip_without_lib()
  fig  <- plot_spatial_voting(VOTERS, sq = SQ)
  fig2 <- layer_centroid(fig, VOTERS)
  expect_s3_class(fig2, "plotly")
})

test_that("layer_centroid adds exactly one trace", {
  skip_without_lib()
  fig  <- plot_spatial_voting(VOTERS, sq = SQ)
  n_before <- length(fig$x$attrs)
  fig2 <- layer_centroid(fig, VOTERS)
  expect_equal(length(fig2$x$attrs), n_before + 1L)
})

# ---------------------------------------------------------------------------
# layer_marginal_median
# ---------------------------------------------------------------------------

test_that("layer_marginal_median returns a plotly object", {
  skip_without_lib()
  fig  <- plot_spatial_voting(VOTERS, sq = SQ)
  fig2 <- layer_marginal_median(fig, VOTERS)
  expect_s3_class(fig2, "plotly")
})

test_that("layer_marginal_median adds exactly one trace", {
  skip_without_lib()
  fig  <- plot_spatial_voting(VOTERS, sq = SQ)
  n_before <- length(fig$x$attrs)
  fig2 <- layer_marginal_median(fig, VOTERS)
  expect_equal(length(fig2$x$attrs), n_before + 1L)
})
