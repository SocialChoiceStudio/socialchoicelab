# Add a winset polygon layer

Add a winset polygon layer

## Usage

``` r
layer_winset(
  fig,
  winset = NULL,
  fill_color = NULL,
  line_color = NULL,
  fill_colour = NULL,
  line_colour = NULL,
  name = "Winset",
  voters = NULL,
  sq = NULL,
  dist_config = NULL,
  theme = "dark2"
)
```

## Arguments

- fig:

  A widget from
  [`plot_spatial_voting`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md).

- winset:

  A
  [`Winset`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/Winset.md)
  object, or `NULL` for auto-compute.

- fill_color, line_color:

  Colours; `NULL` uses the theme default.

- fill_colour, line_colour:

  UK spellings accepted.

- name:

  Ignored (canvas legend uses fixed labels); retained for parity.

- voters, sq:

  Required when `winset` is `NULL`.

- dist_config:

  Used only when auto-computing the winset.

- theme:

  Colour theme: `"dark2"` (default, ColorBrewer Dark2, colorblind-safe),
  `"set2"`, `"okabe_ito"`, `"paired"`, or `"bw"` (black-and-white
  print).

## Value

Updated widget.
