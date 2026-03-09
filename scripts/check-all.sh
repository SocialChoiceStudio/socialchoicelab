#!/usr/bin/env bash
# check-all.sh — run the full local check suite before pushing.
#
# Usage:
#   ./scripts/check-all.sh
#
# Environment (auto-detected if not set):
#   SCS_LIB_PATH      defaults to <repo>/build
#   SCS_INCLUDE_PATH  defaults to <repo>/include
#
# All 7 steps run in order. The script exits immediately on the first failure
# and tells you what to fix. When it exits 0, it is safe to push.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

export SCS_LIB_PATH="${SCS_LIB_PATH:-$ROOT/build}"
export SCS_INCLUDE_PATH="${SCS_INCLUDE_PATH:-$ROOT/include}"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

step() {
  echo ""
  echo "══════════════════════════════════════════════════════════"
  printf "  %s\n" "$1"
  echo "══════════════════════════════════════════════════════════"
}

fail() {
  echo "" >&2
  echo "✗  FAILED: $1" >&2
  echo "   Fix the errors above, then re-run ./scripts/check-all.sh." >&2
  exit 1
}

# ---------------------------------------------------------------------------
# 1. Format check
# ---------------------------------------------------------------------------

step "1/7  Format check"
./lint.sh format
git diff --exit-code -- include/ src/ tests/unit/ \
  || fail "Format check — run './lint.sh format', commit the changes, then re-run."

# ---------------------------------------------------------------------------
# 2. C++ build
# ---------------------------------------------------------------------------

step "2/7  C++ build"
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build \
  || fail "C++ build"

# ---------------------------------------------------------------------------
# 3. C++ tests
# ---------------------------------------------------------------------------

step "3/7  C++ tests"
(cd build && ctest --output-on-failure -LE benchmark) \
  || fail "C++ tests"

# ---------------------------------------------------------------------------
# 4. Strict lint
# ---------------------------------------------------------------------------

step "4/7  Strict lint (cpplint)"
./lint.sh --strict lint \
  || fail "Strict lint — fix all cpplint errors before pushing."

# ---------------------------------------------------------------------------
# 5. R CMD check + pytest
# ---------------------------------------------------------------------------

step "5/7  Binding checks (R CMD check + pytest)"
./scripts/check-bindings.sh \
  || fail "Binding checks"

# ---------------------------------------------------------------------------
# 6. R pkgdown
# ---------------------------------------------------------------------------

step "6/7  R pkgdown site build"
if command -v Rscript &>/dev/null; then
  (
    cd r
    Rscript -e "pkgdown::build_site(preview = FALSE)"
  ) || fail "R pkgdown build — check for missing topics in _pkgdown.yml or undocumented args."
else
  echo "  Rscript not found — skipping pkgdown build."
  echo "  Install R and run this script again before pushing r/ changes."
fi

# ---------------------------------------------------------------------------
# 7. Python mkdocs
# ---------------------------------------------------------------------------

step "7/7  Python mkdocs site build"
MKDOCS_CMD=""
if command -v mkdocs &>/dev/null; then
  MKDOCS_CMD="mkdocs"
elif python3 -m mkdocs --version &>/dev/null 2>&1; then
  MKDOCS_CMD="python3 -m mkdocs"
fi

if [[ -n "$MKDOCS_CMD" ]]; then
  $MKDOCS_CMD build --config-file docs/mkdocs.yml --strict \
    || fail "Python mkdocs build — check for broken references or missing docstrings."
else
  echo "  mkdocs not found — skipping."
  echo "  Install with: pip install mkdocs mkdocstrings[python] mkdocs-material mkdocs-jupyter"
  echo "  Then re-run this script before pushing python/ or docs/ changes."
fi

# ---------------------------------------------------------------------------
# Done
# ---------------------------------------------------------------------------

echo ""
echo "══════════════════════════════════════════════════════════"
echo "  ✓  All checks passed. Safe to push."
echo "══════════════════════════════════════════════════════════"
echo ""
