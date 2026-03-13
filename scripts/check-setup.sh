#!/usr/bin/env bash
# check-setup.sh — verify prerequisites and run build + tests (for new or synced machine).
#
# Run from repo root after pulling. If prerequisites are missing, reports what to install.
# If all present, runs ./scripts/setup-build.sh and ctest -LE benchmark.
#
# Usage (from repo root):
#   ./scripts/check-setup.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# Prefer Homebrew on macOS so cmake/CGAL are found
if [[ -d /opt/homebrew/bin ]]; then
  export PATH="/opt/homebrew/bin:$PATH"
fi
if [[ -d /usr/local/bin ]]; then
  export PATH="/usr/local/bin:$PATH"
fi

MISSING=()
HAVE_CMAKE=false
HAVE_CXX=false
HAVE_CGAL=false

if command -v cmake &>/dev/null; then
  HAVE_CMAKE=true
else
  MISSING+=("cmake (e.g. brew install cmake)")
fi

if command -v clang++ &>/dev/null || command -v g++ &>/dev/null; then
  HAVE_CXX=true
else
  MISSING+=("C++20 compiler (e.g. xcode-select --install or build-essential)")
fi

# CGAL: lightweight check (pkg-config or common install paths)
if pkg-config --exists cgal &>/dev/null; then
  HAVE_CGAL=true
elif [[ -d /opt/homebrew/lib/cmake/CGAL ]] || [[ -d /usr/local/lib/cmake/CGAL ]]; then
  HAVE_CGAL=true
else
  MISSING+=("CGAL (e.g. brew install cgal or libcgal-dev)")
fi

if [[ ${#MISSING[@]} -gt 0 ]]; then
  echo "Missing prerequisites:"
  printf '  - %s\n' "${MISSING[@]}"
  echo ""
  echo "See docs/development/setup_checklist.md for install commands. Then run:"
  echo "  ./scripts/check-setup.sh"
  exit 1
fi

echo "Prerequisites OK. Running setup-build.sh..."
./scripts/setup-build.sh

echo ""
echo "Running tests (excluding benchmarks)..."
cd build && ctest --output-on-failure -LE benchmark
