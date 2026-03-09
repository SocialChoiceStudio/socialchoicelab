# Add a centroid (mean voter position) marker layer

Displays the coordinate-wise arithmetic mean of voter ideal points as a
labelled marker. Computed via
[`centroid_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/centroid_2d.md).

## Usage

``` r
layer_centroid(fig, voters, color = NULL, name = "Centroid", theme = "dark2")
```

## Arguments

- fig:

  A plotly figure from
  [`plot_spatial_voting`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md).

- voters:

  Flat numeric vector `[x0, y0, ...]` of voter ideal coordinates.

- color:

  Marker and text colour. `NULL` uses the theme slot for alternative
  points.

- name:

  Legend entry label.

- theme:

  Colour theme — see
  [`plot_spatial_voting`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md).

## Value

The updated plotly figure.

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
fig <- plot_spatial_voting(voters)
fig <- layer_centroid(fig, voters)
fig
} # }
```
