# tests/testthat/test_scenarios.R

test_that("list_scenarios returns available scenario names", {
  scenarios <- list_scenarios()
  expect_type(scenarios, "character")
  expect_true(length(scenarios) > 0)
  expect_true("laing_olmsted_bear" %in% scenarios)
  expect_true("tovey_regular_polygon" %in% scenarios)
  expect_true(all(nzchar(scenarios)))  # all non-empty
})

test_that("list_scenarios returns sorted names", {
  scenarios <- list_scenarios()
  expect_identical(scenarios, sort(scenarios))
})

test_that("load_scenario returns correct structure", {
  sc <- load_scenario("laing_olmsted_bear")
  expect_type(sc, "list")
  expect_true("name" %in% names(sc))
  expect_true("description" %in% names(sc))
  expect_true("source" %in% names(sc))
  expect_true("n_voters" %in% names(sc))
  expect_true("n_dimensions" %in% names(sc))
  expect_true("space" %in% names(sc))
  expect_true("decision_rule" %in% names(sc))
  expect_true("dim_names" %in% names(sc))
  expect_true("voters" %in% names(sc))
  expect_true("voter_names" %in% names(sc))
  expect_true("status_quo" %in% names(sc))
})

test_that("load_scenario returns correct types", {
  sc <- load_scenario("laing_olmsted_bear")
  expect_type(sc$name, "character")
  expect_length(sc$name, 1)
  expect_type(sc$description, "character")
  expect_type(sc$source, "character")
  expect_type(sc$n_voters, "integer")
  expect_type(sc$n_dimensions, "integer")
  expect_type(sc$decision_rule, "double")
  expect_type(sc$dim_names, "character")
  expect_type(sc$voters, "double")
  expect_type(sc$voter_names, "character")
  expect_type(sc$status_quo, "double")
})

test_that("load_scenario laing_olmsted_bear has correct dimensions", {
  sc <- load_scenario("laing_olmsted_bear")
  expect_equal(sc$n_voters, 5L)
  expect_equal(sc$n_dimensions, 2L)
  expect_equal(length(sc$voters), 10L)  # 5 voters * 2 dims
  expect_equal(length(sc$voter_names), 5L)
  expect_equal(length(sc$status_quo), 2L)
  expect_equal(length(sc$dim_names), 2L)
})

test_that("load_scenario laing_olmsted_bear has correct data", {
  sc <- load_scenario("laing_olmsted_bear")
  expect_equal(sc$n_voters, 5L)
  expect_equal(sc$voters[1:2], c(20.8, 79.6), tolerance = 1e-5)
  expect_equal(sc$status_quo, c(100.0, 100.0), tolerance = 1e-5)
  expect_equal(sc$decision_rule, 0.5)
})

test_that("load_scenario tovey_regular_polygon has correct dimensions", {
  sc <- load_scenario("tovey_regular_polygon")
  expect_equal(sc$n_voters, 11L)
  expect_equal(sc$n_dimensions, 2L)
  expect_equal(length(sc$voters), 22L)  # 11 voters * 2 dims
  expect_equal(length(sc$voter_names), 11L)
})

test_that("load_scenario with unknown name raises error", {
  expect_error(load_scenario("nonexistent_scenario"), "Unknown scenario")
  expect_error(load_scenario("nonexistent_scenario"), "Available scenarios:")
})

test_that("load_scenario space contains ranges", {
  sc <- load_scenario("laing_olmsted_bear")
  expect_type(sc$space, "list")
  expect_true("x_range" %in% names(sc$space))
  expect_true("y_range" %in% names(sc$space))
  expect_equal(sc$space$x_range, c(0, 200))
  expect_equal(sc$space$y_range, c(0, 200))
})
