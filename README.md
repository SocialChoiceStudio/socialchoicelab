# socialchoicelab

**A computational platform for spatial voting, electoral competition, and axiomatic analysis.**

Part of the [SocialChoiceStudio](https://github.com/SocialChoiceStudio) project.

---

## What It Does

Social choice theory has rich mathematical foundations but fragmented computational tools. `socialchoicelab` unifies three research traditions in a single, reproducible platform:

**Spatial Voting Analysis** — Compute winsets, the core, the yolk, the uncovered set, the heart, Copeland winners, and other solution concepts for voter configurations in two-dimensional Euclidean space. Supports veto players, weighted voting, and supermajority rules.

**Multi-Candidate Electoral Competition** — Simulate adaptive candidate competition using behaviorally-grounded strategies (Sticker, Hunter, Aggregator, Predator), as introduced by Laver (2005) and extended by Laver & Sergenti (2011). Supports plurality and proportional seat conversion, convergence and cycle detection, and batch parameter sweeps with full trace recording.

**Voting Rule Property Simulation** — Evaluate voting rules (plurality, Borda, approval, Copeland, minimax, and others) against axiomatic criteria (Condorcet consistency, Pareto efficiency, monotonicity, strategy-proofness, and more) through Monte Carlo simulation across configurable preference distributions.

---

## Reproducibility

All randomness is controlled through a **named-stream pseudorandom number generator** architecture. A single recorded seed exactly reproduces any result — across R and Python, across machines, across time.

---

## Architecture

- **C++ core** — High-performance geometry, aggregation, and simulation.
- **Stable C API** — Binary-stable boundary for language bindings.
- **R package** — Native-feeling interface with Plotly-compatible output and built-in scenarios.
- **Python package** — Identical API with NumPy, pandas, and Plotly integration.
- **Jupyter notebooks** — Recommended entry point for new users and classroom use.
- **Command-line interface** — For parameter sweeps, batch jobs, and scripted pipelines.

---

## Quick Start

```r
# R
library(socialchoicelab)

# Spatial voting: winset from the origin, 5 voters
voters <- c(0.0, 1.0, 0.0, -1.0, 0.87, 0.5, -0.87, 0.5, 0.0, 0.0)
ws <- winset_2d(c(0, 0), voters)
ws$is_empty()  # TRUE — Plott symmetry holds; origin is the core

# Electoral competition: two Hunters converging to the median
trace <- competition_run(
  competitor_positions = c(0.2, 0.8),
  strategy_kinds       = c("hunter", "hunter"),
  voter_ideals         = runif(1000),
  dist_config          = make_dist_config(n_dims = 1L),
  engine_config        = make_competition_engine_config(max_rounds = 100L)
)
trace$termination()  # Converged
```

```python
# Python
from socialchoicelab import winset_2d, competition_run

voters = [0.0, 1.0, 0.0, -1.0, 0.87, 0.5, -0.87, 0.5, 0.0, 0.0]
ws = winset_2d([0.0, 0.0], voters)
ws.is_empty()  # True
```

---

## Documentation

| Resource | Link |
|----------|------|
| Architecture | [docs/architecture/design_document.md](docs/architecture/design_document.md) |
| Roadmap | [docs/status/ROADMAP.md](docs/status/ROADMAP.md) |
| Current status | [docs/status/where_we_are.md](docs/status/where_we_are.md) |
| C API reference | [docs/architecture/c_api_design.md](docs/architecture/c_api_design.md) |
| Full docs index | [docs/README.md](docs/README.md) |

---

## Status

`socialchoicelab` is under active development. The current release line is **`0.2.0`** (core + C API + geometry + aggregation + R/Python bindings + visualization). The **`0.3.0`** track adds multi-candidate electoral competition and is in final refinement.

The test suite contains over 300 unit tests, a substantial portion of which are grounded directly in established social choice theorems.

---

## License

[GPL-3.0](LICENSE)
