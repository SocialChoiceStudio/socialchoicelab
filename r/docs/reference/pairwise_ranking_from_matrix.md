# Rank alternatives by Copeland score derived from a pairwise matrix

Rank alternatives by Copeland score derived from a pairwise matrix

## Usage

``` r
pairwise_ranking_from_matrix(
  matrix,
  n_alts = nrow(matrix),
  tie_break = "smallest_index",
  stream_manager = NULL,
  stream_name = NULL
)
```

## Arguments

- matrix:

  Integer matrix (n_alts × n_alts) of pairwise results. Typically the
  output of
  [`pairwise_matrix_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/pairwise_matrix_2d.md).
  Values: `1` = WIN, `0` = TIE, `-1` = LOSS.

- n_alts:

  Integer. Number of alternatives. Defaults to `nrow(matrix)`.

- tie_break:

  `"smallest_index"` or `"random"`.

- stream_manager:

  A
  [`StreamManager`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/StreamManager.md)
  object.

- stream_name:

  Character.

## Value

Integer vector of 1-based alternative indices, best (highest Copeland
score) first.

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1, -1, 1, -1, 1, 1)
alts   <- c(0, 0, 2, 0, -2, 0)
mat    <- pairwise_matrix_2d(alts, voters)
pairwise_ranking_from_matrix(mat)
} # }
```
