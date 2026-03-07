#' Winset: a 2D majority winset region
#'
#' Wraps the \code{SCS_Winset} C handle. Create winset objects via the factory
#' functions (\code{\link{winset_2d}}, \code{\link{weighted_winset_2d}},
#' \code{\link{winset_with_veto_2d}}) rather than \code{Winset$new()} directly.
#'
#' Boolean operations (\code{$union()}, \code{$intersection()},
#' \code{$difference()}, \code{$symmetric_difference()}) return new
#' \code{Winset} objects; the inputs are never modified.
#'
#' @export
Winset <- R6::R6Class(
  "Winset",
  private = list(ptr = NULL),
  public = list(

    #' @description Internal. Use factory functions instead.
    #' @param ptr External pointer returned by a C factory function.
    initialize = function(ptr) {
      private$ptr <- ptr
      invisible(self)
    },

    #' @description Release the underlying C handle. Safe to call manually for
    #'   deterministic cleanup; also called automatically by the GC.
    finalize = function() {
      .Call("r_scs_winset_destroy", private$ptr)
    },

    #' @description Test whether the winset is empty (no policy beats the SQ).
    #' @return Logical scalar.
    is_empty = function() {
      .Call("r_scs_winset_is_empty", private$ptr)
    },

    #' @description Test whether a point lies strictly inside the winset.
    #' @param x,y Point coordinates.
    #' @return Logical scalar.
    contains = function(x, y) {
      .Call("r_scs_winset_contains_point_2d", private$ptr,
            as.double(x), as.double(y))
    },

    #' @description Axis-aligned bounding box of the winset.
    #' @return Named list \code{list(min_x, min_y, max_x, max_y)}, or
    #'   \code{NULL} if the winset is empty.
    bbox = function() {
      .Call("r_scs_winset_bbox_2d", private$ptr)
    },

    #' @description Export the boundary as a matrix of sampled vertices.
    #' @return Named list with elements:
    #'   \describe{
    #'     \item{xy}{Numeric matrix (n_vertices × 2), columns \code{x} and
    #'       \code{y}.}
    #'     \item{path_starts}{Integer vector of 1-based row indices where each
    #'       boundary path starts.}
    #'     \item{is_hole}{Logical vector; \code{TRUE} if the path is a hole.}
    #'   }
    boundary = function() {
      .Call("r_scs_winset_boundary_2d", private$ptr)
    },

    #' @description Deep copy of this winset.
    #' @return A new \code{Winset} object.
    clone_winset = function() {
      Winset$new(.Call("r_scs_winset_clone", private$ptr))
    },

    #' @description Union with another winset (this ∪ other).
    #' @param other A \code{Winset} object.
    #' @return A new \code{Winset}.
    union = function(other) {
      Winset$new(.Call("r_scs_winset_union", private$ptr, .ws_ptr(other)))
    },

    #' @description Intersection with another winset (this ∩ other).
    #' @param other A \code{Winset} object.
    #' @return A new \code{Winset}.
    intersection = function(other) {
      Winset$new(.Call("r_scs_winset_intersection", private$ptr, .ws_ptr(other)))
    },

    #' @description Set difference (this \ other).
    #' @param other A \code{Winset} object.
    #' @return A new \code{Winset}.
    difference = function(other) {
      Winset$new(.Call("r_scs_winset_difference", private$ptr, .ws_ptr(other)))
    },

    #' @description Symmetric difference (this △ other).
    #' @param other A \code{Winset} object.
    #' @return A new \code{Winset}.
    symmetric_difference = function(other) {
      Winset$new(.Call("r_scs_winset_symmetric_difference",
                       private$ptr, .ws_ptr(other)))
    }
  )
)

# ---------------------------------------------------------------------------
# Factory functions
# ---------------------------------------------------------------------------

#' Compute the 2D k-majority winset of a status quo
#'
#' Returns the set of all policy points that at least \code{k} voters prefer to
#' the status quo, under the given distance metric.
#'
#' @param status_quo Numeric vector of length 2: \code{c(x, y)}.
#' @param voter_ideals Numeric vector \code{[x0, y0, x1, y1, ...]} of length
#'   2 * n_voters.
#' @param dist_config Distance configuration from \code{\link{make_dist_config}}.
#' @param k Majority threshold: \code{"simple"} for ⌊n/2⌋ + 1 voters, or a
#'   positive integer.
#' @param n_samples Boundary approximation quality (>= 4). Higher values give
#'   smoother boundaries at the cost of computation. Default: 64.
#' @return A \code{\link{Winset}} object.
#' @export
winset_2d <- function(status_quo,
                      voter_ideals,
                      dist_config = make_dist_config(),
                      k           = "simple",
                      n_samples   = 64L) {
  sq <- as.double(status_quo)
  vi <- as.double(voter_ideals)
  Winset$new(.Call("r_scs_winset_2d",
                   sq[1], sq[2], vi,
                   .resolve_k(k),
                   as.integer(n_samples),
                   dist_config))
}

#' Compute the weighted-majority 2D winset of a status quo
#'
#' Like \code{\link{winset_2d}} but with voter weights: a policy beats the
#' status quo if the total weight of its supporters meets or exceeds
#' \code{threshold * sum(weights)}.
#'
#' @param status_quo Numeric vector of length 2.
#' @param voter_ideals Numeric vector \code{[x0, y0, ...]} of length 2 * n_voters.
#' @param weights Positive numeric vector of voter weights (length = n_voters).
#' @param threshold Weight fraction in (0, 1] required for majority. Use 0.5
#'   for simple weighted majority.
#' @param dist_config Distance configuration from \code{\link{make_dist_config}}.
#' @param n_samples Boundary quality (>= 4). Default: 64.
#' @return A \code{\link{Winset}} object.
#' @export
weighted_winset_2d <- function(status_quo,
                               voter_ideals,
                               weights,
                               threshold   = 0.5,
                               dist_config = make_dist_config(),
                               n_samples   = 64L) {
  sq <- as.double(status_quo)
  vi <- as.double(voter_ideals)
  w  <- as.double(weights)
  Winset$new(.Call("r_scs_weighted_winset_2d",
                   sq[1], sq[2], vi, w,
                   as.double(threshold),
                   as.integer(n_samples),
                   dist_config))
}

#' Compute the k-majority winset constrained by veto players
#'
#' A veto player at point \code{v} blocks any move to point \code{x} unless
#' \code{x} is strictly inside \code{v}'s preferred-to set at the status quo.
#'
#' @param status_quo Numeric vector of length 2.
#' @param voter_ideals Numeric vector \code{[x0, y0, ...]} of length 2 * n_voters.
#' @param veto_ideals Numeric vector of veto player ideal points
#'   \code{[x0, y0, ...]}, or \code{NULL} for no veto constraint.
#' @param dist_config Distance configuration from \code{\link{make_dist_config}}.
#' @param k Majority threshold: \code{"simple"} or a positive integer.
#' @param n_samples Boundary quality (>= 4). Default: 64.
#' @return A \code{\link{Winset}} object.
#' @export
winset_with_veto_2d <- function(status_quo,
                                voter_ideals,
                                veto_ideals = NULL,
                                dist_config = make_dist_config(),
                                k           = "simple",
                                n_samples   = 64L) {
  sq   <- as.double(status_quo)
  vi   <- as.double(voter_ideals)
  veto <- if (is.null(veto_ideals)) double(0L) else as.double(veto_ideals)
  Winset$new(.Call("r_scs_winset_with_veto_2d",
                   sq[1], sq[2], vi, veto,
                   .resolve_k(k),
                   as.integer(n_samples),
                   dist_config))
}
