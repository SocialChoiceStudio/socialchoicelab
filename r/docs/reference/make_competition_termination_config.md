# Create a competition termination configuration

Create a competition termination configuration

## Usage

``` r
make_competition_termination_config(
  stop_on_convergence = FALSE,
  position_epsilon = 1e-08,
  stop_on_cycle = FALSE,
  cycle_memory = 50L,
  signature_resolution = 1e-06,
  stop_on_no_improvement = FALSE,
  no_improvement_window = 10L,
  objective_epsilon = 1e-08
)
```

## Arguments

- stop_on_convergence:

  Logical.

- position_epsilon:

  Numeric.

- stop_on_cycle:

  Logical.

- cycle_memory:

  Integer.

- signature_resolution:

  Numeric.

- stop_on_no_improvement:

  Logical.

- no_improvement_window:

  Integer.

- objective_epsilon:

  Numeric.

## Value

Named list accepted by competition wrappers.
