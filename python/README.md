# socialchoicelab — Python Binding

Spatial and general social choice analysis: distance and loss functions, majority
preference, winsets, uncovered set, Copeland scores, preference profiles, positional
voting rules, centrality measures, and **canvas-based** interactive visualization (self-contained HTML via `save_plot`).

Calls the pre-built `libscs_api` shared library via the C ABI; no C++ compilation
is required at install time.

---

## Quick start

```python
import numpy as np
import socialchoicelab as scl

# 5 voters in a 2D policy space — flat [x0, y0, x1, y1, ...] format
voters = np.array([-1.0, -0.5,  0.0, 0.0,  0.8, 0.6, -0.4, 0.8,  0.5, -0.7])
sq = np.array([0.0, 0.0])   # status quo
a  = np.array([0.6, 0.4])   # reform A
b  = np.array([-0.5, 0.3])  # reform B

# Majority preference
scl.majority_prefers_2d(a, sq, voters)   # True/False

# Winset
ws = scl.winset_2d(sq[0], sq[1], voters)
scl.winset_is_empty(ws)                  # True if SQ is an equilibrium

# Copeland and Condorcet
alts = np.concatenate([sq, a, b])
scl.copeland_scores_2d(alts, voters)     # array of scores
scl.has_condorcet_winner_2d(alts, voters)

# Centrality
scl.marginal_median_2d(voters)           # (x, y) coordinate-wise median
scl.centroid_2d(voters)                  # (x, y) arithmetic mean

# Ordinal profile and voting rules
prof = scl.profile_build_spatial(alts, voters)
scl.plurality_scores(prof)
scl.borda_scores(prof)

# Built-in scenarios
names = scl.list_scenarios()
s = scl.load_scenario("laing_olmsted_bear")
s["voters"]       # flat voter ideals
s["status_quo"]   # [x, y]
```

### Visualization

```python
import socialchoicelab.plots as sclp

fig = sclp.plot_spatial_voting(
    voters, sq=sq,
    voter_names=[f"V{i+1}" for i in range(5)],
    title="5-voter legislature",
)
fig = sclp.layer_winset(fig, ws)
fig = sclp.layer_ic(fig, voters, sq, color_by_voter=True)
fig = sclp.layer_centroid(fig, voters)
fig = sclp.layer_marginal_median(fig, voters)
sclp.save_plot(fig, "analysis.html")    # self-contained HTML (open in a browser)
```

---

## Installation (development)

### 1. Build the C library

Run from the **repository root**:

```bash
cmake -S . -B build
cmake --build build
```

This produces `build/libscs_api.dylib` (macOS) or `build/libscs_api.so` (Linux).

### 2. Set the library path

```bash
export SCS_LIB_PATH=/path/to/socialchoicelab/build
```

Add this to `~/.zshrc` or `~/.bash_profile` so it persists across sessions.

### 3. Create a virtual environment and install

From the `python/` directory:

```bash
python -m venv .venv
source .venv/bin/activate
pip install -e ".[dev]"
```

### 4. Verify

```bash
python -c "import socialchoicelab as scl; print(scl.scs_version())"
# {'major': 0, 'minor': 1, 'patch': 0}
```

---

## Running tests

```bash
cd python
pytest
```

---

## After C library changes

If you modify C source and rebuild `libscs_api`, no reinstall is needed —
restart your Python kernel/session so the new `.dylib` is reloaded.

---

## Key API modules

| Module | Contents |
|--------|----------|
| `socialchoicelab` | Distance, loss, level sets, majority, winsets, geometry, profiles, voting rules, scenarios |
| `socialchoicelab.plots` | Canvas-backed spatial plots (`plot_spatial_voting`, `layer_*`, `save_plot` → standalone HTML) |
| `socialchoicelab.palette` | Colorblind-safe palette and theme utilities (`scl_palette`, `scl_theme_colors`) |
