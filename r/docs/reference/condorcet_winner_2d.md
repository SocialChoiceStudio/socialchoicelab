# Find the Condorcet winner in a finite alternative set

Find the Condorcet winner in a finite alternative set

## Usage

``` r
condorcet_winner_2d(
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

Integer (1-based alternative index), or `NA_integer_` if no Condorcet
winner exists.

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1, -1, 1, -1, 1, 1)
alts   <- c(0, 0, 2, 0, -2, 0)
condorcet_winner_2d(alts, voters)  # NA if none, otherwise 1-based index
} # }
```
