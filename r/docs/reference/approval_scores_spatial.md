# Compute approval scores under the spatial threshold model

A voter approves an alternative if its distance from the voter's ideal
point is at most `threshold`.

## Usage

``` r
approval_scores_spatial(
  alts,
  voter_ideals,
  threshold,
  dist_config = make_dist_config()
)
```

## Arguments

- alts:

  Flat numeric vector `[x0, y0, ...]` of alternative coordinates (length
  = 2 \* n_alts).

- voter_ideals:

  Flat numeric vector of voter ideal coordinates.

- threshold:

  Non-negative approval radius.

- dist_config:

  Distance configuration from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md).

## Value

Integer vector of length n_alts (approval counts).

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1, -1, 1, -1, 1, 1)
alts   <- c(0, 0, 2, 0, -2, 0)
approval_scores_spatial(alts, voters, threshold = 1.5)
} # }
```
