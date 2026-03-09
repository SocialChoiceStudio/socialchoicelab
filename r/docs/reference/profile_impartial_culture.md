# Generate a profile under the impartial culture model

Each voter's ranking is drawn independently and uniformly from all
`n_alts!` orderings (Fisher-Yates shuffle). Purely ordinal; no spatial
component.

## Usage

``` r
profile_impartial_culture(n_voters, n_alts, stream_manager, stream_name)
```

## Arguments

- n_voters:

  Integer. Number of voters.

- n_alts:

  Integer. Number of alternatives.

- stream_manager:

  A
  [`StreamManager`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/StreamManager.md)
  object.

- stream_name:

  Character. Named stream to use for randomness.

## Value

A
[`Profile`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/Profile.md)
object.

## Examples

``` r
if (FALSE) { # \dontrun{
sm   <- stream_manager(42, "draws")
prof <- profile_impartial_culture(n_voters = 50L, n_alts = 4L,
                                  stream_manager = sm, stream_name = "draws")
prof$dims()
} # }
```
