#' Profile: an ordinal preference profile
#'
#' Wraps the \code{SCS_Profile} C handle. Create profiles via the factory
#' functions (\code{\link{profile_build_spatial}},
#' \code{\link{profile_from_utility_matrix}}, etc.) rather than
#' \code{Profile$new()} directly.
#'
#' @export
Profile <- R6::R6Class(
  "Profile",
  private = list(
    ptr = NULL,
    finalize = function() {
      .Call("r_scs_profile_destroy", private$ptr,
            PACKAGE = "socialchoicelab")
    }
  ),
  public = list(

    #' @description Internal. Use factory functions instead.
    #' @param ptr External pointer returned by a C factory function.
    initialize = function(ptr) {
      private$ptr <- ptr
      invisible(self)
    },

    #' @description Query profile dimensions.
    #' @return Named list \code{list(n_voters, n_alts)}.
    dims = function() {
      .Call("r_scs_profile_dims", private$ptr,
      PACKAGE = "socialchoicelab")
    },

    #' @description Get one voter's ranking as a 1-based integer vector.
    #' @param voter 1-based voter index.
    #' @return Integer vector of length n_alts. \code{result[r]} is the 1-based
    #'   alternative index at rank \code{r} (rank 1 = most preferred).
    get_ranking = function(voter) {
      voter <- as.integer(voter)
      d <- .Call("r_scs_profile_dims", private$ptr,
      PACKAGE = "socialchoicelab")
      if (voter < 1L || voter > d$n_voters) {
        stop("get_ranking: voter ", voter, " is out of range [1, ",
             d$n_voters, "].")
      }
      .Call("r_scs_profile_get_ranking", private$ptr, voter - 1L,
      PACKAGE = "socialchoicelab")
    },

    #' @description Export all rankings as an integer matrix.
    #' @return Integer matrix (n_voters × n_alts). Entry \code{[v, r]} is the
    #'   1-based alternative index at rank \code{r} for voter \code{v}.
    #'   Row names are \code{"voter1", "voter2", ...}; column names are
    #'   \code{"rank1", "rank2", ...}.
    export = function() {
      .Call("r_scs_profile_export_rankings", private$ptr,
      PACKAGE = "socialchoicelab")
    },

    #' @description Deep copy of this profile.
    #' @return A new \code{Profile} object.
    clone_profile = function() {
      Profile$new(.Call("r_scs_profile_clone", private$ptr,
      PACKAGE = "socialchoicelab"))
    }
  )
)

# ---------------------------------------------------------------------------
# Factory functions
# ---------------------------------------------------------------------------

#' Build an ordinal preference profile from a 2D spatial model
#'
#' Each voter ranks alternatives by distance: closest first, ties broken by
#' smallest alternative index.
#'
#' @param alt_xy Numeric vector \code{[x0, y0, x1, y1, ...]} of alternative
#'   coordinates (length = 2 * n_alts).
#' @param voter_ideals Numeric vector \code{[x0, y0, ...]} of voter ideal
#'   point coordinates (length = 2 * n_voters).
#' @param dist_config Distance configuration from \code{\link{make_dist_config}}.
#' @return A \code{\link{Profile}} object.
#' @export
profile_build_spatial <- function(alt_xy,
                                  voter_ideals,
                                  dist_config = make_dist_config()) {
  Profile$new(.Call("r_scs_profile_build_spatial",
                    as.double(alt_xy), as.double(voter_ideals), dist_config,
      PACKAGE = "socialchoicelab"))
}

#' Build a profile from a utility matrix
#'
#' @param utilities Numeric matrix (n_voters × n_alts), or a flat row-major
#'   vector of length n_voters * n_alts. \code{utilities[v, a]} is the utility
#'   of voter \code{v} for alternative \code{a}. Higher utility = preferred.
#' @param n_voters Integer. Number of voters.
#' @param n_alts Integer. Number of alternatives.
#' @return A \code{\link{Profile}} object.
#' @export
profile_from_utility_matrix <- function(utilities, n_voters, n_alts) {
  Profile$new(.Call("r_scs_profile_from_utility_matrix",
                    as.double(utilities),
                    as.integer(n_voters), as.integer(n_alts),
      PACKAGE = "socialchoicelab"))
}

#' Generate a profile under the impartial culture model
#'
#' Each voter's ranking is drawn independently and uniformly from all
#' \code{n_alts!} orderings (Fisher-Yates shuffle). Purely ordinal; no spatial
#' component.
#'
#' @param n_voters Integer. Number of voters.
#' @param n_alts Integer. Number of alternatives.
#' @param stream_manager A \code{\link{StreamManager}} object.
#' @param stream_name Character. Named stream to use for randomness.
#' @return A \code{\link{Profile}} object.
#' @export
profile_impartial_culture <- function(n_voters, n_alts,
                                      stream_manager, stream_name) {
  Profile$new(.Call("r_scs_profile_impartial_culture",
                    as.integer(n_voters), as.integer(n_alts),
                    .sm_ptr(stream_manager),
                    as.character(stream_name),
      PACKAGE = "socialchoicelab"))
}

#' Generate a spatial profile with voters drawn from a uniform 2D distribution
#'
#' Both voter ideal points and alternative positions are drawn from the uniform
#' distribution on \code{[lo, hi]^2}. Only 2D is supported.
#'
#' @param n_voters Integer. Number of voters.
#' @param n_alts Integer. Number of alternatives.
#' @param lo Numeric. Lower bound of the uniform distribution.
#' @param hi Numeric. Upper bound of the uniform distribution (must satisfy lo < hi).
#' @param dist_config Distance configuration from \code{\link{make_dist_config}}.
#' @param stream_manager A \code{\link{StreamManager}} object.
#' @param stream_name Character. Named stream to use for randomness.
#' @return A \code{\link{Profile}} object.
#' @export
profile_uniform_spatial <- function(n_voters, n_alts,
                                    lo = -1.0, hi = 1.0,
                                    dist_config    = make_dist_config(),
                                    stream_manager,
                                    stream_name) {
  Profile$new(.Call("r_scs_profile_uniform_spatial",
                    as.integer(n_voters), as.integer(n_alts),
                    as.double(lo), as.double(hi),
                    dist_config,
                    .sm_ptr(stream_manager),
                    as.character(stream_name),
      PACKAGE = "socialchoicelab"))
}

#' Generate a spatial profile with voter ideals drawn from N(mean, stddev²)
#'
#' Both voter ideal points and alternative positions are drawn from the same
#' normal distribution per dimension. Only 2D is supported.
#'
#' @param n_voters Integer. Number of voters.
#' @param n_alts Integer. Number of alternatives.
#' @param mean,stddev Numeric. Normal distribution parameters.
#' @param dist_config Distance configuration from \code{\link{make_dist_config}}.
#' @param stream_manager A \code{\link{StreamManager}} object.
#' @param stream_name Character. Named stream to use for randomness.
#' @return A \code{\link{Profile}} object.
#' @export
profile_gaussian_spatial <- function(n_voters, n_alts,
                                     mean = 0.0, stddev = 1.0,
                                     dist_config    = make_dist_config(),
                                     stream_manager,
                                     stream_name) {
  Profile$new(.Call("r_scs_profile_gaussian_spatial",
                    as.integer(n_voters), as.integer(n_alts),
                    as.double(mean), as.double(stddev),
                    dist_config,
                    .sm_ptr(stream_manager),
                    as.character(stream_name),
      PACKAGE = "socialchoicelab"))
}
