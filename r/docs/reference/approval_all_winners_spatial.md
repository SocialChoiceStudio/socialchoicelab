# Return all winners under spatial approval voting

Return all winners under spatial approval voting

## Usage

``` r
approval_all_winners_spatial(
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

Integer vector of 1-based alternative indices (those with the highest
approval count, and at least one approver).

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1, -1, 1, -1, 1, 1)
alts   <- c(0, 0, 2, 0, -2, 0)
approval_all_winners_spatial(alts, voters, threshold = 1.5)
} # }
```
