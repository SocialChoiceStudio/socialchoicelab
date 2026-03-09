# Return all anti-plurality winners

Return all anti-plurality winners

## Usage

``` r
antiplurality_all_winners(profile)
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
antiplurality_all_winners(prof)
} # }
```
