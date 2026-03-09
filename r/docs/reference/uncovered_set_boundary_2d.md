# Approximate the boundary of the continuous uncovered set

Uses a grid-based approximation. Increase `grid_resolution` for finer
boundaries at higher computation cost.

## Usage

``` r
uncovered_set_boundary_2d(
  voter_ideals,
  dist_config = make_dist_config(),
  grid_resolution = 15L,
  k = "simple"
)
```

## Arguments

- voter_ideals:

  Flat numeric vector of voter ideal coordinates.

- dist_config:

  Distance configuration from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md).

- grid_resolution:

  Integer. Grid resolution for the approximation. Default: 15 (the
  library's `SCS_DEFAULT_BOUNDARY_GRID_RESOLUTION`).

- k:

  Majority threshold.

## Value

Numeric matrix (n_pts × 2) with columns `x` and `y`.

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1, -1, 1, -1, 1, 1)
bnd <- uncovered_set_boundary_2d(voters, grid_resolution = 10L)
plot(bnd, type = "l", asp = 1)
} # }
```
