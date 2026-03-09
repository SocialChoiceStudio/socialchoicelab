# Return a single plurality winner with tie-breaking

Return a single plurality winner with tie-breaking

## Usage

``` r
plurality_one_winner(
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

Integer scalar (1-based alternative index).

## Examples

``` r
if (FALSE) { # \dontrun{
alts   <- c(0, 0, 2, 0, -2, 0)
voters <- c(-1, -1, 1, -1, 1, 1)
prof   <- profile_build_spatial(alts, voters)
plurality_one_winner(prof)

# With random tie-breaking:
sm <- stream_manager(42, "ties")
plurality_one_winner(prof, tie_break = "random",
                     stream_manager = sm, stream_name = "ties")
} # }
```
