# Add a marginal median marker layer

Displays the coordinate-wise median of voter ideal points as a labelled
marker (issue-by-issue median voter; Black 1948). Computed via
[`marginal_median_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/marginal_median_2d.md).

## Usage

``` r
layer_marginal_median(
  fig,
  voters,
  color = NULL,
  name = "Marginal Median",
  theme = "dark2"
)
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
fig <- layer_marginal_median(fig, voters)
fig
} # }
```
