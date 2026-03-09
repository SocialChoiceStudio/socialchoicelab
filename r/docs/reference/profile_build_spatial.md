# Build an ordinal preference profile from a 2D spatial model

Each voter ranks alternatives by distance: closest first, ties broken by
smallest alternative index.

## Usage

``` r
profile_build_spatial(alt_xy, voter_ideals, dist_config = make_dist_config())
```

## Arguments

- alt_xy:

  Numeric vector `[x0, y0, x1, y1, ...]` of alternative coordinates
  (length = 2 \* n_alts).

- voter_ideals:

  Numeric vector `[x0, y0, ...]` of voter ideal point coordinates
  (length = 2 \* n_voters).

- dist_config:

  Distance configuration from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md).

## Value

A
[`Profile`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/Profile.md)
object.

## Examples

``` r
if (FALSE) { # \dontrun{
alts   <- c(0, 0, 2, 0, -2, 0)
voters <- c(-1, -1, 1, -1, 1, 1)
prof   <- profile_build_spatial(alts, voters)
prof$dims()
} # }
```
