# Add a convex hull layer

Overlays the convex hull boundary. If the hull is empty the figure is
returned unchanged.

## Usage

``` r
layer_convex_hull(
  fig,
  hull_xy,
  fill_color = NULL,
  line_color = NULL,
  name = "Convex Hull",
  theme = "dark2"
)
```

## Arguments

- fig:

  A plotly figure from
  [`plot_spatial_voting`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md).

- hull_xy:

  Numeric matrix (n_hull × 2) with columns `x` and `y`, as returned by
  [`convex_hull_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/convex_hull_2d.md).

- fill_color:

  Fill colour (CSS rgba). `NULL` uses the theme default.

- line_color:

  Outline colour (CSS rgba). `NULL` uses the theme default.

- name:

  Legend entry label.

- theme:

  Colour theme — see
  [`plot_spatial_voting`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md).

## Value

The updated plotly figure.

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1.0, -0.5,  0.0, 0.0,  0.8, 0.6,  -0.4, 0.8,  0.5, -0.7)
hull <- convex_hull_2d(voters)
fig  <- plot_spatial_voting(voters)
fig  <- layer_convex_hull(fig, hull)
fig
} # }
```
