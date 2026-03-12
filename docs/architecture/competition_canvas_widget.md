# Competition canvas widget — layout and sizing

Quick reference for how the candidate-competition canvas chart is laid out and sized. (Pulled from agent thread [3982a5d8-fea4-4c33-93fe-362e1ae9dd80].)

## Default widget size (R)

- **Viewer (RStudio):** 800×700 px  
- **Browser:** 900×800 px  

Set via `animate_competition_canvas(..., width = ..., height = ...)`. If `width`/`height` are `NULL`, htmlwidgets uses the sizing policy above.

## Possible widget sizes (reference)

Use these when you want to override the default. Pass `width` and `height` in pixels. The square plot is always centered; the numbers below are total widget size.

| Context        | Width | Height |
|----------------|-------|--------|
| Default viewer | 800   | 700    |
| Default browser| 900   | 800    |
| Compact       | 600   | 500    |
| Large / slides| 1000  | 800    |
| Wide (more legend room) | 1000 | 700 |

Example (R): `animate_competition_canvas(trace, width = 1000, height = 800)`.

## How the JS splits the widget

1. **Total widget:** `width` × `height` (from htmlwidgets; fallback in JS: 700×600 if missing).

2. **Controls bar (fixed height):** 96 px at the bottom.  
   - **Plot area height** = `height - 96` (e.g. 700 − 96 = 604 px in viewer).

3. **Padding (around the square plot):**  
   - `top: 68`, `right: adaptive` (see below), `bottom: 60`, `left: 64` px.  
   - **Right padding** = max(120, 8 + 13 + *longest competitor name width* + 14) so the legend fits.

4. **Square plot:**  
   - **Available width** = widget width − left − right padding.  
   - **Available height** = plot area height − top − bottom padding.  
   - **Plot side** = `min(availableWidth, availableHeight)`; the square is centered in the leftover space (extra width or height is split as margin on both sides).

5. **Scrub bar / controls:**  
   - Left padding of the controls row = plot left edge (`pad.left`).  
   - Right padding = so the controls row ends at the plot right edge.  
   - So the scrub bar spans the **same width as the square chart**.

## Summary table (example: 800×700 viewer)

| Element            | Value |
|--------------------|-------|
| Widget             | 800 × 700 px |
| Controls bar       | 96 px tall (bottom) |
| Plot area          | 800 × 604 px |
| Padding (T,R,B,L)  | 68, adaptive, 60, 64 px |
| Square plot side   | min(availW, availH), centered |
| Scrub bar width    | Same as plot side (aligned) |

## Geometric overlays

The JSON payload also carries optional overlay fields for voter-space geometry
(centroid, yolk, pareto set, etc.) and per-frame candidate geometry (winsets,
indifference boundaries, candidate regions). See
[canvas_overlay_schema.md](canvas_overlay_schema.md) for the full payload
schema and naming conventions.

## Where it’s implemented

- **R:** `r/R/competition_canvas.R` — `width`/`height` args and `sizingPolicy` (viewer/browser defaults).
- **JS:** `r/inst/htmlwidgets/competition_canvas.js` — `CompetitionCanvas.prototype.resize()` (lines ~611–660): `controlsH = 96`, padding object, `availW`/`availH`, `side = min(availW, availH)`, centering, and controls row `paddingLeft`/`paddingRight` so the scrub bar matches the chart width.
