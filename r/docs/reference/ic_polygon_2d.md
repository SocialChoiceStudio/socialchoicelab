# Compute the IC boundary polygon in a single call

Compound convenience function equivalent to calling
[`calculate_distance`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/calculate_distance.md),
[`distance_to_utility`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/distance_to_utility.md),
[`level_set_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/level_set_2d.md),
and
[`level_set_to_polygon`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/level_set_to_polygon.md)
in sequence, but without intermediate round-trips across the C API
boundary.

## Usage

``` r
ic_polygon_2d(
  ideal_x,
  ideal_y,
  sq_x,
  sq_y,
  loss_config = make_loss_config(),
  dist_config = make_dist_config(),
  n_samples = 64L
)
```

## Arguments

- ideal_x, ideal_y:

  Voter's ideal point coordinates.

- sq_x, sq_y:

  Reference point coordinates (status quo or seat position).

- loss_config:

  Loss configuration from
  [`make_loss_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_loss_config.md).

- dist_config:

  Distance configuration from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md).

- n_samples:

  Integer \>= 3. Boundary samples for smooth shapes. Ignored for polygon
  shapes (exact 4 vertices). Default: 64.

## Value

Numeric matrix (n_pts x 2) with columns `x` and `y`.
