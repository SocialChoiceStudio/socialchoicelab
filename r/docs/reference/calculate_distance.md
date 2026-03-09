# Compute the distance between two n-dimensional points

Compute the distance between two n-dimensional points

## Usage

``` r
calculate_distance(ideal, alt, dist_config = make_dist_config())
```

## Arguments

- ideal:

  Numeric vector of length n (ideal point coordinates).

- alt:

  Numeric vector of length n (alternative coordinates).

- dist_config:

  Distance configuration from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md).
  `n_weights` in `dist_config` must equal `length(ideal)`.

## Value

Scalar double.

## Examples

``` r
if (FALSE) { # \dontrun{
# Euclidean distance between origin and (3, 4) — result should be 5.
calculate_distance(c(0, 0), c(3, 4))

# Manhattan distance:
calculate_distance(c(0, 0), c(3, 4), make_dist_config("manhattan"))
} # }
```
