# Plot 2D competition trajectories from a CompetitionTrace

Plot 2D competition trajectories from a CompetitionTrace

## Usage

``` r
plot_competition_trajectories(
  trace,
  voters = numeric(0),
  competitor_names = NULL,
  title = "Competition Trajectories",
  theme = "dark2",
  width = 700L,
  height = 600L
)
```

## Arguments

- trace:

  A
  [`CompetitionTrace`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/CompetitionTrace.md)
  object.

- voters:

  Optional flat numeric voter vector or `numeric(0)`.

- competitor_names:

  Optional character labels for competitors.

- title:

  Plot title.

- theme:

  Colour theme.

- width, height:

  Plot dimensions in pixels.

## Value

A `plotly` figure object.
