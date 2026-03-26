# competition_canvas.R — Canvas-based competition trajectory animation widget
#
# Data is stored once and frames are drawn on demand in the browser, so it
# scales to long runs. Returns an htmlwidget.

#' @importFrom htmlwidgets createWidget sizingPolicy
NULL

.title_case <- function(s) {
  paste0(toupper(substring(s, 1L, 1L)), substring(s, 2L))
}

.names_from_strategies <- function(kinds) {
  tbl <- table(kinds)
  labels <- character(length(kinds))
  counters <- list()
  for (i in seq_along(kinds)) {
    k <- kinds[i]
    nice <- .title_case(k)
    if (tbl[[k]] == 1L) {
      labels[i] <- nice
    } else {
      idx <- counters[[k]]
      if (is.null(idx)) idx <- 0L
      counters[[k]] <- idx + 1L
      labels[i] <- paste0(nice, " ", LETTERS[counters[[k]]])
    }
  }
  labels
}

.annotate_names_with_strategies <- function(names, kinds) {
  vapply(seq_along(names), function(i) {
    paste0(names[i], " (", .title_case(kinds[i]), ")")
  }, character(1))
}

# Serialise a named list of overlay objects to the overlays_static JSON format.
# Point overlays (centroid, marginal_median, geometric_mean): list(x, y).
# Polygon overlays (pareto_set): convex_hull_2d matrix → list of [x,y] pairs.
.serialise_overlays_static <- function(overlays) {
  if (is.null(overlays) || length(overlays) == 0L) return(NULL)
  point_keys   <- c("centroid", "marginal_median", "geometric_mean")
  polygon_keys <- c("pareto_set")
  result <- list()
  for (nm in names(overlays)) {
    obj <- overlays[[nm]]
    if (nm %in% point_keys) {
      if (!is.list(obj) || is.null(obj$x) || is.null(obj$y)) {
        stop("Overlay '", nm, "' must be a list(x, y) from the corresponding _2d() function.")
      }
      result[[nm]] <- list(x = unname(obj$x), y = unname(obj$y))
    } else if (nm %in% polygon_keys) {
      if (!is.matrix(obj) || ncol(obj) != 2L) {
        stop("Overlay '", nm, "' must be a 2-column matrix from convex_hull_2d().")
      }
      result[[nm]] <- list(
        polygon = unname(lapply(seq_len(nrow(obj)), function(i) {
          list(unname(obj[i, 1L]), unname(obj[i, 2L]))
        }))
      )
    } else {
      stop("Unknown overlay key: '", nm, "'. ",
           "Valid keys: ", paste(c(point_keys, polygon_keys), collapse = ", "), ".")
    }
  }
  result
}

# Internal helper: build the 1D canvas payload and return an htmlwidget.
.animate_competition_canvas_1d <- function(
    trace, d, competitor_names, voters, title, theme,
    width, height, trail, trail_len, dim_names, overlays,
    compute_ic, ic_loss_config, ic_dist_config, ic_num_samples, ic_max_curves,
    compute_winset = FALSE
) {
  n_rounds     <- d$n_rounds
  n_competitors <- d$n_competitors

  positions <- lapply(seq_len(n_rounds), function(r) trace$round_positions(r))
  positions[[n_rounds + 1L]] <- trace$final_positions()
  frame_names <- c(paste("Round", seq_len(n_rounds)), "Final")

  vote_shares_list <- lapply(seq_len(n_rounds), function(r) {
    unname(as.list(trace$round_vote_shares(r)))
  })
  vote_shares_list[[n_rounds + 1L]] <- unname(as.list(trace$final_vote_shares()))

  # Voters — 1D flat numeric vector (each element is one voter's ideal point)
  voters_x <- unname(as.double(voters))

  all_x <- c(voters_x, unlist(lapply(positions, function(p) p[, 1L])))
  xlim  <- .range_with_origin(all_x)

  # 1D candidate colours use a green-free palette (voter dots are lime green).
  colors      <- .candidate_colors_1d(n_competitors, alpha = 0.95)
  voter_color <- .voter_point_color(theme)

  # 1D overlays — centroid (mean) and marginal median; others skipped with a warning
  overlays_out <- list()
  valid_1d <- c("centroid", "marginal_median")
  if (!is.null(overlays) && length(overlays) > 0L) {
    for (nm in names(overlays)) {
      if (nm == "centroid") {
        overlays_out[["centroid"]] <- list(x = unname(mean(voters_x)))
      } else if (nm == "marginal_median") {
        overlays_out[["marginal_median"]] <- list(x = unname(stats::median(voters_x)))
      } else {
        warning(
          "Overlay '", nm, "' is not supported for 1D traces and will be skipped. ",
          "Valid 1D overlays: ", paste(valid_1d, collapse = ", "), ".",
          call. = FALSE
        )
      }
    }
  }
  overlays_serialised <- if (length(overlays_out) > 0L) overlays_out else NULL

  # 1D IC computation
  ic_data               <- NULL
  ic_competitor_indices <- NULL
  if (isTRUE(compute_ic)) {
    n_voters <- length(voters_x)
    if (n_voters == 0L) stop("compute_ic = TRUE requires voters to be provided.")
    if (is.null(ic_loss_config)) ic_loss_config <- make_loss_config()
    if (is.null(ic_dist_config)) ic_dist_config <- make_dist_config(n_dims = 1L)
    ic_num_samples <- as.integer(ic_num_samples)
    if (ic_num_samples < 3L) stop("ic_num_samples must be >= 3, got ", ic_num_samples, ".")

    # Extract salience weight for level_set_1d (R dist config uses $weights)
    w1d <- if (!is.null(ic_dist_config$weights) && length(ic_dist_config$weights) >= 1L) {
      as.double(ic_dist_config$weights[1L])
    } else {
      1.0
    }

    n_frames <- n_rounds + 1L
    seat_idxs_per_frame <- vector("list", n_frames)
    for (r in seq_len(n_rounds)) {
      st <- trace$round_seat_totals(r)
      seat_idxs_per_frame[[r]] <- which(st > 0L)
    }
    final_ss <- trace$final_seat_shares()
    seat_idxs_per_frame[[n_frames]] <- which(final_ss > 0)
    # No ic_max_curves guard for 1D: each IC is just two numbers (lo, hi),
    # so the computation is negligible regardless of voter/frame count.

    ic_data               <- vector("list", n_frames)
    ic_competitor_indices <- vector("list", n_frames)
    for (frame_i in seq_len(n_frames)) {
      seat_idxs    <- seat_idxs_per_frame[[frame_i]]
      pos_mat      <- positions[[frame_i]]
      frame_curves <- vector("list", length(seat_idxs))
      frame_cidxs  <- integer(length(seat_idxs))
      for (s_i in seq_along(seat_idxs)) {
        s        <- unname(seat_idxs[s_i])
        seat_x   <- unname(pos_mat[s, 1L])
        frame_cidxs[s_i] <- s - 1L
        voter_curves <- vector("list", n_voters)
        for (v in seq_len(n_voters)) {
          voter_x_v <- voters_x[v]
          dist_val   <- calculate_distance(voter_x_v, seat_x, ic_dist_config)
          util_level <- distance_to_utility(dist_val, ic_loss_config)
          pts        <- level_set_1d(voter_x_v, w1d, util_level, ic_loss_config)
          voter_curves[[v]] <- as.list(unname(pts))
        }
        frame_curves[[s_i]] <- voter_curves
      }
      ic_data[[frame_i]]               <- frame_curves
      ic_competitor_indices[[frame_i]] <- as.list(frame_cidxs)
    }
  }

  # Seat-holder indices (unconditional, used by JS stats block)
  n_frames_all <- n_rounds + 1L
  seat_holder_indices_list <- vector("list", n_frames_all)
  for (r in seq_len(n_rounds)) {
    st <- trace$round_seat_totals(r)
    seat_holder_indices_list[[r]] <- as.list(unname(which(st > 0L) - 1L))
  }
  final_ss_all <- trace$final_seat_shares()
  seat_holder_indices_list[[n_frames_all]] <- as.list(unname(which(final_ss_all > 0) - 1L))

  dim_name_1d <- if (length(dim_names) >= 1L) as.character(dim_names[1L]) else "Dimension 1"

  payload <- list(
    n_dims            = 1L,
    voters_x          = unname(as.list(voters_x)),
    voter_color       = voter_color,
    xlim              = unname(as.list(xlim)),
    dim_names         = list(dim_name_1d),
    rounds            = unname(as.list(frame_names)),
    positions         = unname(lapply(positions, function(m) {
      unname(lapply(seq_len(nrow(m)), function(i) list(unname(m[i, 1L]))))
    })),
    competitor_names  = unname(as.list(competitor_names)),
    competitor_colors = unname(as.list(colors)),
    vote_shares       = unname(vote_shares_list),
    trail             = trail,
    trail_length      = trail_len,
    title             = title,
    seat_holder_indices = seat_holder_indices_list
  )
  if (!is.null(overlays_serialised)) {
    payload$overlays_static <- overlays_serialised
  }
  if (!is.null(ic_data)) {
    payload$indifference_curves   <- ic_data
    payload$ic_competitor_indices <- ic_competitor_indices
  }
  # WinSet intervals per frame (Euclidean 1D, exact O(n^2) sweep)
  if (isTRUE(compute_winset)) {
    if (length(voters_x) == 0L)
      stop("compute_winset = TRUE requires voters to be provided.")
    voters_x_dbl <- as.double(voters_x)
    n_frames_ws  <- n_rounds + 1L
    ws_data <- vector("list", n_frames_ws)
    for (frame_i in seq_len(n_frames_ws)) {
      seat_idxs <- if (frame_i <= n_rounds) {
        which(trace$round_seat_totals(frame_i) > 0L)
      } else {
        which(trace$final_seat_shares() > 0)
      }
      pos_mat <- positions[[frame_i]]
      frame_ws <- vector("list", length(seat_idxs))
      for (s_i in seq_along(seat_idxs)) {
        s      <- unname(seat_idxs[s_i])
        seat_x <- unname(pos_mat[s, 1L])
        iv     <- .Call("r_scs_winset_interval_1d", voters_x_dbl, as.double(seat_x))
        frame_ws[[s_i]] <- list(
          lo             = unname(iv[1L]),
          hi             = unname(iv[2L]),
          competitor_idx = s - 1L
        )
      }
      ws_data[[frame_i]] <- frame_ws
    }
    payload$winset_intervals_1d <- ws_data
  }

  htmlwidgets::createWidget(
    name = "competition_canvas",
    x = payload,
    width = width,
    height = height,
    package = "socialchoicelab",
    sizingPolicy = htmlwidgets::sizingPolicy(
      viewer.defaultWidth  = 800L,
      viewer.defaultHeight = 400L,
      viewer.fill = FALSE,
      browser.defaultWidth = 900L,
      browser.defaultHeight = 400L,
      browser.fill = FALSE,
      padding = 0
    )
  )
}

#' Animate 2D competition trajectories (canvas backend)
#'
#' Renders competition trajectories using a canvas-based \code{htmlwidget}.
#' Data is sent once; the browser draws frames on demand. This scales to long
#' runs (hundreds or thousands of rounds) without large HTML payloads.
#'
#' @param trace A \code{\link{CompetitionTrace}} object.
#' @param voters Optional flat numeric voter vector or \code{numeric(0)}.
#' @param competitor_names Optional character labels for competitors.
#' @param title Plot title (used for layout; currently not drawn on canvas).
#' @param theme Colour theme (same options as \code{\link{plot_spatial_voting}}).
#' @param width,height Widget dimensions in pixels.
#' @param trail Trail style: \code{"fade"}, \code{"full"}, or \code{"none"}.
#' @param trail_length When \code{trail = "fade"}, either \code{"short"},
#'   \code{"medium"}, \code{"long"}, or a positive integer.
#' @param dim_names Length-2 character vector for axis titles.
#' @param overlays Optional named list of static overlays to draw on every
#'   frame. Each element is the result of a geometry function:
#'   \describe{
#'     \item{\code{centroid}}{From \code{\link{centroid_2d}}: \code{list(x,y)}.}
#'     \item{\code{marginal_median}}{From \code{\link{marginal_median_2d}}: \code{list(x,y)}.}
#'     \item{\code{geometric_mean}}{From \code{\link{geometric_mean_2d}}: \code{list(x,y)}.
#'       Requires all voter coordinates to be strictly positive.}
#'     \item{\code{pareto_set}}{From \code{\link{convex_hull_2d}}: matrix of hull vertices.
#'       Valid only under Euclidean distance with uniform salience.}
#'   }
#' @param compute_ic Logical. When \code{TRUE}, compute per-frame indifference
#'   curves for every voter through each seat holder's position and bundle them
#'   into the widget payload. A toggle is then shown in the player UI. Default
#'   \code{FALSE}. Requires \code{voters} to be provided.
#' @param ic_loss_config Loss configuration for IC computation (from
#'   \code{\link{make_loss_config}}). Default \code{NULL} uses linear loss with
#'   sensitivity 1, producing pure iso-distance curves that match competition
#'   vote logic.
#' @param ic_dist_config Distance configuration for IC computation (from
#'   \code{\link{make_dist_config}}). Should match the \code{dist_config} used
#'   in \code{\link{competition_run}}. Default \code{NULL} uses 2D Euclidean.
#' @param ic_num_samples Integer >= 3. Number of polygon vertices sampled per
#'   indifference curve for smooth shapes (circle / ellipse / superellipse).
#'   Exact polygons always use 4 vertices regardless of this value. Default 32.
#' @param ic_max_curves Maximum total indifference curves allowed across all
#'   frames (\code{n_voters × seats_per_frame × n_frames}). Exceeding this
#'   raises an error. Default 2000.
#' @param compute_winset Logical. When \code{TRUE}, compute per-frame WinSet
#'   boundaries for each seat holder's current position and bundle them into
#'   the widget payload. A toggle is then shown in the player UI. Default
#'   \code{FALSE}. Requires \code{voters} to be provided. The WinSet is
#'   computed using the configured voting rule (\code{winset_k}) and distance
#'   metric (\code{winset_dist_config}); no assumption is made about which
#'   rule is in use.
#' @param winset_dist_config Distance configuration for WinSet computation
#'   (from \code{\link{make_dist_config}}). Should match the
#'   \code{dist_config} used in \code{\link{competition_run}}. Default
#'   \code{NULL} uses 2D Euclidean.
#' @param winset_k Voting rule threshold passed to \code{\link{winset_2d}}.
#'   Use \code{"simple"} for simple majority, or a positive integer \code{k}.
#'   Default \code{"simple"}.
#' @param winset_num_samples Integer >= 4. Boundary approximation quality
#'   (number of sample points on the WinSet boundary). Default \code{64L}.
#' @param winset_max Maximum total WinSet computations allowed
#'   (\code{seats_per_frame × n_frames}). Exceeding this raises an error.
#'   Default \code{1000L}.
#' @param compute_voronoi Logical. When \code{TRUE}, compute Euclidean Voronoi
#'   (candidate) regions per frame and show a "Voronoi" toggle in the canvas.
#'   Currently only Euclidean (L2, uniform weights) is supported; a
#'   non-Euclidean \code{voronoi_dist_config} raises an error. Default
#'   \code{FALSE}.
#' @param voronoi_dist_config Distance configuration for Voronoi (must be
#'   Euclidean when \code{compute_voronoi} is \code{TRUE}). Default
#'   \code{NULL} uses 2D Euclidean. If provided, must be Euclidean with
#'   uniform salience; otherwise an error is raised.
#' @return An \code{htmlwidget} object of class \code{competition_canvas}.
#' @export
animate_competition_canvas <- function(trace,
                                      voters = numeric(0),
                                      competitor_names = NULL,
                                      title = "Competition Trajectories",
                                      theme = "dark2",
                                      width = NULL,
                                      height = NULL,
                                      trail = c("fade", "full", "none"),
                                      trail_length = "medium",
                                      dim_names = c("Dimension 1", "Dimension 2"),
                                      overlays = NULL,
                                      compute_ic = FALSE,
                                      ic_loss_config = NULL,
                                      ic_dist_config = NULL,
                                      ic_num_samples = 32L,
                                      ic_max_curves  = 5000L,
                                      compute_winset = FALSE,
                                      winset_dist_config = NULL,
                                      winset_k           = "simple",
                                      winset_num_samples = 64L,
                                      winset_max         = 1000L,
                                      compute_voronoi    = FALSE,
                                      voronoi_dist_config = NULL) {
  if (!inherits(trace, "CompetitionTrace")) {
    stop("Expected a CompetitionTrace object, got ", class(trace)[1], ".")
  }
  d <- trace$dims()

  # Validate common parameters before dispatching on dimensionality.
  strategy_kinds <- trace$strategy_kinds()
  if (is.null(competitor_names)) {
    competitor_names <- .names_from_strategies(strategy_kinds)
  } else {
    if (length(competitor_names) != d$n_competitors) {
      stop("competitor_names length must match n_competitors.")
    }
    competitor_names <- .annotate_names_with_strategies(
      competitor_names, strategy_kinds
    )
  }
  trail <- match.arg(trail)
  if (!is.character(trail_length) && !is.numeric(trail_length)) {
    stop('trail_length must be an integer or one of "short", "medium", "long".')
  }
  if (is.character(trail_length) && !trail_length %in% c("short", "medium", "long")) {
    stop('trail_length "', trail_length, '" is not recognised. ',
         'Use "short" (1/3 rounds), "medium" (1/2 rounds), "long" (3/4 rounds), or a positive integer.')
  }

  n_seg <- max(d$n_rounds, 0L)
  trail_len <- if (is.character(trail_length)) {
    switch(trail_length,
      short  = max(1L, floor(n_seg / 3L)),
      medium = max(1L, floor(n_seg / 2L)),
      long   = max(1L, floor(n_seg * 3L / 4L))
    )
  } else {
    tl <- as.integer(trail_length)
    if (tl < 1L) stop("trail_length must be >= 1, got ", tl, ".")
    tl
  }

  if (d$n_dims == 1L) {
    return(.animate_competition_canvas_1d(
      trace, d, competitor_names, voters, title, theme,
      width, height, trail, trail_len, dim_names, overlays,
      compute_ic, ic_loss_config, ic_dist_config, ic_num_samples, ic_max_curves,
      compute_winset = compute_winset
    ))
  }
  if (d$n_dims != 2L) {
    stop("animate_competition_canvas supports 1D and 2D traces, got n_dims = ",
         d$n_dims, ".")
  }

  if (isTRUE(compute_voronoi)) {
    vdc <- voronoi_dist_config %||% make_dist_config()
    type_ok <- identical(vdc$distance_type, 0L) ||
               identical(vdc$distance_type, "euclidean")
    weights <- as.numeric(vdc$weights)
    uniform_ok <- length(weights) >= 2L && length(unique(weights)) == 1L
    if (!type_ok || !uniform_ok) {
      stop(
        "compute_voronoi = TRUE requires Euclidean (L2) distance with uniform ",
        "salience. Voronoi for non-Euclidean metrics is planned; use ",
        "voronoi_dist_config = NULL or make_dist_config() for Euclidean."
      )
    }
  }

  vxy <- .flat_to_xy(voters)
  positions <- lapply(seq_len(d$n_rounds), function(r) trace$round_positions(r))
  positions[[d$n_rounds + 1L]] <- trace$final_positions()
  frame_names <- c(paste("Round", seq_len(d$n_rounds)), "Final")

  vote_shares_list <- lapply(seq_len(d$n_rounds), function(r) {
    unname(as.list(trace$round_vote_shares(r)))
  })
  vote_shares_list[[d$n_rounds + 1L]] <- unname(as.list(trace$final_vote_shares()))

  all_x <- c(vxy$x, unlist(lapply(positions, function(p) p[, 1L])))
  all_y <- c(vxy$y, unlist(lapply(positions, function(p) p[, 2L])))
  xlim <- .range_with_origin(all_x)
  ylim <- .range_with_origin(all_y)

  colors <- scl_palette(theme, d$n_competitors, alpha = 0.95)
  voter_color <- .voter_point_color(theme)

  overlays_serialised <- .serialise_overlays_static(overlays)

  # ── Indifference-curve computation ──────────────────────────────────────────
  ic_data               <- NULL
  ic_competitor_indices <- NULL
  if (isTRUE(compute_ic)) {
    n_voters <- length(vxy$x)
    if (n_voters == 0L) {
      stop("compute_ic = TRUE requires voters to be provided.")
    }
    if (is.null(ic_loss_config)) ic_loss_config <- make_loss_config()
    if (is.null(ic_dist_config)) ic_dist_config <- make_dist_config()
    ic_num_samples <- as.integer(ic_num_samples)
    ic_max_curves  <- as.integer(ic_max_curves)
    if (ic_num_samples < 3L) {
      stop("ic_num_samples must be >= 3, got ", ic_num_samples, ".")
    }

    n_frames <- d$n_rounds + 1L
    seat_idxs_per_frame <- vector("list", n_frames)
    for (r in seq_len(d$n_rounds)) {
      st <- trace$round_seat_totals(r)
      seat_idxs_per_frame[[r]] <- which(st > 0L)
    }
    final_ss <- trace$final_seat_shares()
    seat_idxs_per_frame[[n_frames]] <- which(final_ss > 0)

    total_ics <- sum(sapply(seat_idxs_per_frame, function(s) n_voters * length(s)))
    if (total_ics > ic_max_curves) {
      n_seats_max <- max(sapply(seat_idxs_per_frame, length))
      stop(
        "compute_ic: total IC count (", total_ics, ") exceeds ic_max_curves (",
        ic_max_curves, "). Got n_voters=", n_voters, ", max seats per frame=",
        n_seats_max, ", n_frames=", n_frames, ". ",
        "Reduce the number of voters, seats, or rounds, or increase ic_max_curves."
      )
    }

    ic_data               <- vector("list", n_frames)
    ic_competitor_indices <- vector("list", n_frames)
    for (frame_i in seq_len(n_frames)) {
      seat_idxs    <- seat_idxs_per_frame[[frame_i]]
      pos_mat      <- positions[[frame_i]]
      frame_curves <- vector("list", length(seat_idxs))
      frame_cidxs  <- integer(length(seat_idxs))
      for (s_i in seq_along(seat_idxs)) {
        s        <- unname(seat_idxs[s_i])
        seat_pos <- unname(pos_mat[s, ])
        frame_cidxs[s_i] <- s - 1L
        voter_curves <- vector("list", n_voters)
        for (v in seq_len(n_voters)) {
          poly_mat <- ic_polygon_2d(
            vxy$x[v], vxy$y[v], seat_pos[1L], seat_pos[2L],
            ic_loss_config, ic_dist_config, ic_num_samples
          )
          voter_curves[[v]] <- as.list(as.vector(t(poly_mat)))
        }
        frame_curves[[s_i]] <- voter_curves
      }
      ic_data[[frame_i]]               <- frame_curves
      ic_competitor_indices[[frame_i]] <- as.list(frame_cidxs)
    }
  }

  # ── WinSet computation ───────────────────────────────────────────────────────
  ws_data               <- NULL
  ws_competitor_indices <- NULL
  if (isTRUE(compute_winset)) {
    n_voters <- length(vxy$x)
    if (n_voters == 0L) {
      stop("compute_winset = TRUE requires voters to be provided.")
    }
    if (is.null(winset_dist_config)) winset_dist_config <- make_dist_config()
    winset_num_samples <- as.integer(winset_num_samples)
    winset_max         <- as.integer(winset_max)
    if (winset_num_samples < 4L) {
      stop("winset_num_samples must be >= 4, got ", winset_num_samples, ".")
    }

    n_frames <- d$n_rounds + 1L
    seat_idxs_per_frame <- vector("list", n_frames)
    for (r in seq_len(d$n_rounds)) {
      st <- trace$round_seat_totals(r)
      seat_idxs_per_frame[[r]] <- which(st > 0L)
    }
    final_ss <- trace$final_seat_shares()
    seat_idxs_per_frame[[n_frames]] <- which(final_ss > 0)

    total_winsets <- sum(sapply(seat_idxs_per_frame, length))
    if (total_winsets > winset_max) {
      n_seats_max <- max(sapply(seat_idxs_per_frame, length))
      stop(
        "compute_winset: total WinSet count (", total_winsets, ") exceeds ",
        "winset_max (", winset_max, "). Got max seats per frame=", n_seats_max,
        ", n_frames=", n_frames, ". ",
        "Reduce the number of seats or rounds, or increase winset_max."
      )
    }

    voter_ideals_flat <- as.double(c(rbind(vxy$x, vxy$y)))

    ws_data               <- vector("list", n_frames)
    ws_competitor_indices <- vector("list", n_frames)
    for (frame_i in seq_len(n_frames)) {
      seat_idxs <- seat_idxs_per_frame[[frame_i]]
      pos_mat   <- positions[[frame_i]]
      frame_ws  <- vector("list", length(seat_idxs))
      frame_cidxs <- integer(length(seat_idxs))
      for (s_i in seq_along(seat_idxs)) {
        s        <- unname(seat_idxs[s_i])
        seat_pos <- unname(pos_mat[s, ])
        frame_cidxs[s_i] <- s - 1L
        ws <- winset_2d(
          seat_pos, voter_ideals_flat,
          dist_config = winset_dist_config,
          k           = winset_k,
          n_samples   = winset_num_samples
        )
        if (ws$is_empty()) {
          frame_ws[[s_i]] <- NULL
        } else {
          bnd     <- ws$boundary()
          xy      <- bnd$xy
          starts  <- bnd$path_starts
          is_hole <- bnd$is_hole
          n_paths <- length(starts)
          ends    <- c(starts[-1L] - 1L, nrow(xy))
          paths_list <- lapply(seq_len(n_paths), function(p_i) {
            rows <- starts[p_i]:ends[p_i]
            as.list(as.vector(t(xy[rows, ])))
          })
          frame_ws[[s_i]] <- list(
            paths         = paths_list,
            is_hole       = as.list(as.integer(is_hole)),
            competitor_idx = frame_cidxs[s_i]
          )
        }
      }
      ws_data[[frame_i]]               <- frame_ws
      ws_competitor_indices[[frame_i]] <- as.list(frame_cidxs)
    }
  }

  # ── Voronoi (Euclidean) per frame ────────────────────────────────────────────
  voronoi_cells <- NULL
  if (isTRUE(compute_voronoi)) {
    n_frames_v <- d$n_rounds + 1L
    bbox <- c(xlim[1], ylim[1], xlim[2], ylim[2])
    voronoi_cells <- vector("list", n_frames_v)
    for (frame_i in seq_len(n_frames_v)) {
      pos_mat <- positions[[frame_i]]
      sites_xy <- as.double(as.vector(t(pos_mat)))
      n_sites <- nrow(pos_mat)
      frame_cells <- .Call(
        "r_scs_voronoi_cells_2d",
        sites_xy,
        as.integer(n_sites),
        as.double(bbox),
        PACKAGE = "socialchoicelab"
      )
      voronoi_cells[[frame_i]] <- frame_cells
    }
  }

  # ── Seat-holder indices (unconditional, used by JS stats block) ─────────────
  # 0-based competitor indices of seat holders per frame; minimal payload cost.
  n_frames_all <- d$n_rounds + 1L
  seat_holder_indices_list <- vector("list", n_frames_all)
  for (r in seq_len(d$n_rounds)) {
    st <- trace$round_seat_totals(r)
    seat_holder_indices_list[[r]] <- as.list(unname(which(st > 0L) - 1L))
  }
  final_ss_all <- trace$final_seat_shares()
  seat_holder_indices_list[[n_frames_all]] <- as.list(unname(which(final_ss_all > 0) - 1L))

  payload <- list(
    voters_x = unname(as.list(vxy$x)),
    voters_y = unname(as.list(vxy$y)),
    voter_color = voter_color,
    xlim = unname(as.list(xlim)),
    ylim = unname(as.list(ylim)),
    dim_names = unname(as.list(dim_names)),
    rounds = unname(as.list(frame_names)),
    positions = unname(lapply(positions, function(m) {
      unname(lapply(seq_len(nrow(m)), function(i) as.list(unname(m[i, ]))))
    })),
    competitor_names = unname(as.list(competitor_names)),
    competitor_colors = unname(as.list(colors)),
    vote_shares = unname(vote_shares_list),
    trail = trail,
    trail_length = trail_len,
    title = title,
    seat_holder_indices = seat_holder_indices_list
  )
  if (!is.null(overlays_serialised)) {
    payload$overlays_static <- overlays_serialised
  }
  if (!is.null(ic_data)) {
    payload$indifference_curves       <- ic_data
    payload$ic_competitor_indices     <- ic_competitor_indices
  }
  if (!is.null(ws_data)) {
    payload$winsets                    <- ws_data
    payload$winset_competitor_indices  <- ws_competitor_indices
  }
  if (!is.null(voronoi_cells)) {
    payload$voronoi_cells <- voronoi_cells
  }

  htmlwidgets::createWidget(
    name = "competition_canvas",
    x = payload,
    width = width,
    height = height,
    package = "socialchoicelab",
    sizingPolicy = htmlwidgets::sizingPolicy(
      viewer.defaultWidth  = 800L,
      viewer.defaultHeight = 700L,
      viewer.fill = FALSE,
      browser.defaultWidth = 900L,
      browser.defaultHeight = 800L,
      browser.fill = FALSE,
      padding = 0
    )
  )
}

#' Save a competition canvas widget to a .scscanvas file
#'
#' Serialises the pre-computed animation payload (positions, ICs, WinSet,
#' Cutlines, etc.) to a JSON file with metadata.  The file can be reloaded
#' later with \code{\link{load_competition_canvas}} without recomputing
#' anything.  R and Python packages can read each other's files.
#'
#' @param widget An \code{htmlwidget} returned by
#'   \code{\link{animate_competition_canvas}}.
#' @param path Path to the output file.  By convention use the
#'   \code{.scscanvas} extension.
#' @return \code{path}, invisibly.
#' @export
save_competition_canvas <- function(widget, path) {
  if (!inherits(widget, "htmlwidget")) {
    stop(
      "save_competition_canvas: 'widget' must be an htmlwidget ",
      "(returned by animate_competition_canvas())."
    )
  }
  envelope <- list(
    format    = "scscanvas",
    version   = "1",
    created   = format(Sys.time(), "%Y-%m-%dT%H:%M:%SZ", tz = "UTC"),
    generator = paste0("socialchoicelab/r/",
                       utils::packageVersion("socialchoicelab")),
    width     = widget$width,
    height    = widget$height,
    payload   = widget$x
  )
  jsonlite::write_json(envelope, path, auto_unbox = TRUE, digits = NA)
  invisible(path)
}

#' Load a competition canvas from a .scscanvas file
#'
#' Reads a file written by \code{\link{save_competition_canvas}} (or its Python
#' equivalent) and returns an \code{htmlwidget} that can be displayed in the
#' RStudio Viewer, Shiny, or R Markdown, or saved to a standalone HTML file
#' with \code{htmlwidgets::saveWidget()}.
#'
#' @param path Path to a \code{.scscanvas} JSON file.
#' @param width,height Widget dimensions in pixels.  \code{NULL} uses the
#'   values stored in the file (or package defaults when those are also
#'   \code{NULL}).
#' @return An \code{htmlwidget} of class \code{competition_canvas}.
#' @export
load_competition_canvas <- function(path, width = NULL, height = NULL) {
  if (!file.exists(path)) {
    stop("load_competition_canvas: file not found: ", path)
  }
  envelope <- jsonlite::read_json(path, simplifyVector = FALSE)
  if (!identical(envelope$format, "scscanvas")) {
    stop(
      "load_competition_canvas: unexpected format '", envelope$format,
      "' (expected 'scscanvas'). Is '", path, "' a .scscanvas file?"
    )
  }
  w <- if (!is.null(width))  width  else envelope$width
  h <- if (!is.null(height)) height else envelope$height
  is_1d <- identical(envelope$payload$n_dims, 1L) ||
            identical(envelope$payload$n_dims, 1)
  sizing <- if (isTRUE(is_1d)) {
    htmlwidgets::sizingPolicy(
      viewer.defaultWidth  = 800L,
      viewer.defaultHeight = 400L,
      viewer.fill = FALSE,
      browser.defaultWidth = 900L,
      browser.defaultHeight = 400L,
      browser.fill = FALSE,
      padding = 0
    )
  } else {
    htmlwidgets::sizingPolicy(
      viewer.defaultWidth  = 800L,
      viewer.defaultHeight = 700L,
      viewer.fill = FALSE,
      browser.defaultWidth = 900L,
      browser.defaultHeight = 800L,
      browser.fill = FALSE,
      padding = 0
    )
  }
  htmlwidgets::createWidget(
    name         = "competition_canvas",
    x            = envelope$payload,
    width        = w,
    height       = h,
    package      = "socialchoicelab",
    sizingPolicy = sizing
  )
}
