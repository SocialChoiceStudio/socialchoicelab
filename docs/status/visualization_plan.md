# Visualization Plan — R and Python Plotting Layer

**Phase:** C10–C12 (Mid-term: weeks 3–6 post-binding)

Short-to-medium-term plan to build spatial voting visualization helpers in R and Python. After this milestone, users can plot voter ideal points, policy alternatives, winsets, yolk, uncovered set, and convex hull with a few lines of code.

**Authority:** This plan is the source for "what's next" for visualization work.
When a step is done, mark it ✅ Done and update [where_we_are.md](where_we_are.md).

**References:**
- [Design Document](../architecture/design_document.md) — Visualization section
- [ROADMAP.md](ROADMAP.md) — mid/long-term context
- Completed: [binding_plan_completed.md](archive/binding_plan_completed.md) (R and Python bindings)

---

## Design Goals

1. **Single unified API across R and Python** — same function names, parameter order, return types (e.g. `plot_spatial_voting()` in both).
2. **HTML/interactive output (Plotly)** — not static PNG. Users can hover, zoom, pan, toggle layers.
3. **Composable layers** — users can build complex plots by stacking functions: `base + voters + sq + alternatives + winset + yolk`.
4. **Minimal external dependencies** — `ggplot2` + `plotly` for R; `plotly` (via `plotly.express` or `plotly.graph_objects`) for Python. No heavy geospatial libraries.
5. **Reproducible inline examples** — all public functions have `@examples` (R) or docstring examples (Python) that run in vignettes/notebooks without network.

---

## Phases

### C10: Design & Prototype (R)

**Goal:** Prototype the core plotting API in R; establish the design that Python will mirror.

**Tasks:**

- [ ] **C10.1** Design the base plot object and layering system.
  - Decide: S3 class vs. R6? (Recommend S3 for simplicity; return a `ggplot` object that users can add `+` to.)
  - Decide: `plotly` via `ggplotly()` or direct `plotly::plot_ly()`? (Recommend direct `plot_ly()` for more control over interactivity.)
  - Sketch function signatures: `plot_spatial_voting(voters, alternatives, sq = NULL, title = NULL, ...)`

- [ ] **C10.2** Implement core plotting functions (R).
  - `plot_spatial_voting(voters, alternatives, sq, title, width, height)` — base 2D scatter with voters and alternatives.
  - `layer_winset(plot, winset, fill_alpha = 0.3)` — overlay a winset polygon.
  - `layer_yolk(plot, center, radius, color = "red")` — overlay yolk as a circle.
  - `layer_uncovered_set(plot, uncovered_set, color = "green")` — overlay uncovered set boundary.
  - `layer_convex_hull(plot, points, color = "blue")` — overlay convex hull boundary.

- [ ] **C10.3** Test and document (R vignette).
  - Add a vignette section to `r/vignettes/spatial_voting.Rmd` demonstrating all plotting layers.
  - Add `@examples` to each function (wrapped in `\dontrun{}` if they require user data; or inline with synthetic data).

- [ ] **C10.4** Unit tests for plotting output.
  - Test that plotting functions return valid `plotly` objects.
  - Test that layers can be composed without error.
  - Test edge cases: empty winset, null parameters, single voter.

---

### C11: Mirror in Python

**Goal:** Implement the same plotting API in Python, with identical function names and parameter order.

**Tasks:**

- [ ] **C11.1** Implement core plotting functions (Python).
  - `plot_spatial_voting(voters, alternatives, sq=None, title=None, width=900, height=700)` — base plot.
  - `layer_winset(fig, winset, fill_alpha=0.3)` — overlay winset.
  - `layer_yolk(fig, center, radius, color="red")` — overlay yolk.
  - `layer_uncovered_set(fig, uncovered_set, color="green")` — overlay uncovered set.
  - `layer_convex_hull(fig, points, color="blue")` — overlay convex hull.

- [ ] **C11.2** Test and document (Python notebook).
  - Add a section to `python/notebooks/spatial_voting.ipynb` demonstrating all plotting layers.
  - Add docstring examples to each function.

- [ ] **C11.3** Unit tests for plotting output.
  - Test that plotting functions return valid `plotly.graph_objects.Figure` objects.
  - Test layer composition.
  - Test edge cases.

---

### C12: CI & Polish

**Goal:** Integrate plotting tests into CI, document best practices, commit and deploy.

**Tasks:**

- [ ] **C12.1** Add plotting tests to CI.
  - R: `devtools::test()` picks up plotting unit tests.
  - Python: `pytest` picks up plotting unit tests.

- [ ] **C12.2** Documentation pass.
  - Add plotting section to main README (with a static screenshot or embedded interactive plot).
  - Add best-practices guide to [design_document.md](../architecture/design_document.md) § Visualization.

- [ ] **C12.3** Optional: Gallery of example plots.
  - Create `docs/python/examples/plotting_gallery.ipynb` or similar showcasing realistic voting scenarios.

- [ ] **C12.4** Commit, test, push, and mark complete.

---

## Key Design Decisions (TBD)

| Decision | R | Python | Rationale |
|----------|---|--------|-----------|
| Base plot object | S3 (ggplot-like) or R6? | plotly Figure | S3 is simpler for piping; R6 offers state management. Recommend S3. |
| Winset polygon representation | sf/sp or manual? | shapely or manual? | Manual (iterate boundary, plot line). Avoids heavyweight geo libs. |
| Yolk circle | `geom_circle()` or annotate? | `go.Circle` or manual? | Manual for consistency across R/Python. |
| Uncovered set boundary | Convex hull of points or exact boundary? | Convex hull or exact? | Start with convex hull for speed; exact boundary is nice-to-have. |
| Color palette | viridis + manual override? | plotly defaults + manual? | Colorblind-friendly by default. |
| Export formats | PNG, SVG, PDF via `plotly::export()`? | PNG, SVG via `fig.write_*()` | Defer to Plotly's export. Users can download from Plotly widget. |

---

## Acceptance Criteria

✅ **Definition of Done per task:**

- Every public function has at least one inline example (R `@examples`, Python docstring).
- Every function has ≥2 unit tests (success case + edge case).
- No warnings in `R CMD check` or `pytest`.
- Vignette and notebook sections render without error.
- CI passes on both R and Python tests.
- Code follows project style (C++ clang-format, R/Python already linted).

✅ **Milestone gate (C10–C12 complete):**

- Both R and Python have identical plotting API (function names, parameter order, return types).
- All core layers (voters, alternatives, SQ, winset, yolk, uncovered set, convex hull) are plotted correctly.
- Documentation and examples are comprehensive and reproducible.
- All tests pass locally and in CI.

---

## Timeline & Effort Estimate

| Phase | Subtask | Effort | Owner | Status |
|-------|---------|--------|-------|--------|
| C10 | Design & R proto | 2–3 days | — | ⏳ Pending |
| C10 | R core + tests | 2–3 days | — | ⏳ Pending |
| C10 | R vignette & docs | 1 day | — | ⏳ Pending |
| C11 | Python mirror | 1–2 days | — | ⏳ Pending |
| C11 | Python notebook & tests | 1 day | — | ⏳ Pending |
| C12 | CI integration & polish | 1 day | — | ⏳ Pending |
| **Total** | — | **~1 week** | — | ⏳ Pending |

---

## Blocked By

- None. R and Python bindings are complete (Phase B6–B9 ✅).

---

## Unresolved Questions

1. **Should we include 3D plotting?** (e.g., 3D scatter for N-D voter spaces) — Defer to Phase C13+.
2. **Should we auto-detect yolk & uncovered set?** (i.e., compute them if not provided) — Yes, as an optional convenience.
3. **Color scheme / colorblind palette:** — Use Viridis (R) and Plotly's default (Python) to start.
4. **Export / save plots:** — Defer to Plotly's built-in export widget for now.

---

## Notes for Developers

- **Consistency is key:** Keep R and Python function signatures as close as possible. Test by implementing the same scenario in both languages.
- **Start minimal:** Begin with voters + alternatives + SQ. Add layers iteratively.
- **Test coverage:** Unit tests should verify plot correctness (e.g., that voters are plotted at the right coordinates).
- **User feedback loop:** Encourage early testing via the notebook to catch usability issues before finalizing the API.
