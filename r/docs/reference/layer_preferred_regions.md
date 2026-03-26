# Add voter preferred-to regions

Add voter preferred-to regions

## Usage

``` r
layer_preferred_regions(
  fig,
  voters,
  sq,
  dist_config = NULL,
  color_by_voter = FALSE,
  fill_color = NULL,
  line_color = NULL,
  line_colour = NULL,
  palette = "auto",
  voter_names = NULL,
  name = "Preferred Region",
  theme = "dark2"
)
```

## Arguments

- fig:

  A widget from
  [`plot_spatial_voting`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md).

- voters:

  Flat numeric vector `[x0, y0, x1, y1, ...]` of voter ideal points
  (length 2 \* n_voters).

- sq:

  Numeric vector `c(x, y)` for the status quo, or `NULL`.

- dist_config:

  Distance metric from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md).

- color_by_voter:

  Per-voter colours from `palette` when `TRUE`.

- fill_color:

  Fill for closed curves; `NULL` means no fill unless
  `color_by_voter = TRUE`.

- line_colour, line_color:

  Outline colour when `color_by_voter = FALSE`.

- palette:

  Palette name when `color_by_voter = TRUE`.

- voter_names:

  Character vector of voter labels. `NULL` uses `"V1"`, `"V2"`, etc.

- name:

  Legend label when `color_by_voter = FALSE`.

- theme:

  Colour theme: `"dark2"` (default, ColorBrewer Dark2, colorblind-safe),
  `"set2"`, `"okabe_ito"`, `"paired"`, or `"bw"` (black-and-white
  print).

## Value

Updated widget.
