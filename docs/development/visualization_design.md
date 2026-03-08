# Visualization Design

This document is the canonical specification for all spatial voting plots produced
by `socialchoicelab`. Both the R (`plots.R`) and Python (`plots.py`) implementations
must conform to it. Deviations require an update here first.

---

## Layer stack order

Layers are rendered bottom-to-top in the following fixed order.
`finalize_plot()` / `finalize_plot()` enforces this order regardless of the
sequence in which `layer_*` functions are called.

| Z | Layer | Function | Status |
|---|---|---|---|
| 1 | Indifference curves | `layer_ic()` | Planned (C13.2) |
| 2 | Pareto set (convex hull) | `layer_convex_hull()` | Implemented |
| 3 | Heart | `layer_heart()` | Planned |
| 4 | Uncovered set | `layer_uncovered_set()` | Implemented |
| 5 | Yolk | `layer_yolk()` | Implemented |
| 6 | Win-set | `layer_winset()` | Implemented |
| 7 | Cut lines / Voronoi | `layer_cut_lines()` | Planned |
| 8 | Voter ideal points | (base plot) | Implemented |
| 9 | Status quo / Alternatives | (base plot) | Implemented |

---

## Color system

### Area objects (filled regions)

Each area object has an **outline colour** and a **fill colour**. The fill is
always a lighter, more transparent version of the outline using the same hue.

| Layer | Outline (line_color) | Fill (fill_color) |
|---|---|---|
| Pareto / convex hull | `rgba(130, 80,190, 0.55)` | `rgba(130, 80,190, 0.12)` |
| Uncovered set | `rgba( 30,150, 60, 0.65)` | `rgba( 50,180, 80, 0.18)` |
| Yolk | `rgba(210, 50, 50, 0.70)` | `rgba(210, 50, 50, 0.22)` |
| Win-set | `rgba( 50,100,220, 0.80)` | `rgba(100,150,255, 0.28)` |

**Opacity increases with stack depth**: higher layers are more opaque so they
read as more prominent. The fill opacity range is 0.12 → 0.28; the line opacity
range is 0.55 → 0.80.

### Indifference curves

- **Default**: all ICs drawn in a single neutral colour (`rgba(150,150,200,0.40)`).
- **`color_by_voter = TRUE/True`**: each voter receives a unique colour drawn
  from a categorical palette; each voter's colour appears in the legend. The
  same colour is used for the voter's IC and their ideal-point marker.

### Point objects

| Object | Shape | Fill |
|---|---|---|
| Voter ideal points | Circle | Solid `rgba(60,120,210,0.85)` |
| Status quo | Star | Solid `rgba(20,20,20,0.90)` |
| Policy alternatives | Diamond | Solid `rgba(210,70,30,0.90)` |

Points are always solid and differentiated by shape so they read clearly in
black-and-white.

On-graph text labels for points are **off by default** (`show_labels = FALSE`).
The legend entry and hover tooltip are the primary identification mechanism.

---

## Line types

A unique dash pattern is assigned to each area-object layer so that plots are
legible in black-and-white or greyscale.

| Layer | Plotly dash value |
|---|---|
| Indifference curves | `"dot"` |
| Pareto / convex hull | `"dash"` |
| Heart | `"dashdot"` |
| Uncovered set | `"longdash"` |
| Yolk | `"longdashdot"` |
| Win-set | `"solid"` |
| Cut lines / Voronoi | `"dot"` |

---

## `finalize_plot(fig)`

Reorders all traces in the figure to match the stack order table above.
Call once, immediately before `fig.show()` / `print(fig)`.

```r
# R
fig <- plot_spatial_voting(voters, sq = sq)
fig <- layer_winset(fig, ws)
fig <- layer_convex_hull(fig, hull)
fig <- finalize_plot(fig)   # correct stack order enforced here
print(fig)
```

```python
# Python
fig = sclp.plot_spatial_voting(voters, sq=sq)
fig = sclp.layer_winset(fig, ws)
fig = sclp.layer_convex_hull(fig, hull)
fig = sclp.finalize_plot(fig)   # correct stack order enforced here
fig.show()
```

Trace identity is determined by the `name` / `legendgroup` field that each
`layer_*` function sets. Custom traces added directly via the Plotly API and
not using one of the standard names are placed at z=7 (between cut lines and
voter points).

---

## Planned layers reference

| Layer | Key design notes |
|---|---|
| `layer_ic(fig, voters, sq)` | One circle per voter, radius = d(voter, sq). Default: uniform colour. With `color_by_voter=TRUE`: unique colour per voter shown in legend. Dash: `"dot"`. |
| `layer_heart(fig, ...)` | Filled region. Dash: `"dashdot"`. Fill/line colours TBD at implementation. |
| `layer_cut_lines(fig, ...)` | Line objects (no fill). Dash: `"dot"`. Includes perpendicular bisectors and Voronoi edges. |
