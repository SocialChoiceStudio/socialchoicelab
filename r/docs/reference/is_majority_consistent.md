# Test whether an alternative is majority-consistent

An alternative is majority-consistent iff it IS the Condorcet winner
(i.e. it beats every other alternative by strict majority). Returns
`FALSE` for every candidate when no Condorcet winner exists.

## Usage

``` r
is_majority_consistent(profile, alt)
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
is_majority_consistent(prof, 1L)
} # }
```
