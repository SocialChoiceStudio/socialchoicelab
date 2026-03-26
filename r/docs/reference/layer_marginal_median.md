# Add a marginal median marker layer

Add a marginal median marker layer

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

  A widget from
  [`plot_spatial_voting`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md).

- voters:

  Flat voter vector.

- color:

  Marker colour; `NULL` uses theme default.

- name:

  Ignored for canvas drawing; retained for API parity.

- theme:

  Colour theme: `"dark2"` (default, ColorBrewer Dark2, colorblind-safe),
  `"set2"`, `"okabe_ito"`, `"paired"`, or `"bw"` (black-and-white
  print).

## Value

Updated widget.
