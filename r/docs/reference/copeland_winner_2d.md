# Find the Copeland winner in a set of 2D alternatives

Ties are broken by smallest alternative index (1-based).

## Usage

``` r
copeland_winner_2d(
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

Integer scalar (1-based alternative index).

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1, -1, 1, -1, 1, 1)
alts   <- c(0, 0, 2, 0, -2, 0)
copeland_winner_2d(alts, voters)  # 1-based index of winner
} # }
```
