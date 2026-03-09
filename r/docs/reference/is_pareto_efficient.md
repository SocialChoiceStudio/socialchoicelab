# Test whether a single alternative is Pareto-efficient

Test whether a single alternative is Pareto-efficient

## Usage

``` r
is_pareto_efficient(profile, alt)
```

## Arguments

- profile:

  A
  [`Profile`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/Profile.md)
  object.

- alt:

  Integer (1-based). The alternative to test.

## Value

Logical scalar.

## Examples

``` r
if (FALSE) { # \dontrun{
prof <- profile_build_spatial(c(0, 0, 2, 0, -2, 0), c(-1, -1, 1, -1, 1, 1))
is_pareto_efficient(prof, 1L)
} # }
```
