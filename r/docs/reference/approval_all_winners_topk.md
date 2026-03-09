# Return all winners under the top-k approval rule

Return all winners under the top-k approval rule

## Usage

``` r
approval_all_winners_topk(profile, k)
```

## Arguments

- profile:

  A
  [`Profile`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/Profile.md)
  object.

- k:

  Integer in `[1, n_alts]`.

## Value

Integer vector of 1-based alternative indices.

## Examples

``` r
if (FALSE) { # \dontrun{
prof <- profile_build_spatial(c(0, 0, 2, 0, -2, 0), c(-1, -1, 1, -1, 1, 1))
approval_all_winners_topk(prof, k = 2L)
} # }
```
