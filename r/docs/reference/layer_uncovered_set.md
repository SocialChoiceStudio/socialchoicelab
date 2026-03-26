# Add an uncovered set boundary layer

Add an uncovered set boundary layer

## Usage

``` r
layer_uncovered_set(
  fig,
  boundary_xy = NULL,
  fill_color = NULL,
  line_color = NULL,
  fill_colour = NULL,
  line_colour = NULL,
  name = "Uncovered Set",
  voters = NULL,
  grid_resolution = 20L,
  theme = "dark2"
)
```

## Arguments

- fig:

  A widget from
  [`plot_spatial_voting`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md).

- boundary_xy:

  Matrix with columns `x`, `y`, or `NULL` for auto-compute from
  `voters`.

- fill_color, line_color:

  Colours; `NULL` uses the theme default.

- fill_colour, line_colour:

  UK spellings accepted.

- name:

  Ignored (canvas legend uses fixed labels); retained for parity.

- grid_resolution:

  Grid resolution when auto-computing.

- theme:

  Colour theme: `"dark2"` (default, ColorBrewer Dark2, colorblind-safe),
  `"set2"`, `"okabe_ito"`, `"paired"`, or `"bw"` (black-and-white
  print).

## Value

Updated widget.
