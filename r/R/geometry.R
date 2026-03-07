# geometry.R — B3.5: Copeland scores/winner, Condorcet winner (discrete),
#              core (continuous), uncovered set (discrete and continuous)
#
# All alternative index inputs and outputs are 1-based (R convention).

# ---------------------------------------------------------------------------
# Copeland
# ---------------------------------------------------------------------------

#' Compute Copeland scores for a set of 2D alternatives
#'
#' The Copeland score of alternative \code{i} equals the number of alternatives
#' it beats in pairwise majority comparisons minus the number it loses to.
#'
#' @param alts Flat numeric vector \code{[x0, y0, ...]} of alternative
#'   coordinates (length = 2 * n_alts).
#' @param voter_ideals Flat numeric vector of voter ideal coordinates.
#' @param dist_config Distance configuration from \code{\link{make_dist_config}}.
#' @param k Majority threshold: \code{"simple"} or a positive integer.
#' @return Integer vector of length n_alts. Names are \code{"alt1", "alt2", ...}.
#' @export
copeland_scores_2d <- function(alts, voter_ideals,
                                dist_config = make_dist_config(),
                                k = "simple") {
  alt_xy <- as.double(alts)
  n_alts <- as.integer(length(alt_xy) / 2L)
  scores <- .Call("r_scs_copeland_scores_2d",
                  alt_xy, as.double(voter_ideals), dist_config, .resolve_k(k),
      PACKAGE = "socialchoicelab")
  names(scores) <- paste0("alt", seq_len(n_alts))
  scores
}

#' Find the Copeland winner in a set of 2D alternatives
#'
#' Ties are broken by smallest alternative index (1-based).
#'
#' @inheritParams copeland_scores_2d
#' @return Integer scalar (1-based alternative index).
#' @export
copeland_winner_2d <- function(alts, voter_ideals,
                                dist_config = make_dist_config(),
                                k = "simple") {
  .Call("r_scs_copeland_winner_2d",
        as.double(alts), as.double(voter_ideals), dist_config, .resolve_k(k),
      PACKAGE = "socialchoicelab")
}

# ---------------------------------------------------------------------------
# Condorcet winner (discrete alternative set)
# ---------------------------------------------------------------------------

#' Test whether a Condorcet winner exists in a finite alternative set
#'
#' A Condorcet winner beats every other alternative under a \code{k}-majority.
#'
#' @inheritParams copeland_scores_2d
#' @return Logical scalar.
#' @export
has_condorcet_winner_2d <- function(alts, voter_ideals,
                                     dist_config = make_dist_config(),
                                     k = "simple") {
  .Call("r_scs_has_condorcet_winner_2d",
        as.double(alts), as.double(voter_ideals), dist_config, .resolve_k(k),
      PACKAGE = "socialchoicelab")
}

#' Find the Condorcet winner in a finite alternative set
#'
#' @inheritParams copeland_scores_2d
#' @return Integer (1-based alternative index), or \code{NA_integer_} if no
#'   Condorcet winner exists.
#' @export
condorcet_winner_2d <- function(alts, voter_ideals,
                                 dist_config = make_dist_config(),
                                 k = "simple") {
  .Call("r_scs_condorcet_winner_2d",
        as.double(alts), as.double(voter_ideals), dist_config, .resolve_k(k),
      PACKAGE = "socialchoicelab")
}

# ---------------------------------------------------------------------------
# Core (continuous 2D space)
# ---------------------------------------------------------------------------

#' Compute the core in continuous 2D space
#'
#' The core is non-empty only when a Condorcet point exists in the policy
#' space. For most voter configurations the core is empty.
#'
#' @param voter_ideals Flat numeric vector of voter ideal coordinates.
#' @param dist_config Distance configuration from \code{\link{make_dist_config}}.
#' @param k Majority threshold.
#' @return Named list \code{list(found, x, y)}.
#'   \code{found} is \code{TRUE} if the core is non-empty; \code{x} and
#'   \code{y} are \code{NA} when \code{found == FALSE}.
#' @export
core_2d <- function(voter_ideals,
                    dist_config = make_dist_config(),
                    k = "simple") {
  .Call("r_scs_core_2d",
        as.double(voter_ideals), dist_config, .resolve_k(k),
      PACKAGE = "socialchoicelab")
}

# ---------------------------------------------------------------------------
# Uncovered set
# ---------------------------------------------------------------------------

#' Compute the uncovered set over a finite alternative set
#'
#' An alternative is uncovered if no other alternative beats it both directly
#' (by majority) and transitively (Miller 1980 covering relation).
#'
#' @inheritParams copeland_scores_2d
#' @return Integer vector of 1-based alternative indices in the uncovered set.
#' @export
uncovered_set_2d <- function(alts, voter_ideals,
                              dist_config = make_dist_config(),
                              k = "simple") {
  .Call("r_scs_uncovered_set_2d",
        as.double(alts), as.double(voter_ideals), dist_config, .resolve_k(k),
      PACKAGE = "socialchoicelab")
}

#' Approximate the boundary of the continuous uncovered set
#'
#' Uses a grid-based approximation. Increase \code{grid_resolution} for
#' finer boundaries at higher computation cost.
#'
#' @param voter_ideals Flat numeric vector of voter ideal coordinates.
#' @param dist_config Distance configuration from \code{\link{make_dist_config}}.
#' @param grid_resolution Integer. Grid resolution for the approximation.
#'   Default: 15 (the library's \code{SCS_DEFAULT_BOUNDARY_GRID_RESOLUTION}).
#' @param k Majority threshold.
#' @return Numeric matrix (n_pts × 2) with columns \code{x} and \code{y}.
#' @export
uncovered_set_boundary_2d <- function(voter_ideals,
                                       dist_config = make_dist_config(),
                                       grid_resolution = 15L,
                                       k = "simple") {
  .Call("r_scs_uncovered_set_boundary_2d",
        as.double(voter_ideals), dist_config,
        as.integer(grid_resolution), .resolve_k(k),
      PACKAGE = "socialchoicelab")
}
