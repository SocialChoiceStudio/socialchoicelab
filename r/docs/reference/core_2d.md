# Compute the core in continuous 2D space

The core is non-empty only when a Condorcet point exists in the policy
space. For most voter configurations the core is empty.

## Usage

``` r
core_2d(voter_ideals, dist_config = make_dist_config(), k = "simple")
```

## Arguments

- voter_ideals:

  Flat numeric vector of voter ideal coordinates.

- dist_config:

  Distance configuration from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md).

- k:

  Majority threshold.

## Value

Named list `list(found, x, y)`. `found` is `TRUE` if the core is
non-empty; `x` and `y` are `NA` when `found == FALSE`.

## Examples

``` r
if (FALSE) { # \dontrun{
# 3 symmetric voters: core will typically be empty.
voters <- c(-1, -1, 1, -1, 1, 1)
res <- core_2d(voters)
res$found  # TRUE or FALSE
if (res$found) cat("Core at:", res$x, res$y, "\n")
} # }
```
