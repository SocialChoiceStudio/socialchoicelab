# Return a single winner under a positional scoring rule with tie-breaking

Return a single winner under a positional scoring rule with tie-breaking

## Usage

``` r
scoring_rule_one_winner(
  profile,
  score_weights,
  tie_break = "smallest_index",
  stream_manager = NULL,
  stream_name = NULL
)
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

- tie_break:

  `"smallest_index"` (default) or `"random"`.

- stream_manager:

  A
  [`StreamManager`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/StreamManager.md)
  object; required when `tie_break = "random"`.

- stream_name:

  Character. Named stream for randomness; required when
  `tie_break = "random"`.

## Value

Integer scalar (1-based).

## Examples

``` r
if (FALSE) { # \dontrun{
prof <- profile_build_spatial(c(0, 0, 2, 0, -2, 0), c(-1, -1, 1, -1, 1, 1))
scoring_rule_one_winner(prof, c(2, 1, 0))
} # }
```
