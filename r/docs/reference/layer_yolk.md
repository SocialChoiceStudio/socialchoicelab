# Add a yolk circle layer

Draws the yolk as a filled circle.

## Usage

``` r
layer_yolk(
  fig,
  center_x,
  center_y,
  radius,
  fill_color = NULL,
  line_color = NULL,
  name = "Yolk",
  theme = "dark2"
)
```

## Arguments

- fig:

  A plotly figure from
  [`plot_spatial_voting`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md).

- center_x, center_y:

  Yolk center coordinates.

- radius:

  Yolk radius (same units as the plot axes).

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
voters <- c(-1.0, -0.5,  0.0, 0.0,  0.8, 0.6)
fig <- plot_spatial_voting(voters, c(0.0, 0.0))
fig <- layer_yolk(fig, center_x = 0.1, center_y = 0.05, radius = 0.3)
fig
} # }
```
