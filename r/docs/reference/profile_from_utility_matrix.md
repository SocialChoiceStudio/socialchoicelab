# Build a profile from a utility matrix

Build a profile from a utility matrix

## Usage

``` r
profile_from_utility_matrix(utilities, n_voters, n_alts)
```

## Arguments

- utilities:

  Numeric matrix (n_voters × n_alts), or a flat row-major vector of
  length n_voters \* n_alts. `utilities[v, a]` is the utility of voter
  `v` for alternative `a`. Higher utility = preferred.

- n_voters:

  Integer. Number of voters.

- n_alts:

  Integer. Number of alternatives.

## Value

A
[`Profile`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/Profile.md)
object.

## Examples

``` r
if (FALSE) { # \dontrun{
# 2 voters, 3 alternatives.
# Voter 1 prefers: alt1 > alt2 > alt3 (utilities 3, 2, 1).
# Voter 2 prefers: alt3 > alt1 > alt2 (utilities 2, 1, 3).
u    <- matrix(c(3, 2, 1, 2, 1, 3), nrow = 2, byrow = TRUE)
prof <- profile_from_utility_matrix(u, n_voters = 2L, n_alts = 3L)
prof$export()
} # }
```
