# Test whether the weighted majority prefers point A to point B

Returns `TRUE` if the total weight of voters preferring A meets or
exceeds `threshold * sum(weights)`.

## Usage

``` r
weighted_majority_prefers_2d(
  a,
  b,
  voter_ideals,
  weights,
  dist_config = make_dist_config(),
  threshold = 0.5
)
```

## Arguments

- a, b:

  Numeric vectors of length 2: ideal-point coordinates.

- voter_ideals:

  Flat numeric vector of voter ideal coordinates.

- weights:

  Positive numeric vector of voter weights (length = n_voters).

- dist_config:

  Distance configuration from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md).

- threshold:

  Weight fraction in `(0, 1]`. Use 0.5 for simple weighted majority.

## Value

Logical scalar.

## Examples

``` r
if (FALSE) { # \dontrun{
voters  <- c(-1, -1, 1, -1, 1, 1)
weights <- c(10, 1, 1)  # first voter has most weight
# A = (-0.5,-0.5) is close to voter 1; B = (2, 0) is not.
weighted_majority_prefers_2d(c(-0.5, -0.5), c(2, 0), voters, weights)
} # }
```
