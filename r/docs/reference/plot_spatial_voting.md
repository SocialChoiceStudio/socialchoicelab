# Create a base 2D spatial voting plot

Plots voter ideal points and policy alternatives in a 2D issue space
using Plotly. Add optional geometric layers via
[`layer_winset`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/layer_winset.md),
[`layer_yolk`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/layer_yolk.md),
[`layer_uncovered_set`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/layer_uncovered_set.md),
[`layer_convex_hull`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/layer_convex_hull.md),
[`layer_ic`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/layer_ic.md),
and
[`layer_preferred_regions`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/layer_preferred_regions.md).

## Usage

``` r
plot_spatial_voting(
  voters,
  alternatives = numeric(0),
  sq = NULL,
  voter_names = NULL,
  alt_names = NULL,
  dim_names = c("Dimension 1", "Dimension 2"),
  title = "Spatial Voting Analysis",
  show_labels = FALSE,
  xlim = NULL,
  ylim = NULL,
  theme = "dark2",
  width = 700L,
  height = 600L
)
```

## Arguments

- voters:

  Flat numeric vector `[x0, y0, x1, y1, ...]` of voter ideal points
  (length 2 \* n_voters).

- alternatives:

  Flat numeric vector `[x0, y0, ...]` of policy alternative coordinates,
  or `numeric(0)`.

- sq:

  Numeric vector `c(x, y)` for the status quo, or `NULL`.

- voter_names:

  Character vector of voter labels. `NULL` uses `"V1"`, `"V2"`, etc.

- alt_names:

  Character vector of alternative labels. `NULL` uses `"Alt 1"`,
  `"Alt 2"`, etc.

- dim_names:

  Length-2 character vector for axis titles.

- title:

  Plot title string.

- show_labels:

  Logical. If `TRUE`, point labels are drawn on the graph. Default
  `FALSE`.

- xlim, ylim:

  Length-2 numeric vectors `c(min, max)` for explicit axis ranges.
  `NULL` auto-computes a padded range from the data.

- theme:

  Colour theme: `"dark2"` (default, ColorBrewer Dark2, colorblind-safe),
  `"set2"`, `"okabe_ito"`, `"paired"`, or `"bw"` (black-and-white
  print). Passed unchanged to each `layer_*` call for coordinated
  colours.

- width, height:

  Plot dimensions in pixels.

## Value

A `plotly` figure object.

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1.0, -0.5,  0.0, 0.0,  0.8, 0.6,  -0.4, 0.8,  0.5, -0.7)
sq     <- c(0.0,  0.0)
fig <- plot_spatial_voting(voters, sq = sq, theme = "dark2")
fig
} # }
```
