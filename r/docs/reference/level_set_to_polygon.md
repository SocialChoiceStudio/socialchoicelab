# Sample a 2D level set as a closed polygon

Converts the result of
[`level_set_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/level_set_2d.md)
to a matrix of sampled vertex coordinates. For circle / ellipse /
superellipse shapes, `n_samples` points are distributed uniformly around
the curve. For polygon shapes the 4 exact vertices are returned
regardless of `n_samples`.

## Usage

``` r
level_set_to_polygon(level_set, n_samples = 64L)
```

## Arguments

- level_set:

  Named list returned by
  [`level_set_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/level_set_2d.md).

- n_samples:

  Integer \>= 3. Number of sample points for smooth shapes. Default: 64.

## Value

Numeric matrix (n_vertices × 2) with columns `x` and `y`.

## Examples

``` r
if (FALSE) { # \dontrun{
ls   <- level_set_2d(ideal_x = 0, ideal_y = 0, utility_level = -1)
poly <- level_set_to_polygon(ls, n_samples = 32L)
plot(poly, type = "l", asp = 1)
} # }
```
