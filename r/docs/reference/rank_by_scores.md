# Sort alternatives by descending score with tie-breaking

Produces a full ordering of all alternatives from a numeric score
vector.

## Usage

``` r
rank_by_scores(
  scores,
  tie_break = "smallest_index",
  stream_manager = NULL,
  stream_name = NULL
)
```

## Arguments

- scores:

  Numeric vector of scores (one per alternative, any order).

- tie_break:

  `"smallest_index"` (default) or `"random"`.

- stream_manager:

  A
  [`StreamManager`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/StreamManager.md)
  object; required when `tie_break = "random"`.

- stream_name:

  Character. Named stream for randomness.

## Value

Integer vector of 1-based alternative indices, best (highest score)
first.

## Examples

``` r
if (FALSE) { # \dontrun{
# Rank 3 alternatives by score: alt3 wins, alt1 second, alt2 last.
rank_by_scores(c(1.5, 0.5, 2.0))  # c(3, 1, 2)
} # }
```
