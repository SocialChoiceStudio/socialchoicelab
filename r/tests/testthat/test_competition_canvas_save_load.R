# test_competition_canvas_save_load.R — Round-trip tests for save/load_competition_canvas.

# ---------------------------------------------------------------------------
# Fixtures (reuse 1D and 2D trace helpers from sibling test files)
# ---------------------------------------------------------------------------

.voters_sl_1d <- c(-0.5, 0.1, 1.8)

.trace_sl_1d <- function() {
  competition_run(
    competitor_positions = c(0.0, 2.0),
    strategy_kinds       = c("sticker", "sticker"),
    voter_ideals         = .voters_sl_1d,
    dist_config          = make_dist_config(n_dims = 1L),
    engine_config        = make_competition_engine_config(
      seat_count  = 1L,
      seat_rule   = "plurality_top_k",
      max_rounds  = 2L,
      step_config = make_competition_step_config(kind = "fixed", fixed_step_size = 0.5)
    )
  )
}

.voters_sl_2d <- c(-0.5, 0.0,  0.0, 0.2,  1.8, 0.1)  # 3 voters, 2D flat

.trace_sl_2d <- function() {
  competition_run(
    competitor_positions = c(0.0, 0.0,  2.0, 0.0),
    strategy_kinds       = c("sticker", "sticker"),
    voter_ideals         = .voters_sl_2d,
    dist_config          = make_dist_config(n_dims = 2L),
    engine_config        = make_competition_engine_config(
      seat_count  = 1L,
      seat_rule   = "plurality_top_k",
      max_rounds  = 2L,
      step_config = make_competition_step_config(kind = "fixed", fixed_step_size = 0.5)
    )
  )
}

# ---------------------------------------------------------------------------
# 1D round-trip
# ---------------------------------------------------------------------------

test_that("save_competition_canvas writes a valid .scscanvas file (1D)", {
  skip_without_lib()
  trace <- .trace_sl_1d()
  w <- animate_competition_canvas(trace, voters = .voters_sl_1d, width = 900L, height = 400L)
  tmp <- tempfile(fileext = ".scscanvas")
  on.exit(unlink(tmp))

  result <- save_competition_canvas(w, tmp)

  expect_equal(result, tmp)
  expect_true(file.exists(tmp))
  raw <- jsonlite::read_json(tmp, simplifyVector = FALSE)
  expect_equal(raw$format, "scscanvas")
  expect_equal(raw$version, "1")
  expect_true(!is.null(raw$created))
  expect_true(startsWith(raw$generator, "socialchoicelab/r/"))
  expect_equal(raw$width,  900L)
  expect_equal(raw$height, 400L)
  expect_true("payload" %in% names(raw))
})

test_that("load_competition_canvas returns a widget with matching payload (1D)", {
  skip_without_lib()
  trace <- .trace_sl_1d()
  w_orig <- animate_competition_canvas(trace, voters = .voters_sl_1d, width = 900L, height = 400L)
  tmp <- tempfile(fileext = ".scscanvas")
  on.exit(unlink(tmp))
  save_competition_canvas(w_orig, tmp)

  w_loaded <- load_competition_canvas(tmp)

  expect_s3_class(w_loaded, "htmlwidget")
  expect_equal(w_loaded$x$n_dims,       w_orig$x$n_dims)
  expect_equal(w_loaded$x$voters_x,     w_orig$x$voters_x)
  expect_equal(w_loaded$x$rounds,       w_orig$x$rounds)
  expect_equal(w_loaded$x$positions,    w_orig$x$positions)
  expect_equal(w_loaded$x$vote_shares,  w_orig$x$vote_shares)
})

test_that("load_competition_canvas width/height override works (1D)", {
  skip_without_lib()
  trace <- .trace_sl_1d()
  w_orig <- animate_competition_canvas(trace, voters = .voters_sl_1d, width = 900L, height = 400L)
  tmp <- tempfile(fileext = ".scscanvas")
  on.exit(unlink(tmp))
  save_competition_canvas(w_orig, tmp)

  w_loaded <- load_competition_canvas(tmp, width = 1200L, height = 500L)

  expect_equal(w_loaded$width,  1200L)
  expect_equal(w_loaded$height, 500L)
})

# ---------------------------------------------------------------------------
# 2D round-trip
# ---------------------------------------------------------------------------

test_that("save_competition_canvas writes a valid .scscanvas file (2D)", {
  skip_without_lib()
  trace <- .trace_sl_2d()
  w <- animate_competition_canvas(trace, voters = .voters_sl_2d, width = 900L, height = 800L)
  tmp <- tempfile(fileext = ".scscanvas")
  on.exit(unlink(tmp))

  save_competition_canvas(w, tmp)

  raw <- jsonlite::read_json(tmp, simplifyVector = FALSE)
  expect_equal(raw$format, "scscanvas")
  expect_equal(raw$width,  900L)
  expect_equal(raw$height, 800L)
  expect_true("voters_x" %in% names(raw$payload))
  expect_true("ylim"     %in% names(raw$payload))
})

test_that("load_competition_canvas returns a widget with matching payload (2D)", {
  skip_without_lib()
  trace <- .trace_sl_2d()
  w_orig <- animate_competition_canvas(trace, voters = .voters_sl_2d, width = 900L, height = 800L)
  tmp <- tempfile(fileext = ".scscanvas")
  on.exit(unlink(tmp))
  save_competition_canvas(w_orig, tmp)

  w_loaded <- load_competition_canvas(tmp)

  expect_s3_class(w_loaded, "htmlwidget")
  expect_equal(w_loaded$x$voters_x,     w_orig$x$voters_x)
  expect_equal(w_loaded$x$ylim,         w_orig$x$ylim)
  expect_equal(w_loaded$x$positions,    w_orig$x$positions)
  expect_equal(w_loaded$x$vote_shares,  w_orig$x$vote_shares)
})

# ---------------------------------------------------------------------------
# Error cases
# ---------------------------------------------------------------------------

test_that("save_competition_canvas errors on non-widget input", {
  expect_error(
    save_competition_canvas(list(x = 1), tempfile()),
    "must be an htmlwidget"
  )
})

test_that("load_competition_canvas errors on missing file", {
  expect_error(
    load_competition_canvas(tempfile(fileext = ".scscanvas")),
    "file not found"
  )
})

test_that("load_competition_canvas errors on wrong format", {
  skip_without_lib()
  tmp <- tempfile(fileext = ".scscanvas")
  on.exit(unlink(tmp))
  jsonlite::write_json(list(format = "other", payload = list()), tmp, auto_unbox = TRUE)
  expect_error(
    load_competition_canvas(tmp),
    "unexpected format"
  )
})
