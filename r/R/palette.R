# palette.R — C13.4: Palette and theme utilities for spatial voting visualization
#
# Provides a small set of curated, colorblind-safe colour palettes drawn from
# ColorBrewer and the Okabe-Ito palette, plus a theme system that maps each
# layer type to a coordinated colour automatically.
#
# Usage:
#   colors <- scl_palette("dark2", n = 5L)
#   cols   <- scl_theme_colors("winset", theme = "dark2")
#   fig    <- plot_spatial_voting(voters, sq = sq, theme = "dark2")
#   fig    <- layer_winset(fig, ws, theme = "dark2")

# ---------------------------------------------------------------------------
# Raw palette data
# ---------------------------------------------------------------------------

#' Approved qualitative palettes.
#' Source: ColorBrewer 2.0 (Cynthia Brewer) and Okabe & Ito (2008).
.PALETTE_HEX <- list(
  dark2 = c(
    "#1B9E77",  # teal
    "#D95F02",  # orange
    "#7570B3",  # purple
    "#E7298A",  # magenta
    "#66A61E",  # lime green
    "#E6AB02",  # gold
    "#A6761D",  # brown
    "#666666"   # grey
  ),
  set2 = c(
    "#66C2A5",  # mint
    "#FC8D62",  # salmon
    "#8DA0CB",  # periwinkle
    "#E78AC3",  # pink
    "#A6D854",  # yellow-green
    "#FFD92F",  # yellow
    "#E5C494",  # tan
    "#B3B3B3"   # light grey
  ),
  okabe_ito = c(
    "#0072B2",  # blue
    "#D55E00",  # vermilion
    "#009E73",  # bluish green
    "#E69F00",  # orange
    "#56B4E9",  # sky blue
    "#CC79A7",  # reddish purple
    "#000000"   # black
  ),
  paired = c(
    "#A6CEE3",  # light blue
    "#1F78B4",  # blue
    "#B2DF8A",  # light green
    "#33A02C",  # green
    "#FB9A99",  # light red
    "#E31A1C",  # red
    "#FDBF6F",  # light orange
    "#FF7F00",  # orange
    "#CAB2D6",  # light purple
    "#6A3D9A",  # purple
    "#FFFF99",  # light yellow
    "#B15928"   # brown
  )
)

# Layer type → palette slot (1-indexed for R).
# Region layers: slots 1-4.  Point layers: slots 5-6.
.LAYER_SLOT <- c(
  winset        = 1L,
  yolk          = 2L,
  uncovered_set = 3L,
  convex_hull   = 4L,
  voters        = 5L,
  alternatives  = 6L
)

# (fill_alpha, line_alpha) — opacity increases with z-order depth.
.LAYER_ALPHAS <- list(
  winset        = c(fill = 0.28, line = 0.80),
  yolk          = c(fill = 0.22, line = 0.70),
  uncovered_set = c(fill = 0.18, line = 0.65),
  convex_hull   = c(fill = 0.12, line = 0.55),
  voters        = c(fill = 0.85, line = 1.00),
  alternatives  = c(fill = 0.90, line = 1.00)
)

# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

.hex_to_rgb <- function(h) {
  h <- sub("^#", "", h)
  list(
    r = strtoi(substr(h, 1L, 2L), 16L),
    g = strtoi(substr(h, 3L, 4L), 16L),
    b = strtoi(substr(h, 5L, 6L), 16L)
  )
}

.rgba <- function(hex_color, alpha) {
  rgb <- .hex_to_rgb(hex_color)
  sprintf("rgba(%d,%d,%d,%.3g)", rgb$r, rgb$g, rgb$b, alpha)
}

.best_palette_for_n <- function(n) {
  if (n <= 7L) "okabe_ito" else if (n <= 8L) "dark2" else "paired"
}

# Green-free candidate colour sequence for 1D competition canvases.
# Voter dots are lime green (#66A61E); overlay centroid is crimson; overlay
# marginal median is indigo-violet.  All three are excluded from this list.
# Blue (#0072B2) is demoted to slot 4 so the most common 2- or 3-candidate
# demos never put a blue candidate next to the indigo-violet median overlay.
.COMPETITION_1D_HEX <- c(
  "#D55E00",  # vermilion
  "#CC79A7",  # reddish purple / mauve
  "#E69F00",  # amber / orange
  "#56B4E9",  # sky blue  (clearly lighter than indigo-violet overlay)
  "#F0E442",  # yellow
  "#000000",  # black
  "#A6761D"   # brown
)

.candidate_colors_1d <- function(n, alpha = 0.95) {
  hex <- .COMPETITION_1D_HEX
  if (n > length(hex)) hex <- rep(hex, length.out = n)
  vapply(seq_len(n), function(i) .rgba(hex[i], alpha), character(1L))
}

.resolve_palette <- function(name, n) {
  resolved <- if (name == "auto") .best_palette_for_n(n) else name
  pal <- .PALETTE_HEX[[resolved]]
  if (is.null(pal)) {
    stop(sprintf(
      "scl_palette: unknown palette '%s'. Available: %s.",
      name, paste(names(.PALETTE_HEX), collapse = ", ")
    ))
  }
  pal
}

.layer_fill_color <- function(layer_type, theme) {
  alphas <- .LAYER_ALPHAS[[layer_type]] %||% c(fill = 0.18, line = 0.65)
  if (theme == "bw") return(sprintf("rgba(200,200,200,%.3g)", alphas["fill"]))
  pal  <- .resolve_palette(theme, 8L)
  slot <- (.LAYER_SLOT[[layer_type]] %||% 1L)
  .rgba(pal[((slot - 1L) %% length(pal)) + 1L], alphas["fill"])
}

.layer_line_color <- function(layer_type, theme) {
  alphas <- .LAYER_ALPHAS[[layer_type]] %||% c(fill = 0.18, line = 0.65)
  if (theme == "bw") return(sprintf("rgba(30,30,30,%.3g)", alphas["line"]))
  pal  <- .resolve_palette(theme, 8L)
  slot <- (.LAYER_SLOT[[layer_type]] %||% 1L)
  .rgba(pal[((slot - 1L) %% length(pal)) + 1L], alphas["line"])
}

.voter_point_color <- function(theme) {
  if (theme == "bw") return("rgba(30,30,30,0.85)")
  pal  <- .resolve_palette(theme, 8L)
  slot <- .LAYER_SLOT[["voters"]] %||% 5L
  .rgba(pal[((slot - 1L) %% length(pal)) + 1L], 0.85)
}

.alt_point_color <- function(theme) {
  if (theme == "bw") return("rgba(60,60,60,0.90)")
  pal  <- .resolve_palette(theme, 8L)
  slot <- .LAYER_SLOT[["alternatives"]] %||% 6L
  .rgba(pal[((slot - 1L) %% length(pal)) + 1L], 0.90)
}

# Centroid / marginal-median overlay colours — keep in sync with
# inst/htmlwidgets/competition_canvas.js COLORS.overlay.
.overlay_centroid_fill         <- "rgba(185,10,10,0.95)"
.overlay_marginal_median_fill  <- "rgba(100,0,180,0.90)"
.overlay_point_stroke          <- "rgba(255,255,255,0.85)"

.centroid_overlay_color <- function(theme) {
  if (theme == "bw") "rgba(25,25,25,0.95)" else .overlay_centroid_fill
}

.marginal_median_overlay_color <- function(theme) {
  if (theme == "bw") "rgba(45,45,45,0.95)" else .overlay_marginal_median_fill
}

.overlay_triangle_stroke <- function(theme) {
  if (theme == "bw") "rgba(240,240,240,0.95)" else .overlay_point_stroke
}

.ic_uniform_line <- function(theme) {
  if (theme == "bw") "rgba(80,80,80,0.40)" else "rgba(120,120,160,0.40)"
}

.preferred_uniform_fill <- function(theme) {
  if (theme == "bw") "rgba(160,160,160,0.08)" else "rgba(120,120,160,0.08)"
}

.preferred_uniform_line <- function(theme) {
  if (theme == "bw") "rgba(80,80,80,0.28)" else "rgba(120,120,160,0.28)"
}

# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

#' Return n RGBA colour strings from a named palette
#'
#' A convenience utility for retrieving coordinated colours from the same
#' palettes used by the \code{layer_*} plotting functions. Useful when adding
#' custom drawing (e.g. extra canvas layers or external graphics) that should
#' remain visually consistent with the rest of the plot.
#'
#' @param name Palette name: \code{"dark2"} (ColorBrewer Dark2, 8 colours,
#'   colorblind-safe), \code{"set2"} (ColorBrewer Set2, 8 colours, softer),
#'   \code{"okabe_ito"} (Okabe-Ito, 7 colours, print-safe), \code{"paired"}
#'   (ColorBrewer Paired, 12 colours), or \code{"auto"} (selects the best
#'   palette for \code{n}: Okabe-Ito for n ≤ 7, Dark2 for n ≤ 8, Paired for
#'   n > 8).
#' @param n Number of colours to return. If \code{n} exceeds the palette size,
#'   colours are cycled.
#' @param alpha Opacity applied uniformly to all returned colours (0–1).
#' @return Character vector of \code{n} RGBA strings.
#' @examples
#' scl_palette("dark2", n = 4L)
#' scl_palette("auto",  n = 5L, alpha = 0.7)
#' @export
scl_palette <- function(name = "auto", n = 8L, alpha = 1.0) {
  n   <- as.integer(n)
  pal <- .resolve_palette(name, n)
  vapply(seq_len(n), function(i) {
    .rgba(pal[((i - 1L) %% length(pal)) + 1L], alpha)
  }, character(1L))
}

#' Return the fill and line colours for a layer type in a theme
#'
#' Returns the \code{(fill_color, line_color)} pair that \code{layer_*}
#' functions will use for a given layer type and theme. Useful when adding
#' custom drawing that should match the rest of the plot.
#'
#' @param layer_type One of \code{"winset"}, \code{"yolk"},
#'   \code{"uncovered_set"}, \code{"convex_hull"}, \code{"voters"},
#'   \code{"alternatives"}.
#' @param theme Theme name — same options as \code{plot_spatial_voting()}'s
#'   \code{theme} argument: \code{"dark2"} (default), \code{"set2"},
#'   \code{"okabe_ito"}, \code{"paired"}, \code{"bw"} (black-and-white print).
#' @return Named list with elements \code{fill} and \code{line} (RGBA strings).
#' @examples
#' cols <- scl_theme_colors("winset", theme = "dark2")
#' cols$fill
#' cols$line
#' @export
scl_theme_colors <- function(layer_type, theme = "dark2") {
  valid <- c(names(.PALETTE_HEX), "bw")
  if (!theme %in% valid) {
    stop(sprintf(
      "scl_theme_colors: unknown theme '%s'. Available: %s.",
      theme, paste(valid, collapse = ", ")
    ))
  }
  list(
    fill = .layer_fill_color(layer_type, theme),
    line = .layer_line_color(layer_type, theme)
  )
}
