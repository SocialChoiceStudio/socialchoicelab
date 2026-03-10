#' @useDynLib socialchoicelab, .registration = TRUE
#' @keywords internal
"_PACKAGE"

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
#' @examples
#' # Euclidean (default):
#' cfg <- make_dist_config()
#'
#' # Manhattan with equal weights:
#' cfg_l1 <- make_dist_config(distance_type = "manhattan")
#'
#' # Euclidean with asymmetric salience weights:
#' cfg_w <- make_dist_config(weights = c(2.0, 1.0))
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
#' @examples
#' # Linear loss (default):
#' cfg <- make_loss_config()
#'
#' # Quadratic (proximity-squared) loss:
#' cfg_q <- make_loss_config(loss_type = "quadratic")
#'
#' # Gaussian loss with custom parameters:
#' cfg_g <- make_loss_config(loss_type = "gaussian", max_loss = 2.0,
#'                           steepness = 0.5)
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

#' Create a competition step-policy configuration
#'
#' @param kind Character. One of \code{"fixed"}, \code{"random_uniform"}, or
#'   \code{"share_delta_proportional"}.
#' @param fixed_step_size Numeric. Used when \code{kind = "fixed"}.
#' @param min_step_size,max_step_size Numeric. Bounds for random-uniform steps.
#' @param proportionality_constant Numeric. Scale for share-delta-proportional
#'   step sizing.
#' @param jitter Numeric. Uniform noise magnitude added to each step after
#'   the primary step size is computed. Default \code{0.0} (no jitter).
#' @return Named list accepted by competition wrappers.
#' @export
make_competition_step_config <- function(kind = "fixed",
                                         fixed_step_size = 1.0,
                                         min_step_size = 0.0,
                                         max_step_size = 1.0,
                                         proportionality_constant = 1.0,
                                         jitter = 0.0) {
  kind_int <- switch(kind,
    fixed = 0L,
    random_uniform = 1L,
    share_delta_proportional = 2L,
    stop(
      "make_competition_step_config: unknown kind '", kind, "'. ",
      "Choose one of: fixed, random_uniform, share_delta_proportional."
    )
  )
  list(
    kind = kind_int,
    fixed_step_size = as.double(fixed_step_size),
    min_step_size = as.double(min_step_size),
    max_step_size = as.double(max_step_size),
    proportionality_constant = as.double(proportionality_constant),
    jitter = as.double(jitter)
  )
}

#' Create a competition termination configuration
#'
#' @param stop_on_convergence Logical.
#' @param position_epsilon Numeric.
#' @param stop_on_cycle Logical.
#' @param cycle_memory Integer.
#' @param signature_resolution Numeric.
#' @param stop_on_no_improvement Logical.
#' @param no_improvement_window Integer.
#' @param objective_epsilon Numeric.
#' @return Named list accepted by competition wrappers.
#' @export
make_competition_termination_config <- function(
    stop_on_convergence = FALSE,
    position_epsilon = 1e-8,
    stop_on_cycle = FALSE,
    cycle_memory = 50L,
    signature_resolution = 1e-6,
    stop_on_no_improvement = FALSE,
    no_improvement_window = 10L,
    objective_epsilon = 1e-8) {
  list(
    stop_on_convergence = as.logical(stop_on_convergence),
    position_epsilon = as.double(position_epsilon),
    stop_on_cycle = as.logical(stop_on_cycle),
    cycle_memory = as.integer(cycle_memory),
    signature_resolution = as.double(signature_resolution),
    stop_on_no_improvement = as.logical(stop_on_no_improvement),
    no_improvement_window = as.integer(no_improvement_window),
    objective_epsilon = as.double(objective_epsilon)
  )
}

#' Create a competition engine configuration
#'
#' @param seat_count Integer number of seats.
#' @param seat_rule Character. One of \code{"plurality_top_k"} or
#'   \code{"hare_largest_remainder"}.
#' @param step_config A step config from \code{\link{make_competition_step_config}}.
#' @param boundary_policy Character. One of \code{"project_to_box"},
#'   \code{"stay_put"}, or \code{"reflect"}.
#' @param objective_kind Character. One of \code{"vote_share"} or
#'   \code{"seat_share"}.
#' @param max_rounds Integer.
#' @param termination A termination config from
#'   \code{\link{make_competition_termination_config}}.
#' @return Named list accepted by competition wrappers.
#' @export
make_competition_engine_config <- function(
    seat_count = 1L,
    seat_rule = "plurality_top_k",
    step_config = make_competition_step_config(),
    boundary_policy = "project_to_box",
    objective_kind = "vote_share",
    max_rounds = 10L,
    termination = make_competition_termination_config()) {
  seat_rule_int <- switch(seat_rule,
    plurality_top_k = 0L,
    hare_largest_remainder = 1L,
    stop(
      "make_competition_engine_config: unknown seat_rule '", seat_rule, "'. ",
      "Choose one of: plurality_top_k, hare_largest_remainder."
    )
  )
  boundary_policy_int <- switch(boundary_policy,
    project_to_box = 0L,
    stay_put = 1L,
    reflect = 2L,
    stop(
      "make_competition_engine_config: unknown boundary_policy '",
      boundary_policy, "'. Choose one of: project_to_box, stay_put, reflect."
    )
  )
  objective_kind_int <- switch(objective_kind,
    vote_share = 0L,
    seat_share = 1L,
    stop(
      "make_competition_engine_config: unknown objective_kind '",
      objective_kind, "'. Choose one of: vote_share, seat_share."
    )
  )
  list(
    seat_count = as.integer(seat_count),
    seat_rule = seat_rule_int,
    step_config = step_config,
    boundary_policy = boundary_policy_int,
    objective_kind = objective_kind_int,
    max_rounds = as.integer(max_rounds),
    termination = termination
  )
}

# ---------------------------------------------------------------------------
# Internal helpers (not exported)
# ---------------------------------------------------------------------------

# Null-coalescing operator: return lhs unless it is NULL, then return rhs.
`%||%` <- function(lhs, rhs) if (!is.null(lhs)) lhs else rhs

# Validate that tie_break = "random" is accompanied by a stream_manager and
# stream_name. Call this at the top of every one_winner / rank_by_scores wrapper.
.check_tie_break_args <- function(tie_break, stream_manager, stream_name) {
  if (identical(tie_break, "random") &&
      (is.null(stream_manager) || is.null(stream_name))) {
    stop(
      "tie_break = \"random\" requires both stream_manager and stream_name. ",
      "Provide a StreamManager object and a registered stream name, or use ",
      "tie_break = \"smallest_index\"."
    )
  }
}

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

.sm_ptr_optional <- function(mgr) {
  if (is.null(mgr)) return(NULL)
  .sm_ptr(mgr)
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

.competition_trace_reason <- function(code) {
  switch(as.character(as.integer(code)),
    `0` = "max_rounds",
    `1` = "converged",
    `2` = "cycle_detected",
    `3` = "no_improvement_window",
    paste0("unknown(", code, ")")
  )
}
