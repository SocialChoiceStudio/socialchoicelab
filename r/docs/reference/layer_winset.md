# Add a winset polygon layer

Overlays the majority-rule winset boundary. Pass either a pre-computed
[`Winset`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/Winset.md)
object or raw `voters` and `sq` for auto-compute. If the winset is empty
the figure is returned unchanged.

## Usage

``` r
layer_winset(
  fig,
  winset = NULL,
  fill_color = NULL,
  line_color = NULL,
  name = "Winset",
  voters = NULL,
  sq = NULL,
  dist_config = NULL,
  theme = "dark2"
)
```

## Arguments

- fig:

  A plotly figure from
  [`plot_spatial_voting`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md).

- winset:

  A
  [`Winset`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/Winset.md)
  object, or `NULL` for auto-compute.

- fill_color:

  Fill colour (CSS rgba). `NULL` uses the theme default.

- line_color:

  Outline colour (CSS rgba). `NULL` uses the theme default.

- name:

  Legend entry label.

- voters:

  Flat voter vector (required when `winset = NULL`).

- sq:

  Status quo `c(x, y)` (required when `winset = NULL`).

- dist_config:

  Distance metric configuration from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md).
  `NULL` (default) uses Euclidean distance. Only used in the
  auto-compute path (`winset = NULL`); ignored when a pre-computed
  `winset` is supplied.

- theme:

  Colour theme — see
  [`plot_spatial_voting`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md).

## Value

The updated plotly figure.

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1.0, -0.5,  0.0, 0.0,  0.8, 0.6,  -0.4, 0.8,  0.5, -0.7)
sq     <- c(0.0,  0.0)
ws  <- winset_2d(sq, voters)
fig <- plot_spatial_voting(voters, sq = sq)
fig <- layer_winset(fig, ws)
fig
} # }
```
