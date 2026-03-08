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

### C10: Design & Prototype (R) ✅ Done

- ✅ **C10.1** Design decisions — direct `plotly::plot_ly()`, functional layer API (no R6/S3 class needed).
- ✅ **C10.2** `plot_spatial_voting()`, `layer_winset()`, `layer_yolk()`, `layer_uncovered_set()`, `layer_convex_hull()` implemented in `r/R/plots.R`. `alternatives` defaults to `numeric(0)` (optional).
- ✅ **C10.3** Vignette section 9 added to `r/vignettes/spatial_voting.Rmd`; all functions have `@examples`.
- ✅ **C10.4** 17 unit tests in `r/tests/testthat/test_plots.R`; all pass.

---

### C11: Mirror in Python ✅ Done

- ✅ **C11.1** `socialchoicelab.plots` submodule implemented in `python/src/socialchoicelab/plots.py`; identical API to R.
- ✅ **C11.2** Notebook section 9 added to `python/notebooks/spatial_voting.ipynb`; all functions have NumPy-style docstrings.
- ✅ **C11.3** 13 unit tests in `python/tests/test_plots.py`; all pass.

---

### C12: CI & Polish ✅ Done

- ✅ **C12.1** `plotly >= 4.10.0` added to R `DESCRIPTION` Imports; `plotly >= 5.0` added to Python `pyproject.toml` dependencies. Both CI jobs (R and Python) install and test plotly.
- ✅ **C12.2** `docs/python/api/plots.md` API reference page added; `_pkgdown.yml` Visualization section added for R.
- ✅ **C12.3** CI green on both Ubuntu and macOS for all three jobs (core, r-binding, python-binding).

---

### C13: Visualization Refinements ✅ Done (C13.A–C13.8)

**Goal:** Polish the plotting layer based on real-scenario feedback and add missing capabilities.

- ✅ **C13.A** Standard built-in scenario datasets with convenience functions.
  - JSON format designed (metadata + voters + SQ + decision rule + axis labels).
  - 33 built-in scenarios stored in R `inst/extdata/scenarios/` and Python `src/socialchoicelab/data/scenarios/`.
  - `load_scenario(name)` and `list_scenarios()` in both R and Python; users never see JSON.
  - Full unit test coverage (R + Python).
  - Dev/test scripts updated to use `load_scenario()`.

- ⏭ **C13.1** Load scenario from external file — moved to ROADMAP (external CSV/JSON format TBD).

- ✅ **C13.2** `layer_ic(fig, voters, sq, color_by_voter=FALSE)` — individual voter indifference curves (circles centred at each ideal point, radius = distance to SQ). Dash: `"dot"`. Uniform or per-voter Okabe-Ito colour. R + Python.

- ✅ **C13.3** Auto-compute in `layer_winset()` and `layer_uncovered_set()` — pass `voters` + `sq` (or `voters` alone) and the layer function computes the geometry internally. Both R and Python.

- ⏭ **C13.4** Colorblind-safe default palette — Okabe-Ito palette defined as internal helper `_okabe_ito_cycle()` in both R and Python; used for `color_by_voter` mode. Full audit of all layer defaults deferred to a future design pass (see `visualization_design.md`).

- ✅ **C13.5** `xlim`/`ylim` in `plot_spatial_voting()` — auto-computes a 12%-padded range from all plotted points; explicit `xlim`/`ylim` args override.

- ✅ **C13.6** `layer_preferred_regions(fig, voters, sq, color_by_voter=FALSE)` — filled circles showing the preferred-to-SQ region for each voter. R + Python.

- ⏭ **C13.7** Shapley-Owen annotation — blocked on C API exposure. Deferred to ROADMAP.

- ✅ **C13.8** `save_plot(fig, path)` — HTML export via `htmlwidgets::saveWidget()` / `fig.write_html()`; image export via `plotly::save_image()` / `fig.write_image()` (requires kaleido). Clear error messages when kaleido is missing. R + Python.

- ⏭ **C13.9** Gallery notebook — deferred; `devScript_MAIN.R/.py` and `TestScript_11_Tovey.R/.py` serve as the current scenario gallery. A dedicated gallery page is a documentation task for a future milestone.

**When to start C13:** After at least one full scenario has been plotted end-to-end in both R and Python and user feedback on the current API has been collected.

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

| Phase | Subtask | Effort | Status |
|-------|---------|--------|--------|
| C10 | Design & R proto | 2–3 days | ✅ Done |
| C10 | R core + tests | 2–3 days | ✅ Done |
| C10 | R vignette & docs | 1 day | ✅ Done |
| C11 | Python mirror | 1–2 days | ✅ Done |
| C11 | Python notebook & tests | 1 day | ✅ Done |
| C12 | CI integration & polish | 1 day | ✅ Done |
| C13 | Visualization refinements | ~3 days | ✅ Done (C13.A–C13.8) |

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
