# test_functions.R — Tests for distance, utility, level-set, convex hull,
#                   and majority-preference functions.
# Tests call the C library via the package; skip if libscs_api is unavailable.

# Pure-R data — no C calls at file-load time.
.voters <- c(-1, -1,  1, -1,  1,  1)
.alts   <- c( 0,  0,  2,  0, -2,  0)

# ---------------------------------------------------------------------------
# scs_version
# ---------------------------------------------------------------------------

test_that("scs_version returns named integer list", {
  skip_without_lib()
  v <- scs_version()
  expect_type(v, "list")
  expect_named(v, c("major", "minor", "patch"))
  expect_true(all(sapply(v, is.integer) | sapply(v, is.double)))
  expect_true(all(unlist(v) >= 0))
})

# ---------------------------------------------------------------------------
# calculate_distance
# ---------------------------------------------------------------------------

test_that("calculate_distance returns 0 for identical points", {
  skip_without_lib()
  expect_equal(calculate_distance(c(0, 0), c(0, 0)), 0.0)
})

test_that("calculate_distance returns 5 for Euclidean (3,4) triangle", {
  skip_without_lib()
  expect_equal(calculate_distance(c(0, 0), c(3, 4)), 5.0, tolerance = 1e-10)
})

test_that("calculate_distance returns correct Manhattan distance", {
  skip_without_lib()
  d <- calculate_distance(c(0, 0), c(3, 4), make_dist_config("manhattan"))
  expect_equal(d, 7.0, tolerance = 1e-10)
})

test_that("calculate_distance is non-negative", {
  skip_without_lib()
  d <- calculate_distance(c(1, 2), c(-1, -3))
  expect_gte(d, 0.0)
})

# ---------------------------------------------------------------------------
# distance_to_utility
# ---------------------------------------------------------------------------

test_that("distance_to_utility at 0 is 0 under linear loss", {
  skip_without_lib()
  expect_equal(distance_to_utility(0), 0.0, tolerance = 1e-10)
})

test_that("distance_to_utility is non-increasing", {
  skip_without_lib()
  u0 <- distance_to_utility(0)
  u1 <- distance_to_utility(1)
  u2 <- distance_to_utility(2)
  expect_gte(u0, u1)
  expect_gte(u1, u2)
})

# ---------------------------------------------------------------------------
# normalize_utility
# ---------------------------------------------------------------------------

test_that("normalize_utility returns 1 at distance 0", {
  skip_without_lib()
  u0 <- distance_to_utility(0)
  expect_equal(normalize_utility(u0, max_distance = 5), 1.0, tolerance = 1e-10)
})

test_that("normalize_utility returns value in [0, 1]", {
  skip_without_lib()
  u <- distance_to_utility(2)
  n <- normalize_utility(u, max_distance = 5)
  expect_gte(n, 0.0)
  expect_lte(n, 1.0)
})

# ---------------------------------------------------------------------------
# level_set_1d
# ---------------------------------------------------------------------------

test_that("level_set_1d returns a numeric vector of length 0 to 2", {
  skip_without_lib()
  pts <- level_set_1d(ideal = 0, weight = 1, utility_level = -1)
  expect_type(pts, "double")
  expect_true(length(pts) %in% 0:2)
})

# ---------------------------------------------------------------------------
# level_set_2d
# ---------------------------------------------------------------------------

test_that("level_set_2d returns a named list with expected elements", {
  skip_without_lib()
  ls <- level_set_2d(ideal_x = 0, ideal_y = 0, utility_level = -1)
  expect_type(ls, "list")
  expect_true(all(c("type", "center_x", "center_y", "param0") %in% names(ls)))
  expect_true(ls$type %in% c("circle", "ellipse", "superellipse", "polygon"))
})

# ---------------------------------------------------------------------------
# level_set_to_polygon
# ---------------------------------------------------------------------------

test_that("level_set_to_polygon returns a n×2 numeric matrix", {
  skip_without_lib()
  ls   <- level_set_2d(0, 0, utility_level = -1)
  poly <- level_set_to_polygon(ls, n_samples = 16L)
  expect_true(is.matrix(poly))
  expect_identical(ncol(poly), 2L)
  expect_gte(nrow(poly), 3L)
})

# ---------------------------------------------------------------------------
# ic_polygon_2d
# ---------------------------------------------------------------------------

test_that("ic_polygon_2d matches four-call path", {
  skip_without_lib()
  lc <- make_loss_config(loss_type = "linear", sensitivity = 1)
  dc <- make_dist_config(distance_type = "euclidean", weights = c(1, 1))
  n  <- 32L
  p1 <- ic_polygon_2d(1, 2, 4, 6, lc, dc, n)
  d  <- calculate_distance(c(1, 2), c(4, 6), dc)
  ul <- distance_to_utility(d, lc)
  ls <- level_set_2d(1, 2, ul, lc, dc)
  p4 <- level_set_to_polygon(ls, n)
  expect_equal(as.numeric(p1), as.numeric(p4), tolerance = 1e-12)
})

# ---------------------------------------------------------------------------
# convex_hull_2d
# ---------------------------------------------------------------------------

test_that("convex_hull_2d returns a matrix with 2 columns", {
  skip_without_lib()
  pts  <- c(0, 0, 1, 0, 1, 1, 0, 1, 0.5, 0.5)
  hull <- convex_hull_2d(pts)
  expect_true(is.matrix(hull))
  expect_identical(ncol(hull), 2L)
  expect_identical(nrow(hull), 4L)
})

# ---------------------------------------------------------------------------
# majority_prefers_2d
# ---------------------------------------------------------------------------

test_that("majority_prefers_2d returns a logical scalar", {
  skip_without_lib()
  result <- majority_prefers_2d(c(0, 0), c(5, 0), .voters)
  expect_type(result, "logical")
  expect_length(result, 1L)
})

test_that("majority_prefers_2d: clearly dominant point wins", {
  skip_without_lib()
  expect_true(majority_prefers_2d(c(0, 0), c(5, 0), .voters))
})

# ---------------------------------------------------------------------------
# pairwise_matrix_2d
# ---------------------------------------------------------------------------

test_that("pairwise_matrix_2d returns a square integer matrix", {
  skip_without_lib()
  mat <- pairwise_matrix_2d(.alts, .voters)
  expect_true(is.matrix(mat))
  expect_identical(nrow(mat), 3L)
  expect_identical(ncol(mat), 3L)
  expect_type(mat, "integer")
})

test_that("pairwise_matrix_2d has zero diagonal", {
  skip_without_lib()
  mat <- pairwise_matrix_2d(.alts, .voters)
  expect_equal(unname(diag(mat)), rep(0L, 3L))
})

test_that("pairwise_matrix_2d is antisymmetric", {
  skip_without_lib()
  mat <- pairwise_matrix_2d(.alts, .voters)
  expect_equal(mat, -t(mat))
})

test_that("pairwise_matrix_2d entries are in {-1, 0, 1}", {
  skip_without_lib()
  mat <- pairwise_matrix_2d(.alts, .voters)
  expect_true(all(mat %in% c(-1L, 0L, 1L)))
})

test_that("pairwise_matrix_2d has correct dimnames", {
  skip_without_lib()
  mat <- pairwise_matrix_2d(.alts, .voters)
  expect_identical(rownames(mat), c("alt1", "alt2", "alt3"))
  expect_identical(colnames(mat), c("alt1", "alt2", "alt3"))
})

# ---------------------------------------------------------------------------
# weighted_majority_prefers_2d
# ---------------------------------------------------------------------------

test_that("weighted_majority_prefers_2d returns logical", {
  skip_without_lib()
  result <- weighted_majority_prefers_2d(
    c(0, 0), c(5, 0), .voters, weights = c(1, 1, 1)
  )
  expect_type(result, "logical")
  expect_length(result, 1L)
})
