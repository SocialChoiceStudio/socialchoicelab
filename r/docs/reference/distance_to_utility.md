# Convert a distance to a utility value

Convert a distance to a utility value

## Usage

``` r
distance_to_utility(distance, loss_config = make_loss_config())
```

## Arguments

- distance:

  Non-negative scalar distance.

- loss_config:

  Loss configuration from
  [`make_loss_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_loss_config.md).

## Value

Scalar double (≤ 0 for the supported loss functions).

## Examples

``` r
if (FALSE) { # \dontrun{
# Linear loss: utility at distance 0 is 0; at distance 2 is -2.
distance_to_utility(0)
distance_to_utility(2)

# Quadratic loss:
distance_to_utility(2, make_loss_config("quadratic"))
} # }
```
