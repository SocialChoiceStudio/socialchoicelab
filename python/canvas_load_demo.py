"""canvas_load_demo.py — Reload pre-computed competition canvases from .scscanvas files.

Companion to canvas_save_demo.py.  Reads the .scscanvas files saved by that
script (written to the project root) and opens the canvases without
recomputing any geometry (ICs, WinSet, Cutlines, etc.).

Run canvas_save_demo.py first to generate the input files, then run this:

  export SCS_LIB_PATH=$(pwd)/build
  python python/canvas_load_demo.py

The .scscanvas format is cross-language: files written by the R demo
(canvas_save_demo.R / save_competition_canvas()) can be loaded here too, and
vice versa.
"""

from __future__ import annotations

import subprocess
from pathlib import Path

import socialchoicelab.plots as sclp

# Paths match those written by canvas_save_demo.py (tmp/ in project root).
PATH_1D    = "tmp/canvas_1d_demo.scscanvas"
PATH_2D    = "tmp/canvas_2d_demo.scscanvas"
PATH_3CAND = "tmp/canvas_3cand_2d_demo.scscanvas"

# ===========================================================================
# Load and open the 1D canvas
# ===========================================================================
print(f"Loading 1D canvas from: {PATH_1D}")
html_1d = sclp.load_competition_canvas(PATH_1D)

out_1d = "/tmp/canvas_1d_loaded.html"
Path(out_1d).write_text(html_1d, encoding="utf-8")
print(f"Written to: {out_1d}")
subprocess.Popen(["open", out_1d])

# ===========================================================================
# Load and open the 2D (2-candidate) canvas
# ===========================================================================
print(f"Loading 2D (2-cand) canvas from: {PATH_2D}")
html_2d = sclp.load_competition_canvas(PATH_2D)

out_2d = "/tmp/canvas_2d_loaded.html"
Path(out_2d).write_text(html_2d, encoding="utf-8")
print(f"Written to: {out_2d}")
subprocess.Popen(["open", out_2d])

# ===========================================================================
# Load and open the 2D (3-candidate) canvas
# ===========================================================================
print(f"Loading 2D (3-cand) canvas from: {PATH_3CAND}")
html_3c = sclp.load_competition_canvas(PATH_3CAND)

out_3c = "/tmp/canvas_3cand_2d_loaded.html"
Path(out_3c).write_text(html_3c, encoding="utf-8")
print(f"Written to: {out_3c}")
subprocess.Popen(["open", out_3c])

print("\nDone. No recomputation was needed.")
