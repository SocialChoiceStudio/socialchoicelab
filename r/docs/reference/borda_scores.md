# Compute Borda scores

Each voter awards `n_alts - 1` points to their top alternative,
`n_alts - 2` to the second, and so on.

## Usage

``` r
borda_scores(profile)
```

## Arguments

- profile:

  A
  [`Profile`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/Profile.md)
  object.

## Value

Integer vector of length n_alts. Names are `"alt1", "alt2", ...`.

## Examples

``` r
if (FALSE) { # \dontrun{
alts   <- c(0, 0, 2, 0, -2, 0)
voters <- c(-1, -1, 1, -1, 1, 1)
prof   <- profile_build_spatial(alts, voters)
borda_scores(prof)
} # }
```
