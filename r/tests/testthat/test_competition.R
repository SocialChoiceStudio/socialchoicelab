# test_competition.R — Competition trace and experiment binding smoke tests.

test_that("competition_run returns a CompetitionTrace with usable exports", {
  skip_without_lib()
  competitors <- c(0.0, 3.0)
  voters <- c(0.1, 0.2, 2.9)
  cfg <- make_competition_engine_config(
    seat_count = 1L,
    seat_rule = "plurality_top_k",
    max_rounds = 2L,
    step_config = make_competition_step_config(
      kind = "fixed",
      fixed_step_size = 1.0
    )
  )
  trace <- competition_run(
    competitors,
    c("sticker", "sticker"),
    voters,
    dist_config = make_dist_config(n_dims = 1L),
    engine_config = cfg
  )
  expect_s3_class(trace, "CompetitionTrace")
  expect_equal(trace$dims(), list(n_rounds = 2L, n_competitors = 2L, n_dims = 1L))
  term <- trace$termination()
  expect_false(term$terminated_early)
  expect_identical(term$reason, "max_rounds")
  round0 <- trace$round_positions(1L)
  expect_true(is.matrix(round0))
  expect_identical(dim(round0), c(2L, 1L))
  expect_equal(unname(drop(round0[, 1])), c(0, 3), tolerance = 1e-12)
  expect_equal(
    unname(trace$round_vote_shares(1L)),
    c(2 / 3, 1 / 3),
    tolerance = 1e-12
  )
  expect_identical(unname(trace$round_vote_totals(1L)), c(2L, 1L))
  expect_identical(unname(trace$round_seat_totals(1L)), c(1L, 0L))
  expect_equal(unname(trace$final_vote_shares()), c(2 / 3, 1 / 3), tolerance = 1e-12)
  expect_equal(unname(trace$final_seat_shares()), c(1, 0), tolerance = 1e-12)
})

test_that("competition_run_experiment returns summary statistics", {
  skip_without_lib()
  competitors <- c(0.0, 3.0)
  voters <- c(0.1, 0.2, 2.9)
  cfg <- make_competition_engine_config(
    seat_count = 1L,
    seat_rule = "plurality_top_k",
    max_rounds = 3L,
    step_config = make_competition_step_config(
      kind = "fixed",
      fixed_step_size = 1.0
    ),
    termination = make_competition_termination_config(
      stop_on_convergence = TRUE,
      position_epsilon = 1e-12
    )
  )
  experiment <- competition_run_experiment(
    competitors,
    c("sticker", "sticker"),
    voters,
    dist_config = make_dist_config(n_dims = 1L),
    engine_config = cfg,
    master_seed = 2026,
    num_runs = 3L
  )
  expect_s3_class(experiment, "CompetitionExperiment")
  expect_equal(experiment$dims(),
               list(n_runs = 3L, n_competitors = 2L, n_dims = 1L))
  expect_equal(experiment$summary()$mean_rounds, 1.0, tolerance = 1e-12)
  expect_equal(experiment$summary()$early_termination_rate, 1.0, tolerance = 1e-12)
  expect_equal(unname(experiment$mean_final_vote_shares()),
               c(2 / 3, 1 / 3), tolerance = 1e-12)
  expect_identical(as.integer(experiment$run_round_counts()), c(1L, 1L, 1L))
  expect_identical(experiment$run_termination_reasons(),
                   c("converged", "converged", "converged"))
  expect_identical(as.logical(experiment$run_terminated_early_flags()),
                   c(TRUE, TRUE, TRUE))
  expect_equal(experiment$run_final_vote_shares(),
               matrix(rep(c(2 / 3, 1 / 3), 3), nrow = 3, byrow = TRUE),
               tolerance = 1e-12)
  expect_equal(experiment$run_final_seat_shares(),
               matrix(rep(c(1, 0), 3), nrow = 3, byrow = TRUE),
               tolerance = 1e-12)
  expect_equal(experiment$run_final_positions(),
               array(c(0, 3, 0, 3, 0, 3), dim = c(3, 2, 1)),
               tolerance = 1e-12)
})
