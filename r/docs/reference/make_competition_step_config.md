# Create a competition step-policy configuration

Create a competition step-policy configuration

## Usage

``` r
make_competition_step_config(
  kind = "fixed",
  fixed_step_size = 1,
  min_step_size = 0,
  max_step_size = 1,
  proportionality_constant = 1,
  jitter = 0
)
```

## Arguments

- kind:

  Character. One of `"fixed"`, `"random_uniform"`, or
  `"share_delta_proportional"`.

- fixed_step_size:

  Numeric. Used when `kind = "fixed"`.

- min_step_size, max_step_size:

  Numeric. Bounds for random-uniform steps.

- proportionality_constant:

  Numeric. Scale for share-delta-proportional step sizing.

- jitter:

  Numeric. Uniform noise magnitude added to each step after the primary
  step size is computed. Default `0.0` (no jitter).

## Value

Named list accepted by competition wrappers.
