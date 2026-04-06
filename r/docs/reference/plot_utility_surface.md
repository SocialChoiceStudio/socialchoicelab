# Plot a voter's 3D utility surface over 2D policy space

Renders the utility function \\u(x, y)\\ as an interactive 3D surface
using Three.js (WebGL). The surface peaks at `(ideal_x, ideal_y)` and
falls off according to `loss_config` and `dist_config`. Horizontal
slices through the surface at fixed utility levels correspond to the 2D
indifference curves produced by
[`level_set_polygon_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/level_set_polygon_2d.md)
and
[`ic_polygon_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/ic_polygon_2d.md).

Drag the plot with the mouse to rotate the surface.

## Usage

``` r
plot_utility_surface(
  ideal_x,
  ideal_y,
  dist_config = make_dist_config(),
  loss_config = make_loss_config("quadratic"),
  xlim = c(-3, 3),
  ylim = c(-3, 3),
  resolution = 60L,
  n_levels = 5L,
  axis_labels = c("Dimension 1", "Dimension 2"),
  title = NULL,
  width = NULL,
  height = NULL
)
```

## Arguments

- ideal_x, ideal_y:

  Numeric scalars: voter ideal-point coordinates.

- dist_config:

  Distance configuration from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md).
  `n_weights` must be 2.

- loss_config:

  Loss configuration from
  [`make_loss_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_loss_config.md).

- xlim, ylim:

  Numeric vectors of length 2: policy-space extents. Defaults to
  `c(-3, 3)` for both axes.

- resolution:

  Integer: number of grid points per axis. Higher values produce
  smoother surfaces. Default `60L`.

- n_levels:

  Integer: number of indifference rings to draw on the surface. Default
  `5L`.

- axis_labels:

  Character vector of length 2: labels for the two policy dimensions.
  Default `c("Dimension 1", "Dimension 2")`.

- title:

  Optional character string for a plot title.

- width, height:

  Widget width and height in pixels.

## Value

An htmlwidget object.

## Examples

``` r
if (FALSE) { # \dontrun{
# Euclidean distance + quadratic loss (circular tent)
plot_utility_surface(
  ideal_x = 1, ideal_y = 0.5,
  dist_config = make_dist_config("euclidean"),
  loss_config = make_loss_config("quadratic")
)

# Manhattan distance produces a diamond-shaped tent
plot_utility_surface(
  ideal_x = 0, ideal_y = 0,
  dist_config = make_dist_config("manhattan"),
  loss_config = make_loss_config("linear"),
  axis_labels = c("Guns", "Butter"),
  title = "Voter C utility surface"
)
} # }
```
