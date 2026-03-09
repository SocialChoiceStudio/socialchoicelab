# Create a loss function configuration

Returns a named list understood by all C API wrappers that accept a loss
configuration. The element order is fixed by contract with
`build_loss_config()` in `src/scs_r_helpers.h`; do not reorder.

## Usage

``` r
make_loss_config(
  loss_type = "linear",
  sensitivity = 1,
  max_loss = 1,
  steepness = 1,
  threshold = 0
)
```

## Arguments

- loss_type:

  Character. One of `"linear"` (default), `"quadratic"`, `"gaussian"`,
  or `"threshold"`.

- sensitivity:

  Numeric. Scale factor α used by linear, quadratic, and threshold loss.

- max_loss:

  Numeric. Asymptote α used by Gaussian loss.

- steepness:

  Numeric. Rate parameter β used by Gaussian loss.

- threshold:

  Numeric. Indifference radius τ used by threshold loss.

## Value

A named list accepted by all functions that take a loss configuration
argument.

## Examples

``` r
# Linear loss (default):
cfg <- make_loss_config()

# Quadratic (proximity-squared) loss:
cfg_q <- make_loss_config(loss_type = "quadratic")

# Gaussian loss with custom parameters:
cfg_g <- make_loss_config(loss_type = "gaussian", max_loss = 2.0,
                          steepness = 0.5)
```
