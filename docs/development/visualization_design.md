# Visualization Design

This document is the canonical specification for all spatial voting plots produced
by `socialchoicelab`. Both the R (`spatial_voting_canvas.R`) and Python
(`plots.py`) implementations must conform to it. Deviations require an update here first.

---

## Layer stack order

Layers are rendered bottom-to-top in the following fixed order. The **canvas
widget** (`spatial_voting_canvas.js`) applies this order when drawing, regardless
of the sequence in which `layer_*` functions append data to the payload.

| Z | Layer | Function | Status |
|---|---|---|---|
| 1 | Indifference curves / preferred regions | `layer_ic()`, `layer_preferred_regions()` | Implemented (C13.2, C13.6) |
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

Colours are managed by the **theme system** in `palette.R` / `palette.py`.
Each `layer_*` function and `plot_spatial_voting()` accepts a `theme` argument.
Layer functions also accept explicit `fill_color` / `line_color` overrides that
take priority over the theme.

### Available themes

| Theme | Source | Colorblind-safe? | Character |
|---|---|---|---|
| `"dark2"` **(default)** | ColorBrewer Dark2, 8 colours | Yes | Vivid, high contrast |
| `"set2"` | ColorBrewer Set2, 8 colours | Yes | Softer, pastel |
| `"okabe_ito"` | Okabe & Ito (2008), 7 colours | Yes | Print/B&W-safe |
| `"paired"` | ColorBrewer Paired, 12 colours | Moderate | Useful for n > 8 |
| `"bw"` | Greyscale | N/A | Black outlines, light-grey fills |

### Palette slot assignments

Each layer type is assigned a fixed slot index from the active palette so that
switching themes keeps the same semantic mapping.

| Slot | Layer type | Fill opacity | Line opacity |
|---|---|---|---|
| 0 | Win-set | 0.28 | 0.80 |
| 1 | Yolk | 0.22 | 0.70 |
| 2 | Uncovered set | 0.18 | 0.65 |
| 3 | Convex hull (Pareto) | 0.12 | 0.55 |
| 4 | Voter ideal points | 0.85 (solid) | — |
| 5 | Policy alternatives | 0.90 (solid) | — |

**Opacity increases with stack depth**: higher layers use more opaque colours so
they read as more visually prominent. The fill range is 0.12 → 0.28; the line
range is 0.55 → 0.80.

**Status quo** always uses near-black (`rgba(20,20,20,0.90)` or `rgba(0,0,0,0.90)`
in BW mode) — it is not drawn from the palette.

### Indifference curves and preferred regions

- **Uniform mode** (`color_by_voter = FALSE/False`): neutral slate-blue
  (`rgba(120,120,160,...)`) that coordinates with any theme.  IC line alpha 0.40;
  preferred-region fill 0.08, line 0.28.
- **Per-voter mode** (`color_by_voter = TRUE/True`): colours drawn from
  `scl_palette(palette, n_voters, alpha=...)`.  `palette="auto"` (default)
  resolves to the active `theme` palette.  IC line alpha 0.70, IC fill alpha 0.06;
  preferred-region line alpha 0.65, fill alpha 0.10.

### Point objects

| Object | Shape | Colour source |
|---|---|---|
| Voter ideal points | Circle | Palette slot 4, alpha 0.85 |
| Status quo | Star | Near-black (fixed) |
| Policy alternatives | Diamond | Palette slot 5, alpha 0.90 |

Points are always solid and differentiated by shape so they read clearly in
black-and-white.

On-graph text labels for voter/alternative points are **off by default**
(`show_labels = FALSE` in R, `show_labels=False` in Python). The legend entry
and hover tooltip are the primary identification mechanism.

**Status quo:** the SQ marker never receives an on-plot text label (no `"SQ"`
string), even when `show_labels` is `TRUE` for alternatives — use the legend
(`Status Quo`) and hover.

### Centrality overlays (`layer_centroid`, `layer_marginal_median`)

Symbols and colours are aligned with the **competition canvas** player:

| Measure | Canvas rendering | Notes |
|---------|------------------|--------|
| Centroid | Cross (two strokes) | Colours from theme helpers (`_centroid_overlay_color` / R equivalent). |
| Marginal median | Filled triangle-up | Outline colour for contrast on dark themes. |

### Non-Euclidean distance (`dist_config`)

When `dist_config` / `DistanceConfig` is **omitted**, `layer_ic()` and
`layer_preferred_regions()` use the historical Euclidean circle construction.
When **provided**, boundaries come from the core level-set pipeline
(`level_set_2d` → polygon) with linear loss, so Manhattan, Chebyshev, Minkowski
\(p\), etc. match the geometry layer.

`layer_winset()` auto-compute (`voters` + `sq`, no precomputed winset) passes
`dist_config` through to `winset_2d` so the plotted region matches the metric.

**Polygon closure:** for polygons from level sets, the first vertex is repeated
at the end of coordinate arrays so fills and strokes both close (same convention
as the core boundary export).

### Utility functions

```r
# R
colors <- scl_palette("dark2", n = 5L, alpha = 0.7)   # 5 RGBA strings
cols   <- scl_theme_colors("winset", theme = "dark2")  # list(fill=..., line=...)
```

```python
# Python
colors = sclp.scl_palette("dark2", n=5, alpha=0.7)     # list of 5 RGBA strings
fill, line = sclp.scl_theme_colors("winset", theme="dark2")
```

---

## Line types

A unique dash pattern is assigned to each area-object layer so that plots are
legible in black-and-white or greyscale.

| Layer | Dash style (canvas stroke) |
|---|---|
| Indifference curves | `"dot"` |
| Pareto / convex hull | `"dash"` |
| Heart | `"dashdot"` |
| Uncovered set | `"longdash"` |
| Yolk | `"longdashdot"` |
| Win-set | `"solid"` |
| Cut lines / Voronoi | `"dot"` |

---

## Display and export

- **R:** `print(fig)` renders the `spatial_voting_canvas` htmlwidget; `save_plot(fig, path)`
  writes self-contained HTML (`htmlwidgets::saveWidget`). Raster/vector export
  (`PNG`/`SVG`) is not implemented for canvas plots — save HTML and use the
  browser’s print or screenshot tools.
- **Python:** build the payload with `plot_spatial_voting` / `layer_*`, then
  `save_plot(fig, path)` for HTML. There is no `fig.show()`; open the saved file
  in a browser or use `IPython.display.HTML` in notebooks.

---

## Planned layers reference

| Layer | Key design notes |
|---|---|
| `layer_ic(fig, voters, sq, dist_config=…)` | One indifference curve per voter through SQ. **Euclidean (default):** circle, radius = d(voter, sq). **Non-Euclidean:** boundary from `level_set_2d` for the active `DistanceConfig`. Default: uniform colour. With `color_by_voter=TRUE`: unique colour per voter. Dash: `"dot"`. |
| `layer_heart(fig, ...)` | Filled region. Dash: `"dashdot"`. Fill/line colours TBD at implementation. |
| `layer_cut_lines(fig, ...)` | Line objects (no fill). Dash: `"dot"`. Includes perpendicular bisectors and Voronoi edges. |
