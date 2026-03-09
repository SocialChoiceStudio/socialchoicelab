# Load a built-in named scenario

Reads a bundled JSON scenario file and returns a list ready to pass to
[`plot_spatial_voting()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md)
and the computation functions.

## Usage

``` r
load_scenario(name)
```

## Arguments

- name:

  Character. Scenario identifier — one of the names returned by
  [`list_scenarios()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/list_scenarios.md)
  (e.g. `"laing_olmsted_bear"`).

## Value

A named list with elements:

- `name`:

  Full display name (character).

- `description`:

  Short description (character).

- `source`:

  Bibliographic source (character).

- `n_voters`:

  Number of voters (integer).

- `n_dimensions`:

  Number of policy dimensions (integer).

- `space`:

  Named list with `x_range` and `y_range` (each length-2 numeric).

- `decision_rule`:

  Majority threshold, e.g. `0.5` (numeric).

- `dim_names`:

  Axis labels (character vector, length `n_dimensions`).

- `voters`:

  Flat numeric vector `[x0, y0, x1, y1, ...]` of length
  `n_voters * n_dimensions`.

- `voter_names`:

  Character vector of length `n_voters`.

- `status_quo`:

  Numeric vector of length `n_dimensions`, or `NULL`.

## Examples

``` r
sc <- load_scenario("laing_olmsted_bear")
sc$n_voters          # 5
#> [1] 5
sc$voters            # flat [x0, y0, x1, y1, ...]
#>  [1]  20.8  79.6 100.4  69.2 121.2 115.2 100.4   6.4   7.2  20.0
sc$status_quo        # c(100, 100)
#> [1] 100 100

sc2 <- load_scenario("tovey_regular_polygon")
sc2$voter_names      # "XX0" .. "XX10"
#>  [1] "XX0"  "XX1"  "XX2"  "XX3"  "XX4"  "XX5"  "XX6"  "XX7"  "XX8"  "XX9" 
#> [11] "XX10"
```
