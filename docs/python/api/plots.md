# Spatial Voting Visualization

Static spatial plots use an **HTML5 canvas** payload (`dict`) built by
`plot_spatial_voting()` and `layer_*()`; `save_plot()` writes self-contained HTML.
Competition animation uses `animate_competition_canvas()`.

```python
import socialchoicelab.plots as sclp
```

## Base plot

::: socialchoicelab.plots.plot_spatial_voting

## Layers

::: socialchoicelab.plots.layer_winset

::: socialchoicelab.plots.layer_yolk

::: socialchoicelab.plots.layer_uncovered_set

::: socialchoicelab.plots.layer_convex_hull
