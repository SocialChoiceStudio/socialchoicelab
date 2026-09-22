"""utility_surface_demo.py — Interactive 3D utility surface (plot_utility_surface)

Shows how the utility tent changes across different distance metrics and loss
functions. Each example saves a self-contained HTML file and opens it in the
default browser.

Run from the repository root::

    export SCS_LIB_PATH=$(pwd)/build
    python python/utility_surface_demo.py
"""

from __future__ import annotations

import tempfile
import webbrowser
from pathlib import Path

import socialchoicelab as scl
import socialchoicelab.plots as sclp

outdir = Path(tempfile.mkdtemp())
print(f"utility_surface_demo — writing HTML files to {outdir}\n")


# ---------------------------------------------------------------------------
# 1. Euclidean distance + quadratic loss — circular tent (textbook Fig. I-12)
# ---------------------------------------------------------------------------
print("--- 1. Euclidean + quadratic (circular tent) ---")
fig1 = sclp.plot_utility_surface(
    ideal_x     = 1.0,
    ideal_y     = 0.5,
    dist_config = scl.make_dist_config("euclidean"),
    loss_config = scl.make_loss_config("quadratic"),
    xlim        = (-3.0, 3.0),
    ylim        = (-3.0, 3.0),
    resolution  = 60,
    n_levels    = 5,
    axis_labels = ("Guns", "Butter"),
    title       = "Voter C — Euclidean / quadratic",
)
p1 = outdir / "utility_surface_euclidean_quadratic.html"
sclp.save_utility_surface(fig1, p1)
print(f"  saved: {p1}")
webbrowser.open(p1.as_uri())

# ---------------------------------------------------------------------------
# 2. Manhattan distance + linear loss — diamond-shaped tent
# ---------------------------------------------------------------------------
print("\n--- 2. Manhattan + linear (diamond tent) ---")
fig2 = sclp.plot_utility_surface(
    ideal_x     = 0.0,
    ideal_y     = 0.0,
    dist_config = scl.make_dist_config("manhattan"),
    loss_config = scl.make_loss_config("linear"),
    xlim        = (-3.0, 3.0),
    ylim        = (-3.0, 3.0),
    resolution  = 60,
    n_levels    = 5,
    axis_labels = ("Dimension 1", "Dimension 2"),
    title       = "Voter — Manhattan / linear",
)
p2 = outdir / "utility_surface_manhattan_linear.html"
sclp.save_utility_surface(fig2, p2)
print(f"  saved: {p2}")
webbrowser.open(p2.as_uri())

# ---------------------------------------------------------------------------
# 3. Euclidean + Gaussian loss — soft bell curve
# ---------------------------------------------------------------------------
print("\n--- 3. Euclidean + Gaussian (bell curve) ---")
fig3 = sclp.plot_utility_surface(
    ideal_x     = -0.5,
    ideal_y     = 0.8,
    dist_config = scl.make_dist_config("euclidean"),
    loss_config = scl.make_loss_config("gaussian", steepness=1.5),
    xlim        = (-3.0, 3.0),
    ylim        = (-3.0, 3.0),
    resolution  = 60,
    n_levels    = 5,
    axis_labels = ("Dimension 1", "Dimension 2"),
    title       = "Voter — Euclidean / Gaussian loss",
)
p3 = outdir / "utility_surface_euclidean_gaussian.html"
sclp.save_utility_surface(fig3, p3)
print(f"  saved: {p3}")
webbrowser.open(p3.as_uri())

print("\nDone. Drag each plot to rotate.")
