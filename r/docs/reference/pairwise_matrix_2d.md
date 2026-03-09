# Compute the n_alts × n_alts pairwise preference matrix

Entry `[i, j]` is `1` (WIN), `0` (TIE), or `-1` (LOSS) from alternative
`i`'s perspective. The matrix is anti-symmetric: `M[i,j] = -M[j,i]`.

## Usage

``` r
pairwise_matrix_2d(
  alts,
  voter_ideals,
  dist_config = make_dist_config(),
  k = "simple"
)
```

## Arguments

- alts:

  Flat numeric vector `[x0, y0, ...]` of alternative coordinates (length
  = 2 \* n_alts).

- voter_ideals:

  Flat numeric vector of voter ideal coordinates.

- dist_config:

  Distance configuration from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md).

- k:

  Majority threshold.

## Value

Integer matrix (n_alts × n_alts) with row and column names
`"alt1", "alt2", ...`.

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1, -1, 1, -1, 1, 1)
alts   <- c(0, 0, 2, 0, -2, 0)
mat <- pairwise_matrix_2d(alts, voters)
mat["alt1", "alt2"]  # 1 (alt1 beats alt2) or -1 or 0
} # }
```
