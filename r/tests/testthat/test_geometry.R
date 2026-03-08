# test_geometry.R — Tests for Copeland, Condorcet, core, uncovered set,
#                   and centrality measures.

.voters <- c(-1, -1,  1, -1,  1,  1)
.alts   <- c( 0,  0,  2,  0, -2,  0)

# ---------------------------------------------------------------------------
# copeland_scores_2d
# ---------------------------------------------------------------------------

test_that("copeland_scores_2d returns named integer vector of correct length", {
  skip_without_lib()
  scores <- copeland_scores_2d(.alts, .voters)
  expect_type(scores, "integer")
  expect_length(scores, 3L)
  expect_identical(names(scores), c("alt1", "alt2", "alt3"))
})

# ---------------------------------------------------------------------------
# copeland_winner_2d
# ---------------------------------------------------------------------------

test_that("copeland_winner_2d returns a 1-based integer in [1, n_alts]", {
  skip_without_lib()
  w <- copeland_winner_2d(.alts, .voters)
  expect_type(w, "integer")
  expect_length(w, 1L)
  expect_gte(w, 1L)
  expect_lte(w, 3L)
})

# ---------------------------------------------------------------------------
# has_condorcet_winner_2d
# ---------------------------------------------------------------------------

test_that("has_condorcet_winner_2d returns a logical scalar", {
  skip_without_lib()
  result <- has_condorcet_winner_2d(.alts, .voters)
  expect_type(result, "logical")
  expect_length(result, 1L)
})

# ---------------------------------------------------------------------------
# condorcet_winner_2d
# ---------------------------------------------------------------------------

test_that("condorcet_winner_2d returns 1-based integer or NA", {
  skip_without_lib()
  w <- condorcet_winner_2d(.alts, .voters)
  expect_true(is.na(w) || (is.integer(w) && w >= 1L && w <= 3L))
})

test_that("has/condorcet_winner_2d agree", {
  skip_without_lib()
  has_w <- has_condorcet_winner_2d(.alts, .voters)
  w     <- condorcet_winner_2d(.alts, .voters)
  if (has_w) {
    expect_false(is.na(w))
  } else {
    expect_true(is.na(w))
  }
})

# ---------------------------------------------------------------------------
# core_2d
# ---------------------------------------------------------------------------

test_that("core_2d returns a list with found, x, y", {
  skip_without_lib()
  res <- core_2d(.voters)
  expect_type(res, "list")
  expect_true(all(c("found", "x", "y") %in% names(res)))
  expect_type(res$found, "logical")
  if (res$found) {
    expect_true(is.finite(res$x))
    expect_true(is.finite(res$y))
  } else {
    expect_true(is.na(res$x))
    expect_true(is.na(res$y))
  }
})

# ---------------------------------------------------------------------------
# uncovered_set_2d
# ---------------------------------------------------------------------------

test_that("uncovered_set_2d returns non-empty 1-based integer vector", {
  skip_without_lib()
  uc <- uncovered_set_2d(.alts, .voters)
  expect_type(uc, "integer")
  expect_gte(length(uc), 1L)
  expect_true(all(uc >= 1L & uc <= 3L))
})

test_that("uncovered_set_2d indices are a subset of alternatives", {
  skip_without_lib()
  uc <- uncovered_set_2d(.alts, .voters)
  expect_true(all(uc %in% 1:3))
})

# ---------------------------------------------------------------------------
# uncovered_set_boundary_2d
# ---------------------------------------------------------------------------

test_that("uncovered_set_boundary_2d returns an n×2 numeric matrix", {
  skip_without_lib()
  bnd <- uncovered_set_boundary_2d(.voters, grid_resolution = 5L)
  expect_true(is.matrix(bnd))
  expect_identical(ncol(bnd), 2L)
})

# ---------------------------------------------------------------------------
# Centrality measures
# ---------------------------------------------------------------------------

# Three collinear voters at (-1,0), (0,0), (1,0): median = (0,0), centroid = (0,0).
.cv3 <- c(-1, 0,  0, 0,  1, 0)

# Two voters at (1,2) and (4,8): geometric mean x = sqrt(4)=2, y = sqrt(16)=4.
.gm2 <- c(1, 2,  4, 8)

test_that("marginal_median_2d returns a list with x and y", {
  skip_without_lib()
  res <- marginal_median_2d(.cv3)
  expect_type(res, "list")
  expect_identical(names(res), c("x", "y"))
  expect_equal(res$x, 0.0, tolerance = 1e-10)
  expect_equal(res$y, 0.0, tolerance = 1e-10)
})

test_that("marginal_median_2d even-n uses average of two middle values", {
  skip_without_lib()
  # Four voters on x-axis: -2, -1, 1, 2 → median x = (-1+1)/2 = 0
  res <- marginal_median_2d(c(-2, 0,  -1, 0,  1, 0,  2, 0))
  expect_equal(res$x, 0.0, tolerance = 1e-10)
})

test_that("marginal_median_2d errors on empty input", {
  skip_without_lib()
  expect_error(marginal_median_2d(numeric(0)))
})

test_that("centroid_2d returns the arithmetic mean", {
  skip_without_lib()
  res <- centroid_2d(.cv3)
  expect_type(res, "list")
  expect_identical(names(res), c("x", "y"))
  expect_equal(res$x, 0.0, tolerance = 1e-10)
  expect_equal(res$y, 0.0, tolerance = 1e-10)
})

test_that("centroid_2d single voter returns that voter's position", {
  skip_without_lib()
  res <- centroid_2d(c(3.5, -7.2))
  expect_equal(res$x, 3.5, tolerance = 1e-10)
  expect_equal(res$y, -7.2, tolerance = 1e-10)
})

test_that("centroid_2d errors on empty input", {
  skip_without_lib()
  expect_error(centroid_2d(numeric(0)))
})

test_that("geometric_mean_2d returns the geometric mean for positive coords", {
  skip_without_lib()
  res <- geometric_mean_2d(.gm2)
  expect_equal(res$x, 2.0, tolerance = 1e-10)
  expect_equal(res$y, 4.0, tolerance = 1e-10)
})

test_that("geometric_mean_2d errors on non-positive coordinates", {
  skip_without_lib()
  expect_error(geometric_mean_2d(.voters))  # .voters has negative coords
})

test_that("geometric_mean_2d errors on empty input", {
  skip_without_lib()
  expect_error(geometric_mean_2d(numeric(0)))
})
