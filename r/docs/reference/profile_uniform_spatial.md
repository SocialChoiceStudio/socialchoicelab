# Generate a spatial profile with voters drawn from a uniform 2D distribution

Both voter ideal points and alternative positions are drawn from the
uniform distribution on `[lo, hi]^2`. Only 2D is supported.

## Usage

``` r
profile_uniform_spatial(
  n_voters,
  n_alts,
  lo = -1,
  hi = 1,
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

- lo:

  Numeric. Lower bound of the uniform distribution.

- hi:

  Numeric. Upper bound of the uniform distribution (must satisfy lo \<
  hi).

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
sm   <- stream_manager(42, "spatial")
prof <- profile_uniform_spatial(n_voters = 20L, n_alts = 3L,
                                lo = -1, hi = 1,
                                stream_manager = sm, stream_name = "spatial")
prof$dims()
} # }
```
