# test_competition_canvas_winset.R — Unit tests for animate_competition_canvas WinSet feature.

# Fixture: 3 2D voters, 2 sticker competitors, 1 seat, 2 rounds.
# Competitor 0 at (0,0) wins every round (closer to 2 of 3 voters).
# This gives n_frames = 3 (Round 1, Round 2, Final) and 1 seat holder per frame.
.ws_trace_2d <- function() {
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

WS_VOTERS <- c(-0.5, 0.0, 0.0, 0.2, 1.8, 0.1)  # 3 voters, 2D flat


test_that("compute_winset = FALSE leaves winsets absent from payload", {
  skip_without_lib()
  trace <- .ws_trace_2d()
  w <- animate_competition_canvas(trace, voters = WS_VOTERS)
  expect_false("winsets" %in% names(w$x))
  expect_false("winset_competitor_indices" %in% names(w$x))
})

test_that("compute_winset = TRUE produces correctly shaped winset payload", {
  skip_without_lib()
  trace <- .ws_trace_2d()
  w <- animate_competition_canvas(
    trace,
    voters         = WS_VOTERS,
    compute_winset = TRUE
  )
  d <- trace$dims()
  n_frames <- d$n_rounds + 1L   # 3

  expect_true("overlays_frames" %in% names(w$x))
  expect_false("winsets" %in% names(w$x))
  expect_false("winset_competitor_indices" %in% names(w$x))

  frame_overlays <- w$x$overlays_frames

  expect_length(frame_overlays, n_frames)

  for (f in seq_len(n_frames)) {
    canonical_entries <- frame_overlays[[f]]$winsets
    expect_gte(length(canonical_entries), 1L)
    expect_true(all(vapply(canonical_entries, function(e) {
      e$candidate >= 0L && e$candidate < d$n_competitors
    }, logical(1L))))
  }
})

test_that("compute_winset = TRUE without voters raises an error", {
  skip_without_lib()
  trace <- .ws_trace_2d()
  expect_error(
    animate_competition_canvas(trace, compute_winset = TRUE),
    "requires voters"
  )
})

test_that("compute_winset = TRUE with winset_max exceeded raises an error", {
  skip_without_lib()
  trace <- .ws_trace_2d()
  # 1 seat × 3 frames = 3, which exceeds winset_max = 1.
  expect_error(
    animate_competition_canvas(
      trace,
      voters      = WS_VOTERS,
      compute_winset = TRUE,
      winset_max     = 1L
    ),
    "winset_max"
  )
})

test_that("non-empty winset entry has paths, is_hole, and competitor_idx keys", {
  skip_without_lib()
  trace <- .ws_trace_2d()
  w <- animate_competition_canvas(
    trace,
    voters         = WS_VOTERS,
    compute_winset = TRUE
  )
  ws <- w$x$overlays_frames
  d  <- trace$dims()

  # Find the first non-null entry.
  found <- FALSE
  for (f in seq_along(ws)) {
    for (s_i in seq_along(ws[[f]]$winsets)) {
      entry <- ws[[f]]$winsets[[s_i]]
      if (!is.null(entry)) {
        expect_true("polygons" %in% names(entry))
        expect_true("candidate" %in% names(entry))

        expect_gte(length(entry$polygons), 1L)
        first_ring <- entry$polygons[[1]]$ring
        expect_true(is.list(first_ring))
        expect_gte(length(first_ring), 2L)
        expect_true(all(vapply(first_ring, length, integer(1L)) == 2L))
        expect_true(all(is.numeric(unlist(first_ring))))

        expect_true(all(vapply(entry$polygons, function(poly) "hole" %in% names(poly), logical(1L))))

        expect_gte(entry$candidate, 0L)
        expect_lt(entry$candidate, d$n_competitors)

        found <- TRUE
        break
      }
    }
    if (found) break
  }
  expect_true(found, info = "No non-null winset entry found in payload.")
})

test_that("winset_num_samples affects boundary vertex count", {
  skip_without_lib()
  trace <- .ws_trace_2d()
  w16 <- animate_competition_canvas(
    trace, voters = WS_VOTERS, compute_winset = TRUE,
    winset_num_samples = 16L
  )
  w64 <- animate_competition_canvas(
    trace, voters = WS_VOTERS, compute_winset = TRUE,
    winset_num_samples = 64L
  )

  # Find the first non-null entry in each and compare path vertex counts.
  .first_path_len <- function(frame_payload) {
    for (f in seq_along(frame_payload)) {
      for (s_i in seq_along(frame_payload[[f]]$winsets)) {
        entry <- frame_payload[[f]]$winsets[[s_i]]
        if (!is.null(entry) && length(entry$polygons) > 0L) {
          return(length(entry$polygons[[1L]]$ring))
        }
      }
    }
    NA_integer_
  }

  len16 <- .first_path_len(w16$x$overlays_frames)
  len64 <- .first_path_len(w64$x$overlays_frames)
  expect_false(is.na(len16), info = "No non-null winset path found at num_samples=16.")
  expect_false(is.na(len64), info = "No non-null winset path found at num_samples=64.")
  # Higher num_samples should produce more vertices (or at least not fewer).
  expect_gte(len64, len16)
})
