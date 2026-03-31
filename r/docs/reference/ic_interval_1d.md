# Compute the 1D IC interval in a single call

Compound convenience function equivalent to calling
[`calculate_distance`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/calculate_distance.md),
[`distance_to_utility`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/distance_to_utility.md),
and
[`level_set_1d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/level_set_1d.md)
in sequence, but without intermediate round-trips across the C API
boundary. The distance configuration must be 1D (`n_weights == 1`).

## Usage

``` r
ic_interval_1d(
  ideal,
  reference_x,
  loss_config = make_loss_config(),
  dist_config = make_dist_config(n_dims = 1L)
)
```

## Arguments

- ideal:

  Voter ideal coordinate (scalar).

- reference_x:

  Reference coordinate (status quo or seat).

- loss_config:

  Loss configuration from
  [`make_loss_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_loss_config.md).

- dist_config:

  Distance configuration from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md)
  with `n_dims = 1L`.

## Value

Numeric vector of length 0, 1, or 2 (indifference points).
