# competition_canvas.R — Canvas-based competition trajectory animation widget
#
# Parallel implementation to animate_competition_trajectories (Plotly). Data is
# stored once and frames are drawn on demand in the browser, so it scales to
# hundreds or thousands of rounds. Returns an htmlwidget, not a plotly figure.

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

#' Animate 2D competition trajectories (canvas backend)
#'
#' Renders the same competition animation as \code{\link{animate_competition_trajectories}}
#' using a canvas-based widget. Data is sent once; the browser draws frames on
#' demand. This scales to long runs (hundreds or thousands of rounds) without the
#' large HTML and slow load of the Plotly frame-based approach.
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
                                      overlays = NULL) {
  if (!inherits(trace, "CompetitionTrace")) {
    stop("Expected a CompetitionTrace object, got ", class(trace)[1], ".")
  }
  d <- trace$dims()
  if (d$n_dims != 2L) {
    stop("animate_competition_canvas currently supports only 2D traces, got n_dims = ",
         d$n_dims, ".")
  }
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

  n_segments_max <- max(length(frame_names) - 1L, 0L)
  trail_len <- if (is.character(trail_length)) {
    switch(trail_length,
      short  = max(1L, floor(n_segments_max / 3L)),
      medium = max(1L, floor(n_segments_max / 2L)),
      long   = max(1L, floor(n_segments_max * 3L / 4L))
    )
  } else {
    tl <- as.integer(trail_length)
    if (tl < 1L) stop("trail_length must be >= 1, got ", tl, ".")
    tl
  }

  colors <- scl_palette(theme, d$n_competitors, alpha = 0.95)
  voter_color <- .voter_point_color(theme)

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
    overlays_static = .serialise_overlays_static(overlays)
  )

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
