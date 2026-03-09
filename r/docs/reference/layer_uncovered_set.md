# Add an uncovered set boundary layer

Overlays the approximate uncovered set boundary. Pass either a
pre-computed matrix or raw `voters` for auto-compute. Returns the figure
unchanged if the boundary is empty.

## Usage

``` r
layer_uncovered_set(
  fig,
  boundary_xy = NULL,
  fill_color = NULL,
  line_color = NULL,
  name = "Uncovered Set",
  voters = NULL,
  grid_resolution = 20L,
  theme = "dark2"
)
```

## Arguments

- fig:

  A plotly figure from
  [`plot_spatial_voting`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md).

- boundary_xy:

  Numeric matrix (n_pts × 2) with columns `x` and `y`, or `NULL` for
  auto-compute.

- fill_color:

  Fill colour (CSS rgba). `NULL` uses the theme default.

- line_color:

  Outline colour (CSS rgba). `NULL` uses the theme default.

- name:

  Legend entry label.

- voters:

  Flat voter vector (required when `boundary_xy = NULL`).

- grid_resolution:

  Integer grid resolution for auto-compute (default 20).

- theme:

  Colour theme — see
  [`plot_spatial_voting`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md).

## Value

The updated plotly figure.

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1.0, -0.5,  0.0, 0.0,  0.8, 0.6,  -0.4, 0.8,  0.5, -0.7)
bnd <- uncovered_set_boundary_2d(voters, grid_resolution = 10L)
fig <- plot_spatial_voting(voters)
fig <- layer_uncovered_set(fig, bnd)
fig
} # }
```
