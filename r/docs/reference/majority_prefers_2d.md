# Test whether a k-majority of voters prefer point A to point B

Voter `i` strictly prefers `A` iff `d(ideal_i, A) < d(ideal_i, B)`. Ties
in distance contribute 0 votes.

## Usage

``` r
majority_prefers_2d(
  a,
  b,
  voter_ideals,
  dist_config = make_dist_config(),
  k = "simple"
)
```

## Arguments

- a:

  Numeric vector of length 2: `c(x, y)` of point A.

- b:

  Numeric vector of length 2: `c(x, y)` of point B.

- voter_ideals:

  Flat numeric vector `[x0, y0, ...]` of voter ideal points (length = 2
  \* n_voters).

- dist_config:

  Distance configuration from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md).

- k:

  Majority threshold: `"simple"` for ⌊n/2⌋ + 1, or a positive integer.

## Value

Logical scalar: `TRUE` if at least `k` voters prefer A.

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1, -1, 1, -1, 1, 1)  # 3 voters
# A = (0,0) is closer to all 3 voters than B = (5,0).
majority_prefers_2d(c(0, 0), c(5, 0), voters)  # TRUE
} # }
```
