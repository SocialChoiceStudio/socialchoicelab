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
  overlays = NULL,
  compute_ic = FALSE,
  ic_loss_config = NULL,
  ic_dist_config = NULL,
  ic_num_samples = 32L,
  ic_max_curves = 5000L,
  compute_winset = FALSE,
  winset_dist_config = NULL,
  winset_k = "simple",
  winset_num_samples = 64L,
  winset_max = 1000L,
  compute_voronoi = FALSE,
  voronoi_dist_config = NULL
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

- compute_ic:

  Logical. When `TRUE`, compute per-frame indifference curves for every
  voter through each seat holder's position and bundle them into the
  widget payload. A toggle is then shown in the player UI. Default
  `FALSE`. Requires `voters` to be provided.

- ic_loss_config:

  Loss configuration for IC computation (from
  [`make_loss_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_loss_config.md)).
  Default `NULL` uses linear loss with sensitivity 1, producing pure
  iso-distance curves that match competition vote logic.

- ic_dist_config:

  Distance configuration for IC computation (from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md)).
  Should match the `dist_config` used in
  [`competition_run`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/competition_run.md).
  Default `NULL` uses 2D Euclidean.

- ic_num_samples:

  Integer \>= 3. Number of polygon vertices sampled per indifference
  curve for smooth shapes (circle / ellipse / superellipse). Exact
  polygons always use 4 vertices regardless of this value. Default 32.

- ic_max_curves:

  Maximum total indifference curves allowed across all frames
  (`n_voters × seats_per_frame × n_frames`). Exceeding this raises an
  error. Default 2000.

- compute_winset:

  Logical. When `TRUE`, compute per-frame WinSet boundaries for each
  seat holder's current position and bundle them into the widget
  payload. A toggle is then shown in the player UI. Default `FALSE`.
  Requires `voters` to be provided. The WinSet is computed using the
  configured voting rule (`winset_k`) and distance metric
  (`winset_dist_config`); no assumption is made about which rule is in
  use.

- winset_dist_config:

  Distance configuration for WinSet computation (from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md)).
  Should match the `dist_config` used in
  [`competition_run`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/competition_run.md).
  Default `NULL` uses 2D Euclidean.

- winset_k:

  Voting rule threshold passed to
  [`winset_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/winset_2d.md).
  Use `"simple"` for simple majority, or a positive integer `k`. Default
  `"simple"`.

- winset_num_samples:

  Integer \>= 4. Boundary approximation quality (number of sample points
  on the WinSet boundary). Default `64L`.

- winset_max:

  Maximum total WinSet computations allowed
  (`seats_per_frame × n_frames`). Exceeding this raises an error.
  Default `1000L`.

- compute_voronoi:

  Logical. When `TRUE`, compute Euclidean Voronoi (candidate) regions
  per frame and show a "Voronoi" toggle in the canvas. Currently only
  Euclidean (L2, uniform weights) is supported; a non-Euclidean
  `voronoi_dist_config` raises an error. Default `FALSE`.

- voronoi_dist_config:

  Distance configuration for Voronoi (must be Euclidean when
  `compute_voronoi` is `TRUE`). Default `NULL` uses 2D Euclidean. If
  provided, must be Euclidean with uniform salience; otherwise an error
  is raised.

## Value

An `htmlwidget` object of class `competition_canvas`.
