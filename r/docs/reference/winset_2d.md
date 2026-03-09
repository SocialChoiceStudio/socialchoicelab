# Compute the 2D k-majority winset of a status quo

Returns the set of all policy points that at least `k` voters prefer to
the status quo, under the given distance metric.

## Usage

``` r
winset_2d(
  status_quo,
  voter_ideals,
  dist_config = make_dist_config(),
  k = "simple",
  n_samples = 64L
)
```

## Arguments

- status_quo:

  Numeric vector of length 2: `c(x, y)`.

- voter_ideals:

  Numeric vector `[x0, y0, x1, y1, ...]` of length 2 \* n_voters.

- dist_config:

  Distance configuration from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md).

- k:

  Majority threshold: `"simple"` for ⌊n/2⌋ + 1 voters, or a positive
  integer.

- n_samples:

  Boundary approximation quality (\>= 4). Higher values give smoother
  boundaries at the cost of computation. Default: 64.

## Value

A
[`Winset`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/Winset.md)
object.

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1, -1, 1, -1, 1, 1)
ws <- winset_2d(c(0, 0), voters)
ws$is_empty()
ws$bbox()
} # }
```
