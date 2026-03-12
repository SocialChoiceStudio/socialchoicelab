# test_voter_sampler.R — Tests for make_voter_sampler() and draw_voters().

test_that("make_voter_sampler returns correct structure for gaussian", {
  s <- make_voter_sampler("gaussian", mean = 0.5, sd = 0.3)
  expect_s3_class(s, "VoterSamplerConfig")
  expect_equal(s$kind,   1L)
  expect_equal(s$param1, 0.5)
  expect_equal(s$param2, 0.3)
})

test_that("make_voter_sampler returns correct structure for uniform", {
  s <- make_voter_sampler("uniform", lo = -2.0, hi = 2.0)
  expect_s3_class(s, "VoterSamplerConfig")
  expect_equal(s$kind,   0L)
  expect_equal(s$param1, -2.0)
  expect_equal(s$param2,  2.0)
})

test_that("make_voter_sampler errors on bad sd", {
  expect_error(make_voter_sampler("gaussian", sd = 0))
  expect_error(make_voter_sampler("gaussian", sd = -1))
})

test_that("make_voter_sampler errors when lo >= hi", {
  expect_error(make_voter_sampler("uniform", lo = 1, hi = 1))
  expect_error(make_voter_sampler("uniform", lo = 2, hi = 1))
})

test_that("draw_voters returns numeric vector of correct length (gaussian)", {
  skip_without_lib()
  sm <- stream_manager(42L, "vs")
  s  <- make_voter_sampler("gaussian", mean = 0, sd = 0.4)
  v  <- draw_voters(50L, s, sm, "vs")
  expect_type(v, "double")
  expect_length(v, 100L)
  expect_true(all(is.finite(v)))
})

test_that("draw_voters returns numeric vector of correct length (uniform)", {
  skip_without_lib()
  sm <- stream_manager(7L, "vs")
  s  <- make_voter_sampler("uniform", lo = -1, hi = 1)
  v  <- draw_voters(30L, s, sm, "vs")
  expect_type(v, "double")
  expect_length(v, 60L)
  expect_true(all(v >= -1 & v <= 1))
})

test_that("draw_voters is reproducible given same seed", {
  skip_without_lib()
  s    <- make_voter_sampler("gaussian", mean = 0, sd = 1)
  sm1  <- stream_manager(99L, "vs")
  sm2  <- stream_manager(99L, "vs")
  expect_equal(draw_voters(20L, s, sm1, "vs"),
               draw_voters(20L, s, sm2, "vs"))
})

test_that("draw_voters result is usable as voter_ideals in competition_run", {
  skip_without_lib()
  sm      <- stream_manager(1L, c("vs", "competition_adaptation_hunter"))
  s       <- make_voter_sampler("gaussian", mean = 0, sd = 0.4)
  voters  <- draw_voters(30L, s, sm, "vs")
  headings <- c(0.71, 0.71, -0.71, -0.71)
  trace <- competition_run(
    competitor_positions = c(-0.5, -0.5, 0.5, 0.5),
    competitor_headings  = headings,
    strategy_kinds       = c("hunter", "hunter"),
    voter_ideals         = voters,
    dist_config          = make_dist_config(n_dims = 2L),
    engine_config        = make_competition_engine_config(
      seat_count = 1L,
      seat_rule  = "plurality_top_k",
      max_rounds = 5L,
      step_config = make_competition_step_config(
        kind = "fixed", fixed_step_size = 0.1
      )
    ),
    stream_manager = sm
  )
  expect_s3_class(trace, "CompetitionTrace")
  expect_gte(trace$dims()$n_rounds, 1L)
})

test_that("draw_voters errors when sampler is not VoterSamplerConfig", {
  skip_without_lib()
  sm <- stream_manager(1L, "vs")
  expect_error(draw_voters(10L, list(kind = 0L), sm, "vs"))
})
