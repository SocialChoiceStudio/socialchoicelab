# Create a competition engine configuration

Create a competition engine configuration

## Usage

``` r
make_competition_engine_config(
  seat_count = 1L,
  seat_rule = "plurality_top_k",
  step_config = make_competition_step_config(),
  boundary_policy = "project_to_box",
  objective_kind = "vote_share",
  max_rounds = 10L,
  termination = make_competition_termination_config()
)
```

## Arguments

- seat_count:

  Integer number of seats.

- seat_rule:

  Character. One of `"plurality_top_k"` or `"hare_largest_remainder"`.

- step_config:

  A step config from
  [`make_competition_step_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_competition_step_config.md).

- boundary_policy:

  Character. One of `"project_to_box"`, `"stay_put"`, or `"reflect"`.

- objective_kind:

  Character. One of `"vote_share"` or `"seat_share"`.

- max_rounds:

  Integer.

- termination:

  A termination config from
  [`make_competition_termination_config`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_competition_termination_config.md).

## Value

Named list accepted by competition wrappers.
