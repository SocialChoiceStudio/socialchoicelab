# social.R — B3.8: Social rankings and social-choice properties (C7)
#             B3.9: stream_manager() convenience factory
#
# All alternative index inputs/outputs are 1-based.

# ---------------------------------------------------------------------------
# C7.1 — Rank by scores
# ---------------------------------------------------------------------------

#' Sort alternatives by descending score with tie-breaking
#'
#' Produces a full ordering of all alternatives from a numeric score vector.
#'
#' @param scores Numeric vector of scores (one per alternative, any order).
#' @param tie_break \code{"smallest_index"} (default) or \code{"random"}.
#' @param stream_manager A \code{\link{StreamManager}} object; required when
#'   \code{tie_break = "random"}.
#' @param stream_name Character. Named stream for randomness.
#' @return Integer vector of 1-based alternative indices, best (highest score)
#'   first.
#' @export
rank_by_scores <- function(scores,
                            tie_break      = "smallest_index",
                            stream_manager = NULL,
                            stream_name    = NULL) {
  .check_tie_break_args(tie_break, stream_manager, stream_name)
  tb  <- .resolve_tie_break(tie_break)
  mgr <- if (is.null(stream_manager)) NULL else .sm_ptr(stream_manager)
  sn  <- if (is.null(stream_name)) character(0L) else as.character(stream_name)
  .Call("r_scs_rank_by_scores", as.double(scores), tb, mgr, sn,
      PACKAGE = "socialchoicelab")
}

# ---------------------------------------------------------------------------
# C7.2 — Pairwise ranking from matrix
# ---------------------------------------------------------------------------

#' Rank alternatives by Copeland score derived from a pairwise matrix
#'
#' @param matrix Integer matrix (n_alts × n_alts) of pairwise results.
#'   Typically the output of \code{\link{pairwise_matrix_2d}}.
#'   Values: \code{1} = WIN, \code{0} = TIE, \code{-1} = LOSS.
#' @param n_alts Integer. Number of alternatives. Defaults to \code{nrow(matrix)}.
#' @param tie_break \code{"smallest_index"} or \code{"random"}.
#' @param stream_manager A \code{\link{StreamManager}} object.
#' @param stream_name Character.
#' @return Integer vector of 1-based alternative indices, best (highest
#'   Copeland score) first.
#' @export
pairwise_ranking_from_matrix <- function(matrix,
                                          n_alts         = nrow(matrix),
                                          tie_break      = "smallest_index",
                                          stream_manager = NULL,
                                          stream_name    = NULL) {
  .check_tie_break_args(tie_break, stream_manager, stream_name)
  tb  <- .resolve_tie_break(tie_break)
  mgr <- if (is.null(stream_manager)) NULL else .sm_ptr(stream_manager)
  sn  <- if (is.null(stream_name)) character(0L) else as.character(stream_name)
  .Call("r_scs_pairwise_ranking_from_matrix",
        as.integer(matrix), as.integer(n_alts), tb, mgr, sn,
      PACKAGE = "socialchoicelab")
}

# ---------------------------------------------------------------------------
# C7.3 — Pareto efficiency
# ---------------------------------------------------------------------------

#' Return the Pareto set of a profile
#'
#' The Pareto set contains all alternatives not Pareto-dominated by any other
#' alternative. It is always non-empty.
#'
#' @param profile A \code{\link{Profile}} object.
#' @return Integer vector of 1-based alternative indices.
#' @export
pareto_set <- function(profile) {
  .Call("r_scs_pareto_set", .prof_ptr(profile),
      PACKAGE = "socialchoicelab")
}

#' Test whether a single alternative is Pareto-efficient
#'
#' @param profile A \code{\link{Profile}} object.
#' @param alt Integer (1-based). The alternative to test.
#' @return Logical scalar.
#' @export
is_pareto_efficient <- function(profile, alt) {
  alt <- as.integer(alt)
  d   <- profile$dims()
  if (alt < 1L || alt > d$n_alts) {
    stop("is_pareto_efficient: alt ", alt, " is out of range [1, ",
         d$n_alts, "].")
  }
  .Call("r_scs_is_pareto_efficient", .prof_ptr(profile), alt - 1L,
      PACKAGE = "socialchoicelab")
}

# ---------------------------------------------------------------------------
# C7.4 — Condorcet and majority-selection predicates (profile-based)
# ---------------------------------------------------------------------------

#' Test whether the profile has a Condorcet winner
#'
#' A Condorcet winner beats every other alternative by strict majority (more
#' than half of voters).
#'
#' @param profile A \code{\link{Profile}} object.
#' @return Logical scalar.
#' @export
has_condorcet_winner_profile <- function(profile) {
  .Call("r_scs_has_condorcet_winner_profile", .prof_ptr(profile),
      PACKAGE = "socialchoicelab")
}

#' Return the Condorcet winner from a profile
#'
#' @param profile A \code{\link{Profile}} object.
#' @return Integer (1-based), or \code{NA_integer_} if no Condorcet winner.
#' @export
condorcet_winner_profile <- function(profile) {
  .Call("r_scs_condorcet_winner_profile", .prof_ptr(profile),
      PACKAGE = "socialchoicelab")
}

#' Test whether an alternative is Condorcet-consistent
#'
#' Returns \code{TRUE} if either no Condorcet winner exists (vacuously
#' consistent) or the given alternative IS the Condorcet winner.
#'
#' @param profile A \code{\link{Profile}} object.
#' @param alt Integer (1-based). The alternative to test.
#' @return Logical scalar.
#' @export
is_condorcet_consistent <- function(profile, alt) {
  alt <- as.integer(alt)
  d   <- profile$dims()
  if (alt < 1L || alt > d$n_alts) {
    stop("is_condorcet_consistent: alt ", alt, " is out of range [1, ",
         d$n_alts, "].")
  }
  .Call("r_scs_is_condorcet_consistent", .prof_ptr(profile), alt - 1L,
      PACKAGE = "socialchoicelab")
}

#' Test whether an alternative is majority-consistent
#'
#' An alternative is majority-consistent iff it IS the Condorcet winner (i.e.
#' it beats every other alternative by strict majority). Returns \code{FALSE}
#' for every candidate when no Condorcet winner exists.
#'
#' @param profile A \code{\link{Profile}} object.
#' @param alt Integer (1-based). The alternative to test.
#' @return Logical scalar.
#' @export
is_majority_consistent <- function(profile, alt) {
  alt <- as.integer(alt)
  d   <- profile$dims()
  if (alt < 1L || alt > d$n_alts) {
    stop("is_majority_consistent: alt ", alt, " is out of range [1, ",
         d$n_alts, "].")
  }
  .Call("r_scs_is_majority_consistent", .prof_ptr(profile), alt - 1L,
      PACKAGE = "socialchoicelab")
}

# ---------------------------------------------------------------------------
# B3.9 — StreamManager convenience factory
# ---------------------------------------------------------------------------

#' Create a StreamManager (convenience factory)
#'
#' Equivalent to \code{StreamManager$new(master_seed, stream_names)}.
#'
#' @param master_seed Integer or double. Master seed.
#' @param stream_names Optional character vector of stream names to register
#'   immediately.
#' @return A \code{\link{StreamManager}} object.
#' @export
stream_manager <- function(master_seed, stream_names = NULL) {
  StreamManager$new(master_seed = master_seed, stream_names = stream_names)
}
