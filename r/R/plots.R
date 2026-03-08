# plots.R — C10/C13: Spatial voting visualization helpers
#
# All functions return plotly figures. Layers compose by passing the figure
# returned by one function into the next:
#
#   fig <- plot_spatial_voting(voters, sq = sq, theme = "dark2")
#   fig <- layer_ic(fig, voters, sq, theme = "dark2")
#   fig <- layer_convex_hull(fig, hull, theme = "dark2")
#   fig <- layer_winset(fig, ws, theme = "dark2")
#   fig <- layer_yolk(fig, cx, cy, r, theme = "dark2")
#   fig <- layer_uncovered_set(fig, bnd, theme = "dark2")
#   fig

# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

.flat_to_xy <- function(v) {
  n <- length(v) %/% 2L
  if (n == 0L) return(list(x = double(0L), y = double(0L)))
  list(x = v[seq(1L, 2L * n, 2L)],
       y = v[seq(2L, 2L * n, 2L)])
}

.circle_pts <- function(cx, cy, r, n = 64L) {
  theta <- seq(0, 2 * pi, length.out = n + 1L)
  list(x = cx + r * cos(theta), y = cy + r * sin(theta))
}

.padded_range <- function(vals, pad_frac = 0.12, min_pad = 0.5) {
  rng <- range(vals, na.rm = TRUE)
  pad <- max((rng[2L] - rng[1L]) * pad_frac, min_pad)
  c(rng[1L] - pad, rng[2L] + pad)
}

# ---------------------------------------------------------------------------
# Base plot
# ---------------------------------------------------------------------------

#' Create a base 2D spatial voting plot
#'
#' Plots voter ideal points and policy alternatives in a 2D issue space using
#' Plotly. Add optional geometric layers via \code{\link{layer_winset}},
#' \code{\link{layer_yolk}}, \code{\link{layer_uncovered_set}},
#' \code{\link{layer_convex_hull}}, \code{\link{layer_ic}}, and
#' \code{\link{layer_preferred_regions}}.
#'
#' @param voters Flat numeric vector \code{[x0, y0, x1, y1, ...]} of voter
#'   ideal points (length 2 * n_voters).
#' @param alternatives Flat numeric vector \code{[x0, y0, ...]} of policy
#'   alternative coordinates, or \code{numeric(0)}.
#' @param sq Numeric vector \code{c(x, y)} for the status quo, or \code{NULL}.
#' @param voter_names Character vector of voter labels. \code{NULL} uses
#'   \code{"V1"}, \code{"V2"}, etc.
#' @param alt_names Character vector of alternative labels. \code{NULL} uses
#'   \code{"Alt 1"}, \code{"Alt 2"}, etc.
#' @param dim_names Length-2 character vector for axis titles.
#' @param title Plot title string.
#' @param show_labels Logical. If \code{TRUE}, point labels are drawn on the
#'   graph. Default \code{FALSE}.
#' @param xlim,ylim Length-2 numeric vectors \code{c(min, max)} for explicit
#'   axis ranges. \code{NULL} auto-computes a padded range from the data.
#' @param theme Colour theme: \code{"dark2"} (default, ColorBrewer Dark2,
#'   colorblind-safe), \code{"set2"}, \code{"okabe_ito"}, \code{"paired"}, or
#'   \code{"bw"} (black-and-white print). Passed unchanged to each
#'   \code{layer_*} call for coordinated colours.
#' @param width,height Plot dimensions in pixels.
#' @return A \code{plotly} figure object.
#' @examples
#' \dontrun{
#' voters <- c(-1.0, -0.5,  0.0, 0.0,  0.8, 0.6,  -0.4, 0.8,  0.5, -0.7)
#' sq     <- c(0.0,  0.0)
#' fig <- plot_spatial_voting(voters, sq = sq, theme = "dark2")
#' fig
#' }
#' @export
plot_spatial_voting <- function(voters,
                                 alternatives = numeric(0),
                                 sq          = NULL,
                                 voter_names = NULL,
                                 alt_names   = NULL,
                                 dim_names   = c("Dimension 1", "Dimension 2"),
                                 title       = "Spatial Voting Analysis",
                                 show_labels = FALSE,
                                 xlim        = NULL,
                                 ylim        = NULL,
                                 theme       = "dark2",
                                 width       = 700L,
                                 height      = 600L) {
  vxy <- .flat_to_xy(voters)
  axy <- .flat_to_xy(alternatives)
  n_v <- length(vxy$x)
  n_a <- length(axy$x)
  if (is.null(voter_names)) voter_names <- paste0("V", seq_len(n_v))
  if (is.null(alt_names))   alt_names   <- paste0("Alt ", seq_len(n_a))

  voter_col <- .voter_point_color(theme)
  alt_col   <- .alt_point_color(theme)
  sq_col    <- if (theme == "bw") "rgba(0,0,0,0.90)" else "rgba(20,20,20,0.90)"

  all_x <- c(vxy$x, if (n_a > 0L) axy$x, if (!is.null(sq)) sq[1L])
  all_y <- c(vxy$y, if (n_a > 0L) axy$y, if (!is.null(sq)) sq[2L])
  if (is.null(xlim) && length(all_x) > 0L) xlim <- .padded_range(all_x)
  if (is.null(ylim) && length(all_y) > 0L) ylim <- .padded_range(all_y)

  fig <- plotly::plot_ly(width = as.integer(width), height = as.integer(height))

  fig <- plotly::add_trace(
    fig, x = vxy$x, y = vxy$y, type = "scatter", mode = "markers",
    name   = "Voters",
    marker = list(symbol = "circle", size = 12, color = voter_col,
                  line = list(color = "white", width = 1.5)),
    text          = voter_names,
    hovertemplate = "%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>",
    zorder        = 8L
  )

  if (n_a > 0L) {
    fig <- plotly::add_trace(
      fig, x = axy$x, y = axy$y, type = "scatter",
      mode   = if (show_labels) "markers+text" else "markers",
      name   = "Alternatives",
      marker = list(symbol = "diamond", size = 14, color = alt_col,
                    line = list(color = "white", width = 1.5)),
      text          = alt_names,
      textposition  = "top center",
      hovertemplate = "%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>",
      zorder        = 9L
    )
  }

  if (!is.null(sq)) {
    fig <- plotly::add_trace(
      fig, x = sq[1L], y = sq[2L], type = "scatter",
      mode   = if (show_labels) "markers+text" else "markers",
      name   = "Status Quo",
      marker = list(symbol = "star", size = 18, color = sq_col,
                    line = list(color = "white", width = 1.5)),
      text          = "SQ",
      textposition  = "top center",
      hovertemplate = "Status Quo<br>(%{x:.3f}, %{y:.3f})<extra></extra>",
      zorder        = 9L
    )
  }

  plotly::layout(
    fig,
    title       = list(text = title, x = 0.05),
    xaxis       = list(title = dim_names[1L], zeroline = TRUE,
                       zerolinecolor = "#cccccc", zerolinewidth = 1,
                       range = xlim),
    yaxis       = list(title = dim_names[2L], zeroline = TRUE,
                       zerolinecolor = "#cccccc", zerolinewidth = 1,
                       scaleanchor = "x", scaleratio = 1,
                       range = ylim),
    legend      = list(x = 1.02, y = 1, xanchor = "left"),
    hovermode     = "closest",
    plot_bgcolor  = "#f8f9fa",
    paper_bgcolor = "white"
  )
}

# ---------------------------------------------------------------------------
# Layer: indifference curves
# ---------------------------------------------------------------------------

#' Add voter indifference curves
#'
#' Draws a circle for each voter centred at their ideal point with radius
#' equal to the Euclidean distance to the status quo. The circle is the
#' voter's indifference curve through the SQ.
#'
#' @param fig A plotly figure from \code{\link{plot_spatial_voting}}.
#' @param voters Flat numeric vector of voter ideal points.
#' @param sq Numeric vector \code{c(x, y)} for the status quo.
#' @param color_by_voter Logical. \code{FALSE} (default): all curves share a
#'   single neutral colour with one legend entry. \code{TRUE}: each voter gets
#'   a unique colour from \code{palette} shown individually in the legend.
#' @param fill_color Fill colour (CSS rgba string), or \code{NULL} for no fill
#'   (default). Overrides theme when not \code{NULL}.
#' @param line_color Uniform line colour used when \code{color_by_voter = FALSE}.
#'   \code{NULL} uses the theme default.
#' @param palette Palette name for \code{color_by_voter} mode. \code{"auto"}
#'   (default) uses the \code{theme}'s palette.
#' @param voter_names Character vector of voter labels.
#' @param name Legend group label (when \code{color_by_voter = FALSE}).
#' @param theme Colour theme — see \code{\link{plot_spatial_voting}}.
#' @return The updated plotly figure.
#' @examples
#' \dontrun{
#' voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
#' sq     <- c(0.1, 0.1)
#' fig <- plot_spatial_voting(voters, sq = sq)
#' fig <- layer_ic(fig, voters, sq)
#' fig
#' }
#' @export
layer_ic <- function(fig,
                     voters,
                     sq,
                     color_by_voter = FALSE,
                     fill_color     = NULL,
                     line_color     = NULL,
                     palette        = "auto",
                     voter_names    = NULL,
                     name           = "Indifference Curves",
                     theme          = "dark2") {
  vxy <- .flat_to_xy(voters)
  n_v <- length(vxy$x)
  if (is.null(voter_names)) voter_names <- paste0("V", seq_len(n_v))

  pal_name <- if (palette == "auto") theme else palette

  if (color_by_voter) {
    line_colors <- scl_palette(pal_name, n_v, alpha = 0.70)
    fill_colors <- if (!is.null(fill_color)) rep(fill_color, n_v) else {
      scl_palette(pal_name, n_v, alpha = 0.06)
    }
  } else {
    lc <- line_color %||% .ic_uniform_line(theme)
    line_colors <- rep(lc, n_v)
    fill_colors <- rep(fill_color %||% "rgba(0,0,0,0)", n_v)
  }

  for (i in seq_len(n_v)) {
    vx <- vxy$x[i]; vy <- vxy$y[i]
    r  <- sqrt((vx - sq[1L])^2 + (vy - sq[2L])^2)
    circ    <- .circle_pts(vx, vy, r)
    lname   <- if (color_by_voter) voter_names[i] else name
    lgroup  <- if (color_by_voter) voter_names[i] else name
    show_lg <- if (color_by_voter) TRUE else (i == 1L)
    use_fill <- !is.null(fill_color) || color_by_voter
    fig <- plotly::add_trace(
      fig, x = circ$x, y = circ$y, type = "scatter", mode = "lines",
      fill          = if (use_fill) "toself" else "none",
      fillcolor     = fill_colors[i],
      line          = list(color = line_colors[i], width = 1, dash = "dot"),
      name          = lname,
      legendgroup   = lgroup,
      showlegend    = show_lg,
      hovertemplate = paste0(voter_names[i], " IC (r=", round(r, 3L),
                             ")<extra></extra>"),
      zorder        = 1L
    )
  }
  fig
}

# ---------------------------------------------------------------------------
# Layer: preferred regions
# ---------------------------------------------------------------------------

#' Add voter preferred-to regions
#'
#' Draws a filled circle for each voter centred at their ideal point with
#' radius equal to the Euclidean distance to the status quo. The interior
#' is the set of policies that voter strictly prefers to the SQ.
#'
#' @param fig A plotly figure from \code{\link{plot_spatial_voting}}.
#' @param voters Flat numeric vector of voter ideal points.
#' @param sq Numeric vector \code{c(x, y)} for the status quo.
#' @param color_by_voter Logical. \code{FALSE} (default): all circles share
#'   one neutral colour. \code{TRUE}: each voter gets a unique colour from
#'   \code{palette} shown individually in the legend.
#' @param fill_color Default fill colour. \code{NULL} uses the theme default.
#' @param line_color Default outline colour. \code{NULL} uses the theme default.
#' @param palette Palette name for \code{color_by_voter} mode.
#' @param voter_names Character vector of voter labels.
#' @param name Legend group label (when \code{color_by_voter = FALSE}).
#' @param theme Colour theme — see \code{\link{plot_spatial_voting}}.
#' @return The updated plotly figure.
#' @examples
#' \dontrun{
#' voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
#' sq     <- c(0.1, 0.1)
#' fig <- plot_spatial_voting(voters, sq = sq)
#' fig <- layer_preferred_regions(fig, voters, sq)
#' fig
#' }
#' @export
layer_preferred_regions <- function(fig,
                                     voters,
                                     sq,
                                     color_by_voter = FALSE,
                                     fill_color     = NULL,
                                     line_color     = NULL,
                                     palette        = "auto",
                                     voter_names    = NULL,
                                     name           = "Preferred Region",
                                     theme          = "dark2") {
  vxy <- .flat_to_xy(voters)
  n_v <- length(vxy$x)
  if (is.null(voter_names)) voter_names <- paste0("V", seq_len(n_v))

  pal_name <- if (palette == "auto") theme else palette

  if (color_by_voter) {
    line_colors <- scl_palette(pal_name, n_v, alpha = 0.65)
    fill_colors <- scl_palette(pal_name, n_v, alpha = 0.10)
  } else {
    fill_colors <- rep(fill_color %||% .preferred_uniform_fill(theme), n_v)
    line_colors <- rep(line_color %||% .preferred_uniform_line(theme), n_v)
  }

  for (i in seq_len(n_v)) {
    vx <- vxy$x[i]; vy <- vxy$y[i]
    r  <- sqrt((vx - sq[1L])^2 + (vy - sq[2L])^2)
    circ    <- .circle_pts(vx, vy, r)
    lname   <- if (color_by_voter) voter_names[i] else name
    lgroup  <- if (color_by_voter) voter_names[i] else name
    show_lg <- if (color_by_voter) TRUE else (i == 1L)
    fig <- plotly::add_trace(
      fig, x = circ$x, y = circ$y, type = "scatter", mode = "lines",
      fill          = "toself",
      fillcolor     = fill_colors[i],
      line          = list(color = line_colors[i], width = 1, dash = "dot"),
      name          = lname,
      legendgroup   = lgroup,
      showlegend    = show_lg,
      hovertemplate = paste0(voter_names[i], " preferred region (r=",
                             round(r, 3L), ")<extra></extra>"),
      zorder        = 1L
    )
  }
  fig
}

# ---------------------------------------------------------------------------
# Layer: winset
# ---------------------------------------------------------------------------

#' Add a winset polygon layer
#'
#' Overlays the majority-rule winset boundary. Pass either a pre-computed
#' \code{\link{Winset}} object or raw \code{voters} and \code{sq} for
#' auto-compute. If the winset is empty the figure is returned unchanged.
#'
#' @param fig A plotly figure from \code{\link{plot_spatial_voting}}.
#' @param winset A \code{\link{Winset}} object, or \code{NULL} for auto-compute.
#' @param fill_color Fill colour (CSS rgba). \code{NULL} uses the theme default.
#' @param line_color Outline colour (CSS rgba). \code{NULL} uses the theme default.
#' @param name Legend entry label.
#' @param voters Flat voter vector (required when \code{winset = NULL}).
#' @param sq Status quo \code{c(x, y)} (required when \code{winset = NULL}).
#' @param theme Colour theme — see \code{\link{plot_spatial_voting}}.
#' @return The updated plotly figure.
#' @examples
#' \dontrun{
#' voters <- c(-1.0, -0.5,  0.0, 0.0,  0.8, 0.6,  -0.4, 0.8,  0.5, -0.7)
#' sq     <- c(0.0,  0.0)
#' ws  <- winset_2d(sq, voters)
#' fig <- plot_spatial_voting(voters, sq = sq)
#' fig <- layer_winset(fig, ws)
#' fig
#' }
#' @export
layer_winset <- function(fig,
                          winset     = NULL,
                          fill_color = NULL,
                          line_color = NULL,
                          name       = "Winset",
                          voters     = NULL,
                          sq         = NULL,
                          theme      = "dark2") {
  if (is.null(winset)) {
    if (is.null(voters) || is.null(sq)) {
      stop(
        "layer_winset: provide either 'winset' (a Winset object) ",
        "or both 'voters' and 'sq' to compute the winset automatically."
      )
    }
    winset <- winset_2d(sq, voters)
  }

  fill_color <- fill_color %||% .layer_fill_color("winset", theme)
  line_color <- line_color %||% .layer_line_color("winset", theme)

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
      line          = list(color = line_color, width = 2, dash = "solid"),
      name          = name,
      showlegend    = show_legend,
      legendgroup   = name,
      hoverinfo     = "skip",
      zorder        = 6L
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
#' Draws the yolk as a filled circle.
#'
#' @param fig A plotly figure from \code{\link{plot_spatial_voting}}.
#' @param center_x,center_y Yolk center coordinates.
#' @param radius Yolk radius (same units as the plot axes).
#' @param fill_color Fill colour (CSS rgba). \code{NULL} uses the theme default.
#' @param line_color Outline colour (CSS rgba). \code{NULL} uses the theme default.
#' @param name Legend entry label.
#' @param theme Colour theme — see \code{\link{plot_spatial_voting}}.
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
                        fill_color = NULL,
                        line_color = NULL,
                        name       = "Yolk",
                        theme      = "dark2") {
  fill_color <- fill_color %||% .layer_fill_color("yolk", theme)
  line_color <- line_color %||% .layer_line_color("yolk", theme)
  circ <- .circle_pts(as.double(center_x), as.double(center_y), as.double(radius))
  plotly::add_trace(
    fig, x = circ$x, y = circ$y, type = "scatter", mode = "lines",
    fill          = "toself",
    fillcolor     = fill_color,
    line          = list(color = line_color, width = 2, dash = "longdashdot"),
    name          = name,
    hovertemplate = paste0(name, " (r=", round(radius, 4L), ")<extra></extra>"),
    zorder        = 5L
  )
}

# ---------------------------------------------------------------------------
# Layer: uncovered set
# ---------------------------------------------------------------------------

#' Add an uncovered set boundary layer
#'
#' Overlays the approximate uncovered set boundary. Pass either a pre-computed
#' matrix or raw \code{voters} for auto-compute. Returns the figure unchanged
#' if the boundary is empty.
#'
#' @param fig A plotly figure from \code{\link{plot_spatial_voting}}.
#' @param boundary_xy Numeric matrix (n_pts × 2) with columns \code{x} and
#'   \code{y}, or \code{NULL} for auto-compute.
#' @param fill_color Fill colour (CSS rgba). \code{NULL} uses the theme default.
#' @param line_color Outline colour (CSS rgba). \code{NULL} uses the theme default.
#' @param name Legend entry label.
#' @param voters Flat voter vector (required when \code{boundary_xy = NULL}).
#' @param grid_resolution Integer grid resolution for auto-compute (default 20).
#' @param theme Colour theme — see \code{\link{plot_spatial_voting}}.
#' @return The updated plotly figure.
#' @examples
#' \dontrun{
#' voters <- c(-1.0, -0.5,  0.0, 0.0,  0.8, 0.6,  -0.4, 0.8,  0.5, -0.7)
#' bnd <- uncovered_set_boundary_2d(voters, grid_resolution = 10L)
#' fig <- plot_spatial_voting(voters)
#' fig <- layer_uncovered_set(fig, bnd)
#' fig
#' }
#' @export
layer_uncovered_set <- function(fig,
                                 boundary_xy     = NULL,
                                 fill_color      = NULL,
                                 line_color      = NULL,
                                 name            = "Uncovered Set",
                                 voters          = NULL,
                                 grid_resolution = 20L,
                                 theme           = "dark2") {
  if (is.null(boundary_xy)) {
    if (is.null(voters)) {
      stop(
        "layer_uncovered_set: provide either 'boundary_xy' ",
        "or 'voters' to compute the boundary automatically."
      )
    }
    boundary_xy <- uncovered_set_boundary_2d(voters,
                                             grid_resolution = as.integer(grid_resolution))
  }
  fill_color <- fill_color %||% .layer_fill_color("uncovered_set", theme)
  line_color <- line_color %||% .layer_line_color("uncovered_set", theme)

  if (nrow(boundary_xy) == 0L) return(fig)
  plotly::add_trace(
    fig,
    x = c(boundary_xy[, "x"], boundary_xy[1L, "x"]),
    y = c(boundary_xy[, "y"], boundary_xy[1L, "y"]),
    type = "scatter", mode = "lines",
    fill          = "toself",
    fillcolor     = fill_color,
    line          = list(color = line_color, width = 1.5, dash = "longdash"),
    name          = name,
    hovertemplate = paste0(name, "<extra></extra>"),
    zorder        = 4L
  )
}

# ---------------------------------------------------------------------------
# Layer: convex hull
# ---------------------------------------------------------------------------

#' Add a convex hull layer
#'
#' Overlays the convex hull boundary. If the hull is empty the figure is
#' returned unchanged.
#'
#' @param fig A plotly figure from \code{\link{plot_spatial_voting}}.
#' @param hull_xy Numeric matrix (n_hull × 2) with columns \code{x} and
#'   \code{y}, as returned by \code{\link{convex_hull_2d}}.
#' @param fill_color Fill colour (CSS rgba). \code{NULL} uses the theme default.
#' @param line_color Outline colour (CSS rgba). \code{NULL} uses the theme default.
#' @param name Legend entry label.
#' @param theme Colour theme — see \code{\link{plot_spatial_voting}}.
#' @return The updated plotly figure.
#' @examples
#' \dontrun{
#' voters <- c(-1.0, -0.5,  0.0, 0.0,  0.8, 0.6,  -0.4, 0.8,  0.5, -0.7)
#' hull <- convex_hull_2d(voters)
#' fig  <- plot_spatial_voting(voters)
#' fig  <- layer_convex_hull(fig, hull)
#' fig
#' }
#' @export
layer_convex_hull <- function(fig,
                               hull_xy,
                               fill_color = NULL,
                               line_color = NULL,
                               name       = "Convex Hull",
                               theme      = "dark2") {
  fill_color <- fill_color %||% .layer_fill_color("convex_hull", theme)
  line_color <- line_color %||% .layer_line_color("convex_hull", theme)

  if (nrow(hull_xy) == 0L) return(fig)
  plotly::add_trace(
    fig,
    x = c(hull_xy[, "x"], hull_xy[1L, "x"]),
    y = c(hull_xy[, "y"], hull_xy[1L, "y"]),
    type = "scatter", mode = "lines",
    fill          = "toself",
    fillcolor     = fill_color,
    line          = list(color = line_color, width = 1.5, dash = "dash"),
    name          = name,
    hovertemplate = paste0(name, "<extra></extra>"),
    zorder        = 2L
  )
}

# ---------------------------------------------------------------------------
# save_plot
# ---------------------------------------------------------------------------

#' Save a spatial voting plot to file
#'
#' Exports a Plotly figure to HTML or an image format. HTML export uses
#' \code{htmlwidgets::saveWidget()} and requires no additional packages.
#' Image export (\code{.png}, \code{.svg}, \code{.pdf}) uses
#' \code{plotly::save_image()} which requires the \code{kaleido} Python
#' package; see \url{https://github.com/plotly/Kaleido} for installation.
#'
#' @param fig A plotly figure.
#' @param path Output file path. Extension determines format: \code{.html}
#'   (recommended), \code{.png}, \code{.svg}, \code{.pdf}, \code{.jpeg},
#'   or \code{.webp}.
#' @param width,height Optional pixel dimensions for image output.
#' @return \code{path} (invisibly).
#' @examples
#' \dontrun{
#' voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
#' fig    <- plot_spatial_voting(voters, sq = c(0.0, 0.0))
#' save_plot(fig, "my_plot.html")
#' }
#' @export
save_plot <- function(fig, path, width = NULL, height = NULL) {
  ext <- tolower(tools::file_ext(path))

  if (!is.null(width) || !is.null(height)) {
    layout_args <- Filter(Negate(is.null), list(width = width, height = height))
    fig <- do.call(plotly::layout, c(list(fig), layout_args))
  }

  if (ext == "html") {
    if (!requireNamespace("htmlwidgets", quietly = TRUE)) {
      stop(
        "save_plot: HTML export requires the 'htmlwidgets' package. ",
        "Install it with: install.packages('htmlwidgets')"
      )
    }
    htmlwidgets::saveWidget(plotly::as_widget(fig), file = path,
                            selfcontained = TRUE)
  } else if (ext %in% c("png", "svg", "pdf", "jpeg", "webp")) {
    plotly::save_image(fig, file = path)
  } else {
    stop(sprintf(
      "save_plot: unsupported file extension '.%s'. Supported formats: .html, .png, .svg, .pdf, .jpeg, .webp.",
      ext
    ))
  }
  invisible(path)
}

# ---------------------------------------------------------------------------
# finalize_plot
# ---------------------------------------------------------------------------

#' Enforce the canonical layer stack order
#'
#' Layer ordering is handled natively via the \code{zorder} attribute on each
#' trace. This function is kept for API compatibility.
#'
#' @param fig A plotly figure.
#' @return The same plotly figure unchanged.
#' @examples
#' \dontrun{
#' voters <- c(-1.0, -0.5,  0.0, 0.0,  0.8, 0.6)
#' sq     <- c(0.0, 0.0)
#' fig <- plot_spatial_voting(voters, sq = sq)
#' fig <- finalize_plot(fig)
#' print(fig)
#' }
#' @export
finalize_plot <- function(fig) {
  fig
}
