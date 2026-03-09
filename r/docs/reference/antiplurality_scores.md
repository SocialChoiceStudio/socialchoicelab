# Compute anti-plurality scores

`scores[j]` = number of voters for whom alternative `j` is NOT ranked
last. Higher is better.

## Usage

``` r
antiplurality_scores(profile)
```

## Arguments

- profile:

  A
  [`Profile`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/Profile.md)
  object.

## Value

Integer vector of length n_alts.

## Examples

``` r
if (FALSE) { # \dontrun{
prof <- profile_build_spatial(c(0, 0, 2, 0, -2, 0), c(-1, -1, 1, -1, 1, 1))
antiplurality_scores(prof)
} # }
```
