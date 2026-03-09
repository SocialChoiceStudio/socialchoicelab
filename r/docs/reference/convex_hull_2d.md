# Compute the 2D convex hull of a set of points

Vertices are returned in counter-clockwise order. Under Euclidean
preferences the convex hull of voter ideal points is the Pareto set.

## Usage

``` r
convex_hull_2d(points)
```

## Arguments

- points:

  Flat numeric vector `[x0, y0, x1, y1, ...]` of length 2 \* n_points.

## Value

Numeric matrix (n_hull × 2) with columns `x` and `y`.

## Examples

``` r
if (FALSE) { # \dontrun{
# 5 points; the interior point (0.5, 0.5) is not on the hull.
pts  <- c(0, 0, 1, 0, 1, 1, 0, 1, 0.5, 0.5)
hull <- convex_hull_2d(pts)
nrow(hull)  # 4
} # }
```
