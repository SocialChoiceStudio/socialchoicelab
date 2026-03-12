# Animate 2D competition trajectories (canvas backend)

Renders the same competition animation as
[`animate_competition_trajectories`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/animate_competition_trajectories.md)
using a canvas-based widget. Data is sent once; the browser draws frames
on demand. This scales to long runs (hundreds or thousands of rounds)
without the large HTML and slow load of the Plotly frame-based approach.

## Usage

``` r
animate_competition_canvas(
  trace,
  voters = numeric(0),
  competitor_names = NULL,
  title = "Competition Trajectories",
  theme = "dark2",
  width = NULL,
  height = NULL,
  trail = c("fade", "full", "none"),
  trail_length = "medium",
  dim_names = c("Dimension 1", "Dimension 2"),
  overlays = NULL
)
```

## Arguments

- trace:

  A
  [`CompetitionTrace`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/CompetitionTrace.md)
  object.

- voters:

  Optional flat numeric voter vector or `numeric(0)`.

- competitor_names:

  Optional character labels for competitors.

- title:

  Plot title (used for layout; currently not drawn on canvas).

- theme:

  Colour theme (same options as
  [`plot_spatial_voting`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md)).

- width, height:

  Widget dimensions in pixels.

- trail:

  Trail style: `"fade"`, `"full"`, or `"none"`.

- trail_length:

  When `trail = "fade"`, either `"short"`, `"medium"`, `"long"`, or a
  positive integer.

- dim_names:

  Length-2 character vector for axis titles.

- overlays:

  Optional named list of static overlays to draw on every frame. Each
  element is the result of a geometry function:

  `centroid`

  :   From
      [`centroid_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/centroid_2d.md):
      `list(x,y)`.

  `marginal_median`

  :   From
      [`marginal_median_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/marginal_median_2d.md):
      `list(x,y)`.

  `geometric_mean`

  :   From
      [`geometric_mean_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/geometric_mean_2d.md):
      `list(x,y)`. Requires all voter coordinates to be strictly
      positive.

  `pareto_set`

  :   From
      [`convex_hull_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/convex_hull_2d.md):
      matrix of hull vertices. Valid only under Euclidean distance with
      uniform salience.

## Value

An `htmlwidget` object of class `competition_canvas`.
