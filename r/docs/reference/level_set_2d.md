# Compute the exact 2D level set (indifference curve)

Returns a named list describing the exact shape of the indifference
curve at `utility_level`. The shape depends on the distance and loss
configuration:

- circle:

  Euclidean distance with equal salience weights.

- ellipse:

  Euclidean distance with unequal weights.

- superellipse:

  Minkowski distance with `1 < p < ∞`.

- polygon:

  Manhattan or Chebyshev distance (4 vertices).

## Usage

``` r
level_set_2d(
  ideal_x,
  ideal_y,
  utility_level,
  loss_config = make_loss_config(),
  dist_config = make_dist_config()
)
```

## Arguments

- ideal_x, ideal_y:

  Ideal point coordinates.

- utility_level:

  Target utility level.

- loss_config:

  Loss configuration from
  [`make_loss_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_loss_config.md).

- dist_config:

  Distance configuration from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md).

## Value

Named list with elements: `type` (character), `center_x`, `center_y`,
`param0`, `param1`, `exponent_p` (numerics), `vertices` (4 × 2 matrix
for polygons, `NULL` otherwise).

## Details

Pass the returned list to
[`level_set_to_polygon`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/level_set_to_polygon.md)
to get sampled vertices.

## Examples

``` r
if (FALSE) { # \dontrun{
# Circle centred at (1, 2) with linear loss at utility -0.5.
ls <- level_set_2d(ideal_x = 1, ideal_y = 2, utility_level = -0.5)
ls$type  # "circle"
ls$param0  # radius ≈ 0.5
} # }
```
