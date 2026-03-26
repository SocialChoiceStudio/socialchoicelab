# Add a yolk circle layer

Add a yolk circle layer

## Usage

``` r
layer_yolk(
  fig,
  center_x,
  center_y,
  radius,
  fill_color = NULL,
  line_color = NULL,
  fill_colour = NULL,
  line_colour = NULL,
  name = "Yolk",
  theme = "dark2"
)
```

## Arguments

- fig:

  A widget from
  [`plot_spatial_voting`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md).

- center_x, center_y, radius:

  Circle geometry.

- fill_color, line_color:

  Colours; `NULL` uses the theme default.

- fill_colour, line_colour:

  UK spellings accepted.

- name:

  Ignored (canvas legend uses fixed labels); retained for parity.

- theme:

  Colour theme: `"dark2"` (default, ColorBrewer Dark2, colorblind-safe),
  `"set2"`, `"okabe_ito"`, `"paired"`, or `"bw"` (black-and-white
  print).

## Value

Updated widget.
