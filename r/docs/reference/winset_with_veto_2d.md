# Compute the k-majority winset constrained by veto players

A veto player at point `v` blocks any move to point `x` unless `x` is
strictly inside `v`'s preferred-to set at the status quo.

## Usage

``` r
winset_with_veto_2d(
  status_quo,
  voter_ideals,
  veto_ideals = NULL,
  dist_config = make_dist_config(),
  k = "simple",
  n_samples = 64L
)
```

## Arguments

- status_quo:

  Numeric vector of length 2.

- voter_ideals:

  Numeric vector `[x0, y0, ...]` of length 2 \* n_voters.

- veto_ideals:

  Numeric vector of veto player ideal points `[x0, y0, ...]`, or `NULL`
  for no veto constraint.

- dist_config:

  Distance configuration from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md).

- k:

  Majority threshold: `"simple"` or a positive integer.

- n_samples:

  Boundary quality (\>= 4). Default: 64.

## Value

A
[`Winset`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/Winset.md)
object.

## Examples

``` r
if (FALSE) { # \dontrun{
voters <- c(-1, -1, 1, -1, 1, 1)
veto   <- c(0.5, 0.5)  # one veto player at (0.5, 0.5)
ws     <- winset_with_veto_2d(c(0, 0), voters, veto)
ws$is_empty()
} # }
```
