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

  expect_true("indifference_curves" %in% names(w$x))
  expect_true("ic_competitor_indices" %in% names(w$x))

  ic     <- w$x$indifference_curves
  ic_idx <- w$x$ic_competitor_indices

  # Outer dimension: one entry per frame.
  expect_length(ic,     n_frames)
  expect_length(ic_idx, n_frames)

  for (f in seq_len(n_frames)) {
    seat_curves <- ic[[f]]
    seat_idxs   <- ic_idx[[f]]

    # At least one seat holder per frame.
    expect_gte(length(seat_curves), 1L)
    expect_length(seat_idxs, length(seat_curves))

    for (s_i in seq_along(seat_curves)) {
      voter_curves <- seat_curves[[s_i]]

      # One curve per voter.
      expect_length(voter_curves, n_voters)

      for (v in seq_len(n_voters)) {
        verts <- voter_curves[[v]]
        # Flat [x0,y0,...]: 16 samples × 2 coords = 32 numbers.
        expect_length(verts, 2L * 16L)
        expect_true(all(is.numeric(unlist(verts))))
      }

      # Competitor index is 0-based and within range.
      ci <- seat_idxs[[s_i]]
      expect_gte(ci, 0L)
      expect_lt(ci, d$n_competitors)
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
  # Each voter curve under 8 samples should have 16 values (8*2), under 32 should have 64.
  expect_length(w8$x$indifference_curves[[1]][[1]][[1]],  16L)
  expect_length(w32$x$indifference_curves[[1]][[1]][[1]], 64L)
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
  ic <- w$x$indifference_curves
  for (f in seq_along(ic)) {
    for (s_i in seq_along(ic[[f]])) {
      for (v in seq_len(3L)) {
        flat    <- unlist(ic[[f]][[s_i]][[v]])
        xs      <- flat[seq(1L, length(flat), 2L)]
        ys      <- flat[seq(2L, length(flat), 2L)]
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
  expect_length(w$x$indifference_curves[[1]][[1]][[1]], 8L)  # 4 verts × 2 = 8
})
