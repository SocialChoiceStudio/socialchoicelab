# test_plots.R — C10.4: Unit tests for spatial voting visualization helpers
#
# Tests that do NOT call the C library (plot_spatial_voting, layer_yolk,
# layer_convex_hull, layer_uncovered_set with synthetic data) run without
# skip_without_lib(). Tests that wrap C handles (layer_winset) require the lib.

# ---------------------------------------------------------------------------
# plot_spatial_voting
# ---------------------------------------------------------------------------

test_that("plot_spatial_voting returns a plotly object", {
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
  alts   <- c(0.0,  0.0,  0.6, 0.4)
  fig    <- plot_spatial_voting(voters, alts)
  expect_s3_class(fig, "plotly")
})

test_that("plot_spatial_voting with sq returns a plotly object", {
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
  alts   <- c(0.0,  0.0,  0.6, 0.4)
  fig    <- plot_spatial_voting(voters, alts, sq = c(0.0, 0.0))
  expect_s3_class(fig, "plotly")
})

test_that("plot_spatial_voting accepts custom labels", {
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
  alts   <- c(0.0,  0.0)
  fig    <- plot_spatial_voting(
    voters, alts,
    voter_names = c("Alice", "Bob", "Carol"),
    alt_names   = "Origin",
    dim_names   = c("Economic", "Social"),
    title       = "Test Legislature"
  )
  expect_s3_class(fig, "plotly")
})

test_that("plot_spatial_voting accepts custom dimensions", {
  voters <- c(-1.0, -0.5, 0.0, 0.0)
  alts   <- c(0.0,  0.0)
  fig    <- plot_spatial_voting(voters, alts, width = 800L, height = 500L)
  expect_s3_class(fig, "plotly")
})

# ---------------------------------------------------------------------------
# layer_yolk
# ---------------------------------------------------------------------------

test_that("layer_yolk returns a plotly object", {
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
  fig    <- plot_spatial_voting(voters, c(0.0, 0.0))
  fig2   <- layer_yolk(fig, center_x = 0.1, center_y = 0.05, radius = 0.3)
  expect_s3_class(fig2, "plotly")
})

test_that("layer_yolk with custom color returns a plotly object", {
  fig  <- plot_spatial_voting(c(-1, -1, 1, 1), c(0, 0))
  fig2 <- layer_yolk(fig, 0, 0, 0.5, color = "rgba(0,0,255,0.4)", name = "Approx. Yolk")
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
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6, -0.4, 0.8, 0.5, -0.7)
  hull   <- convex_hull_2d(voters)
  fig    <- plot_spatial_voting(voters, c(0.0, 0.0))
  fig2   <- layer_convex_hull(fig, hull)
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
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6, -0.4, 0.8, 0.5, -0.7)
  bnd    <- uncovered_set_boundary_2d(voters, grid_resolution = 8L)
  fig    <- plot_spatial_voting(voters, c(0.0, 0.0))
  fig2   <- layer_uncovered_set(fig, bnd)
  expect_s3_class(fig2, "plotly")
})

# ---------------------------------------------------------------------------
# layer_winset
# ---------------------------------------------------------------------------

test_that("layer_winset with non-empty winset returns a plotly object", {
  skip_without_lib()
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6, -0.4, 0.8, 0.5, -0.7)
  ws     <- winset_2d(c(0.0, 0.0), voters)
  fig    <- plot_spatial_voting(voters, c(0.0, 0.0), sq = c(0.0, 0.0))
  fig2   <- layer_winset(fig, ws)
  expect_s3_class(fig2, "plotly")
})

test_that("layer_winset with empty winset returns figure unchanged", {
  skip_without_lib()
  voters <- c(0.0, 0.0, 0.0, 0.0, 0.0, 0.0)
  ws     <- winset_2d(c(0.0, 0.0), voters)
  fig    <- plot_spatial_voting(voters, c(0.0, 0.0))
  fig2   <- layer_winset(fig, ws)
  expect_s3_class(fig2, "plotly")
})

# ---------------------------------------------------------------------------
# Full composition
# ---------------------------------------------------------------------------

test_that("all layers can be composed without error", {
  skip_without_lib()
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6, -0.4, 0.8, 0.5, -0.7)
  alts   <- c(0.0,  0.0,  0.6, 0.4, -0.5, 0.3)
  sq     <- c(0.0,  0.0)

  ws   <- winset_2d(sq, voters)
  hull <- convex_hull_2d(voters)
  bnd  <- uncovered_set_boundary_2d(voters, grid_resolution = 8L)

  fig <- plot_spatial_voting(voters, alts, sq = sq, title = "Composition test")
  fig <- layer_winset(fig, ws)
  fig <- layer_uncovered_set(fig, bnd)
  fig <- layer_convex_hull(fig, hull)
  fig <- layer_yolk(fig, center_x = 0.1, center_y = 0.05, radius = 0.25)
  expect_s3_class(fig, "plotly")
})
