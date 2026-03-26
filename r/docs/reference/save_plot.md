# Save a spatial voting canvas widget to file

Only `.html` is supported. For raster or vector output, save HTML and
use the browser's screenshot or print-to-PDF tools.

## Usage

``` r
save_plot(fig, path, width = NULL, height = NULL)
```

## Arguments

- fig:

  Widget from
  [`plot_spatial_voting`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md)
  (and layers).

- path:

  Output path; extension `.html` is supported.

- width, height:

  Optional; reserved for future use (ignored).

## Value

`path` invisibly.
