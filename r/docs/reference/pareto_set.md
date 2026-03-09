# Return the Pareto set of a profile

The Pareto set contains all alternatives not Pareto-dominated by any
other alternative. It is always non-empty.

## Usage

``` r
pareto_set(profile)
```

## Arguments

- profile:

  A
  [`Profile`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/Profile.md)
  object.

## Value

Integer vector of 1-based alternative indices.

## Examples

``` r
if (FALSE) { # \dontrun{
prof <- profile_build_spatial(c(0, 0, 2, 0, -2, 0), c(-1, -1, 1, -1, 1, 1))
pareto_set(prof)
} # }
```
