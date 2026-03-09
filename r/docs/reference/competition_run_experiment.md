# Run replicated competition simulations

Run replicated competition simulations

## Usage

``` r
competition_run_experiment(
  competitor_positions,
  strategy_kinds,
  voter_ideals,
  competitor_headings = NULL,
  dist_config = make_dist_config(n_dims = 1L),
  engine_config = make_competition_engine_config(),
  master_seed = 1,
  num_runs = 10L,
  retain_traces = FALSE
)
```

## Arguments

- competitor_positions, strategy_kinds, voter_ideals,
  competitor_headings:

  See
  [`competition_run`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/competition_run.md).

- dist_config, engine_config:

  See
  [`competition_run`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/competition_run.md).

- master_seed:

  Numeric scalar used to derive per-run seeds.

- num_runs:

  Integer number of replications.

- retain_traces:

  Logical. Whether to retain full per-run traces inside the C++
  experiment result.

## Value

A
[`CompetitionExperiment`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/CompetitionExperiment.md)
object.
