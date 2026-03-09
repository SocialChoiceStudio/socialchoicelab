# Full social ordering by descending Borda score

Full social ordering by descending Borda score

## Usage

``` r
borda_ranking(
  profile,
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

Integer vector of length n_alts. `result[1]` is the most preferred
alternative (1-based).

## Examples

``` r
if (FALSE) { # \dontrun{
prof <- profile_build_spatial(c(0, 0, 2, 0, -2, 0), c(-1, -1, 1, -1, 1, 1))
borda_ranking(prof)  # full ordering, best first
} # }
```
