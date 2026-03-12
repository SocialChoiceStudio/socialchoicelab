# Canvas animation overlay schema

Specification for how geometric overlays (voter-space geometry, per-frame
candidate geometry) travel from the C++ core to the JS canvas player.

This document is the contract between:
- The **R/Python layer** that computes overlays and serialises them to JSON.
- The **JS canvas player** that reads the JSON and renders them.

See also: `Design_Document.md` § Visualization Contract — all social-choice
computation happens in C++; the JS layer is a pure renderer.

---

## Principle: semantic names travel the whole stack

Overlay fields are named after the **social-choice concept**, not the
computational method used to produce them (e.g. `pareto_set`, not
`convex_hull`). The JS player never knows or needs to know which algorithm or
metric produced a given polygon. Implementation detail stays in C++.

---

## Two overlay slots

The JSON payload sent to the JS player contains two top-level overlay fields:

```
"overlays_static"  — computed once from voter positions; drawn every frame
"overlays_frames"  — one entry per frame; drawn at the matching frame only
```

---

## `overlays_static`

A JSON object. Each key is a geometry name; each value is the render-ready
description of that geometry. All fields are optional — omit a key to skip
rendering that overlay.

```jsonc
"overlays_static": {
  "centroid": {
    "x": <number>,
    "y": <number>
  },
  "marginal_median": {
    "x": <number>,
    "y": <number>
  },
  "pareto_set": {
    "polygon": [[x0,y0], [x1,y1], ...]   // closed ring, CCW
  },
  "yolk": {
    "cx": <number>,   // centre x
    "cy": <number>,   // centre y
    "r":  <number>    // radius
  },
  "heart": {
    "polygons": [
      { "ring": [[x,y],...], "hole": false },
      { "ring": [[x,y],...], "hole": true  },
      ...
    ]
  },
  "uncovered_set": {
    "polygons": [
      { "ring": [[x,y],...], "hole": false },
      ...
    ]
  },
  "core": {
    "polygons": [...]   // empty array if core is empty
  }
}
```

### Polygon format

A polygon with possible holes uses the `polygons` array form:
- Each entry has `"ring"`: array of `[x, y]` pairs (closed — last point equals
  first, or JS closes it).
- Each entry has `"hole"`: `true` if the ring is a hole in the preceding outer
  ring, `false` if it is an outer boundary.
- Outer rings are CCW; holes are CW (matching CGAL's `boundary()` export
  convention).

Simple convex shapes (centroid, marginal median, yolk) use flat fields instead
of the polygon array form.

---

## `overlays_frames`

An array with one entry per frame (same length as `rounds`). Each entry is a
JSON object with the same key vocabulary as `overlays_static`, but the values
describe geometry that depends on candidate positions at that frame.

```jsonc
"overlays_frames": [
  // frame 0
  {
    "indifference_boundaries": [
      // one entry per candidate pair (i, j), i < j
      {
        "i": 0, "j": 1,
        "segments": [[x0,y0, x1,y1], ...]   // one or more line segments
                                              // clipped to xlim/ylim
      }
    ],
    "candidate_regions": [
      // one entry per candidate
      {
        "candidate": 0,
        "polygons": [{ "ring": [[x,y],...], "hole": false }, ...]
      }
    ],
    "winsets": [
      // one entry per candidate
      {
        "candidate": 0,
        "polygons": [{ "ring": [[x,y],...], "hole": false }, ...]
      }
    ]
  },
  // frame 1 ...
]
```

### Field vocabulary for `overlays_frames`

| Key | Description | C++ function (to be built or exists) |
|-----|-------------|--------------------------------------|
| `indifference_boundaries` | Equidistant boundary between each candidate pair under the actual `DistanceConfig`. Line(s) per pair. | Not yet built |
| `candidate_regions` | Nearest-candidate partition (generalised Voronoi) under the actual `DistanceConfig`. Polygon per candidate. | Not yet built |
| `winsets` | Set of points that would defeat the candidate's current position by majority rule. | Exists: `scs_winset_2d` |

---

## R/Python API convention

Overlays are passed as optional arguments to `animate_competition_canvas()`:

```r
# R
animate_competition_canvas(
  trace,
  voters      = voters,
  overlays    = list(
    centroid  = compute_centroid(voters),
    yolk      = compute_yolk(voters),
    pareto_set = compute_pareto_set(voters)
  )
)
```

```python
# Python
animate_competition_canvas(
    trace,
    voters=voters,
    overlays={
        "centroid":   compute_centroid(voters),
        "yolk":       compute_yolk(voters),
        "pareto_set": compute_pareto_set(voters),
    },
)
```

The `animate_competition_canvas` function validates the overlay objects,
serialises them to the `overlays_static` / `overlays_frames` JSON fields, and
includes them in the payload. The JS player renders whichever keys are present.

Per-frame overlays (winsets, indifference boundaries, candidate regions) are
expensive to compute and large in the payload, so they are opt-in.

---

## JS rendering defaults (toggles)

Each overlay key gets a checkbox or toggle in the controls bar. All overlays
are **off by default**. The user enables them at runtime. The JS skips
rendering any key that is absent from the payload or whose toggle is off.

| Overlay key | Control type | Options | Default |
|-------------|-------------|---------|---------|
| `centroid` | Checkbox | On / Off | Off |
| `marginal_median` | Checkbox | On / Off | Off |
| `pareto_set` | Checkbox | On / Off | Off |
| `pareto_set` | Checkbox | On / Off | Off — hidden unless a correct general implementation is present; Euclidean-only proxy via convex hull guarded at R/Python layer |
| `yolk` | Checkbox | On / Off | Off |
| `heart` | Checkbox | On / Off | Off |
| `uncovered_set` | Checkbox | On / Off | Off |
| `core` | Checkbox | On / Off | Off |
| `indifference_boundaries` | Dropdown | None / Winner(s) / All | None |
| `candidate_regions` | Dropdown | None / Winner(s) / All | None |
| `winsets` | Dropdown | None / Winner(s) | None |

Toggles for overlays that are absent from the payload are hidden (not greyed
out) so the controls row does not fill up with disabled checkboxes.

---

## Metric support status

Not all geometries are implemented for all distance metrics. The table below
records the current status. Geometries marked "Euclidean only" must not be
offered when a non-Euclidean `DistanceConfig` is in use; the R/Python layer
must guard against this and raise an informative error.

| Overlay | All metrics | Euclidean only | Notes |
|---------|-------------|----------------|-------|
| Centroid | Yes | — | Pure mean; metric-independent |
| Marginal Median | Yes | — | Per-dimension median; metric-independent |
| Indifference Boundaries | Yes | — | `level_set_2d` handles all Minkowski metrics with salience weights |
| Winset | Yes | — | Uses sampled ICs via `DistanceConfig`; passes metric through |
| Convex Hull | Yes | — | The convex hull shape is always valid. It equals the Pareto Set only under Euclidean distance with uniform salience — under other metrics it is an outer bound, not the Pareto Set |
| Pareto Set | Not yet built | — | The Pareto Set is a metric-independent concept. A correct general implementation does not yet exist. Until it does, the convex hull is used as a Euclidean-only proxy and must not be labelled "Pareto Set" when a non-Euclidean `DistanceConfig` is active |
| Candidate Regions | No | Yes | Generalised Voronoi under non-Euclidean not yet built |
| Yolk | Uncertain | Likely | Based on median hyperplanes; non-Euclidean behaviour needs verification |
| Heart | Uncertain | Likely | Defined via winsets; needs verification for non-Euclidean |
| Uncovered Set | Uncertain | Likely | Based on pairwise majority over winsets; needs verification |
| Core | Uncertain | Likely | Same as uncovered set |

The R/Python `animate_competition_canvas()` function must check the
`DistanceConfig` from the trace and either suppress or relabel overlays that
are not valid for the active metric, with a clear warning to the user.

---

## Implementation order (planned)

1. **Payload pipeline** — add `overlays_static` and `overlays_frames` slots to
   the JSON payload; JS renders centroid and convex hull as a proof of concept.
2. **Static overlays** — centroid, marginal median, convex hull / pareto set
   (cheapest; already computed in R/Python plot helpers).
3. **More static overlays** — yolk, heart, uncovered set, core (more expensive
   but fixed).
4. **Per-frame: winsets (winner(s) only)** — C++ already exists; wire up to
   the per-frame payload slot with seat-holder filtering.
5. **Per-frame: indifference boundaries (none / winner(s) / all)** — C++
   `level_set_2d` already handles all metrics; needs R/Python per-frame
   serialisation.
6. **Per-frame: candidate regions** — requires new C++ function for
   general-metric nearest-candidate partition (non-Euclidean deferred to
   roadmap).
