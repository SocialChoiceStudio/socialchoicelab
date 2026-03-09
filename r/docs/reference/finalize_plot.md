# Enforce the canonical layer stack order

Reapplies the package's canonical visual stack: regions, then lines,
then points, then overlays. It is primarily useful when users have
manipulated a figure directly and want to restore package ordering.

## Usage

``` r
finalize_plot(fig)
```

## Arguments

- fig:

  A plotly figure.

## Value

The same plotly figure unchanged.

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1.0, -0.5,  0.0, 0.0,  0.8, 0.6)
sq     <- c(0.0, 0.0)
fig <- plot_spatial_voting(voters, sq = sq)
fig <- finalize_plot(fig)
print(fig)
} # }
```
