# Run a spatial candidate competition simulation

Run a spatial candidate competition simulation

## Usage

``` r
competition_run(
  competitor_positions,
  strategy_kinds,
  voter_ideals,
  competitor_headings = NULL,
  dist_config = make_dist_config(n_dims = 1L),
  engine_config = make_competition_engine_config(),
  stream_manager = NULL
)
```

## Arguments

- competitor_positions:

  Numeric vector of length `n_competitors * n_dims`, row-major by
  competitor.

- strategy_kinds:

  Character vector. Entries are `"sticker"`, `"hunter"`, `"aggregator"`,
  or `"predator"`.

- voter_ideals:

  Numeric vector of length `n_voters * n_dims`.

- competitor_headings:

  Optional numeric vector matching `competitor_positions`, or `NULL`.

- dist_config:

  Distance configuration from
  [`make_dist_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md).

- engine_config:

  Competition engine configuration from
  [`make_competition_engine_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_competition_engine_config.md).

- stream_manager:

  Optional
  [`StreamManager`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/StreamManager.md)
  for stochastic strategies and step policies.

## Value

A
[`CompetitionTrace`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/CompetitionTrace.md)
object.
