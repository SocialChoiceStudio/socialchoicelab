# Normalize a utility value to `[0, 1]`

Normalize a utility value to `[0, 1]`

## Usage

``` r
normalize_utility(utility, max_distance, loss_config = make_loss_config())
```

## Arguments

- utility:

  Raw utility from
  [`distance_to_utility`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/distance_to_utility.md).

- max_distance:

  Maximum possible distance (defines worst utility).

- loss_config:

  Loss configuration (must match how utility was computed).

## Value

Scalar double in `[0, 1]`.

## Examples

``` r
if (FALSE) { # \dontrun{
u <- distance_to_utility(1.5)
normalize_utility(u, max_distance = 3.0)
} # }
```
