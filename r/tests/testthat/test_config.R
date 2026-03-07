# test_config.R — Tests for make_dist_config() and make_loss_config().
# These are pure-R functions; no C library required.

test_that("make_dist_config returns correct structure for defaults", {
  cfg <- make_dist_config()
  expect_type(cfg, "list")
  expect_named(cfg, c("distance_type", "weights", "n_weights", "order_p"))
  expect_identical(cfg$distance_type, 0L)   # euclidean
  expect_equal(cfg$weights, c(1.0, 1.0))
  expect_identical(cfg$n_weights, 2L)
  expect_equal(cfg$order_p, 2.0)
})

test_that("make_dist_config encodes distance types correctly", {
  expect_identical(make_dist_config("euclidean")$distance_type,  0L)
  expect_identical(make_dist_config("manhattan")$distance_type,  1L)
  expect_identical(make_dist_config("chebyshev")$distance_type,  2L)
  expect_identical(make_dist_config("minkowski")$distance_type,  3L)
})

test_that("make_dist_config rejects unknown distance_type", {
  expect_error(make_dist_config("cosine"), "unknown distance_type")
})

test_that("make_dist_config respects custom weights", {
  cfg <- make_dist_config(weights = c(2.0, 0.5))
  expect_equal(cfg$weights, c(2.0, 0.5))
  expect_identical(cfg$n_weights, 2L)
})

test_that("make_dist_config respects n_dims when weights is NULL", {
  cfg <- make_dist_config(n_dims = 3L)
  expect_equal(cfg$weights, c(1.0, 1.0, 1.0))
  expect_identical(cfg$n_weights, 3L)
})

test_that("make_loss_config returns correct structure for defaults", {
  cfg <- make_loss_config()
  expect_type(cfg, "list")
  expect_named(cfg, c("loss_type", "sensitivity", "max_loss", "steepness", "threshold"))
  expect_identical(cfg$loss_type, 0L)   # linear
  expect_equal(cfg$sensitivity, 1.0)
})

test_that("make_loss_config encodes loss types correctly", {
  expect_identical(make_loss_config("linear")$loss_type,    0L)
  expect_identical(make_loss_config("quadratic")$loss_type, 1L)
  expect_identical(make_loss_config("gaussian")$loss_type,  2L)
  expect_identical(make_loss_config("threshold")$loss_type, 3L)
})

test_that("make_loss_config rejects unknown loss_type", {
  expect_error(make_loss_config("sigmoid"), "unknown loss_type")
})

test_that("make_dist_config rejects k='bad' through resolve_k via geometry fn", {
  skip_without_lib()
  alts <- c(0, 0, 1, 0); voters <- c(0, 0, 1, 0)
  expect_error(
    copeland_scores_2d(alts, voters, k = 0L),
    "positive integer"
  )
})

test_that("plurality_one_winner errors on tie_break='random' without stream", {
  skip_without_lib()
  prof <- profile_from_utility_matrix(c(1, 2), n_voters = 1L, n_alts = 2L)
  expect_error(
    plurality_one_winner(prof, tie_break = "random"),
    "requires both"
  )
})

test_that("plurality_one_winner errors on unknown tie_break", {
  skip_without_lib()
  prof <- profile_from_utility_matrix(c(1, 2), n_voters = 1L, n_alts = 2L)
  expect_error(
    plurality_one_winner(prof, tie_break = "median"),
    "must be"
  )
})
