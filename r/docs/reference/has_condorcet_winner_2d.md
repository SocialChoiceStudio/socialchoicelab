# Test whether a Condorcet winner exists in a finite alternative set

A Condorcet winner beats every other alternative under a `k`-majority.

## Usage

``` r
has_condorcet_winner_2d(
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

Logical scalar.

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1, -1, 1, -1, 1, 1)
alts   <- c(0, 0, 2, 0, -2, 0)
has_condorcet_winner_2d(alts, voters)
} # }
```
