# Draw voter ideal points from a spatial distribution

Generates a flat numeric vector of voter ideal-point coordinates using
the distribution specified in `sampler`. The result is in row-major
interleaved order: `[x0, y0, x1, y1, ...]`, which is the format expected
by
[`competition_run`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/competition_run.md)
and
[`profile_build_spatial`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/profile_build_spatial.md).

## Usage

``` r
draw_voters(n, sampler, stream_manager, stream_name, n_dims = 2L)
```

## Arguments

- n:

  Integer scalar. Number of voters to draw (\>= 1).

- sampler:

  A `VoterSamplerConfig` object from
  [`make_voter_sampler`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_voter_sampler.md).

- stream_manager:

  A
  [`StreamManager`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/StreamManager.md)
  object.

- stream_name:

  Character scalar. Name of the stream to use for randomness (will be
  registered if not already present).

- n_dims:

  Integer scalar. Number of spatial dimensions (currently only 2 is
  supported).

## Value

Numeric vector of length `n * n_dims` in row-major order
`[x0, y0, x1, y1, ...]`.

## Examples

``` r
if (FALSE) { # \dontrun{
sm     <- stream_manager(42L, "voters")
s      <- make_voter_sampler("gaussian", mean = 0, sd = 0.4)
voters <- draw_voters(100L, s, sm, "voters")
# voters is a length-200 numeric vector ready for competition_run()
} # }
```
