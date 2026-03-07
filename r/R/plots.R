# plots.R — C10: Spatial voting visualization helpers
#
# All functions return plotly figures. Layers compose by passing the figure
# returned by one function into the next:
#
#   fig <- plot_spatial_voting(voters, alts, sq = sq)
#   fig <- layer_winset(fig, ws)
#   fig <- layer_yolk(fig, cx, cy, r)
#   fig <- layer_uncovered_set(fig, bnd)
#   fig <- layer_convex_hull(fig, hull)
#   fig

# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

.flat_to_xy <- function(v) {
  n <- length(v) %/% 2L
  list(x = v[seq(1L, 2L * n, 2L)],
       y = v[seq(2L, 2L * n, 2L)])
}

.circle_pts <- function(cx, cy, r, n = 64L) {
  theta <- seq(0, 2 * pi, length.out = n + 1L)
  list(x = cx + r * cos(theta), y = cy + r * sin(theta))
}

# ---------------------------------------------------------------------------
# Base plot
# ---------------------------------------------------------------------------

#' Create a base 2D spatial voting plot
#'
#' Plots voter ideal points and policy alternatives in a 2D issue space using
#' Plotly. Add optional geometric layers via \code{\link{layer_winset}},
#' \code{\link{layer_yolk}}, \code{\link{layer_uncovered_set}}, and
#' \code{\link{layer_convex_hull}}.
#'
#' @param voters Flat numeric vector \code{[x0, y0, x1, y1, ...]} of voter
#'   ideal points (length 2 * n_voters).
#' @param alternatives Flat numeric vector \code{[x0, y0, ...]} of policy
#'   alternative coordinates (length 2 * n_alts).
#' @param sq Numeric vector \code{c(x, y)} for the status quo, or \code{NULL}.
#' @param voter_names Character vector of voter labels. \code{NULL} uses
#'   \code{"V1"}, \code{"V2"}, etc.
#' @param alt_names Character vector of alternative labels. \code{NULL} uses
#'   \code{"Alt 1"}, \code{"Alt 2"}, etc.
#' @param dim_names Length-2 character vector for axis titles.
#' @param title Plot title string.
#' @param width,height Plot dimensions in pixels.
#' @return A \code{plotly} figure object.
#' @examples
#' \dontrun{
#' voters <- c(-1.0, -0.5,  0.0, 0.0,  0.8, 0.6,  -0.4, 0.8,  0.5, -0.7)
#' alts   <- c(0.0,  0.0,   0.6, 0.4,  -0.5, 0.3)
#' sq     <- c(0.0,  0.0)
#' fig <- plot_spatial_voting(voters, alts, sq = sq)
#' fig
#' }
#' @export
plot_spatial_voting <- function(voters,
                                 alternatives,
                                 sq          = NULL,
                                 voter_names = NULL,
                                 alt_names   = NULL,
                                 dim_names   = c("Dimension 1", "Dimension 2"),
                                 title       = "Spatial Voting Analysis",
                                 width       = 700L,
                                 height      = 600L) {
  vxy <- .flat_to_xy(voters)
  axy <- .flat_to_xy(alternatives)
  n_v <- length(vxy$x)
  n_a <- length(axy$x)
  if (is.null(voter_names)) voter_names <- paste0("V", seq_len(n_v))
  if (is.null(alt_names))   alt_names   <- paste0("Alt ", seq_len(n_a))

  fig <- plotly::plot_ly(width = as.integer(width), height = as.integer(height))

  # Voters
  fig <- plotly::add_trace(
    fig, x = vxy$x, y = vxy$y, type = "scatter", mode = "markers",
    name   = "Voters",
    marker = list(
      symbol = "circle", size = 12,
      color  = "rgba(60,120,210,0.85)",
      line   = list(color = "white", width = 1.5)
    ),
    text          = voter_names,
    hovertemplate = "%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>"
  )

  # Alternatives
  fig <- plotly::add_trace(
    fig, x = axy$x, y = axy$y, type = "scatter", mode = "markers+text",
    name   = "Alternatives",
    marker = list(
      symbol = "diamond", size = 14,
      color  = "rgba(210,70,30,0.9)",
      line   = list(color = "white", width = 1.5)
    ),
    text          = alt_names,
    textposition  = "top center",
    hovertemplate = "%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>"
  )

  # Status quo
  if (!is.null(sq)) {
    fig <- plotly::add_trace(
      fig, x = sq[1L], y = sq[2L], type = "scatter", mode = "markers+text",
      name   = "Status Quo",
      marker = list(
        symbol = "star", size = 18,
        color  = "rgba(20,20,20,0.9)",
        line   = list(color = "white", width = 1.5)
      ),
      text          = "SQ",
      textposition  = "top center",
      hovertemplate = "Status Quo<br>(%{x:.3f}, %{y:.3f})<extra></extra>"
    )
  }

  plotly::layout(
    fig,
    title       = list(text = title, x = 0.05),
    xaxis       = list(title = dim_names[1L], zeroline = TRUE,
                       zerolinecolor = "#cccccc", zerolinewidth = 1),
    yaxis       = list(title = dim_names[2L], zeroline = TRUE,
                       zerolinecolor = "#cccccc", zerolinewidth = 1,
                       scaleanchor = "x", scaleratio = 1),
    legend      = list(x = 1.02, y = 1, xanchor = "left"),
    hovermode     = "closest",
    plot_bgcolor  = "#f8f9fa",
    paper_bgcolor = "white"
  )
}

# ---------------------------------------------------------------------------
# Layer: winset
# ---------------------------------------------------------------------------

#' Add a winset polygon layer
#'
#' Overlays the boundary of a \code{\link{Winset}} object onto a base plot
#' from \code{\link{plot_spatial_voting}}. If the winset is empty the figure
#' is returned unchanged.
#'
#' @param fig A plotly figure from \code{\link{plot_spatial_voting}}.
#' @param winset A \code{\link{Winset}} object.
#' @param fill_color Fill colour (CSS rgba string). Default: light blue.
#' @param line_color Outline colour (CSS rgba string). Default: medium blue.
#' @param name Legend entry label.
#' @return The updated plotly figure.
#' @examples
#' \dontrun{
#' voters <- c(-1.0, -0.5,  0.0, 0.0,  0.8, 0.6,  -0.4, 0.8,  0.5, -0.7)
#' alts   <- c(0.0,  0.0)
#' sq     <- c(0.0,  0.0)
#' ws  <- winset_2d(sq, voters)
#' fig <- plot_spatial_voting(voters, alts, sq = sq)
#' fig <- layer_winset(fig, ws)
#' fig
#' }
#' @export
layer_winset <- function(fig,
                          winset,
                          fill_color = "rgba(100,150,255,0.25)",
                          line_color = "rgba(50,100,220,0.8)",
                          name       = "Winset") {
  if (winset$is_empty()) return(fig)

  bnd     <- winset$boundary()
  xy      <- bnd$xy
  starts  <- bnd$path_starts
  is_hole <- bnd$is_hole
  n_paths <- length(starts)
  ends    <- c(starts[-1L] - 1L, nrow(xy))

  show_legend <- TRUE
  for (i in seq_len(n_paths)) {
    rows <- starts[i]:ends[i]
    fc   <- if (isTRUE(is_hole[i])) "rgba(248,249,250,0.95)" else fill_color
    fig <- plotly::add_trace(
      fig,
      x = c(xy[rows, "x"], xy[rows[1L], "x"]),
      y = c(xy[rows, "y"], xy[rows[1L], "y"]),
      type = "scatter", mode = "lines",
      fill          = "toself",
      fillcolor     = fc,
      line          = list(color = line_color, width = 1.5),
      name          = name,
      showlegend    = show_legend,
      legendgroup   = name,
      hoverinfo     = "skip"
    )
    show_legend <- FALSE
  }
  fig
}

# ---------------------------------------------------------------------------
# Layer: yolk
# ---------------------------------------------------------------------------

#' Add a yolk circle layer
#'
#' Draws the yolk as a filled circle. Provide the center and radius
#' obtained from \code{yolk_2d()} (when exposed) or computed separately.
#'
#' @param fig A plotly figure from \code{\link{plot_spatial_voting}}.
#' @param center_x,center_y Yolk center coordinates.
#' @param radius Yolk radius (same units as the plot axes).
#' @param color Fill/outline colour (CSS rgba string). Default: semi-transparent red.
#' @param name Legend entry label.
#' @return The updated plotly figure.
#' @examples
#' \dontrun{
#' voters <- c(-1.0, -0.5,  0.0, 0.0,  0.8, 0.6)
#' fig <- plot_spatial_voting(voters, c(0.0, 0.0))
#' fig <- layer_yolk(fig, center_x = 0.1, center_y = 0.05, radius = 0.3)
#' fig
#' }
#' @export
layer_yolk <- function(fig,
                        center_x,
                        center_y,
                        radius,
                        color = "rgba(210,50,50,0.55)",
                        name  = "Yolk") {
  circ <- .circle_pts(as.double(center_x), as.double(center_y), as.double(radius))
  plotly::add_trace(
    fig, x = circ$x, y = circ$y, type = "scatter", mode = "lines",
    fill          = "toself",
    fillcolor     = color,
    line          = list(color = color, width = 2),
    name          = name,
    hovertemplate = paste0(name, " (r=", round(radius, 4L), ")<extra></extra>")
  )
}

# ---------------------------------------------------------------------------
# Layer: uncovered set
# ---------------------------------------------------------------------------

#' Add an uncovered set boundary layer
#'
#' Overlays the approximate boundary of the continuous uncovered set, as
#' returned by \code{\link{uncovered_set_boundary_2d}}. If the boundary is
#' empty the figure is returned unchanged.
#'
#' @param fig A plotly figure from \code{\link{plot_spatial_voting}}.
#' @param boundary_xy Numeric matrix (n_pts \eqn{\times} 2) with columns
#'   \code{x} and \code{y}, as returned by
#'   \code{\link{uncovered_set_boundary_2d}}.
#' @param fill_color Fill colour (CSS rgba string). Default: semi-transparent green.
#' @param line_color Outline colour (CSS rgba string). Default: medium green.
#' @param name Legend entry label.
#' @return The updated plotly figure.
#' @examples
#' \dontrun{
#' voters <- c(-1.0, -0.5,  0.0, 0.0,  0.8, 0.6,  -0.4, 0.8,  0.5, -0.7)
#' bnd <- uncovered_set_boundary_2d(voters, grid_resolution = 10L)
#' fig <- plot_spatial_voting(voters, c(0.0, 0.0))
#' fig <- layer_uncovered_set(fig, bnd)
#' fig
#' }
#' @export
layer_uncovered_set <- function(fig,
                                 boundary_xy,
                                 fill_color = "rgba(50,180,80,0.20)",
                                 line_color = "rgba(30,150,60,0.85)",
                                 name       = "Uncovered Set") {
  if (nrow(boundary_xy) == 0L) return(fig)
  plotly::add_trace(
    fig,
    x = c(boundary_xy[, "x"], boundary_xy[1L, "x"]),
    y = c(boundary_xy[, "y"], boundary_xy[1L, "y"]),
    type = "scatter", mode = "lines",
    fill          = "toself",
    fillcolor     = fill_color,
    line          = list(color = line_color, width = 1.5),
    name          = name,
    hovertemplate = paste0(name, "<extra></extra>")
  )
}

# ---------------------------------------------------------------------------
# Layer: convex hull
# ---------------------------------------------------------------------------

#' Add a convex hull layer
#'
#' Overlays the convex hull boundary, as returned by
#' \code{\link{convex_hull_2d}}. If the hull is empty the figure is returned
#' unchanged.
#'
#' @param fig A plotly figure from \code{\link{plot_spatial_voting}}.
#' @param hull_xy Numeric matrix (n_hull \eqn{\times} 2) with columns
#'   \code{x} and \code{y}, as returned by \code{\link{convex_hull_2d}}.
#' @param color Outline colour (CSS rgba string). Default: semi-transparent purple.
#' @param name Legend entry label.
#' @return The updated plotly figure.
#' @examples
#' \dontrun{
#' voters <- c(-1.0, -0.5,  0.0, 0.0,  0.8, 0.6,  -0.4, 0.8,  0.5, -0.7)
#' hull <- convex_hull_2d(voters)
#' fig <- plot_spatial_voting(voters, c(0.0, 0.0))
#' fig <- layer_convex_hull(fig, hull)
#' fig
#' }
#' @export
layer_convex_hull <- function(fig,
                               hull_xy,
                               color = "rgba(130,80,190,0.75)",
                               name  = "Convex Hull") {
  if (nrow(hull_xy) == 0L) return(fig)
  plotly::add_trace(
    fig,
    x = c(hull_xy[, "x"], hull_xy[1L, "x"]),
    y = c(hull_xy[, "y"], hull_xy[1L, "y"]),
    type = "scatter", mode = "lines",
    fill          = "toself",
    fillcolor     = "rgba(130,80,190,0.12)",
    line          = list(color = color, width = 2),
    name          = name,
    hovertemplate = paste0(name, "<extra></extra>")
  )
}
