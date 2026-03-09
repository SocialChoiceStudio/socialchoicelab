# Test whether the profile has a Condorcet winner

A Condorcet winner beats every other alternative by strict majority
(more than half of voters).

## Usage

``` r
has_condorcet_winner_profile(profile)
```

## Arguments

- profile:

  A
  [`Profile`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/Profile.md)
  object.

## Value

Logical scalar.

## Examples

``` r
if (FALSE) { # \dontrun{
prof <- profile_build_spatial(c(0, 0, 2, 0, -2, 0), c(-1, -1, 1, -1, 1, 1))
has_condorcet_winner_profile(prof)
} # }
```
