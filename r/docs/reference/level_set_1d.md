# Compute the 1D level set (indifference points)

Returns the set of 1D points `x` where the utility at distance
`|x - ideal|` equals `utility_level`. There are 0, 1, or 2 such points.

## Usage

``` r
level_set_1d(
  ideal,
  weight = 1,
  utility_level,
  loss_config = make_loss_config()
)
```

## Arguments

- ideal:

  Ideal point coordinate (scalar).

- weight:

  Salience weight for the single dimension (\> 0).

- utility_level:

  Target utility level.

- loss_config:

  Loss configuration from
  [`make_loss_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_loss_config.md).

## Value

Numeric vector of length 0, 1, or 2.

## Examples

``` r
if (FALSE) { # \dontrun{
# Under linear loss, ideal at 0, utility -1: points at ±1.
level_set_1d(ideal = 0, weight = 1, utility_level = -1)
} # }
```
