# socialchoicelab Python API

Python bindings for **libscs_api** — spatial social choice analysis.

## Quick start

```python
import socialchoicelab as scl

# Create voters and alternatives in 2D
voter_ideals = [-1, -1,  1, -1,  1,  1]   # 3 voters
alt_xy       = [ 0,  0,  2,  0, -2,  0]   # 3 alternatives

# Build a preference profile
prof = scl.profile_build_spatial(alt_xy, voter_ideals)
print(prof.dims())          # (3, 3)
print(scl.borda_scores(prof))

# Compute the winset of the status quo
ws = scl.winset_2d(alt_xy=[0, 0], voter_ideals_xy=voter_ideals)
print("empty:", ws.is_empty())
```

## Installation

```bash
# Build libscs_api first (see repository README), then:
export SCS_LIB_PATH=/path/to/socialchoicelab/build
pip install ./python
```
