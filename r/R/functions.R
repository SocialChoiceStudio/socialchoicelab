# functions.R — B3.1 (version), B3.2 (distance/loss/level-set),
#               B3.3 (convex hull, majority preference)
#
# All spatial coordinate arguments use the flat row-major convention:
#   [x0, y0, x1, y1, ...] as a plain numeric vector of length 2 * n_points.

# ---------------------------------------------------------------------------
# B3.1 — API version
# ---------------------------------------------------------------------------

#' Query the compiled C API version
#'
#' @return Named list \code{list(major, minor, patch)}.
#' @examples
#' \dontrun{
#' # Requires libscs_api to be built. See the package README.
#' scs_version()
#' }
#' @export
scs_version <- function() {
  .Call("r_scs_api_version", NULL,
      PACKAGE = "socialchoicelab")
}

# ---------------------------------------------------------------------------
# B3.2 — Distance functions
# ---------------------------------------------------------------------------

#' Compute the distance between two n-dimensional points
#'
#' @param ideal Numeric vector of length n (ideal point coordinates).
#' @param alt Numeric vector of length n (alternative coordinates).
#' @param dist_config Distance configuration from \code{\link{make_dist_config}}.
#'   \code{n_weights} in \code{dist_config} must equal \code{length(ideal)}.
#' @return Scalar double.
#' @examples
#' \dontrun{
#' # Euclidean distance between origin and (3, 4) — result should be 5.
#' calculate_distance(c(0, 0), c(3, 4))
#'
#' # Manhattan distance:
#' calculate_distance(c(0, 0), c(3, 4), make_dist_config("manhattan"))
#' }
#' @export
calculate_distance <- function(ideal, alt, dist_config = make_dist_config()) {
  .Call("r_scs_calculate_distance",
        as.double(ideal), as.double(alt), dist_config,
      PACKAGE = "socialchoicelab")
}

#' Convert a distance to a utility value
#'
#' @param distance Non-negative scalar distance.
#' @param loss_config Loss configuration from \code{\link{make_loss_config}}.
#' @return Scalar double (≤ 0 for the supported loss functions).
#' @examples
#' \dontrun{
#' # Linear loss: utility at distance 0 is 0; at distance 2 is -2.
#' distance_to_utility(0)
#' distance_to_utility(2)
#'
#' # Quadratic loss:
#' distance_to_utility(2, make_loss_config("quadratic"))
#' }
#' @export
distance_to_utility <- function(distance, loss_config = make_loss_config()) {
  .Call("r_scs_distance_to_utility", as.double(distance), loss_config,
      PACKAGE = "socialchoicelab")
}

#' Normalize a utility value to \code{[0, 1]}
#'
#' @param utility Raw utility from \code{\link{distance_to_utility}}.
#' @param max_distance Maximum possible distance (defines worst utility).
#' @param loss_config Loss configuration (must match how utility was computed).
#' @return Scalar double in \code{[0, 1]}.
#' @examples
#' \dontrun{
#' u <- distance_to_utility(1.5)
#' normalize_utility(u, max_distance = 3.0)
#' }
#' @export
normalize_utility <- function(utility, max_distance,
                               loss_config = make_loss_config()) {
  .Call("r_scs_normalize_utility",
        as.double(utility), as.double(max_distance), loss_config,
      PACKAGE = "socialchoicelab")
}

# ---------------------------------------------------------------------------
# B3.2 — Level-set functions
# ---------------------------------------------------------------------------

#' Compute the 1D level set (indifference points)
#'
#' Returns the set of 1D points \code{x} where the utility at distance
#' \code{|x - ideal|} equals \code{utility_level}. There are 0, 1, or 2 such
#' points.
#'
#' @param ideal Ideal point coordinate (scalar).
#' @param weight Salience weight for the single dimension (> 0).
#' @param utility_level Target utility level.
#' @param loss_config Loss configuration from \code{\link{make_loss_config}}.
#' @return Numeric vector of length 0, 1, or 2.
#' @examples
#' \dontrun{
#' # Under linear loss, ideal at 0, utility -1: points at ±1.
#' level_set_1d(ideal = 0, weight = 1, utility_level = -1)
#' }
#' @export
level_set_1d <- function(ideal, weight = 1.0, utility_level,
                          loss_config = make_loss_config()) {
  .Call("r_scs_level_set_1d",
        as.double(ideal), as.double(weight), as.double(utility_level),
        loss_config,
      PACKAGE = "socialchoicelab")
}

#' Compute the exact 2D level set (indifference curve)
#'
#' Returns a named list describing the exact shape of the indifference curve
#' at \code{utility_level}. The shape depends on the distance and loss
#' configuration:
#' \describe{
#'   \item{circle}{Euclidean distance with equal salience weights.}
#'   \item{ellipse}{Euclidean distance with unequal weights.}
#'   \item{superellipse}{Minkowski distance with \code{1 < p < ∞}.}
#'   \item{polygon}{Manhattan or Chebyshev distance (4 vertices).}
#' }
#'
#' Pass the returned list to \code{\link{level_set_to_polygon}} to get sampled
#' vertices.
#'
#' @param ideal_x,ideal_y Ideal point coordinates.
#' @param utility_level Target utility level.
#' @param loss_config Loss configuration from \code{\link{make_loss_config}}.
#' @param dist_config Distance configuration from \code{\link{make_dist_config}}.
#' @return Named list with elements: \code{type} (character), \code{center_x},
#'   \code{center_y}, \code{param0}, \code{param1}, \code{exponent_p}
#'   (numerics), \code{vertices} (4 × 2 matrix for polygons, \code{NULL}
#'   otherwise).
#' @examples
#' \dontrun{
#' # Circle centred at (1, 2) with linear loss at utility -0.5.
#' ls <- level_set_2d(ideal_x = 1, ideal_y = 2, utility_level = -0.5)
#' ls$type  # "circle"
#' ls$param0  # radius ≈ 0.5
#' }
#' @export
level_set_2d <- function(ideal_x, ideal_y, utility_level,
                          loss_config = make_loss_config(),
                          dist_config = make_dist_config()) {
  .Call("r_scs_level_set_2d",
        as.double(ideal_x), as.double(ideal_y), as.double(utility_level),
        loss_config, dist_config,
      PACKAGE = "socialchoicelab")
}

#' Sample a 2D level set as a closed polygon
#'
#' Converts the result of \code{\link{level_set_2d}} to a matrix of sampled
#' vertex coordinates. For circle / ellipse / superellipse shapes,
#' \code{n_samples} points are distributed uniformly around the curve. For
#' polygon shapes the 4 exact vertices are returned regardless of
#' \code{n_samples}.
#'
#' @param level_set Named list returned by \code{\link{level_set_2d}}.
#' @param n_samples Integer >= 3. Number of sample points for smooth shapes.
#'   Default: 64.
#' @return Numeric matrix (n_vertices × 2) with columns \code{x} and \code{y}.
#' @examples
#' \dontrun{
#' ls   <- level_set_2d(ideal_x = 0, ideal_y = 0, utility_level = -1)
#' poly <- level_set_to_polygon(ls, n_samples = 32L)
#' plot(poly, type = "l", asp = 1)
#' }
#' @export
level_set_to_polygon <- function(level_set, n_samples = 64L) {
  .Call("r_scs_level_set_to_polygon", level_set, as.integer(n_samples),
      PACKAGE = "socialchoicelab")
}

#' Compute the IC boundary polygon in a single call
#'
#' Compound convenience function equivalent to calling
#' \code{\link{calculate_distance}}, \code{\link{distance_to_utility}},
#' \code{\link{level_set_2d}}, and \code{\link{level_set_to_polygon}} in
#' sequence, but without intermediate round-trips across the C API boundary.
#'
#' @param ideal_x,ideal_y Voter's ideal point coordinates.
#' @param sq_x,sq_y Reference point coordinates (status quo or seat position).
#' @param loss_config Loss configuration from \code{\link{make_loss_config}}.
#' @param dist_config Distance configuration from \code{\link{make_dist_config}}.
#' @param n_samples Integer >= 3. Boundary samples for smooth shapes.
#'   Ignored for polygon shapes (exact 4 vertices). Default: 64.
#' @return Numeric matrix (n_pts x 2) with columns \code{x} and \code{y}.
#' @export
ic_polygon_2d <- function(ideal_x, ideal_y, sq_x, sq_y,
                           loss_config = make_loss_config(),
                           dist_config = make_dist_config(),
                           n_samples = 64L) {
  .Call("r_scs_ic_polygon_2d",
        as.double(ideal_x), as.double(ideal_y),
        as.double(sq_x), as.double(sq_y),
        loss_config, dist_config, as.integer(n_samples),
        PACKAGE = "socialchoicelab")
}

# ---------------------------------------------------------------------------
# B3.3 — Convex hull
# ---------------------------------------------------------------------------

#' Compute the 2D convex hull of a set of points
#'
#' Vertices are returned in counter-clockwise order. Under Euclidean
#' preferences the convex hull of voter ideal points is the Pareto set.
#'
#' @param points Flat numeric vector \code{[x0, y0, x1, y1, ...]} of length
#'   2 * n_points.
#' @return Numeric matrix (n_hull × 2) with columns \code{x} and \code{y}.
#' @examples
#' \dontrun{
#' # 5 points; the interior point (0.5, 0.5) is not on the hull.
#' pts  <- c(0, 0, 1, 0, 1, 1, 0, 1, 0.5, 0.5)
#' hull <- convex_hull_2d(pts)
#' nrow(hull)  # 4
#' }
#' @export
convex_hull_2d <- function(points) {
  .Call("r_scs_convex_hull_2d", as.double(points),
      PACKAGE = "socialchoicelab")
}

# ---------------------------------------------------------------------------
# B3.3 — Majority preference
# ---------------------------------------------------------------------------

#' Test whether a k-majority of voters prefer point A to point B
#'
#' Voter \code{i} strictly prefers \code{A} iff
#' \code{d(ideal_i, A) < d(ideal_i, B)}. Ties in distance contribute 0 votes.
#'
#' @param a Numeric vector of length 2: \code{c(x, y)} of point A.
#' @param b Numeric vector of length 2: \code{c(x, y)} of point B.
#' @param voter_ideals Flat numeric vector \code{[x0, y0, ...]} of voter
#'   ideal points (length = 2 * n_voters).
#' @param dist_config Distance configuration from \code{\link{make_dist_config}}.
#' @param k Majority threshold: \code{"simple"} for ⌊n/2⌋ + 1, or a positive
#'   integer.
#' @return Logical scalar: \code{TRUE} if at least \code{k} voters prefer A.
#' @examples
#' \dontrun{
#' voters <- c(-1, -1, 1, -1, 1, 1)  # 3 voters
#' # A = (0,0) is closer to all 3 voters than B = (5,0).
#' majority_prefers_2d(c(0, 0), c(5, 0), voters)  # TRUE
#' }
#' @export
majority_prefers_2d <- function(a, b, voter_ideals,
                                 dist_config = make_dist_config(),
                                 k = "simple") {
  av <- as.double(a); bv <- as.double(b)
  .Call("r_scs_majority_prefers_2d",
        av[1], av[2], bv[1], bv[2],
        as.double(voter_ideals), dist_config, .resolve_k(k),
      PACKAGE = "socialchoicelab")
}

#' Compute the n_alts × n_alts pairwise preference matrix
#'
#' Entry \code{[i, j]} is \code{1} (WIN), \code{0} (TIE), or \code{-1}
#' (LOSS) from alternative \code{i}'s perspective. The matrix is
#' anti-symmetric: \code{M[i,j] = -M[j,i]}.
#'
#' @param alts Flat numeric vector \code{[x0, y0, ...]} of alternative
#'   coordinates (length = 2 * n_alts).
#' @param voter_ideals Flat numeric vector of voter ideal coordinates.
#' @param dist_config Distance configuration from \code{\link{make_dist_config}}.
#' @param k Majority threshold.
#' @return Integer matrix (n_alts × n_alts) with row and column names
#'   \code{"alt1", "alt2", ...}.
#' @examples
#' \dontrun{
#' voters <- c(-1, -1, 1, -1, 1, 1)
#' alts   <- c(0, 0, 2, 0, -2, 0)
#' mat <- pairwise_matrix_2d(alts, voters)
#' mat["alt1", "alt2"]  # 1 (alt1 beats alt2) or -1 or 0
#' }
#' @export
pairwise_matrix_2d <- function(alts, voter_ideals,
                                dist_config = make_dist_config(),
                                k = "simple") {
  alt_xy <- as.double(alts)
  n_alts <- as.integer(length(alt_xy) / 2L)
  result <- .Call("r_scs_pairwise_matrix_2d",
                  alt_xy, as.double(voter_ideals), dist_config, .resolve_k(k),
      PACKAGE = "socialchoicelab")
  alt_names <- paste0("alt", seq_len(n_alts))
  dimnames(result) <- list(alt_names, alt_names)
  result
}

#' Test whether the weighted majority prefers point A to point B
#'
#' Returns \code{TRUE} if the total weight of voters preferring A meets or
#' exceeds \code{threshold * sum(weights)}.
#'
#' @param a,b Numeric vectors of length 2: ideal-point coordinates.
#' @param voter_ideals Flat numeric vector of voter ideal coordinates.
#' @param weights Positive numeric vector of voter weights (length = n_voters).
#' @param dist_config Distance configuration from \code{\link{make_dist_config}}.
#' @param threshold Weight fraction in \code{(0, 1]}. Use 0.5 for simple
#'   weighted majority.
#' @return Logical scalar.
#' @examples
#' \dontrun{
#' voters  <- c(-1, -1, 1, -1, 1, 1)
#' weights <- c(10, 1, 1)  # first voter has most weight
#' # A = (-0.5,-0.5) is close to voter 1; B = (2, 0) is not.
#' weighted_majority_prefers_2d(c(-0.5, -0.5), c(2, 0), voters, weights)
#' }
#' @export
weighted_majority_prefers_2d <- function(a, b, voter_ideals, weights,
                                          dist_config = make_dist_config(),
                                          threshold = 0.5) {
  av <- as.double(a); bv <- as.double(b)
  .Call("r_scs_weighted_majority_prefers_2d",
        av[1], av[2], bv[1], bv[2],
        as.double(voter_ideals), as.double(weights),
        dist_config, as.double(threshold),
      PACKAGE = "socialchoicelab")
}
