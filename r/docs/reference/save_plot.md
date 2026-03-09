# Save a spatial voting plot to file

Exports a Plotly figure to HTML or an image format. HTML export uses
[`htmlwidgets::saveWidget()`](https://rdrr.io/pkg/htmlwidgets/man/saveWidget.html)
and requires no additional packages. Image export (`.png`, `.svg`,
`.pdf`) uses
[`plotly::save_image()`](https://rdrr.io/pkg/plotly/man/save_image.html)
which requires the `kaleido` Python package; see
<https://github.com/plotly/Kaleido> for installation.

## Usage

``` r
save_plot(fig, path, width = NULL, height = NULL)
```

## Arguments

- fig:

  A plotly figure.

- path:

  Output file path. Extension determines format: `.html` (recommended),
  `.png`, `.svg`, `.pdf`, `.jpeg`, or `.webp`.

- width, height:

  Optional pixel dimensions for image output.

## Value

`path` (invisibly).

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
fig    <- plot_spatial_voting(voters, sq = c(0.0, 0.0))
save_plot(fig, "my_plot.html")
} # }
```
