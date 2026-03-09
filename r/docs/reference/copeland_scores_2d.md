# Compute Copeland scores for a set of 2D alternatives

The Copeland score of alternative `i` equals the number of alternatives
it beats in pairwise majority comparisons minus the number it loses to.

## Usage

``` r
copeland_scores_2d(
  alts,
  voter_ideals,
  dist_config = make_dist_config(),
  k = "simple"
)
```

## Arguments

- alts:

  Flat numeric vector `[x0, y0, ...]` of alternative coordinates (length
  = 2 \* n_alts).

- voter_ideals:

  Flat numeric vector of voter ideal coordinates.

- dist_config:

  Distance configuration from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md).

- k:

  Majority threshold: `"simple"` or a positive integer.

## Value

Integer vector of length n_alts. Names are `"alt1", "alt2", ...`.

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1, -1, 1, -1, 1, 1)
alts   <- c(0, 0, 2, 0, -2, 0)
copeland_scores_2d(alts, voters)
} # }
```
