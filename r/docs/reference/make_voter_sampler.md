# Create a voter sampler configuration

Constructs a configuration object that specifies a spatial distribution
for drawing voter ideal points. Pass the result to
[`draw_voters`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/draw_voters.md).

## Usage

``` r
make_voter_sampler(
  kind = c("gaussian", "uniform"),
  mean = 0,
  sd = 1,
  lo = -1,
  hi = 1
)
```

## Arguments

- kind:

  Character scalar. Distribution to draw from: `"gaussian"` (default) or
  `"uniform"`.

- mean:

  Numeric scalar. Mean for the Gaussian distribution (ignored for
  `"uniform"`).

- sd:

  Numeric scalar (\> 0). Standard deviation for the Gaussian
  distribution.

- lo:

  Numeric scalar. Lower bound for the Uniform distribution (ignored for
  `"gaussian"`).

- hi:

  Numeric scalar (\> lo). Upper bound for the Uniform distribution.

## Value

An object of class `VoterSamplerConfig` (a list) for use with
[`draw_voters`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/draw_voters.md).

## Examples

``` r
if (FALSE) { # \dontrun{
s <- make_voter_sampler("gaussian", mean = 0, sd = 0.4)
s <- make_voter_sampler("uniform", lo = -1, hi = 1)
} # }
```
