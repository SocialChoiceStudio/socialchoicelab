#' Create a distance configuration
#'
#' Returns a named list understood by all C API wrappers that accept a distance
#' configuration. The list element order is fixed by contract with the C helper
#' \code{build_dist_config()} in \file{src/scs_r_helpers.h}; do not reorder.
#'
#' @param distance_type Character. One of \code{"euclidean"} (L2, default),
#'   \code{"manhattan"} (L1), \code{"chebyshev"} (L∞), or
#'   \code{"minkowski"} (general Lp).
#' @param weights Numeric vector of salience weights, one per dimension.
#'   Default: all-ones vector of length \code{n_dims}.
#' @param n_dims Integer. Number of dimensions; used only when \code{weights}
#'   is \code{NULL}.
#' @param order_p Numeric. Minkowski exponent p (>= 1); ignored for other
#'   distance types.
#' @return A named list accepted by all functions that take a distance
#'   configuration argument.
#' @export
make_dist_config <- function(distance_type = "euclidean",
                             weights       = NULL,
                             n_dims        = 2L,
                             order_p       = 2.0) {
  type_int <- switch(distance_type,
    euclidean = 0L,
    manhattan = 1L,
    chebyshev = 2L,
    minkowski = 3L,
    stop(
      "make_dist_config: unknown distance_type '", distance_type, "'. ",
      "Choose one of: euclidean, manhattan, chebyshev, minkowski."
    )
  )
  if (is.null(weights)) weights <- rep(1.0, as.integer(n_dims))
  weights <- as.double(weights)
  # Element order matches build_dist_config() in src/scs_r_helpers.h.
  list(
    distance_type = type_int,
    weights       = weights,
    n_weights     = length(weights),
    order_p       = as.double(order_p)
  )
}

#' Create a loss function configuration
#'
#' Returns a named list understood by all C API wrappers that accept a loss
#' configuration. The element order is fixed by contract with
#' \code{build_loss_config()} in \file{src/scs_r_helpers.h}; do not reorder.
#'
#' @param loss_type Character. One of \code{"linear"} (default),
#'   \code{"quadratic"}, \code{"gaussian"}, or \code{"threshold"}.
#' @param sensitivity Numeric. Scale factor α used by linear, quadratic, and
#'   threshold loss.
#' @param max_loss Numeric. Asymptote α used by Gaussian loss.
#' @param steepness Numeric. Rate parameter β used by Gaussian loss.
#' @param threshold Numeric. Indifference radius τ used by threshold loss.
#' @return A named list accepted by all functions that take a loss
#'   configuration argument.
#' @export
make_loss_config <- function(loss_type   = "linear",
                             sensitivity = 1.0,
                             max_loss    = 1.0,
                             steepness   = 1.0,
                             threshold   = 0.0) {
  type_int <- switch(loss_type,
    linear    = 0L,
    quadratic = 1L,
    gaussian  = 2L,
    threshold = 3L,
    stop(
      "make_loss_config: unknown loss_type '", loss_type, "'. ",
      "Choose one of: linear, quadratic, gaussian, threshold."
    )
  )
  # Element order matches build_loss_config() in src/scs_r_helpers.h.
  list(
    loss_type   = type_int,
    sensitivity = as.double(sensitivity),
    max_loss    = as.double(max_loss),
    steepness   = as.double(steepness),
    threshold   = as.double(threshold)
  )
}

# ---------------------------------------------------------------------------
# Internal helpers (not exported)
# ---------------------------------------------------------------------------

# Resolve the majority threshold argument: "simple" → -1L, integer k → k.
.resolve_k <- function(k) {
  if (identical(k, "simple")) return(-1L)
  k <- as.integer(k)
  if (length(k) != 1L || is.na(k) || k < 1L) {
    stop("k must be \"simple\" or a positive integer, not ", deparse(k), ".")
  }
  k
}

# Resolve a tie-break string to the integer code expected by the C API.
.resolve_tie_break <- function(tie_break) {
  switch(tie_break,
    random         = 0L,
    smallest_index = 1L,
    stop(
      "tie_break must be \"random\" or \"smallest_index\", not ",
      deparse(tie_break), "."
    )
  )
}

# Safely extract the raw EXTPTR from a StreamManager R6 object.
.sm_ptr <- function(mgr) {
  if (!inherits(mgr, "StreamManager")) {
    stop("Expected a StreamManager object, got ", class(mgr)[1], ".")
  }
  mgr$.__enclos_env__$private$ptr
}

# Safely extract the raw EXTPTR from a Winset R6 object.
.ws_ptr <- function(ws) {
  if (!inherits(ws, "Winset")) {
    stop("Expected a Winset object, got ", class(ws)[1], ".")
  }
  ws$.__enclos_env__$private$ptr
}

# Safely extract the raw EXTPTR from a Profile R6 object.
.prof_ptr <- function(p) {
  if (!inherits(p, "Profile")) {
    stop("Expected a Profile object, got ", class(p)[1], ".")
  }
  p$.__enclos_env__$private$ptr
}
