# scenarios.R — Built-in named scenario datasets

# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

.scenarios_dir <- function() {
  d <- system.file("extdata", "scenarios", package = "socialchoicelab")
  if (!nzchar(d)) {
    stop(
      "Scenario data directory not found. ",
      "Install the package with devtools::install() before calling load_scenario()."
    )
  }
  d
}

# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

#' List available built-in scenarios
#'
#' Returns the names that can be passed to \code{load_scenario()}.
#'
#' @return Character vector of scenario identifiers.
#'
#' @examples
#' list_scenarios()
#'
#' @export
list_scenarios <- function() {
  files <- list.files(.scenarios_dir(), pattern = "\\.json$", full.names = FALSE)
  sort(sub("\\.json$", "", files))
}

#' Load a built-in named scenario
#'
#' Reads a bundled JSON scenario file and returns a list ready to pass to
#' \code{plot_spatial_voting()} and the computation functions.
#'
#' @param name Character. Scenario identifier — one of the names returned by
#'   \code{list_scenarios()} (e.g. \code{"laing_olmsted_bear"}).
#'
#' @return A named list with elements:
#'   \describe{
#'     \item{`name`}{Full display name (character).}
#'     \item{`description`}{Short description (character).}
#'     \item{`source`}{Bibliographic source (character).}
#'     \item{`n_voters`}{Number of voters (integer).}
#'     \item{`n_dimensions`}{Number of policy dimensions (integer).}
#'     \item{`space`}{Named list with \code{x_range} and \code{y_range} (each length-2 numeric).}
#'     \item{`decision_rule`}{Majority threshold, e.g. \code{0.5} (numeric).}
#'     \item{`dim_names`}{Axis labels (character vector, length \code{n_dimensions}).}
#'     \item{`voters`}{Flat numeric vector \code{[x0, y0, x1, y1, ...]} of length
#'       \code{n_voters * n_dimensions}.}
#'     \item{`voter_names`}{Character vector of length \code{n_voters}.}
#'     \item{`status_quo`}{Numeric vector of length \code{n_dimensions}, or \code{NULL}.}
#'   }
#'
#' @examples
#' sc <- load_scenario("laing_olmsted_bear")
#' sc$n_voters          # 5
#' sc$voters            # flat [x0, y0, x1, y1, ...]
#' sc$status_quo        # c(100, 100)
#'
#' sc2 <- load_scenario("tovey_regular_polygon")
#' sc2$voter_names      # "XX0" .. "XX10"
#'
#' @export
load_scenario <- function(name) {
  available <- list_scenarios()
  f <- system.file("extdata", "scenarios", paste0(name, ".json"),
                   package = "socialchoicelab")
  if (!nzchar(f)) {
    stop(sprintf(
      "Unknown scenario '%s'. Available scenarios: %s. ",
      name, paste(available, collapse = ", ")
    ))
  }

  raw <- jsonlite::fromJSON(f, simplifyVector = TRUE)

  # voters: JSON array of [x, y] pairs → matrix → flat numeric vector [x0,y0,x1,y1,...]
  voters <- as.numeric(t(raw$voters))

  status_quo <- if (!is.null(raw$status_quo)) as.numeric(raw$status_quo) else NULL

  list(
    name          = raw$name,
    description   = raw$description %||% "",
    source        = raw$source %||% "",
    n_voters      = as.integer(raw$n_voters),
    n_dimensions  = as.integer(raw$n_dimensions),
    space         = raw$space,
    decision_rule = raw$decision_rule %||% 0.5,
    dim_names     = as.character(raw$dim_names),
    voters        = voters,
    voter_names   = as.character(raw$voter_names),
    status_quo    = status_quo
  )
}
