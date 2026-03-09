# Return the fill and line colours for a layer type in a theme

Returns the `(fill_color, line_color)` pair that `layer_*` functions
will use for a given layer type and theme. Useful when adding custom
Plotly traces that should match the rest of the plot.

## Usage

``` r
scl_theme_colors(layer_type, theme = "dark2")
```

## Arguments

- layer_type:

  One of `"winset"`, `"yolk"`, `"uncovered_set"`, `"convex_hull"`,
  `"voters"`, `"alternatives"`.

- theme:

  Theme name — same options as
  [`plot_spatial_voting()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md)'s
  `theme` argument: `"dark2"` (default), `"set2"`, `"okabe_ito"`,
  `"paired"`, `"bw"` (black-and-white print).

## Value

Named list with elements `fill` and `line` (RGBA strings).

## Examples

``` r
cols <- scl_theme_colors("winset", theme = "dark2")
cols$fill
#> [1] "rgba(27,158,119,0.28)"
cols$line
#> [1] "rgba(27,158,119,0.8)"
```
