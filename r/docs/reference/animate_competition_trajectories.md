# Animate 2D competition trajectories from a CompetitionTrace

Animate 2D competition trajectories from a CompetitionTrace

## Usage

``` r
animate_competition_trajectories(
  trace,
  voters = numeric(0),
  competitor_names = NULL,
  title = "Competition Trajectories",
  theme = "dark2",
  width = 700L,
  height = 600L,
  trail = c("fade", "full", "none"),
  trail_length = "medium"
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

- trail:

  Trail style for the animated paths. Three modes are available:

  `"fade"`

  :   (default) Each step shows a short fading trail of recent segments
      behind each competitor. Opacity decays exponentially from the
      current position outward; segments older than `trail_length` are
      hidden. Conveys direction and recent history without cluttering
      the plot.

  `"full"`

  :   The complete path from round 1 to the current step is drawn as a
      continuous line. Useful for inspecting the entire trajectory, but
      can become visually busy in long competitions.

  `"none"`

  :   Markers only; no path segments are drawn. Cleanest output,
      suitable when only final positions matter or the animation is
      embedded in a space-constrained layout.

- trail_length:

  Controls how many past segments are shown when `trail = "fade"`.
  Accepts:

  `"short"`

  :   The most recent 1/3 of all rounds. Good for dense or fast-moving
      competitions.

  `"medium"`

  :   The most recent 1/2 of all rounds (default). Balances recency and
      context for most competition lengths.

  `"long"`

  :   The most recent 3/4 of all rounds. Useful when the full trajectory
      arc matters but `"full"` is too cluttered.

  positive integer

  :   Exact number of segments to retain, independent of total round
      count.

  Ignored when `trail` is `"none"` or `"full"`.

## Value

A `plotly` figure object with animation frames.
