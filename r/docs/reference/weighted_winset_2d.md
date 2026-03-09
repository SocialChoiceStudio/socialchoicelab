# Compute the weighted-majority 2D winset of a status quo

Like
[`winset_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/winset_2d.md)
but with voter weights: a policy beats the status quo if the total
weight of its supporters meets or exceeds `threshold * sum(weights)`.

## Usage

``` r
weighted_winset_2d(
  status_quo,
  voter_ideals,
  weights,
  threshold = 0.5,
  dist_config = make_dist_config(),
  n_samples = 64L
)
```

## Arguments

- status_quo:

  Numeric vector of length 2.

- voter_ideals:

  Numeric vector `[x0, y0, ...]` of length 2 \* n_voters.

- weights:

  Positive numeric vector of voter weights (length = n_voters).

- threshold:

  Weight fraction in (0, 1\] required for majority. Use 0.5 for simple
  weighted majority.

- dist_config:

  Distance configuration from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md).

- n_samples:

  Boundary quality (\>= 4). Default: 64.

## Value

A
[`Winset`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/Winset.md)
object.

## Examples

``` r
if (FALSE) { # \dontrun{
voters  <- c(-1, -1, 1, -1, 1, 1)
weights <- c(10, 1, 1)
ws <- weighted_winset_2d(c(0, 0), voters, weights)
ws$is_empty()
} # }
```
