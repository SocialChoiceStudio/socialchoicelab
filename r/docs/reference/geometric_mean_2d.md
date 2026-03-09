# Coordinate-wise geometric mean of voter ideal points

Each output coordinate is `exp(mean(log(xᵢ)))`. Requires all coordinates
to be strictly positive; throws an error for zero or negative values
(e.g. NOMINATE-scale `[-1, 1]` data) — use
[`centroid_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/centroid_2d.md)
or
[`marginal_median_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/marginal_median_2d.md)
instead.

## Usage

``` r
geometric_mean_2d(voter_ideals)
```

## Arguments

- voter_ideals:

  Flat numeric vector `[x0, y0, ...]` of voter ideal coordinates (length
  2 \* n_voters). All values must be \> 0.

## Value

Named list `list(x, y)` with the geometric mean coordinates.

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(1, 2, 4, 8)  # two voters: (1,2) and (4,8)
geometric_mean_2d(voters)  # list(x = 2, y = 4)
} # }
```
