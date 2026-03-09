# Return the Condorcet winner from a profile

Return the Condorcet winner from a profile

## Usage

``` r
condorcet_winner_profile(profile)
```

## Arguments

- profile:

  A
  [`Profile`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/Profile.md)
  object.

## Value

Integer (1-based), or `NA_integer_` if no Condorcet winner.

## Examples

``` r
if (FALSE) { # \dontrun{
prof <- profile_build_spatial(c(0, 0, 2, 0, -2, 0), c(-1, -1, 1, -1, 1, 1))
w <- condorcet_winner_profile(prof)
if (!is.na(w)) cat("Condorcet winner: alt", w, "\n")
} # }
```
