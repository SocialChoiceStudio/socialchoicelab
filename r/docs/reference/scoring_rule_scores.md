# Compute scores under a positional scoring rule

Compute scores under a positional scoring rule

## Usage

``` r
scoring_rule_scores(profile, score_weights)
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

Double vector of length n_alts.

## Examples

``` r
if (FALSE) { # \dontrun{
prof <- profile_build_spatial(c(0, 0, 2, 0, -2, 0), c(-1, -1, 1, -1, 1, 1))
# Borda weights for 3 alternatives: c(2, 1, 0)
scoring_rule_scores(prof, c(2, 1, 0))
} # }
```
