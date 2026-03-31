# test_competition_canvas_voronoi.R — Unit tests for animate_competition_canvas Voronoi feature.

# Reuse the same 2D trace fixture as winset tests.
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

test_that("compute_voronoi = FALSE leaves voronoi_cells absent from payload", {
  skip_without_lib()
  trace <- .ws_trace_2d()
  w <- animate_competition_canvas(trace)
  expect_false("voronoi_cells" %in% names(w$x))
})

test_that("compute_voronoi = TRUE produces correctly shaped voronoi payload", {
  skip_without_lib()
  trace <- .ws_trace_2d()
  w <- animate_competition_canvas(trace, compute_voronoi = TRUE)
  d <- trace$dims()
  n_frames   <- d$n_rounds + 1L
  n_comp     <- d$n_competitors

  expect_true("overlays_frames" %in% names(w$x))
  expect_false("voronoi_cells" %in% names(w$x))
  frame_overlays <- w$x$overlays_frames
  expect_length(frame_overlays, n_frames)

  for (f in seq_len(n_frames)) {
    canonical_entries <- frame_overlays[[f]]$candidate_regions
    expect_length(canonical_entries, n_comp)
    for (entry in canonical_entries) {
      expect_true("polygons" %in% names(entry))
      expect_true("candidate" %in% names(entry))
      expect_gte(length(entry$polygons), 1L)
      expect_gte(entry$candidate, 0L)
      expect_lt(entry$candidate, n_comp)
    }
  }
})

test_that("compute_voronoi = TRUE with non-Euclidean voronoi_dist_config raises", {
  skip_without_lib()
  trace <- .ws_trace_2d()
  expect_error(
    animate_competition_canvas(
      trace,
      compute_voronoi     = TRUE,
      voronoi_dist_config = make_dist_config(distance_type = "manhattan")
    ),
    "Euclidean"
  )
  expect_error(
    animate_competition_canvas(
      trace,
      compute_voronoi     = TRUE,
      voronoi_dist_config = make_dist_config(weights = c(2, 1))
    ),
    "Euclidean"
  )
})
