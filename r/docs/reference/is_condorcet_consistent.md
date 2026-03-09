# Test whether an alternative is Condorcet-consistent

Returns `TRUE` if either no Condorcet winner exists (vacuously
consistent) or the given alternative IS the Condorcet winner.

## Usage

``` r
is_condorcet_consistent(profile, alt)
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
is_condorcet_consistent(prof, 1L)
} # }
```
