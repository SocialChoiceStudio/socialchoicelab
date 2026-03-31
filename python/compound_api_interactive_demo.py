"""compound_api_interactive_demo.py — Interactive HTML demos for compound C API paths
=====================================================================================

Builds a self-contained HTML file (canvas + tabbed panels + sliders). Each slider
step is precomputed in Python via ``ic_interval_1d``, ``scs_winset_2d_export_boundary``,
``uncovered_set_boundary_2d`` (heap), and ``scs_voronoi_cells_2d_heap``.

The page template lives in ``compound_api_interactive_shell.html`` (shared with the
R script ``r/compound_api_interactive_demo.R``). Shared build/validation logic is in
``compound_api_interactive_common.py``.

Run from the repository root::

    export SCS_LIB_PATH=$(pwd)/build
    python python/compound_api_interactive_demo.py

Use ``--strict`` to exit with code 1 if embedded data checks fail (for CI).

Writes ``tmp/compound_api_interactive_py.html`` and opens it in the default browser
unless ``OPEN_BROWSER = False``.
"""

from __future__ import annotations

import sys
import webbrowser
from pathlib import Path

from compound_api_interactive_common import build_payload, format_validation_report

OPEN_BROWSER = True
OUT_NAME = "compound_api_interactive_py.html"


def main() -> None:
    strict = "--strict" in sys.argv
    py_dir = Path(__file__).resolve().parent
    root = py_dir.parent
    shell = py_dir / "compound_api_interactive_shell.html"
    if not shell.is_file():
        raise FileNotFoundError(f"Missing template: {shell}")
    out = root / "tmp" / OUT_NAME
    out.parent.mkdir(parents=True, exist_ok=True)
    payload = build_payload()
    meta = payload["meta"]
    report = format_validation_report(meta["build_checks"], meta["build_all_ok"])
    all_ok = meta["build_all_ok"]
    print(report)
    if strict and not all_ok:
        sys.exit(1)

    import json

    html = shell.read_text(encoding="utf-8")
    html = html.replace("__DATA__", json.dumps(payload)).replace(
        "__SOURCE_TAG__", "Python bindings"
    )
    out.write_text(html, encoding="utf-8")
    print(f"Wrote {out}")
    if OPEN_BROWSER:
        webbrowser.open(out.as_uri())


if __name__ == "__main__":
    main()
