# Return all plurality winners (alternatives tied for most first-place votes)

Return all plurality winners (alternatives tied for most first-place
votes)

## Usage

``` r
plurality_all_winners(profile)
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
alts   <- c(0, 0, 2, 0, -2, 0)
voters <- c(-1, -1, 1, -1, 1, 1)
prof   <- profile_build_spatial(alts, voters)
plurality_all_winners(prof)
} # }
```
