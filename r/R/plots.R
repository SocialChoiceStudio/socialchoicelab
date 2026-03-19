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

.range_with_origin <- function(vals, pad_frac = 0.12, min_pad = 0.5) {
  rng <- .padded_range(vals, pad_frac = pad_frac, min_pad = min_pad)
  c(min(rng[1L], 0), max(rng[2L], 0))
}

.trace_role_priority <- function(role) {
  switch(role,
         region = 1L,
         line = 2L,
         point = 3L,
         overlay = 4L,
         99L)
}

.trace_role <- function(trace) {
  if (!is.null(trace$scl_role)) {
    return(trace$scl_role)
  }
  name <- trace$name %||% ""
  mode <- trace$mode %||% ""
  fill <- trace$fill %||% "none"

  if (identical(mode, "lines+markers")) {
    return("overlay")
  }
  if (name %in% c("Voters", "Alternatives", "Status Quo", "Centroid",
                  "Marginal Median")) {
    return("point")
  }
  if (identical(fill, "toself")) {
    return("region")
  }
  if (grepl("lines", mode, fixed = TRUE)) {
    return("line")
  }
  "point"
}

.tag_last_trace_role <- function(fig, role) {
  n <- length(fig$x$attrs)
  if (n > 0L) {
    fig$x$attrs[[n]]$scl_role <- role
  }
  fig
}

.normalize_plot_roles <- function(fig) {
  n <- length(fig$x$attrs)
  if (n <= 1L) return(fig)
  trace_flags <- vapply(fig$x$attrs, function(trace) {
    !is.null(trace$type)
  }, logical(1))
  if (!any(trace_flags)) return(fig)
  trace_idx <- which(trace_flags)
  priorities <- vapply(fig$x$attrs[trace_idx], function(trace) {
    .trace_role_priority(.trace_role(trace))
  }, integer(1))
  ord <- trace_idx[order(priorities, seq_along(trace_idx))]
  fig$x$attrs[trace_idx] <- fig$x$attrs[ord]
  fig$x$attrs[trace_idx] <- lapply(fig$x$attrs[trace_idx], function(trace) {
    trace$scl_role <- NULL
    trace
  })
  fig
}

.add_role_trace <- function(fig, role, ...) {
  fig <- plotly::add_trace(fig, ...)
  fig <- .tag_last_trace_role(fig, role)
  .normalize_plot_roles(fig)
}

.set_rgba_alpha <- function(color, alpha) {
  if (startsWith(color, "rgba(") && endsWith(color, ")")) {
    parts <- strsplit(sub("^rgba\\((.*)\\)$", "\\1", color), ",")[[1]]
    if (length(parts) == 4L) {
      parts <- trimws(parts)
      parts[4] <- sprintf("%.3f", alpha)
      return(sprintf("rgba(%s)", paste(parts, collapse = ", ")))
    }
  }
  color
}

.apply_plot_config <- function(fig) {
  plotly::config(
    fig,
    displaylogo = FALSE,
    modeBarButtonsToRemove = c("select2d", "lasso2d", "autoScale2d")
  )
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
  if (is.null(xlim) && length(all_x) > 0L) xlim <- .range_with_origin(all_x)
  if (is.null(ylim) && length(all_y) > 0L) ylim <- .range_with_origin(all_y)

  fig <- plotly::plot_ly(width = as.integer(width), height = as.integer(height))

  fig <- .add_role_trace(
    fig, x = vxy$x, y = vxy$y, type = "scatter", mode = "markers",
    role = "point",
    name   = "Voters",
    marker = list(symbol = "circle", size = 8, color = voter_col,
                  line = list(color = "white", width = 1.5)),
    text          = voter_names,
    hovertemplate = "%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>"
  )

  if (n_a > 0L) {
    fig <- .add_role_trace(
      fig, x = axy$x, y = axy$y, type = "scatter",
      role = "point",
      mode   = if (show_labels) "markers+text" else "markers",
      name   = "Alternatives",
      marker = list(symbol = "diamond", size = 14, color = alt_col,
                    line = list(color = "white", width = 1.5)),
      text          = alt_names,
      textposition  = "top center",
      hovertemplate = "%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>"
    )
  }

  if (!is.null(sq)) {
    fig <- .add_role_trace(
      fig, x = sq[1L], y = sq[2L], type = "scatter",
      role = "point",
      mode   = if (show_labels) "markers+text" else "markers",
      name   = "Status Quo",
      marker = list(symbol = "star", size = 18, color = sq_col,
                    line = list(color = "white", width = 1.5)),
      text          = if (show_labels) "SQ" else NULL,
      textposition  = if (show_labels) "top center" else NULL,
      hovertemplate = "Status Quo<br>(%{x:.3f}, %{y:.3f})<extra></extra>"
    )
  }

  fig <- plotly::layout(
    fig,
    title       = list(text = title, x = 0.5, xanchor = "center"),
    margin      = list(t = 95, r = 20, b = 55, l = 65),
    xaxis       = list(title = dim_names[1L], zeroline = TRUE,
                       zerolinecolor = "rgba(140,140,140,0.45)",
                       zerolinewidth = 1, showgrid = TRUE,
                       gridcolor = "rgba(140,140,140,0.25)", gridwidth = 1,
                       range = xlim),
    yaxis       = list(title = dim_names[2L], zeroline = TRUE,
                       zerolinecolor = "rgba(140,140,140,0.45)",
                       zerolinewidth = 1, showgrid = TRUE,
                       gridcolor = "rgba(140,140,140,0.25)", gridwidth = 1,
                       scaleanchor = "x", scaleratio = 1,
                       range = ylim),
    legend      = list(x = 1.02, y = 1, xanchor = "left"),
    hovermode     = "closest",
    plot_bgcolor  = "white",
    paper_bgcolor = "white"
  )
  .apply_plot_config(fig)
}

#' Plot 2D competition trajectories from a CompetitionTrace
#'
#' @param trace A \code{\link{CompetitionTrace}} object.
#' @param voters Optional flat numeric voter vector or \code{numeric(0)}.
#' @param competitor_names Optional character labels for competitors.
#' @param title Plot title.
#' @param theme Colour theme.
#' @param width,height Plot dimensions in pixels.
#' @return A \code{plotly} figure object.
#' @export
plot_competition_trajectories <- function(trace,
                                          voters = numeric(0),
                                          competitor_names = NULL,
                                          title = "Competition Trajectories",
                                          theme = "dark2",
                                          width = 700L,
                                          height = 600L) {
  if (!inherits(trace, "CompetitionTrace")) {
    stop("Expected a CompetitionTrace object, got ", class(trace)[1], ".")
  }
  d <- trace$dims()
  if (d$n_dims != 2L) {
    stop("plot_competition_trajectories currently supports only 2D traces, got n_dims = ",
         d$n_dims, ".")
  }
  if (is.null(competitor_names)) {
    competitor_names <- paste0("Competitor ", seq_len(d$n_competitors))
  }
  if (length(competitor_names) != d$n_competitors) {
    stop("competitor_names length must match n_competitors.")
  }

  fig <- plot_spatial_voting(
    voters = voters,
    title = title,
    theme = theme,
    width = width,
    height = height
  )
  colors <- scl_palette(theme, d$n_competitors, alpha = 0.95)
  positions <- lapply(seq_len(d$n_rounds), function(r) trace$round_positions(r))
  positions[[d$n_rounds + 1L]] <- trace$final_positions()

  for (i in seq_len(d$n_competitors)) {
    xy <- do.call(rbind, lapply(positions, function(pos) pos[i, , drop = FALSE]))
    fig <- .add_role_trace(
      fig,
      role = "overlay",
      x = xy[, 1],
      y = xy[, 2],
      type = "scatter",
      mode = "lines+markers",
      name = competitor_names[i],
      marker = list(symbol = "diamond", size = 12, color = colors[i],
                    line = list(color = "white", width = 1)),
      line = list(color = colors[i], width = 3),
      text = c(paste("Round", seq_len(d$n_rounds)), "Final"),
      hovertemplate = "%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>"
    )
  }

  fig
}

#' Animate 2D competition trajectories from a CompetitionTrace
#'
#' @param trace A \code{\link{CompetitionTrace}} object.
#' @param voters Optional flat numeric voter vector or \code{numeric(0)}.
#' @param competitor_names Optional character labels for competitors.
#' @param title Plot title.
#' @param theme Colour theme.
#' @param width,height Plot dimensions in pixels.
#' @param trail Trail style for the animated paths. Three modes are available:
#'   \describe{
#'     \item{\code{"fade"}}{(default) Each step shows a short fading trail of
#'       recent segments behind each competitor. Opacity decays exponentially
#'       from the current position outward; segments older than
#'       \code{trail_length} are hidden. Conveys direction and recent history
#'       without cluttering the plot.}
#'     \item{\code{"full"}}{The complete path from round 1 to the current step
#'       is drawn as a continuous line. Useful for inspecting the entire
#'       trajectory, but can become visually busy in long competitions.}
#'     \item{\code{"none"}}{Markers only; no path segments are drawn. Cleanest
#'       output, suitable when only final positions matter or the animation is
#'       embedded in a space-constrained layout.}
#'   }
#' @param trail_length Controls how many past segments are shown when
#'   \code{trail = "fade"}. Accepts:
#'   \describe{
#'     \item{\code{"short"}}{The most recent 1/3 of all rounds. Good for dense
#'       or fast-moving competitions.}
#'     \item{\code{"medium"}}{The most recent 1/2 of all rounds (default).
#'       Balances recency and context for most competition lengths.}
#'     \item{\code{"long"}}{The most recent 3/4 of all rounds. Useful when the
#'       full trajectory arc matters but \code{"full"} is too cluttered.}
#'     \item{positive integer}{Exact number of segments to retain, independent
#'       of total round count.}
#'   }
#'   Ignored when \code{trail} is \code{"none"} or \code{"full"}.
#' @return A \code{plotly} figure object with animation frames.
#' @export
animate_competition_trajectories <- function(trace,
                                             voters = numeric(0),
                                             competitor_names = NULL,
                                             title = "Competition Trajectories",
                                             theme = "dark2",
                                             width = 700L,
                                             height = 600L,
                                             trail = c("fade", "full", "none"),
                                             trail_length = "medium") {
  if (!inherits(trace, "CompetitionTrace")) {
    stop("Expected a CompetitionTrace object, got ", class(trace)[1], ".")
  }
  d <- trace$dims()
  if (d$n_dims != 2L) {
    stop("animate_competition_trajectories currently supports only 2D traces, got n_dims = ",
         d$n_dims, ".")
  }
  if (is.null(competitor_names)) {
    competitor_names <- paste0("Competitor ", seq_len(d$n_competitors))
  }
  if (length(competitor_names) != d$n_competitors) {
    stop("competitor_names length must match n_competitors.")
  }
  trail <- match.arg(trail)
  if (!is.character(trail_length) && !is.numeric(trail_length)) {
    stop('trail_length must be an integer or one of "short", "medium", "long".')
  }
  if (is.character(trail_length) && !trail_length %in% c("short", "medium", "long")) {
    stop('trail_length "', trail_length, '" is not recognised. ',
         'Use "short" (1/3 rounds), "medium" (1/2 rounds), "long" (3/4 rounds), or a positive integer.')
  }

  fig <- plot_spatial_voting(
    voters = voters,
    title = title,
    theme = theme,
    width = width,
    height = height
  )
  positions <- lapply(seq_len(d$n_rounds), function(r) trace$round_positions(r))
  positions[[d$n_rounds + 1L]] <- trace$final_positions()
  frame_names <- c(paste("Round", seq_len(d$n_rounds)), "Final")
  colors <- scl_palette(theme, d$n_competitors, alpha = 0.95)
  n_segments_max <- max(length(frame_names) - 1L, 0L)
  trail_length <- if (is.character(trail_length)) {
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

  if (trail == "none") {
    # ── Manual frame construction with explicit trace targeting ──
    #
    # We bypass plotly R's native frame = ~frame path (which goes through
    # registerFrames() in plotly_build) and instead build fig$x$frames
    # manually with an explicit 'traces' field in each frame.
    #
    # Why: registerFrames() uses name-based object-constancy matching,
    # global-trace-range enforcement, and visibility toggling that can
    # produce incorrect interpolation artifacts — specifically, oversized
    # jumps on the first two frame transitions. This is a known class of
    # issues in plotly.R (see plotly/plotly.R #1696, #1903, #1618).
    #
    # The manual approach matches what the Python binding does:
    # 1. Record how many static (non-animated) traces exist before adding
    #    competitor traces.
    # 2. Add unframed base traces at the round-1 positions.
    # 3. Build frames containing ONLY competitor data, with a 'traces'
    #    field that tells plotly.js exactly which trace indices to update.
    # 4. Wire up slider + Play/Pause buttons directly in layout.
    #
    # This talks directly to plotly.js, bypassing registerFrames() entirely.

    # Count existing typed traces (voters, etc.) — these are static.
    n_static <- sum(vapply(fig$x$attrs, function(tr) !is.null(tr$type), logical(1)))

    initial <- positions[[1L]]
    for (i in seq_len(d$n_competitors)) {
      fig <- .add_role_trace(
        fig,
        role = "overlay",
        x = initial[i, 1],
        y = initial[i, 2],
        type = "scatter",
        mode = "markers",
        name = competitor_names[i],
        marker = list(symbol = "diamond", size = 12, color = colors[i],
                      line = list(color = "white", width = 1)),
        text = frame_names[1L],
        hovertemplate = "%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>"
      )
    }

    # 0-based indices of the competitor traces we just added.
    competitor_trace_indices <- as.integer(
      seq.int(n_static, n_static + d$n_competitors - 1L)
    )

    frames <- lapply(seq_along(frame_names), function(frame_idx) {
      frame_data <- list()
      for (i in seq_len(d$n_competitors)) {
        current <- positions[[frame_idx]][i, ]
        frame_data[[length(frame_data) + 1L]] <- list(
          x = I(current[1]),
          y = I(current[2]),
          type = "scatter",
          mode = "markers",
          name = competitor_names[i],
          marker = list(symbol = "diamond", size = 12, color = colors[i],
                        line = list(color = "white", width = 1)),
          text = I(frame_names[frame_idx]),
          hovertemplate = "%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>"
        )
      }
      list(
        name = frame_names[frame_idx],
        data = frame_data,
        traces = as.list(competitor_trace_indices)
      )
    })

    fig$x$frames <- frames
    fig <- plotly::layout(
      fig,
      margin = list(t = 95, r = 20, b = 220, l = 65),
      updatemenus = list(list(
        type = "buttons",
        showactive = FALSE,
        x = 0.5,
        y = -0.36,
        xanchor = "center",
        yanchor = "top",
        pad = list(t = 0, r = 8),
        direction = "right",
        buttons = list(
          list(
            label = "Play",
            method = "animate",
            args = list(frame_names, list(
              frame = list(duration = 700, redraw = TRUE),
              transition = list(duration = 0),
              fromcurrent = FALSE,
              mode = "next"
            ))
          ),
          list(
            label = "Pause",
            method = "animate",
            args = list(list(NULL), list(
              frame = list(duration = 0, redraw = TRUE),
              transition = list(duration = 0),
              mode = "immediate"
            ))
          )
        )
      )),
      sliders = list(list(
        active = -1L,
        x = 0.08,
        y = -0.17,
        len = 0.84,
        currentvalue = list(visible = FALSE),
        pad = list(t = 0, b = 0),
        steps = lapply(frame_names, function(frame_name) {
          list(
            label = "",
            method = "animate",
            args = list(list(frame_name), list(
              frame = list(duration = 0, redraw = TRUE),
              transition = list(duration = 0),
              mode = "immediate"
            ))
          )
        })
      ))
    )
    return(.apply_plot_config(fig))
  }

  # Count static traces before adding overlay traces for explicit trace targeting.
  n_static <- sum(vapply(fig$x$attrs, function(tr) !is.null(tr$type), logical(1)))
  initial <- positions[[1L]]

  if (trail == "fade") {
    # Phase 1: add n_segments_max blank segment traces + 1 marker per competitor.
    for (i in seq_len(d$n_competitors)) {
      for (j in seq_len(n_segments_max)) {
        fig <- .add_role_trace(
          fig,
          role = "overlay",
          x = numeric(0),
          y = numeric(0),
          type = "scatter",
          mode = "lines",
          name = competitor_names[i],
          showlegend = FALSE,
          hoverinfo = "skip",
          line = list(color = colors[i], width = 3)
        )
      }
      fig <- .add_role_trace(
        fig,
        role = "overlay",
        x = initial[i, 1],
        y = initial[i, 2],
        type = "scatter",
        mode = "markers",
        name = competitor_names[i],
        marker = list(symbol = "diamond", size = 12, color = colors[i],
                      line = list(color = "white", width = 1)),
        text = frame_names[1L],
        hovertemplate = "%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>"
      )
    }

    # Phase 2: pre-build to discover the actual 0-based positions of overlay
    # traces in the serialised figure. .normalize_plot_roles() re-sorts traces
    # by visual priority at every .add_role_trace() call, so segment traces
    # (mode="lines", priority "line"=2) can end up sorted before the static
    # Voters trace (priority "point"=3). We cannot rely on n_static alone to
    # predict which indices the overlay traces land on; we must inspect the
    # built output to find out.
    pre_built <- plotly::plotly_build(fig)

    # Classify each built trace: "static" (Voters, etc.), "segment" (fade helper
    # line), or "marker" (competitor diamond).  Segment traces have showlegend=FALSE;
    # marker traces have showlegend=NULL/TRUE.
    trace_roles <- lapply(seq_along(pre_built$x$data), function(bi) {
      tr  <- pre_built$x$data[[bi]]
      nm  <- tr$name %||% ""
      comp <- match(nm, competitor_names)
      if (is.na(comp)) return(list(type = "static"))
      if (isFALSE(tr$showlegend)) return(list(type = "segment", comp = comp, idx0 = bi - 1L))
      list(type = "marker", comp = comp, idx0 = bi - 1L)
    })

    overlay_roles <- Filter(function(r) r$type != "static", trace_roles)
    all_overlay_indices <- as.list(vapply(overlay_roles, function(r) r$idx0, integer(1)))

    # Assign a sequential age (1 = newest, n_segments_max = oldest) to each
    # segment slot per competitor, in the order they appear in the built output.
    seg_counters <- integer(d$n_competitors)
    overlay_roles <- lapply(overlay_roles, function(r) {
      if (r$type == "segment") {
        seg_counters[r$comp] <<- seg_counters[r$comp] + 1L
        r$age <- seg_counters[r$comp]
      }
      r
    })

    # Phase 3: build frames with frame_data ordered to match overlay_roles.
    frames <- lapply(seq_along(frame_names), function(frame_idx) {
      actual_segments <- frame_idx - 1L
      frame_data <- lapply(overlay_roles, function(r) {
        i <- r$comp
        if (r$type == "marker") {
          current <- positions[[frame_idx]][i, ]
          # I() wraps ensure single-point x/y serialise as JSON arrays, not bare
          # scalars. plotly.js requires array-valued trace data during animation.
          list(
            x = I(current[1]),
            y = I(current[2]),
            type = "scatter",
            mode = "markers",
            name = competitor_names[i],
            marker = list(symbol = "diamond", size = 12, color = colors[i],
                          line = list(color = "white", width = 1)),
            text = I(frame_names[frame_idx]),
            hovertemplate = "%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>"
          )
        } else {
          age <- r$age
          if (age <= actual_segments && age <= trail_length) {
            seg_idx <- actual_segments - age + 1L
            p0 <- positions[[seg_idx]][i, ]
            p1 <- positions[[seg_idx + 1L]][i, ]
            alpha <- max(0.12, 0.88 * (0.55 ^ (age - 1L)))
            list(
              x = c(p0[1], p1[1]),
              y = c(p0[2], p1[2]),
              type = "scatter",
              mode = "lines",
              name = competitor_names[i],
              showlegend = FALSE,
              hoverinfo = "skip",
              line = list(color = .set_rgba_alpha(colors[i], alpha), width = 3)
            )
          } else {
            list(x = numeric(0), y = numeric(0), type = "scatter", mode = "lines",
                 showlegend = FALSE, hoverinfo = "skip")
          }
        }
      })
      list(name = frame_names[frame_idx], data = frame_data, traces = all_overlay_indices)
    })
  } else {
    # trail == "full": one lines+markers trace per competitor.
    # "lines+markers" traces get role "overlay" (priority 4) in .normalize_plot_roles(),
    # so they sort after the Voters trace (priority 3). n_static is therefore correct.
    for (i in seq_len(d$n_competitors)) {
      fig <- .add_role_trace(
        fig,
        role = "overlay",
        x = initial[i, 1],
        y = initial[i, 2],
        type = "scatter",
        mode = "lines+markers",
        name = competitor_names[i],
        marker = list(symbol = "diamond", size = 12, color = colors[i],
                      line = list(color = "white", width = 1)),
        line = list(color = colors[i], width = 3),
        text = frame_names[1L],
        hovertemplate = "%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>"
      )
    }
    all_overlay_indices <- as.list(
      seq.int(n_static, n_static + d$n_competitors - 1L)
    )

    frames <- lapply(seq_along(frame_names), function(frame_idx) {
      frame_data <- list()
      for (i in seq_len(d$n_competitors)) {
        xy <- do.call(rbind, lapply(positions[seq_len(frame_idx)], function(pos) {
          pos[i, , drop = FALSE]
        }))
        text <- c(paste("Round", seq_len(min(frame_idx, d$n_rounds))),
                  if (frame_idx == length(frame_names)) "Final" else NULL)
        # I() wraps ensure x, y, and text serialise as JSON arrays even when
        # frame_idx=1 yields a single-row matrix (making xy[,1] a named scalar
        # rather than a vector). Plotly.js drops lines+markers traces whose
        # frame-update coordinates arrive as bare scalars rather than arrays.
        frame_data[[length(frame_data) + 1L]] <- list(
          x = I(xy[, 1]),
          y = I(xy[, 2]),
          type = "scatter",
          mode = "lines+markers",
          name = competitor_names[i],
          marker = list(symbol = "diamond", size = 12, color = colors[i],
                        line = list(color = "white", width = 1)),
          line = list(color = colors[i], width = 3),
          text = I(text),
          hovertemplate = "%{text}<br>(%{x:.3f}, %{y:.3f})<extra></extra>"
        )
      }
      list(name = frame_names[frame_idx], data = frame_data, traces = all_overlay_indices)
    })
  }

  fig$x$frames <- frames
  fig <- plotly::layout(
    fig,
    margin = list(t = 95, r = 20, b = 220, l = 65),
    updatemenus = list(list(
      type = "buttons",
      showactive = FALSE,
      x = 0.5,
      y = -0.36,
      xanchor = "center",
      yanchor = "top",
      pad = list(t = 0, r = 8),
      direction = "right",
      buttons = list(
        list(
          label = "Play",
          method = "animate",
          args = list(frame_names, list(
            frame = list(duration = 700, redraw = TRUE),
            transition = list(duration = 0),
            fromcurrent = FALSE,
            mode = "next"
          ))
        ),
        list(
          label = "Pause",
          method = "animate",
          args = list(list(NULL), list(
            frame = list(duration = 0, redraw = TRUE),
            transition = list(duration = 0),
            mode = "immediate"
          ))
        )
      )
    )),
    sliders = list(list(
      active = -1L,
      x = 0.08,
      y = -0.17,
      len = 0.84,
      currentvalue = list(visible = FALSE),
      pad = list(t = 0, b = 0),
      steps = lapply(frame_names, function(frame_name) {
        list(
          label = "",
          method = "animate",
          args = list(list(frame_name), list(
            frame = list(duration = 0, redraw = TRUE),
            transition = list(duration = 0),
            mode = "immediate"
          ))
        )
      })
    ))
  )
  .apply_plot_config(fig)
}

# ---------------------------------------------------------------------------
# Layer: indifference curves
# ---------------------------------------------------------------------------

#' Add voter indifference curves
#'
#' Draws an indifference contour for each voter centred at their ideal point,
#' passing through the status quo.  Under Euclidean distance (the default)
#' each contour is a circle; other metrics (Manhattan, Chebyshev, Minkowski)
#' produce their respective iso-distance shapes.
#'
#' @param fig A plotly figure from \code{\link{plot_spatial_voting}}.
#' @param voters Flat numeric vector of voter ideal points.
#' @param sq Numeric vector \code{c(x, y)} for the status quo.
#' @param dist_config Distance metric configuration from
#'   \code{\link{make_dist_config}}. \code{NULL} (default) uses Euclidean
#'   distance and draws an efficient circle.
#' @param color_by_voter Logical. \code{FALSE} (default): all curves share a
#'   single neutral colour with one legend entry. \code{TRUE}: each voter gets
#'   a unique colour from \code{palette} shown individually in the legend.
#' @param fill_color Fill colour (CSS rgba string), or \code{NULL} for no fill
#'   (default). Overrides theme when not \code{NULL}.
#' @param line_color Uniform line colour used when \code{color_by_voter = FALSE}.
#'   \code{NULL} uses the theme default.
#' @param line_width Stroke width of the IC contours in pixels. Default \code{1}.
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
                     dist_config    = NULL,
                     color_by_voter = FALSE,
                     fill_color     = NULL,
                     line_color     = NULL,
                     line_width     = 1,
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

  linear_loss <- make_loss_config("linear")

  for (i in seq_len(n_v)) {
    vx <- vxy$x[i]; vy <- vxy$y[i]
    if (is.null(dist_config)) {
      r    <- sqrt((vx - sq[1L])^2 + (vy - sq[2L])^2)
      pts  <- .circle_pts(vx, vy, r)
      px   <- pts$x; py <- pts$y
      hover_d <- r
    } else {
      d    <- calculate_distance(c(vx, vy), sq, dist_config)
      ul   <- distance_to_utility(d, linear_loss)
      ls   <- level_set_2d(vx, vy, ul, linear_loss, dist_config)
      poly <- level_set_to_polygon(ls, 64L)
      px   <- poly[, 1L]; py <- poly[, 2L]
      hover_d <- d
    }
    lname   <- if (color_by_voter) voter_names[i] else name
    lgroup  <- if (color_by_voter) voter_names[i] else name
    show_lg <- if (color_by_voter) TRUE else (i == 1L)
    use_fill <- !is.null(fill_color) || color_by_voter
    fig <- .add_role_trace(
      fig, x = px, y = py, type = "scatter", mode = "lines",
      role = "region",
      fill          = if (use_fill) "toself" else "none",
      fillcolor     = fill_colors[i],
      line          = list(color = line_colors[i], width = line_width, dash = "dot"),
      name          = lname,
      legendgroup   = lgroup,
      showlegend    = show_lg,
      hovertemplate = paste0(voter_names[i], " IC (d=", round(hover_d, 3L),
                             ")<extra></extra>")
    )
  }
  fig
}

# ---------------------------------------------------------------------------
# Layer: preferred regions
# ---------------------------------------------------------------------------

#' Add voter preferred-to regions
#'
#' Draws a filled region for each voter centred at their ideal point bounded
#' by the indifference contour through the status quo.  The interior is the
#' set of policies the voter strictly prefers to the SQ.  Under Euclidean
#' distance (the default) each region is a circle; other metrics produce
#' their respective iso-distance shapes.
#'
#' @param fig A plotly figure from \code{\link{plot_spatial_voting}}.
#' @param voters Flat numeric vector of voter ideal points.
#' @param sq Numeric vector \code{c(x, y)} for the status quo.
#' @param dist_config Distance metric configuration from
#'   \code{\link{make_dist_config}}. \code{NULL} (default) uses Euclidean
#'   distance and draws an efficient circle.
#' @param color_by_voter Logical. \code{FALSE} (default): all regions share
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
                                     dist_config    = NULL,
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

  linear_loss <- make_loss_config("linear")

  for (i in seq_len(n_v)) {
    vx <- vxy$x[i]; vy <- vxy$y[i]
    if (is.null(dist_config)) {
      r    <- sqrt((vx - sq[1L])^2 + (vy - sq[2L])^2)
      pts  <- .circle_pts(vx, vy, r)
      px   <- pts$x; py <- pts$y
      hover_d <- r
    } else {
      d    <- calculate_distance(c(vx, vy), sq, dist_config)
      ul   <- distance_to_utility(d, linear_loss)
      ls   <- level_set_2d(vx, vy, ul, linear_loss, dist_config)
      poly <- level_set_to_polygon(ls, 64L)
      px   <- poly[, 1L]; py <- poly[, 2L]
      hover_d <- d
    }
    lname   <- if (color_by_voter) voter_names[i] else name
    lgroup  <- if (color_by_voter) voter_names[i] else name
    show_lg <- if (color_by_voter) TRUE else (i == 1L)
    fig <- .add_role_trace(
      fig, x = px, y = py, type = "scatter", mode = "lines",
      role = "region",
      fill          = "toself",
      fillcolor     = fill_colors[i],
      line          = list(color = line_colors[i], width = 1, dash = "dot"),
      name          = lname,
      legendgroup   = lgroup,
      showlegend    = show_lg,
      hovertemplate = paste0(voter_names[i], " preferred region (d=",
                             round(hover_d, 3L), ")<extra></extra>")
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
#' @param dist_config Distance metric configuration from
#'   \code{\link{make_dist_config}}. \code{NULL} (default) uses Euclidean
#'   distance. Only used in the auto-compute path (\code{winset = NULL});
#'   ignored when a pre-computed \code{winset} is supplied.
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
                          winset      = NULL,
                          fill_color  = NULL,
                          line_color  = NULL,
                          name        = "Winset",
                          voters      = NULL,
                          sq          = NULL,
                          dist_config = NULL,
                          theme       = "dark2") {
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
    fig <- .add_role_trace(
      fig,
      role = "region",
      x = c(xy[rows, "x"], xy[rows[1L], "x"]),
      y = c(xy[rows, "y"], xy[rows[1L], "y"]),
      type = "scatter", mode = "lines",
      fill          = "toself",
      fillcolor     = fc,
      line          = list(color = line_color, width = 2, dash = "solid"),
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
  .add_role_trace(
    fig, x = circ$x, y = circ$y, type = "scatter", mode = "lines",
    role = "region",
    fill          = "toself",
    fillcolor     = fill_color,
    line          = list(color = line_color, width = 2, dash = "longdashdot"),
    name          = name,
    hovertemplate = paste0(name, " (r=", round(radius, 4L), ")<extra></extra>")
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
  .add_role_trace(
    fig,
    role = "region",
    x = c(boundary_xy[, "x"], boundary_xy[1L, "x"]),
    y = c(boundary_xy[, "y"], boundary_xy[1L, "y"]),
    type = "scatter", mode = "lines",
    fill          = "toself",
    fillcolor     = fill_color,
    line          = list(color = line_color, width = 1.5, dash = "longdash"),
    name          = name,
    hovertemplate = paste0(name, "<extra></extra>")
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
  .add_role_trace(
    fig,
    role = "region",
    x = c(hull_xy[, "x"], hull_xy[1L, "x"]),
    y = c(hull_xy[, "y"], hull_xy[1L, "y"]),
    type = "scatter", mode = "lines",
    fill          = "toself",
    fillcolor     = fill_color,
    line          = list(color = line_color, width = 1.5, dash = "dash"),
    name          = name,
    hovertemplate = paste0(name, "<extra></extra>")
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
#' Reapplies the package's canonical visual stack: regions, then lines, then
#' points, then overlays. It is primarily useful when users have manipulated a
#' figure directly and want to restore package ordering.
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
  .normalize_plot_roles(fig)
}

# ---------------------------------------------------------------------------
# Layer: centroid
# ---------------------------------------------------------------------------

#' Add a centroid (mean voter position) marker layer
#'
#' Displays the coordinate-wise arithmetic mean of voter ideal points as a
#' labelled marker. Computed via \code{\link{centroid_2d}}.
#'
#' @param fig A plotly figure from \code{\link{plot_spatial_voting}}.
#' @param voters Flat numeric vector \code{[x0, y0, ...]} of voter ideal
#'   coordinates.
#' @param color Marker and text colour. \code{NULL} uses the theme slot for
#'   alternative points.
#' @param name Legend entry label.
#' @param theme Colour theme — see \code{\link{plot_spatial_voting}}.
#' @return The updated plotly figure.
#' @examples
#' \dontrun{
#' voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
#' fig <- plot_spatial_voting(voters)
#' fig <- layer_centroid(fig, voters)
#' fig
#' }
#' @export
layer_centroid <- function(fig, voters, color = NULL, name = "Centroid",
                            theme = "dark2") {
  color <- color %||% .alt_point_color(theme)
  pt <- centroid_2d(as.double(voters))
  .add_role_trace(
    fig, x = pt$x, y = pt$y, type = "scatter", mode = "markers+text",
    role = "point",
    marker        = list(symbol = "diamond", size = 12, color = color,
                         line = list(color = color, width = 2)),
    text          = name,
    textposition  = "top center",
    name          = name,
    hovertemplate = paste0(name, " (", round(pt$x, 4L), ", ",
                           round(pt$y, 4L), ")<extra></extra>")
  )
}

# ---------------------------------------------------------------------------
# Layer: marginal median
# ---------------------------------------------------------------------------

#' Add a marginal median marker layer
#'
#' Displays the coordinate-wise median of voter ideal points as a labelled
#' marker (issue-by-issue median voter; Black 1948). Computed via
#' \code{\link{marginal_median_2d}}.
#'
#' @param fig A plotly figure from \code{\link{plot_spatial_voting}}.
#' @param voters Flat numeric vector \code{[x0, y0, ...]} of voter ideal
#'   coordinates.
#' @param color Marker and text colour. \code{NULL} uses the theme slot for
#'   alternative points.
#' @param name Legend entry label.
#' @param theme Colour theme — see \code{\link{plot_spatial_voting}}.
#' @return The updated plotly figure.
#' @examples
#' \dontrun{
#' voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
#' fig <- plot_spatial_voting(voters)
#' fig <- layer_marginal_median(fig, voters)
#' fig
#' }
#' @export
layer_marginal_median <- function(fig, voters, color = NULL,
                                   name = "Marginal Median", theme = "dark2") {
  color <- color %||% .alt_point_color(theme)
  pt <- marginal_median_2d(as.double(voters))
  .add_role_trace(
    fig, x = pt$x, y = pt$y, type = "scatter", mode = "markers+text",
    role = "point",
    marker        = list(symbol = "cross", size = 14, color = color,
                         line = list(color = color, width = 2)),
    text          = name,
    textposition  = "top center",
    name          = name,
    hovertemplate = paste0(name, " (", round(pt$x, 4L), ", ",
                           round(pt$y, 4L), ")<extra></extra>")
  )
}
