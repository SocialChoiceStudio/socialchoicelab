# Return a single anti-plurality winner with tie-breaking

Return a single anti-plurality winner with tie-breaking

## Usage

``` r
antiplurality_one_winner(
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

Integer scalar (1-based).

## Examples

``` r
if (FALSE) { # \dontrun{
prof <- profile_build_spatial(c(0, 0, 2, 0, -2, 0), c(-1, -1, 1, -1, 1, 1))
antiplurality_one_winner(prof)
} # }
```
