# Sample a 2D level set as a polygon in one call

Compound convenience: same result as
[`level_set_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/level_set_2d.md)
followed by
[`level_set_to_polygon`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/level_set_to_polygon.md),
without passing the intermediate named list across the C boundary.

## Usage

``` r
level_set_polygon_2d(
  ideal_x,
  ideal_y,
  utility_level,
  loss_config = make_loss_config(),
  dist_config = make_dist_config(),
  n_samples = 64L
)
```

## Arguments

- ideal_x, ideal_y:

  Ideal point coordinates.

- utility_level:

  Target utility level (same as
  [`level_set_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/level_set_2d.md)).

- loss_config:

  Loss configuration from
  [`make_loss_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_loss_config.md).

- dist_config:

  Distance configuration from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md).

- n_samples:

  Integer. Sample count for smooth shapes; ignored for polygon level
  sets (Manhattan / Chebyshev). Default: 64.

## Value

Numeric matrix (n_pts × 2) with columns `x` and `y`.

## Examples

``` r
if (FALSE) { # \dontrun{
lc <- make_loss_config(loss_type = "linear", sensitivity = 1)
dc <- make_dist_config(distance_type = "euclidean", weights = c(1, 1))
poly <- level_set_polygon_2d(1, -0.5, utility_level = -0.5, lc, dc, n_samples = 32L)
plot(poly, type = "l", asp = 1)
} # }
```
