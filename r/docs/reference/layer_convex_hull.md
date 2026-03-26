# Add a Pareto set layer (convex hull of ideals under Euclidean preferences)

The canvas labels this layer “Pareto Set”. For Euclidean distance-based
utilities, the Pareto-efficient ideal points are exactly the vertices of
the convex hull of voter ideals; the payload still stores geometry as
`convex_hull_xy`.

## Usage

``` r
layer_convex_hull(
  fig,
  hull_xy,
  fill_color = NULL,
  line_color = NULL,
  fill_colour = NULL,
  line_colour = NULL,
  name = "Pareto Set",
  theme = "dark2"
)
```

## Arguments

- fig:

  A widget from
  [`plot_spatial_voting`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md).

- hull_xy:

  Matrix `(n x 2)` with columns `x`, `y`.

- fill_color, line_color:

  Colours; `NULL` uses the theme default.

- fill_colour, line_colour:

  UK spellings accepted.

- name:

  Display name for API parity; the static canvas uses the fixed label
  “Pareto Set”.

- theme:

  Colour theme: `"dark2"` (default, ColorBrewer Dark2, colorblind-safe),
  `"set2"`, `"okabe_ito"`, `"paired"`, or `"bw"` (black-and-white
  print).

## Value

Updated widget.
