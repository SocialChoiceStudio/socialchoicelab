# voter_sampler.R — Standalone voter ideal-point generation.
#
# make_voter_sampler() creates a configuration object (a plain list with a
# class tag) that describes which distribution to draw from and its parameters.
# draw_voters() calls through to the C API to produce a flat numeric vector
# [x0, y0, x1, y1, ...] ready to pass as voter_ideals to competition_run()
# or profile_build_spatial().

#' @importFrom socialchoicelab draw_voters make_voter_sampler
NULL

#' Create a voter sampler configuration
#'
#' Constructs a configuration object that specifies a spatial distribution for
#' drawing voter ideal points.  Pass the result to \code{\link{draw_voters}}.
#'
#' @param kind Character scalar. Distribution to draw from: \code{"gaussian"}
#'   (default) or \code{"uniform"}.
#' @param mean Numeric scalar. Mean for the Gaussian distribution (ignored for
#'   \code{"uniform"}).
#' @param sd Numeric scalar (> 0). Standard deviation for the Gaussian
#'   distribution.
#' @param lo Numeric scalar. Lower bound for the Uniform distribution (ignored
#'   for \code{"gaussian"}).
#' @param hi Numeric scalar (> lo). Upper bound for the Uniform distribution.
#' @return An object of class \code{VoterSamplerConfig} (a list) for use with
#'   \code{\link{draw_voters}}.
#'
#' @examples
#' \dontrun{
#' s <- make_voter_sampler("gaussian", mean = 0, sd = 0.4)
#' s <- make_voter_sampler("uniform", lo = -1, hi = 1)
#' }
#' @export
make_voter_sampler <- function(kind = c("gaussian", "uniform"),
                               mean = 0.0, sd = 1.0,
                               lo = -1.0, hi = 1.0) {
  kind <- match.arg(kind)
  if (kind == "gaussian") {
    if (!is.finite(mean))
      stop("make_voter_sampler: mean must be finite.")
    if (!is.finite(sd) || sd <= 0)
      stop("make_voter_sampler: sd must be a positive finite number, got ", sd, ".")
    kind_int <- 1L
    param1   <- as.double(mean)
    param2   <- as.double(sd)
  } else {
    if (!is.finite(lo) || !is.finite(hi))
      stop("make_voter_sampler: lo and hi must be finite.")
    if (lo >= hi)
      stop("make_voter_sampler: lo must be < hi (got lo=", lo, ", hi=", hi, ").")
    kind_int <- 0L
    param1   <- as.double(lo)
    param2   <- as.double(hi)
  }
  structure(
    list(kind = kind_int, param1 = param1, param2 = param2),
    class = "VoterSamplerConfig"
  )
}

#' Draw voter ideal points from a spatial distribution
#'
#' Generates a flat numeric vector of voter ideal-point coordinates using the
#' distribution specified in \code{sampler}.  The result is in row-major
#' interleaved order: \code{[x0, y0, x1, y1, ...]}, which is the format
#' expected by \code{\link{competition_run}} and
#' \code{\link{profile_build_spatial}}.
#'
#' @param n Integer scalar. Number of voters to draw (>= 1).
#' @param sampler A \code{VoterSamplerConfig} object from
#'   \code{\link{make_voter_sampler}}.
#' @param stream_manager A \code{\link{StreamManager}} object.
#' @param stream_name Character scalar. Name of the stream to use for
#'   randomness (will be registered if not already present).
#' @param n_dims Integer scalar. Number of spatial dimensions (currently only
#'   2 is supported).
#' @return Numeric vector of length \code{n * n_dims} in row-major order
#'   \code{[x0, y0, x1, y1, ...]}.
#'
#' @examples
#' \dontrun{
#' sm     <- stream_manager(42L, "voters")
#' s      <- make_voter_sampler("gaussian", mean = 0, sd = 0.4)
#' voters <- draw_voters(100L, s, sm, "voters")
#' # voters is a length-200 numeric vector ready for competition_run()
#' }
#' @export
draw_voters <- function(n, sampler, stream_manager, stream_name,
                        n_dims = 2L) {
  if (!inherits(sampler, "VoterSamplerConfig"))
    stop("draw_voters: sampler must be a VoterSamplerConfig from make_voter_sampler().")
  .Call("r_scs_draw_voters",
        as.integer(n),
        as.integer(n_dims),
        sampler,
        .sm_ptr(stream_manager),
        as.character(stream_name),
        PACKAGE = "socialchoicelab")
}
