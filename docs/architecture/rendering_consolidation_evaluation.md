# Rendering backend consolidation evaluation

**Status:** Migration complete — R and Python use **HTML5 Canvas** for static spatial plots and competition animation; **Plotly is removed** from dependencies. The sections below retain the historical analysis that informed the decision.

Evaluate whether to consolidate the project's two rendering backends — Plotly
(static spatial voting plots) and JS Canvas 2D (competition animations) — onto
a single Canvas-based renderer, eliminating Plotly as a dependency.

## Decision (2026)

**Adopted:** Full migration to **HTML5 Canvas 2D** for static spatial voting
plots and competition animation. Shared primitives live in
`r/inst/htmlwidgets/scs_canvas_core.js`; static plots use
`spatial_voting_canvas.js` (R: `spatial_voting_canvas.R`, Python: payload dict
in `plots.py`). **Plotly**, **`finalize_plot()`**, and Plotly-only trajectory
helpers are **removed** from R and Python. **`save_plot()`** for static figures
writes **HTML** only; PNG/SVG are not implemented (users save HTML and print or
screenshot; future work may use `canvas.toDataURL` or canvas2svg — see below).

The remainder of this document preserves the **historical technical analysis**
that informed the decision.

---

## Previous state (pre-migration)

### Plotly (static spatial voting plots) — removed

Previously used in R (`plots.R`) and Python (`plots.py`) for static spatial
voting visualizations:

- `plot_spatial_voting()` — base 2D plot: voter ideal points, status quo,
  policy alternatives
- `layer_ic()` — indifference curves
- `layer_preferred_regions()` — preferred-to-SQ regions
- `layer_convex_hull()` — Pareto set
- `layer_winset()` — win-set polygon
- `layer_yolk()` — yolk circle
- `layer_uncovered_set()` — uncovered set polygon
- `layer_centroid()` — centroid point
- `layer_marginal_median()` — marginal median point
- `finalize_plot()` — (removed) enforced layer stacking order in Plotly
- `save_plot()` — previously exported to HTML or image (via Kaleido)

**Dependencies (removed):** R required `plotly >= 4.10.0`; Python required `plotly >= 5.0`
and `kaleido` for static image export. Both were runtime dependencies listed in
package metadata.

**Parity surface (historical):** Every `layer_*()` function was implemented twice
against Plotly's R and Python APIs and had to stay in sync.

### JS Canvas 2D (competition animations; now also static spatial plots)

Used in `r/inst/htmlwidgets/competition_canvas.js` (canonical) and
`python/src/socialchoicelab/competition_canvas.js` (copy). R binding in
`r/R/competition_canvas.R`; Python binding in
`python/src/socialchoicelab/plots.py` (`animate_competition_canvas()`).

Already renders:

- Voter ideal points (circles), candidate positions (diamonds)
- Grid, axes, axis labels
- Convex hull polygon (Pareto set overlay)
- Centroid, marginal median, geometric mean (point overlays)
- KDE heatmap (voter density)
- Animated trails, movement arrows, ghost positions
- Interactive controls: play/pause, scrub bar, keyboard navigation, zoom, pan

**Parity surface:** Both R and Python share the *same* JS file. Per-language
code only marshals data into a JSON payload and hands it to the widget. This
is a much thinner parity surface than the Plotly approach.

---

## Architectural difference: Plotly vs. Canvas

### Plotly renders to SVG, not Canvas

Plotly.js is built on **D3.js** and renders standard chart types as **SVG
elements in the DOM**. Each trace, axis, annotation, and legend item is an SVG
node. When Plotly "exports to SVG," it serializes the DOM subtree it already
built — there is no raster-to-vector conversion involved. PNG export rasterizes
that same SVG via Kaleido (a headless Chromium wrapper). The `scattergl` /
`heatmapgl` trace variants use WebGL for performance with large datasets, but
the standard 2D chart types used by this project are pure SVG.

### Our canvas widget renders to a bitmap

The competition canvas widget uses the **HTML5 Canvas 2D API**, which is an
immediate-mode raster rendering surface. Drawing calls (`fillRect`, `arc`,
`beginPath`, `lineTo`, `fill`, `stroke`) push pixels onto a bitmap. There is
no retained scene graph or DOM structure to serialize — once pixels are drawn,
the vector information that produced them is gone.

This distinction is the reason SVG/vector export requires a different strategy
for a Canvas-based renderer than for Plotly.

---

## Export strategy for a Canvas-based static widget

### PNG export (straightforward)

Canvas natively supports raster export:

```javascript
const dataUrl = canvas.toDataURL('image/png');
```

This produces a base64-encoded PNG at the canvas's pixel resolution. Sufficient
for screen display, web embedding, and many publication workflows. Already
conceptually available in the competition canvas widget.

### SVG / vector export (requires a bridge library)

The most directly applicable off-the-shelf solution is
**[canvas2svg](https://github.com/gliffy/canvas2svg)** (MIT license, ~730
GitHub stars).

**How it works:** canvas2svg provides a mock Canvas 2D rendering context that
implements the same API as `CanvasRenderingContext2D` — `beginPath`, `arc`,
`lineTo`, `fill`, `stroke`, `fillText`, `setTransform`, etc. — but instead of
pushing pixels to a bitmap, it builds an SVG scene graph behind the scenes.

```javascript
// Normal canvas rendering:
const ctx = canvas.getContext('2d');
ctx.beginPath();
ctx.arc(x, y, r, 0, 2 * Math.PI);
ctx.fillStyle = '#3366cc';
ctx.fill();

// canvas2svg rendering (identical API, SVG output):
const ctx = new C2S(width, height);
ctx.beginPath();
ctx.arc(x, y, r, 0, 2 * Math.PI);
ctx.fillStyle = '#3366cc';
ctx.fill();

const svgString = ctx.getSerializedSvg();
```

The key property is that **existing drawing code does not need to be
rewritten** — the same function that draws to a real canvas can draw to the
mock context and produce SVG.

**Caveats:**

- canvas2svg was last updated November 2023. It covers the core Canvas 2D API
  well but may not handle every feature (e.g., complex composite operations,
  certain gradient configurations). A spike must verify it handles the specific
  drawing calls our widget uses.
- Text rendering may differ slightly between Canvas (rasterized with system
  fonts) and SVG (vector text elements). Font metric differences could affect
  label placement.
- The library adds ~15 KB unminified. For a self-contained HTML widget this is
  negligible.

**Alternatives considered:**

- **SVG.js / D3.js:** Full SVG libraries that could be used to build a
  parallel SVG rendering path. More powerful but would require maintaining two
  codepaths (Canvas for interactive display, SVG for export) rather than using
  a single set of drawing calls against a swappable context.
- **Custom dual-context renderer:** For the simple geometric primitives this
  project uses (circles, polygons, lines, labeled points), a lightweight
  hand-rolled SVG emitter is feasible. However, canvas2svg already exists and
  handles the general case; building a custom solution is only justified if
  canvas2svg proves inadequate.
- **Headless browser screenshot:** Use Puppeteer or Playwright to render the
  canvas in a headless browser and capture SVG/PDF. Heavy dependency; only
  justified if canvas2svg fails and vector export is a hard requirement.

---

## Design challenges

### 1. Composability

The current Plotly API has a composable layer-stacking pattern:

```r
plot_spatial_voting(scenario) |>
  layer_ic(voter = 1) |>
  layer_winset() |>
  layer_yolk() |>
  finalize_plot()
```

Each `layer_*()` call adds traces to a Plotly figure object and returns the
modified figure. `finalize_plot()` enforces z-ordering.

A canvas-based replacement needs an equivalent composition mechanism. The
natural approach — already proven in the competition canvas — is a **JSON layer
specification**: each `layer_*()` call in R/Python appends to a list of layer
descriptors, and the JS widget receives the complete spec and renders all layers
in the specified order.

This is analogous to how `overlays_static` already works in the competition
canvas. The static spatial voting widget would extend this pattern with
additional layer types (indifference curves, preferred regions, win-set
polygons, yolk circles, etc.).

### 2. Tooltip / hover interactivity

Plotly provides hover tooltips with data values (voter ID, coordinates, etc.)
for free. The Canvas 2D API has no built-in hit-testing or tooltip system.

The competition canvas already does basic hit-testing for its controls. For
data-point hover in a static widget, the implementation would need:

- Spatial indexing of rendered elements (simple for the point counts involved —
  a flat list with distance checks is sufficient for < 1000 points).
- A floating HTML div positioned at the cursor, populated with the hovered
  element's data.
- Mouse-move event listener on the canvas.

This is standard practice for Canvas-based charting libraries and is not
technically difficult, but it is new code that must be written and tested.

### 3. Zoom and pan

The competition canvas already implements scroll-wheel zoom (map-style, around
cursor) and drag-to-pan with double-click reset. This infrastructure can be
reused directly for the static widget.

### 4. Responsive sizing

The competition canvas uses `ResizeObserver` for responsive layout. The same
approach applies to a static widget.

---

## What a Canvas-based static widget gains

- **Dependency elimination:** No Plotly, no Kaleido in either R or Python.
- **Thinner parity surface:** R and Python share the JS renderer; per-language
  code is limited to data serialization.
- **Full rendering control:** z-ordering, custom styling, pixel-perfect layout
  without working around Plotly's trace-stacking model or annotation system.
- **Consistency:** Static and animated visualizations share the same visual
  language, color system, and interaction patterns.
- **Lighter output:** A self-contained HTML file with Canvas JS is typically
  smaller than a Plotly HTML widget (which embeds the full plotly.js bundle).

## What a Canvas-based static widget costs

- **Implementation effort:** Designing the JSON layer spec, implementing all
  layer renderers in JS, wiring up hit-testing/tooltips, and porting
  `save_plot()` to use canvas2svg and `toDataURL`.
- **SVG export quality risk:** canvas2svg is a third-party library with no
  guaranteed long-term maintenance. If it proves inadequate, a custom SVG
  emitter or alternative approach may be needed.
- **Test migration:** Existing Plotly-based tests check for Plotly object
  structure; these would need to be replaced with tests against the new
  output format.
- **Learning curve for contributors:** Plotly is widely known; a custom Canvas
  widget is project-specific. Documentation and examples become more important.

---

## Evaluation plan (Phase 1 spike)

The spike should produce a working prototype and a clear go/no-go
recommendation. Scope:

1. **Build a minimal static canvas widget** that renders `plot_spatial_voting`
   (voter points, SQ, alternatives, axes, grid) + `layer_ic` (indifference
   curves) + `layer_winset` (win-set polygon). Use the htmlwidgets framework
   in R and self-contained HTML in Python, mirroring the competition canvas
   pattern.

2. **Implement the JSON layer spec** — R/Python `layer_*()` calls build a list
   of layer descriptors; the JS widget consumes them. Verify that the
   composable API ergonomics are acceptable.

3. **Implement basic hover tooltips** for voter points and candidate markers.

4. **Test export:**
   - PNG via `canvas.toDataURL()`.
   - SVG via canvas2svg. Verify output quality for circles, polygons, lines,
     and text labels. Check font rendering and coordinate accuracy.

5. **Compare side-by-side** with the Plotly version: visual quality,
   interactivity, file size, export quality, code complexity.

6. **Decision gate:**
   - **Proceed** if the canvas version matches or exceeds Plotly on visual
     quality and interactivity, canvas2svg produces acceptable SVG, and the
     code is simpler to maintain.
   - **Keep Plotly as optional** if Canvas is better for interactive display
     but SVG export quality is insufficient for publication use cases.
   - **Abandon** if the implementation effort or quality gaps are larger than
     anticipated.

**When:** After `0.3.0` (candidate competition) is stable. The spike does not
block any current milestone.

---

## Decision record

*(To be filled in after the Phase 1 spike is complete.)*

| Date | Decision | Rationale |
|------|----------|-----------|
| — | — | — |
