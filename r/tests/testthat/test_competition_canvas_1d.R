# test_competition_canvas_1d.R — Unit tests for 1D animate_competition_canvas.

# Fixture: 3 1D voters, 2 sticker competitors, 1 seat, 2 rounds.
# Competitor 0 at x=0 wins every round (closest to 2 of 3 voters).
# n_frames = 3 (Round 1, Round 2, Final); 1 seat holder per frame.

.voters_1d <- c(-0.5, 0.1, 1.8)   # 3 ideal points (flat 1D)

.trace_1d <- function() {
  competition_run(
    competitor_positions = c(0.0, 2.0),
    strategy_kinds       = c("sticker", "sticker"),
    voter_ideals         = .voters_1d,
    dist_config          = make_dist_config(n_dims = 1L),
    engine_config        = make_competition_engine_config(
      seat_count  = 1L,
      seat_rule   = "plurality_top_k",
      max_rounds  = 2L,
      step_config = make_competition_step_config(kind = "fixed", fixed_step_size = 0.5)
    )
  )
}

# ---------------------------------------------------------------------------
# Payload shape tests
# ---------------------------------------------------------------------------

test_that("1D payload has n_dims = 1", {
  skip_without_lib()
  trace <- .trace_1d()
  w <- animate_competition_canvas(trace, voters = .voters_1d, width = 900L, height = 400L)
  expect_equal(w$x$n_dims, 1L)
})

test_that("1D payload has voters_x and no voters_y", {
  skip_without_lib()
  trace <- .trace_1d()
  w <- animate_competition_canvas(trace, voters = .voters_1d, width = 900L, height = 400L)
  expect_true("voters_x" %in% names(w$x))
  expect_false("voters_y" %in% names(w$x))
  expect_length(w$x$voters_x, 3L)
})

test_that("1D payload positions have length-1 inner lists", {
  skip_without_lib()
  trace <- .trace_1d()
  w <- animate_competition_canvas(trace, voters = .voters_1d, width = 900L, height = 400L)
  for (frame in w$x$positions) {
    for (comp_pos in frame) {
      expect_length(comp_pos, 1L)
    }
  }
})

test_that("1D payload has no ylim", {
  skip_without_lib()
  trace <- .trace_1d()
  w <- animate_competition_canvas(trace, voters = .voters_1d, width = 900L, height = 400L)
  expect_false("ylim" %in% names(w$x))
})

test_that("1D payload has no winsets", {
  skip_without_lib()
  trace <- .trace_1d()
  w <- animate_competition_canvas(trace, voters = .voters_1d, width = 900L, height = 400L)
  expect_false("winsets" %in% names(w$x))
})

test_that("1D dim_names is a length-1 list", {
  skip_without_lib()
  trace <- .trace_1d()
  w <- animate_competition_canvas(
    trace, voters = .voters_1d, dim_names = "Policy Dimension",
    width = 900L, height = 400L
  )
  expect_length(w$x$dim_names, 1L)
  expect_equal(w$x$dim_names[[1L]], "Policy Dimension")
})

test_that("1D xlim covers all voter and competitor positions", {
  skip_without_lib()
  trace <- .trace_1d()
  w <- animate_competition_canvas(trace, voters = .voters_1d, width = 900L, height = 400L)
  xlim <- unlist(w$x$xlim)
  expect_lte(xlim[1L], min(.voters_1d))
  expect_gte(xlim[2L], max(.voters_1d))
})

# ---------------------------------------------------------------------------
# IC tests
# ---------------------------------------------------------------------------

test_that("compute_ic = FALSE leaves IC keys absent", {
  skip_without_lib()
  trace <- .trace_1d()
  w <- animate_competition_canvas(trace, voters = .voters_1d, width = 900L, height = 400L)
  expect_false("indifference_curves" %in% names(w$x))
  expect_false("ic_competitor_indices" %in% names(w$x))
})

test_that("compute_ic = TRUE produces correctly shaped 1D IC payload", {
  skip_without_lib()
  trace <- .trace_1d()
  w <- animate_competition_canvas(
    trace,
    voters     = .voters_1d,
    compute_ic = TRUE,
    width      = 900L,
    height     = 400L
  )
  d        <- trace$dims()
  n_frames <- d$n_rounds + 1L   # 3
  n_voters <- 3L

  expect_true("indifference_curves" %in% names(w$x))
  ic <- w$x$indifference_curves

  expect_length(ic, n_frames)
  for (frame in ic) {
    # Competitor 0 is the single seat holder every frame.
    expect_length(frame, 1L)
    voter_curves <- frame[[1L]]
    expect_length(voter_curves, n_voters)
    for (pts in voter_curves) {
      # Each IC is 0, 1, or 2 floats.
      expect_lte(length(pts), 2L)
    }
  }
})

test_that("1D IC boundaries are symmetric around the voter ideal", {
  # voter at 0.5, seat at 0.0 → dist = 0.5 → utility = -0.5 under linear loss.
  # level_set_1d(0.5, 1.0, -0.5) = [0.0, 1.0]
  skip_without_lib()
  trace <- competition_run(
    competitor_positions = 0.0,
    strategy_kinds       = "sticker",
    voter_ideals         = 0.5,
    dist_config          = make_dist_config(n_dims = 1L),
    engine_config        = make_competition_engine_config(
      seat_count  = 1L,
      seat_rule   = "plurality_top_k",
      max_rounds  = 1L,
      step_config = make_competition_step_config(kind = "fixed", fixed_step_size = 0.0)
    )
  )
  w <- animate_competition_canvas(
    trace,
    voters         = 0.5,
    compute_ic     = TRUE,
    ic_loss_config = make_loss_config(loss_type = "linear", sensitivity = 1.0),
    ic_dist_config = make_dist_config(n_dims = 1L),
    width          = 900L,
    height         = 400L
  )
  pts <- w$x$indifference_curves[[1L]][[1L]][[1L]]
  sorted_pts <- sort(unlist(pts))
  expect_length(sorted_pts, 2L)
  expect_lt(abs(sorted_pts[1L] - 0.0), 1e-9)
  expect_lt(abs(sorted_pts[2L] - 1.0), 1e-9)
})

# ---------------------------------------------------------------------------
# 1D overlay tests
# ---------------------------------------------------------------------------

test_that("1D centroid and marginal_median overlays are correct", {
  skip_without_lib()
  trace <- .trace_1d()
  w <- animate_competition_canvas(
    trace,
    voters   = .voters_1d,
    overlays = list(centroid = TRUE, marginal_median = TRUE),
    width    = 900L,
    height   = 400L
  )
  ovs <- w$x$overlays_static
  expect_true("centroid" %in% names(ovs))
  expect_true("marginal_median" %in% names(ovs))
  expect_lt(abs(ovs$centroid$x - mean(.voters_1d)),   1e-9)
  expect_lt(abs(ovs$marginal_median$x - median(.voters_1d)), 1e-9)
})

test_that("unsupported 1D overlay emits a warning and is absent", {
  skip_without_lib()
  trace <- .trace_1d()
  expect_warning(
    w <- animate_competition_canvas(
      trace,
      voters   = .voters_1d,
      overlays = list(pareto_set = TRUE),
      width    = 900L,
      height   = 400L
    ),
    regexp = "pareto_set"
  )
  ovs <- w$x[["overlays_static"]]
  if (!is.null(ovs)) expect_false("pareto_set" %in% names(ovs))
})

# ---------------------------------------------------------------------------
# WinSet 1D tests
# ---------------------------------------------------------------------------

test_that("1D winset_intervals_1d absent when compute_winset = FALSE", {
  skip_without_lib()
  trace <- .trace_1d()
  w <- animate_competition_canvas(trace, voters = .voters_1d,
                                  width = 900L, height = 400L,
                                  compute_winset = FALSE)
  expect_null(w$x[["winset_intervals_1d"]])
})

test_that("1D winset_intervals_1d present and correct shape when compute_winset = TRUE", {
  skip_without_lib()
  trace <- .trace_1d()
  w <- animate_competition_canvas(trace, voters = .voters_1d,
                                  width = 900L, height = 400L,
                                  compute_winset = TRUE)
  ws <- w$x[["winset_intervals_1d"]]
  expect_false(is.null(ws))
  # 3 frames (Round 1, Round 2, Final)
  expect_length(ws, 3L)
  # Each frame is a list of seat-holder entries
  for (frame in ws) {
    for (entry in frame) {
      expect_true(all(c("lo", "hi", "competitor_idx") %in% names(entry)))
    }
  }
})

test_that("1D winset lo <= hi when non-empty", {
  skip_without_lib()
  trace <- .trace_1d()
  w <- animate_competition_canvas(trace, voters = .voters_1d,
                                  width = 900L, height = 400L,
                                  compute_winset = TRUE)
  ws <- w$x[["winset_intervals_1d"]]
  for (frame in ws) {
    for (entry in frame) {
      if (!is.na(entry$lo) && !is.na(entry$hi)) {
        expect_lte(entry$lo, entry$hi)
      }
    }
  }
})

# ---------------------------------------------------------------------------
# Error / guard tests
# ---------------------------------------------------------------------------
# Note: the C engine only supports 1D and 2D, so a 3D guard test cannot be
# written without a mock — omitted intentionally.
