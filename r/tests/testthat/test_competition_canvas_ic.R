# test_competition_canvas_ic.R — Unit tests for animate_competition_canvas IC feature.

# Fixture: 3 2D voters, 2 sticker competitors, 1 seat, 2 rounds.
# Competitor 0 at (0,0) wins every round (closer to 2 of 3 voters).
# This gives n_frames = 3 (Round 1, Round 2, Final) and 1 seat holder per frame.
.ic_trace_2d <- function() {
  competition_run(
    competitor_positions = c(0.0, 0.0, 2.0, 0.0),
    strategy_kinds       = c("sticker", "sticker"),
    voter_ideals         = c(-0.5, 0.0, 0.0, 0.2, 1.8, 0.1),
    dist_config          = make_dist_config(n_dims = 2L),
    engine_config        = make_competition_engine_config(
      seat_count = 1L,
      seat_rule  = "plurality_top_k",
      max_rounds = 2L,
      step_config = make_competition_step_config(
        kind = "fixed", fixed_step_size = 0.5
      )
    )
  )
}

IC_VOTERS <- c(-0.5, 0.0, 0.0, 0.2, 1.8, 0.1)  # 3 voters, 2D flat


test_that("compute_ic = FALSE leaves indifference_curves absent from payload", {
  skip_without_lib()
  trace <- .ic_trace_2d()
  w <- animate_competition_canvas(trace, voters = IC_VOTERS)
  expect_false("indifference_curves" %in% names(w$x))
  expect_false("ic_competitor_indices" %in% names(w$x))
})

test_that("compute_ic = TRUE produces correctly shaped IC payload", {
  skip_without_lib()
  trace <- .ic_trace_2d()
  w <- animate_competition_canvas(
    trace,
    voters         = IC_VOTERS,
    compute_ic     = TRUE,
    ic_num_samples = 16L
  )
  d <- trace$dims()
  n_frames  <- d$n_rounds + 1L   # 3
  n_voters  <- 3L

  expect_true("overlays_frames" %in% names(w$x))
  expect_false("indifference_curves" %in% names(w$x))
  expect_false("ic_competitor_indices" %in% names(w$x))

  frame_overlays <- w$x$overlays_frames

  expect_length(frame_overlays, n_frames)

  for (f in seq_len(n_frames)) {
    canonical_entries <- frame_overlays[[f]]$indifference_curves

    expect_gte(length(canonical_entries), 1L)

    for (s_i in seq_along(canonical_entries)) {
      voter_curves <- canonical_entries[[s_i]]$curves
      expect_gte(canonical_entries[[s_i]]$candidate, 0L)
      expect_lt(canonical_entries[[s_i]]$candidate, d$n_competitors)

      expect_length(voter_curves, n_voters)

      for (v in seq_len(n_voters)) {
        ring <- voter_curves[[v]]$ring
        expect_length(ring, 16L)
        expect_true(all(vapply(ring, length, integer(1L)) == 2L))
        expect_true(all(is.numeric(unlist(ring))))
      }
    }
  }
})

test_that("compute_ic = TRUE with ic_max_curves exceeded raises an error", {
  skip_without_lib()
  trace <- .ic_trace_2d()
  # 3 voters × 1 seat × 3 frames = 9, which exceeds ic_max_curves = 5.
  expect_error(
    animate_competition_canvas(
      trace,
      voters        = IC_VOTERS,
      compute_ic    = TRUE,
      ic_max_curves = 5L
    ),
    "ic_max_curves"
  )
})

test_that("compute_ic = TRUE without voters raises an error", {
  skip_without_lib()
  trace <- .ic_trace_2d()
  expect_error(
    animate_competition_canvas(trace, compute_ic = TRUE),
    "requires voters"
  )
})

test_that("compute_ic respects ic_num_samples for polygon vertex count", {
  skip_without_lib()
  trace <- .ic_trace_2d()
  w8  <- animate_competition_canvas(
    trace, voters = IC_VOTERS, compute_ic = TRUE, ic_num_samples = 8L
  )
  w32 <- animate_competition_canvas(
    trace, voters = IC_VOTERS, compute_ic = TRUE, ic_num_samples = 32L
  )
  expect_length(w8$x$overlays_frames[[1]]$indifference_curves[[1]]$curves[[1]]$ring,  8L)
  expect_length(w32$x$overlays_frames[[1]]$indifference_curves[[1]]$curves[[1]]$ring, 32L)
})

test_that("Euclidean IC polygon vertices are all equidistant from the voter ideal", {
  # This is the geometric invariant that — combined with equal-aspect rendering
  # in the JS canvas — guarantees indifference curves appear as true circles.
  # If this test breaks, the binding layer is computing incorrect polygons.
  skip_without_lib()
  trace <- .ic_trace_2d()
  w <- animate_competition_canvas(
    trace, voters = IC_VOTERS, compute_ic = TRUE, ic_num_samples = 64L
  )
  ic <- w$x$overlays_frames
  for (f in seq_along(ic)) {
    for (s_i in seq_along(ic[[f]]$indifference_curves)) {
      for (v in seq_len(3L)) {
        ring    <- ic[[f]]$indifference_curves[[s_i]]$curves[[v]]$ring
        xs      <- vapply(ring, `[[`, numeric(1L), 1L)
        ys      <- vapply(ring, `[[`, numeric(1L), 2L)
        voter_x <- IC_VOTERS[2L * v - 1L]
        voter_y <- IC_VOTERS[2L * v]
        dists   <- sqrt((xs - voter_x)^2 + (ys - voter_y)^2)
        expect_true(
          all(abs(dists - dists[1L]) < 1e-10),
          info = paste("frame", f, "seat", s_i, "voter", v,
                       "not equidistant: range",
                       round(max(dists) - min(dists), 12))
        )
      }
    }
  }
})

test_that("compute_ic with manhattan dist produces non-circular ICs", {
  skip_without_lib()
  trace_manh <- competition_run(
    competitor_positions = c(0.0, 0.0, 2.0, 0.0),
    strategy_kinds       = c("sticker", "sticker"),
    voter_ideals         = c(-0.5, 0.0, 0.0, 0.2, 1.8, 0.1),
    dist_config          = make_dist_config("manhattan", n_dims = 2L),
    engine_config        = make_competition_engine_config(
      seat_count = 1L,
      seat_rule  = "plurality_top_k",
      max_rounds = 2L,
      step_config = make_competition_step_config(kind = "fixed", fixed_step_size = 0.5)
    )
  )
  # Manhattan distance IC is a rotated square (diamond) — exactly 4 vertices.
  w <- animate_competition_canvas(
    trace_manh,
    voters         = IC_VOTERS,
    compute_ic     = TRUE,
    ic_dist_config = make_dist_config("manhattan", n_dims = 2L),
    ic_num_samples = 32L
  )
  # The polygon type for Manhattan is a rotated square; level_set_to_polygon
  # returns exactly 4 vertices regardless of ic_num_samples.
  expect_length(w$x$overlays_frames[[1]]$indifference_curves[[1]]$curves[[1]]$ring, 4L)
})
