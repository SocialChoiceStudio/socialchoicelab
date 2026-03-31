# compound_api_interactive_common.R — payload + validation for interactive HTML
# Sourced by compound_api_interactive_demo.R after devtools::load_all.

interactive_expectations_meta <- function() {
  list(
    ic = list(
      healthy = c(
        "Green vertical line at x = 0 is the voter ideal (fixed).",
        "Orange dashed vertical line is the reference point; it moves with the slider.",
        "Cyan horizontal segment on the midline is the 1D indifference interval; it shrinks as the reference approaches the ideal."
      ),
      broken = c(
        "No cyan segment or pts length wrong: ic_interval_1d or JSON embedding failed.",
        "Slider does nothing: DATA.ic.frames missing or empty."
      )
    ),
    winset = list(
      healthy = c(
        "Three gray dots are fixed voters (triangle).",
        "Orange triangle is the status quo moving on a circle around the centroid.",
        "Blue outline (filled with even–odd rule for holes) is the winset boundary from scs_winset_2d_export_boundary."
      ),
      broken = c(
        "No blue region on any frame: all exports empty or path data not serialised.",
        "Jagged or self-crossing-only shapes: sample count / hole flags may be wrong."
      )
    ),
    uncovered = list(
      healthy = c(
        "Four gray dots are voters (square).",
        "Green closed loop is the approximate uncovered boundary; it should grow smoother as grid resolution increases."
      ),
      broken = c(
        "No green loop but grid label changes: boundary array empty or xy not interleaved x,y.",
        "Nothing moves with the slider: uncovered.frames missing."
      )
    ),
    voronoi = list(
      healthy = c(
        "Three white dots are Voronoi sites.",
        "Colored polygons are clipped Voronoi cells; changing scale zooms the bbox."
      ),
      broken = c(
        "Missing cells or null cell: heap export or JSON mismatch.",
        "Sites drawn but no polygons: cell coordinates empty."
      )
    )
  )
}

winset_export_frame <- function(sq_x, sq_y, voters, dc) {
  b <- .Call(
    "r_scs_winset_2d_export_boundary",
    as.double(sq_x), as.double(sq_y), as.double(voters),
    -1L, 64L, dc,
    PACKAGE = "socialchoicelab"
  )
  if (isTRUE(b$empty)) {
    return(list(empty = TRUE, sq = c(sq_x, sq_y)))
  }
  nh <- as.integer(b$is_hole)
  plist <- vector("list", length(b$paths))
  for (i in seq_along(b$paths)) {
    plist[[i]] <- list(xy = as.numeric(b$paths[[i]]), hole = nh[i])
  }
  list(empty = FALSE, sq = c(sq_x, sq_y), paths = plist)
}

vor_cells_flat <- function(cells) {
  lapply(seq_along(cells), function(i) {
    c <- cells[[i]]
    if (is.null(c)) return(NULL)
    as.numeric(c$paths[[1L]])
  })
}

build_interactive_payload <- function() {
  lc <- make_loss_config(loss_type = "linear", sensitivity = 1)
  dc1 <- make_dist_config(n_dims = 1L, weights = 1)
  ideal_1d <- 0.0
  refs <- seq(-2.4, 2.4, length.out = 97L)
  ic_frames <- lapply(refs, function(ref) {
    pts <- ic_interval_1d(ideal_1d, ref, lc, dc1)
    list(ref = as.double(ref), pts = as.numeric(pts))
  })

  voters_w <- c(0, 0, 2, 0, 1, 2)
  cx <- 1.0
  cy <- 2 / 3
  rad <- 0.82
  # Match python: np.linspace(0, 2*pi, 49)[:-1] -> 48 orbit samples
  angles <- seq(0, 2 * pi, length.out = 49L)
  angles <- angles[-length(angles)]
  dc2 <- make_dist_config(weights = c(1, 1))
  winset_frames <- lapply(angles, function(th) {
    sx <- cx + rad * cos(th)
    sy <- cy + rad * sin(th)
    fr <- winset_export_frame(sx, sy, voters_w, dc2)
    fr$angle_deg <- as.double(th * 180 / pi)
    fr
  })

  voters_u <- c(-1, 0, 0, 1, 1, 0, 0, -1)
  uncov_frames <- lapply(4:18, function(g) {
    bnd <- uncovered_set_boundary_2d(voters_u, grid_resolution = g, k = "simple")
    list(grid = as.integer(g), xy = as.numeric(t(bnd)))
  })

  sites_v <- c(0, 0, 1, 0, 0.5, 1)
  cx_v <- 0.5
  cy_v <- 1 / 3
  hw <- 1.15
  scales <- seq(0.65, 1.35, length.out = 36L)
  vor_frames <- lapply(scales, function(s) {
    h <- hw * s
    bbox <- c(cx_v - h, cy_v - h, cx_v + h, cy_v + h)
    cells <- .Call(
      "r_scs_voronoi_cells_2d",
      as.double(sites_v), 3L, as.double(bbox),
      PACKAGE = "socialchoicelab"
    )
    list(scale = as.double(s), bbox = as.numeric(bbox), cells = vor_cells_flat(cells))
  })

  payload <- list(
    ic = list(ideal = ideal_1d, frames = ic_frames),
    winset = list(voters = as.numeric(voters_w), frames = winset_frames),
    uncovered = list(voters = as.numeric(voters_u), frames = uncov_frames),
    voronoi = list(sites = as.numeric(sites_v), frames = vor_frames)
  )
  vc <- validate_interactive_payload(payload)
  payload$meta <- list(
    binding = "r",
    structure_version = 1L,
    expectations = interactive_expectations_meta(),
    build_checks = vc$checks,
    build_all_ok = vc$all_ok
  )
  payload
}

validate_interactive_payload <- function(p) {
  out <- list()
  ok_all <- TRUE
  push <- function(scenario, id, pass, good, bad) {
    out <<- c(out, list(list(
      scenario = scenario, id = id, ok = isTRUE(pass),
      message = if (isTRUE(pass)) good else bad
    )))
    if (!isTRUE(pass)) ok_all <<- FALSE
  }

  ic <- p$ic
  if (is.null(ic) || is.null(ic$frames)) {
    ok_all <- FALSE
    out <- c(out, list(list(scenario = "ic", id = "structure", ok = FALSE, message = "missing ic.frames")))
  } else {
    frames <- ic$frames
    push("ic", "frame_count", length(frames) == 97L,
         "97 reference frames (slider 0–96).",
         sprintf("expected 97 ic frames, got %d", length(frames)))
    if (length(frames) > 0L) {
      r0 <- frames[[1L]]$ref
      r1 <- frames[[length(frames)]]$ref
      push("ic", "ref_range", !is.null(r0) && !is.null(r1) && r0 < r1,
           sprintf("ref from %s to %s.", format(r0, digits = 6), format(r1, digits = 6)),
           "ic ref range invalid")
    }
    ic_pts_ok <- TRUE
    for (i in seq_along(frames)) {
      fr <- frames[[i]]
      pts <- fr$pts
      if (is.null(pts)) {
        ok_all <- FALSE
        ic_pts_ok <- FALSE
        out <- c(out, list(list(scenario = "ic", id = sprintf("pts_%d", i - 1L), ok = FALSE,
                                message = sprintf("frame %d pts missing", i - 1L))))
        break
      }
      pv <- as.numeric(pts)
      if (length(pv) >= 2L && pv[[1L]] > pv[[2L]]) {
        ok_all <- FALSE
        ic_pts_ok <- FALSE
        out <- c(out, list(list(scenario = "ic", id = "order", ok = FALSE,
                                message = sprintf("frame %d: pts not sorted left≤right", i - 1L))))
        break
      }
    }
    if (isTRUE(ic_pts_ok) && length(frames) > 0L) {
      push("ic", "pts_shape", TRUE,
           "Each frame has pts; when length≥2 endpoints satisfy left≤right.",
           "")
    }
  }

  ws <- p$winset
  if (is.null(ws) || is.null(ws$frames)) {
    ok_all <- FALSE
    out <- c(out, list(list(scenario = "winset", id = "structure", ok = FALSE, message = "missing winset.frames")))
  } else {
    wf <- ws$frames
    push("winset", "frame_count", length(wf) == 48L,
         "48 orbit frames (one per angle step).",
         sprintf("expected 48 winset frames, got %d", length(wf)))
    nonempty <- sum(vapply(wf, function(fr) {
      isTRUE(!isTRUE(fr$empty)) && length(fr$paths) > 0L
    }, logical(1L)))
    push("winset", "nonempty_exports", nonempty > 0L,
         sprintf("%d frames with non-empty boundary paths.", nonempty),
         "all winset frames empty — export or geometry failure")
    ang_ok <- TRUE
    for (i in seq_along(wf)) {
      if (is.null(wf[[i]]$angle_deg)) {
        ok_all <- FALSE
        ang_ok <- FALSE
        out <- c(out, list(list(scenario = "winset", id = "angle", ok = FALSE,
                                message = sprintf("frame %d missing angle_deg", i - 1L))))
        break
      }
    }
  }

  unc <- p$uncovered
  if (is.null(unc) || is.null(unc$frames)) {
    ok_all <- FALSE
    out <- c(out, list(list(scenario = "uncovered", id = "structure", ok = FALSE, message = "missing uncovered.frames")))
  } else {
    uf <- unc$frames
    grids <- vapply(uf, function(fr) fr$grid, integer(1L))
    push("uncovered", "frame_count", length(uf) == 15L,
         "15 frames (grid 4…18).",
         sprintf("expected 15 uncovered frames, got %d", length(uf)))
    push("uncovered", "grid_sequence", identical(as.integer(grids), 4:18),
         "grid runs 4,5,…,18.",
         sprintf("grid sequence wrong: %s", paste(grids, collapse = ",")))
    bad_xy <- NULL
    for (i in seq_along(uf)) {
      xy <- uf[[i]]$xy
      if (length(xy) %% 2L != 0L) {
        bad_xy <- i - 1L
        break
      }
    }
    push("uncovered", "xy_pairs", is.null(bad_xy),
         "uncovered xy arrays have even length (x,y pairs).",
         sprintf("frame %d xy length not even", bad_xy))
  }

  vo <- p$voronoi
  if (is.null(vo) || is.null(vo$frames)) {
    ok_all <- FALSE
    out <- c(out, list(list(scenario = "voronoi", id = "structure", ok = FALSE, message = "missing voronoi.frames")))
  } else {
    vf <- vo$frames
    push("voronoi", "frame_count", length(vf) == 36L,
         "36 bbox scale steps.",
         sprintf("expected 36 voronoi frames, got %d", length(vf)))
    bad_cells <- NULL
    for (i in seq_along(vf)) {
      fr <- vf[[i]]
      cells <- fr$cells
      if (!is.list(cells) || length(cells) != 3L) {
        bad_cells <- list(i - 1L, "cell count")
        break
      }
      for (j in seq_along(cells)) {
        cj <- cells[[j]]
        if (is.null(cj) || length(as.numeric(cj)) < 4L) {
          bad_cells <- list(i - 1L, sprintf("cell %d empty or too short", j - 1L))
          break
        }
      }
      if (!is.null(bad_cells)) break
    }
    msg_bad <- if (is.null(bad_cells)) "" else sprintf("frame %d: %s", bad_cells[[1L]], bad_cells[[2L]])
    push("voronoi", "cells", is.null(bad_cells),
         "Each frame has 3 non-null cell polygons (≥2 vertices).",
         msg_bad)
  }

  list(checks = out, all_ok = ok_all)
}

format_interactive_validation_report <- function(vc) {
  lines <- c("compound_api_interactive — data checks")
  for (c in vc$checks) {
    tag <- if (isTRUE(c$ok)) "PASS" else "FAIL"
    lines <- c(lines, sprintf("  [%s] %s/%s: %s", tag, c$scenario, c$id, c$message))
  }
  suf <- if (isTRUE(vc$all_ok)) "all checks passed" else "one or more checks failed — see above; page may still render for debugging"
  c(lines, sprintf("  SUMMARY: %s", suf))
}
