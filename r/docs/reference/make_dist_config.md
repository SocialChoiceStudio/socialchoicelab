# Create a distance configuration

Returns a named list understood by all C API wrappers that accept a
distance configuration. The list element order is fixed by contract with
the C helper `build_dist_config()` in `src/scs_r_helpers.h`; do not
reorder.

## Usage

``` r
make_dist_config(
  distance_type = "euclidean",
  weights = NULL,
  n_dims = 2L,
  order_p = 2
)
```

## Arguments

- distance_type:

  Character. One of `"euclidean"` (L2, default), `"manhattan"` (L1),
  `"chebyshev"` (L∞), or `"minkowski"` (general Lp).

- weights:

  Numeric vector of salience weights, one per dimension. Default:
  all-ones vector of length `n_dims`.

- n_dims:

  Integer. Number of dimensions; used only when `weights` is `NULL`.

- order_p:

  Numeric. Minkowski exponent p (\>= 1); ignored for other distance
  types.

## Value

A named list accepted by all functions that take a distance
configuration argument.

## Examples

``` r
# Euclidean (default):
cfg <- make_dist_config()

# Manhattan with equal weights:
cfg_l1 <- make_dist_config(distance_type = "manhattan")

# Euclidean with asymmetric salience weights:
cfg_w <- make_dist_config(weights = c(2.0, 1.0))
```
