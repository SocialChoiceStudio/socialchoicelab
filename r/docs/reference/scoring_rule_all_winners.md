# Return all winners under a positional scoring rule

Return all winners under a positional scoring rule

## Usage

``` r
scoring_rule_all_winners(profile, score_weights)
```

## Arguments

- profile:

  A
  [`Profile`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/Profile.md)
  object.

- score_weights:

  Non-increasing numeric vector of length n_alts. `score_weights[r]` is
  awarded to the alternative at rank `r` (rank 1 = most preferred,
  1-indexed).

## Value

Integer vector of 1-based alternative indices.

## Examples

``` r
if (FALSE) { # \dontrun{
prof <- profile_build_spatial(c(0, 0, 2, 0, -2, 0), c(-1, -1, 1, -1, 1, 1))
scoring_rule_all_winners(prof, c(2, 1, 0))
} # }
```
