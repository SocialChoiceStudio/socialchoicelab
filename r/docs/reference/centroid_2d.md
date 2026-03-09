# Coordinate-wise arithmetic mean (centroid) of voter ideal points

Each output coordinate is the arithmetic mean of the corresponding input
coordinates (centre of mass / mean voter position). Not generally a
majority-rule equilibrium in 2D.

## Usage

``` r
centroid_2d(voter_ideals)
```

## Arguments

- voter_ideals:

  Flat numeric vector `[x0, y0, ...]` of voter ideal coordinates (length
  2 \* n_voters).

## Value

Named list `list(x, y)` with the centroid coordinates.

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1, 0, 1, 0, 0, 1)
centroid_2d(voters)  # list(x = 0, y = 1/3)
} # }
```
