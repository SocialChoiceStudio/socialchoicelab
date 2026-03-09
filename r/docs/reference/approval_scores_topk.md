# Compute approval scores under the ordinal top-k model

Each voter approves their top `k` ranked alternatives.

## Usage

``` r
approval_scores_topk(profile, k)
```

## Arguments

- profile:

  A
  [`Profile`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/Profile.md)
  object.

- k:

  Integer in `[1, n_alts]`.

## Value

Integer vector of length n_alts (approval counts).

## Examples

``` r
if (FALSE) { # \dontrun{
prof <- profile_build_spatial(c(0, 0, 2, 0, -2, 0), c(-1, -1, 1, -1, 1, 1))
approval_scores_topk(prof, k = 1L)  # same as plurality scores
} # }
```
