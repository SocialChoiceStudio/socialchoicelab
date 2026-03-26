# spatial_voting_canvas.R — Static 2D spatial voting plots (HTML5 canvas htmlwidget)
#
# Replaces the former Plotly-based plots.R API with the same user-facing
# function names. Figures are htmlwidgets (class spatial_voting_canvas).

# ---------------------------------------------------------------------------
# Internal helpers (also used by competition_canvas.R via package load order)
# ---------------------------------------------------------------------------

.flat_to_xy <- function(v) {
  n <- length(v) %/% 2L
  if (n == 0L) return(list(x = double(0L), y = double(0L)))
  list(
    x = v[seq(1L, 2L * n, 2L)],
    y = v[seq(2L, 2L * n, 2L)]
  )
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

.range_with_origin <- function(vals, pad_frac = 0.12, min_pad = 0.5) {
  rng <- .padded_range(vals, pad_frac = pad_frac, min_pad = min_pad)
  c(min(rng[1L], 0), max(rng[2L], 0))
}

.spatial_canvas_theme_colors <- function(theme) {
  ws   <- scl_theme_colors("winset", theme = theme)
  hull <- scl_theme_colors("convex_hull", theme = theme)
  yolk <- scl_theme_colors("yolk", theme = theme)
  unc  <- scl_theme_colors("uncovered_set", theme = theme)
  list(
    plot_bg    = if (theme == "bw") "#ffffff" else "#fafafa",
    grid       = if (theme == "bw") "rgba(60,60,60,0.15)" else "rgba(140,140,140,0.18)",
    axis       = if (theme == "bw") "rgba(40,40,40,0.5)" else "rgba(100,100,100,0.45)",
    text       = if (theme == "bw") "#111111" else "#2d2d2d",
    text_light = if (theme == "bw") "#555555" else "#888888",
    border     = if (theme == "bw") "#bbbbbb" else "#d4d4d4",
    voter_fill   = .voter_point_color(theme),
    voter_stroke = if (theme == "bw") "rgba(255,255,255,0.7)" else "rgba(255,255,255,0.65)",
    alt_fill     = .alt_point_color(theme),
    sq_fill      = if (theme == "bw") "rgba(40,40,40,0.92)" else "rgba(255,200,0,0.95)",
    winset_fill  = ws$fill,
    winset_line  = ws$line,
    hull_fill    = hull$fill,
    hull_line    = hull$line,
    ic_line      = .ic_uniform_line(theme),
    pref_fill    = .preferred_uniform_fill(theme),
    pref_line    = .preferred_uniform_line(theme),
    yolk_fill    = yolk$fill,
    yolk_line    = yolk$line,
    uncovered_fill = unc$fill,
    uncovered_line = unc$line
  )
}

.assert_spatial_canvas <- function(fig) {
  if (!inherits(fig, "htmlwidget")) {
    stop("Expected an htmlwidget from plot_spatial_voting(), got ", class(fig)[1L], ".")
  }
  if (!inherits(fig, "spatial_voting_canvas")) {
    stop(
      "Expected a spatial voting canvas widget from plot_spatial_voting(), got classes: ",
      paste(class(fig), collapse = ", "), "."
    )
  }
  invisible(fig)
}

.ensure_layers <- function(fig) {
  if (is.null(fig$x$layers)) {
    fig$x$layers <- list()
  }
  fig
}

.matrix_to_flat_xy <- function(m) {
  as.vector(rbind(m[, "x"], m[, "y"]))
}

# ---------------------------------------------------------------------------
# Base plot
# ---------------------------------------------------------------------------

#' Create a base 2D spatial voting plot
#'
#' Plots voter ideal points and policy alternatives in a 2D issue space using
#' an HTML5 canvas \code{htmlwidget}. Add optional geometric layers via
#' \code{\link{layer_winset}}, \code{\link{layer_yolk}},
#' \code{\link{layer_uncovered_set}}, \code{\link{layer_convex_hull}},
#' \code{\link{layer_ic}}, and \code{\link{layer_preferred_regions}}.
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
#' @param show_labels Logical. If \code{TRUE}, alternative names are drawn next
#'   to diamond markers.
#' @param layer_toggles Logical. If \code{TRUE} (default), a bottom bar of checkboxes
#'   toggles layer visibility (like the competition canvas controls; crop below the
#'   plot for slides). The symbol legend stays in a column on the right of the plot,
#'   drawn on the canvas (like the animation widget). If \code{FALSE}, the bottom bar
#'   is omitted for publication-style figures; all layers stay visible.
#' @param xlim,ylim Length-2 numeric vectors \code{c(min, max)} for explicit
#'   axis ranges. \code{NULL} auto-computes a padded range from the data.
#' @param theme Colour theme: \code{"dark2"} (default, ColorBrewer Dark2,
#'   colorblind-safe), \code{"set2"}, \code{"okabe_ito"}, \code{"paired"}, or
#'   \code{"bw"} (black-and-white print).
#' @param width,height Widget dimensions in pixels.
#' @return An \code{htmlwidget} of class \code{spatial_voting_canvas}.
#' @examples
#' \dontrun{
#' voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6, -0.4, 0.8, 0.5, -0.7)
#' sq     <- c(0.0, 0.0)
#' fig <- plot_spatial_voting(voters, sq = sq, theme = "dark2")
#' fig
#' }
#' @importFrom htmlwidgets createWidget sizingPolicy
#' @export
plot_spatial_voting <- function(voters,
                                alternatives = numeric(0),
                                sq           = NULL,
                                voter_names  = NULL,
                                alt_names    = NULL,
                                dim_names    = c("Dimension 1", "Dimension 2"),
                                title        = "Spatial Voting Analysis",
                                show_labels   = FALSE,
                                layer_toggles = TRUE,
                                xlim         = NULL,
                                ylim         = NULL,
                                theme        = "dark2",
                                width        = 700L,
                                height       = 600L) {
  vxy <- .flat_to_xy(voters)
  axy <- .flat_to_xy(alternatives)
  n_v <- length(vxy$x)
  n_a <- length(axy$x)
  if (is.null(voter_names)) voter_names <- paste0("V", seq_len(n_v))
  if (is.null(alt_names)) alt_names <- paste0("Alt ", seq_len(n_a))

  all_x <- c(vxy$x, if (n_a > 0L) axy$x, if (!is.null(sq)) sq[1L])
  all_y <- c(vxy$y, if (n_a > 0L) axy$y, if (!is.null(sq)) sq[2L])
  if (is.null(xlim) && length(all_x) > 0L) xlim <- .range_with_origin(all_x)
  if (is.null(ylim) && length(all_y) > 0L) ylim <- .range_with_origin(all_y)

  alts_flat <- if (n_a > 0L) {
    as.vector(rbind(axy$x, axy$y))
  } else {
    numeric(0L)
  }

  payload <- list(
    voters_x      = as.numeric(vxy$x),
    voters_y      = as.numeric(vxy$y),
    voter_names   = as.character(voter_names),
    alternatives  = alts_flat,
    alternative_names = as.character(alt_names),
    sq            = if (!is.null(sq)) as.numeric(sq)[1:2] else NULL,
    xlim          = as.numeric(xlim),
    ylim          = as.numeric(ylim),
    dim_names     = as.character(dim_names)[1:2],
    title         = as.character(title)[1L],
    theme         = as.character(theme)[1L],
    theme_colors  = .spatial_canvas_theme_colors(theme),
    show_labels   = isTRUE(show_labels),
    layer_toggles = !identical(layer_toggles, FALSE),
    layers        = list()
  )

  htmlwidgets::createWidget(
    name   = "spatial_voting_canvas",
    x      = payload,
    width  = as.integer(width),
    height = as.integer(height),
    package = "socialchoicelab",
    sizingPolicy = htmlwidgets::sizingPolicy(
      viewer.defaultWidth  = as.integer(width),
      viewer.defaultHeight = as.integer(height),
      browser.defaultWidth  = as.integer(width),
      browser.defaultHeight = as.integer(height),
      browser.fill = TRUE,
      padding = 0
    )
  )
}

# ---------------------------------------------------------------------------
# Layers
# ---------------------------------------------------------------------------

#' Add voter indifference curves
#'
#' @param fig A widget from \code{\link{plot_spatial_voting}}.
#' @inheritParams plot_spatial_voting
#' @param dist_config Distance metric from \code{\link{make_dist_config}}.
#' @param color_by_voter Per-voter colours from \code{palette} when \code{TRUE}.
#' @param fill_color Fill for closed curves; \code{NULL} means no fill unless
#'   \code{color_by_voter = TRUE}.
#' @param line_colour,line_color Outline colour when \code{color_by_voter = FALSE}.
#' @param line_width Ignored (canvas stroke is fixed); retained for API parity.
#' @param palette Palette name when \code{color_by_voter = TRUE}.
#' @param name Legend label when \code{color_by_voter = FALSE}.
#' @return Updated widget.
#' @export
layer_ic <- function(fig,
                     voters,
                     sq,
                     dist_config    = NULL,
                     color_by_voter = FALSE,
                     fill_color     = NULL,
                     line_color     = NULL,
                     line_colour    = NULL,
                     line_width     = 1,
                     palette        = "auto",
                     voter_names    = NULL,
                     name           = "Indifference Curves",
                     theme          = "dark2") {
  .assert_spatial_canvas(fig)
  lc_in <- line_colour %||% line_color
  vxy <- .flat_to_xy(voters)
  n_v <- length(vxy$x)
  if (is.null(voter_names)) voter_names <- paste0("V", seq_len(n_v))

  pal_name <- if (palette == "auto") theme else palette

  if (color_by_voter) {
    line_colors <- scl_palette(pal_name, n_v, alpha = 0.70)
    fill_colors <- if (!is.null(fill_color)) {
      rep(fill_color, n_v)
    } else {
      scl_palette(pal_name, n_v, alpha = 0.06)
    }
  } else {
    lc <- lc_in %||% .ic_uniform_line(theme)
    line_colors <- rep(lc, n_v)
    fill_colors <- rep(fill_color %||% "rgba(0,0,0,0)", n_v)
  }

  linear_loss <- make_loss_config("linear")
  fig <- .ensure_layers(fig)
  curves <- fig$x$layers$ic_curves %||% list()

  for (i in seq_len(n_v)) {
    vx <- vxy$x[i]
    vy <- vxy$y[i]
    if (is.null(dist_config)) {
      r   <- sqrt((vx - sq[1L])^2 + (vy - sq[2L])^2)
      pts <- .circle_pts(vx, vy, r)
      px  <- pts$x
      py  <- pts$y
    } else {
      poly <- ic_polygon_2d(vx, vy, sq[1L], sq[2L], linear_loss, dist_config, 64L)
      px   <- c(poly[, 1L], poly[1L, 1L])
      py   <- c(poly[, 2L], poly[1L, 2L])
    }
    flat <- as.vector(rbind(px, py))
    entry <- list(
      xy   = flat,
      line = line_colors[i],
      fill = if (isTRUE(color_by_voter) || !is.null(fill_color)) fill_colors[i] else NULL
    )
    curves <- c(curves, list(entry))
  }
  fig$x$layers$ic_curves <- curves
  fig
}

#' Add voter preferred-to regions
#'
#' @param fig A widget from \code{\link{plot_spatial_voting}}.
#' @inheritParams layer_ic
#' @return Updated widget.
#' @export
layer_preferred_regions <- function(fig,
                                    voters,
                                    sq,
                                    dist_config    = NULL,
                                    color_by_voter = FALSE,
                                    fill_color     = NULL,
                                    line_color     = NULL,
                                    line_colour    = NULL,
                                    palette        = "auto",
                                    voter_names    = NULL,
                                    name           = "Preferred Region",
                                    theme          = "dark2") {
  .assert_spatial_canvas(fig)
  lc_in <- line_colour %||% line_color
  vxy <- .flat_to_xy(voters)
  n_v <- length(vxy$x)
  if (is.null(voter_names)) voter_names <- paste0("V", seq_len(n_v))

  pal_name <- if (palette == "auto") theme else palette

  if (color_by_voter) {
    line_colors <- scl_palette(pal_name, n_v, alpha = 0.65)
    fill_colors <- scl_palette(pal_name, n_v, alpha = 0.10)
  } else {
    fill_colors <- rep(fill_color %||% .preferred_uniform_fill(theme), n_v)
    line_colors <- rep(lc_in %||% .preferred_uniform_line(theme), n_v)
  }

  linear_loss <- make_loss_config("linear")
  fig <- .ensure_layers(fig)
  regs <- fig$x$layers$preferred_regions %||% list()

  for (i in seq_len(n_v)) {
    vx <- vxy$x[i]
    vy <- vxy$y[i]
    if (is.null(dist_config)) {
      r   <- sqrt((vx - sq[1L])^2 + (vy - sq[2L])^2)
      pts <- .circle_pts(vx, vy, r)
      px  <- pts$x
      py  <- pts$y
    } else {
      poly <- ic_polygon_2d(vx, vy, sq[1L], sq[2L], linear_loss, dist_config, 64L)
      px   <- c(poly[, 1L], poly[1L, 1L])
      py   <- c(poly[, 2L], poly[1L, 2L])
    }
    flat <- as.vector(rbind(px, py))
    regs <- c(regs, list(list(
      xy   = flat,
      fill = fill_colors[i],
      line = line_colors[i]
    )))
  }
  fig$x$layers$preferred_regions <- regs
  fig
}

#' Add a winset polygon layer
#'
#' @param fig A widget from \code{\link{plot_spatial_voting}}.
#' @param winset A \code{\link{Winset}} object, or \code{NULL} for auto-compute.
#' @param fill_color,line_color Colours; \code{NULL} uses the theme default.
#' @param fill_colour,line_colour UK spellings accepted.
#' @param name Ignored (canvas legend uses fixed labels); retained for parity.
#' @param voters,sq Required when \code{winset} is \code{NULL}.
#' @param dist_config Used only when auto-computing the winset.
#' @inheritParams plot_spatial_voting
#' @return Updated widget.
#' @export
layer_winset <- function(fig,
                         winset       = NULL,
                         fill_color   = NULL,
                         line_color   = NULL,
                         fill_colour  = NULL,
                         line_colour  = NULL,
                         name         = "Winset",
                         voters       = NULL,
                         sq           = NULL,
                         dist_config  = NULL,
                         theme        = "dark2") {
  .assert_spatial_canvas(fig)
  fc <- fill_colour %||% fill_color
  lc <- line_colour %||% line_color
  if (is.null(winset)) {
    if (is.null(voters) || is.null(sq)) {
      stop(
        "layer_winset: provide either 'winset' (a Winset object) ",
        "or both 'voters' and 'sq' to compute the winset automatically."
      )
    }
    dc <- dist_config %||% make_dist_config()
    winset <- winset_2d(sq, voters, dist_config = dc)
  }

  fill_c <- fc %||% .layer_fill_color("winset", theme)
  line_c <- lc %||% .layer_line_color("winset", theme)

  if (winset$is_empty()) return(fig)

  bnd     <- winset$boundary()
  xy      <- bnd$xy
  starts  <- bnd$path_starts
  is_hole <- bnd$is_hole
  n_paths <- length(starts)
  ends    <- c(starts[-1L] - 1L, nrow(xy))

  fig <- .ensure_layers(fig)
  paths <- fig$x$layers$winset_paths %||% list()
  hole_fill <- "rgba(248,249,250,0.95)"

  for (i in seq_len(n_paths)) {
    rows <- starts[i]:ends[i]
    px <- c(xy[rows, "x"], xy[rows[1L], "x"])
    py <- c(xy[rows, "y"], xy[rows[1L], "y"])
    flat <- as.vector(rbind(px, py))
    fc_i <- if (isTRUE(is_hole[i])) hole_fill else fill_c
    paths <- c(paths, list(list(
      xy   = flat,
      fill = fc_i,
      line = line_c
    )))
  }
  fig$x$layers$winset_paths <- paths
  fig
}

#' Add a yolk circle layer
#'
#' @param fig A widget from \code{\link{plot_spatial_voting}}.
#' @param center_x,center_y,radius Circle geometry.
#' @inheritParams layer_winset
#' @return Updated widget.
#' @export
layer_yolk <- function(fig,
                       center_x,
                       center_y,
                       radius,
                       fill_color  = NULL,
                       line_color  = NULL,
                       fill_colour = NULL,
                       line_colour = NULL,
                       name        = "Yolk",
                       theme       = "dark2") {
  .assert_spatial_canvas(fig)
  fc <- fill_colour %||% fill_color
  lc <- line_colour %||% line_color
  fig <- .ensure_layers(fig)
  fig$x$layers$yolk <- list(
    cx = as.double(center_x),
    cy = as.double(center_y),
    r  = as.double(radius),
    fill = fc %||% .layer_fill_color("yolk", theme),
    line = lc %||% .layer_line_color("yolk", theme)
  )
  fig
}

#' Add an uncovered set boundary layer
#'
#' @param fig A widget from \code{\link{plot_spatial_voting}}.
#' @param boundary_xy Matrix with columns \code{x}, \code{y}, or \code{NULL}
#'   for auto-compute from \code{voters}.
#' @param voters Flat numeric vector \code{[x0, y0, ...]}; required when
#'   \code{boundary_xy} is \code{NULL} for auto-compute.
#' @param grid_resolution Grid resolution when auto-computing.
#' @inheritParams layer_winset
#' @return Updated widget.
#' @export
layer_uncovered_set <- function(fig,
                                boundary_xy     = NULL,
                                fill_color      = NULL,
                                line_color      = NULL,
                                fill_colour     = NULL,
                                line_colour     = NULL,
                                name            = "Uncovered Set",
                                voters          = NULL,
                                grid_resolution = 20L,
                                theme           = "dark2") {
  .assert_spatial_canvas(fig)
  fc <- fill_colour %||% fill_color
  lc <- line_colour %||% line_color
  if (is.null(boundary_xy)) {
    if (is.null(voters)) {
      stop(
        "layer_uncovered_set: provide either 'boundary_xy' ",
        "or 'voters' to compute the boundary automatically."
      )
    }
    boundary_xy <- uncovered_set_boundary_2d(voters,
      grid_resolution = as.integer(grid_resolution)
    )
  }
  fill_c <- fc %||% .layer_fill_color("uncovered_set", theme)
  line_c <- lc %||% .layer_line_color("uncovered_set", theme)

  if (nrow(boundary_xy) == 0L) return(fig)

  fig <- .ensure_layers(fig)
  px <- c(boundary_xy[, "x"], boundary_xy[1L, "x"])
  py <- c(boundary_xy[, "y"], boundary_xy[1L, "y"])
  fig$x$layers$uncovered_xy <- list(
    xy   = as.vector(rbind(px, py)),
    fill = fill_c,
    line = line_c
  )
  fig
}

#' Add a Pareto set layer (convex hull of ideals under Euclidean preferences)
#'
#' The canvas labels this layer \dQuote{Pareto Set}. For Euclidean distance-based
#' utilities, the Pareto-efficient ideal points are exactly the vertices of the
#' convex hull of voter ideals; the payload still stores geometry as
#' \code{convex_hull_xy}.
#'
#' @param fig A widget from \code{\link{plot_spatial_voting}}.
#' @param hull_xy Matrix \code{(n x 2)} with columns \code{x}, \code{y}.
#' @param name Display name for API parity; the static canvas uses the fixed label
#'   \dQuote{Pareto Set}.
#' @inheritParams layer_winset
#' @return Updated widget.
#' @export
layer_convex_hull <- function(fig,
                              hull_xy,
                              fill_color  = NULL,
                              line_color  = NULL,
                              fill_colour = NULL,
                              line_colour = NULL,
                              name        = "Pareto Set",
                              theme       = "dark2") {
  .assert_spatial_canvas(fig)
  fc <- fill_colour %||% fill_color
  lc <- line_colour %||% line_color
  fill_c <- fc %||% .layer_fill_color("convex_hull", theme)
  line_c <- lc %||% .layer_line_color("convex_hull", theme)

  if (nrow(hull_xy) == 0L) return(fig)

  fig <- .ensure_layers(fig)
  px <- c(hull_xy[, "x"], hull_xy[1L, "x"])
  py <- c(hull_xy[, "y"], hull_xy[1L, "y"])
  fig$x$layers$convex_hull_xy <- list(
    xy   = as.vector(rbind(px, py)),
    fill = fill_c,
    line = line_c
  )
  fig
}

#' Add a centroid (mean voter position) marker layer
#'
#' @param fig A widget from \code{\link{plot_spatial_voting}}.
#' @param voters Flat voter vector.
#' @param color Marker colour; \code{NULL} uses theme default.
#' @param name Ignored for canvas drawing; retained for API parity.
#' @inheritParams plot_spatial_voting
#' @return Updated widget.
#' @export
layer_centroid <- function(fig, voters, color = NULL, name = "Centroid",
                           theme = "dark2") {
  .assert_spatial_canvas(fig)
  stroke <- color %||% .centroid_overlay_color(theme)
  pt <- centroid_2d(as.double(voters))
  fig <- .ensure_layers(fig)
  fig$x$layers$centroid <- c(pt$x, pt$y)
  fig$x$layers$centroid_stroke <- stroke
  fig
}

#' Add a marginal median marker layer
#'
#' @param fig A widget from \code{\link{plot_spatial_voting}}.
#' @inheritParams layer_centroid
#' @return Updated widget.
#' @export
layer_marginal_median <- function(fig, voters, color = NULL,
                                    name = "Marginal Median", theme = "dark2") {
  .assert_spatial_canvas(fig)
  fill_c <- color %||% .marginal_median_overlay_color(theme)
  stroke_c <- .overlay_triangle_stroke(theme)
  pt <- marginal_median_2d(as.double(voters))
  fig <- .ensure_layers(fig)
  fig$x$layers$marginal_median <- c(pt$x, pt$y)
  fig$x$layers$marginal_median_fill <- fill_c
  fig$x$layers$marginal_median_stroke <- stroke_c
  fig
}

# ---------------------------------------------------------------------------
# Save
# ---------------------------------------------------------------------------

#' Save a spatial voting canvas widget to file
#'
#' Only \code{.html} is supported. For raster or vector output, save HTML and
#' use the browser's screenshot or print-to-PDF tools.
#'
#' @param fig Widget from \code{\link{plot_spatial_voting}} (and layers).
#' @param path Output path; extension \code{.html} is supported.
#' @param width,height Optional; reserved for future use (ignored).
#' @return \code{path} invisibly.
#' @export
save_plot <- function(fig, path, width = NULL, height = NULL) {
  .assert_spatial_canvas(fig)
  ext <- tolower(tools::file_ext(path))

  if (!is.null(width)) fig$width <- as.integer(width)
  if (!is.null(height)) fig$height <- as.integer(height)

  if (ext == "html") {
    if (!requireNamespace("htmlwidgets", quietly = TRUE)) {
      stop(
        "save_plot: HTML export requires the 'htmlwidgets' package. ",
        "Install it with: install.packages('htmlwidgets')"
      )
    }
    htmlwidgets::saveWidget(fig, file = path, selfcontained = TRUE)
  } else if (ext %in% c("png", "svg", "pdf", "jpeg", "webp")) {
    stop(
      "PNG/SVG export is not yet supported for canvas plots; save as HTML ",
      "and use the browser's print/screenshot tools."
    )
  } else {
    stop(sprintf(
      paste0(
        "save_plot: unsupported file extension '.%s'. ",
        "Supported formats: .html."
      ),
      ext
    ))
  }
  invisible(path)
}
