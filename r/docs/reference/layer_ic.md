# Add voter indifference curves

Draws an indifference contour for each voter centred at their ideal
point, passing through the status quo. Under Euclidean distance (the
default) each contour is a circle; other metrics (Manhattan, Chebyshev,
Minkowski) produce their respective iso-distance shapes.

## Usage

``` r
layer_ic(
  fig,
  voters,
  sq,
  dist_config = NULL,
  color_by_voter = FALSE,
  fill_color = NULL,
  line_color = NULL,
  line_width = 1,
  palette = "auto",
  voter_names = NULL,
  name = "Indifference Curves",
  theme = "dark2"
)
```

## Arguments

- fig:

  A plotly figure from
  [`plot_spatial_voting`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md).

- voters:

  Flat numeric vector of voter ideal points.

- sq:

  Numeric vector `c(x, y)` for the status quo.

- dist_config:

  Distance metric configuration from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md).
  `NULL` (default) uses Euclidean distance and draws an efficient
  circle.

- color_by_voter:

  Logical. `FALSE` (default): all curves share a single neutral colour
  with one legend entry. `TRUE`: each voter gets a unique colour from
  `palette` shown individually in the legend.

- fill_color:

  Fill colour (CSS rgba string), or `NULL` for no fill (default).
  Overrides theme when not `NULL`.

- line_color:

  Uniform line colour used when `color_by_voter = FALSE`. `NULL` uses
  the theme default.

- line_width:

  Stroke width of the IC contours in pixels. Default `1`.

- palette:

  Palette name for `color_by_voter` mode. `"auto"` (default) uses the
  `theme`'s palette.

- voter_names:

  Character vector of voter labels.

- name:

  Legend group label (when `color_by_voter = FALSE`).

- theme:

  Colour theme — see
  [`plot_spatial_voting`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md).

## Value

The updated plotly figure.

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
sq     <- c(0.1, 0.1)
fig <- plot_spatial_voting(voters, sq = sq)
fig <- layer_ic(fig, voters, sq)
fig
} # }
```
