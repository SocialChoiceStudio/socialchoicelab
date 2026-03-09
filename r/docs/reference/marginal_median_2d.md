# Coordinate-wise median of voter ideal points (marginal median)

Each output coordinate is the median of the corresponding input
coordinates computed independently — the issue-by-issue median voter
(Black 1948). For an even number of voters the median is the average of
the two middle values. Not generally a majority-rule equilibrium in 2D.

## Usage

``` r
marginal_median_2d(voter_ideals)
```

## Arguments

- voter_ideals:

  Flat numeric vector `[x0, y0, ...]` of voter ideal coordinates (length
  2 \* n_voters).

## Value

Named list `list(x, y)` with the marginal median coordinates.

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1, -1, 1, -1, 0, 1)
marginal_median_2d(voters)  # list(x = 0, y = -1)
} # }
```
