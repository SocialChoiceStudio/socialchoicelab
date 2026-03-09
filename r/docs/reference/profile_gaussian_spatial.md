# Generate a spatial profile with voter ideals drawn from N(mean, stddev²)

Both voter ideal points and alternative positions are drawn from the
same normal distribution per dimension. Only 2D is supported.

## Usage

``` r
profile_gaussian_spatial(
  n_voters,
  n_alts,
  mean = 0,
  stddev = 1,
  dist_config = make_dist_config(),
  stream_manager,
  stream_name
)
```

## Arguments

- n_voters:

  Integer. Number of voters.

- n_alts:

  Integer. Number of alternatives.

- mean, stddev:

  Numeric. Normal distribution parameters.

- dist_config:

  Distance configuration from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md).

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
sm   <- stream_manager(99, "gaussian")
prof <- profile_gaussian_spatial(n_voters = 30L, n_alts = 3L,
                                 mean = 0, stddev = 1,
                                 stream_manager = sm, stream_name = "gaussian")
prof$dims()
} # }
```
