#!/usr/bin/env bash
# check-bindings.sh — run R CMD check and pytest locally against libscs_api.
#
# Usage:
#   ./scripts/check-bindings.sh            # check both R and Python
#   ./scripts/check-bindings.sh --r-only   # check only R
#   ./scripts/check-bindings.sh --py-only  # check only Python
#
# Required environment variables (set in ~/.Renviron or export before running):
#   SCS_LIB_PATH      Directory containing libscs_api.so or libscs_api.dylib.
#   SCS_INCLUDE_PATH  Directory containing scs_api.h.
#
# Quick start:
#   cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --target scs_api
#   export SCS_LIB_PATH=$(pwd)/build
#   export SCS_INCLUDE_PATH=$(pwd)/include
#   ./scripts/check-bindings.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# ---------------------------------------------------------------------------
# Parse arguments
# ---------------------------------------------------------------------------

RUN_R=1
RUN_PY=1

for arg in "$@"; do
  case "$arg" in
    --r-only)  RUN_PY=0 ;;
    --py-only) RUN_R=0  ;;
    --help|-h)
      sed -n '2,20p' "$0" | sed 's/^# \?//'
      exit 0
      ;;
    *)
      echo "check-bindings: unknown argument '$arg'. Use --r-only, --py-only, or --help." >&2
      exit 1
      ;;
  esac
done

# ---------------------------------------------------------------------------
# Preflight checks
# ---------------------------------------------------------------------------

ERRORS=0

check_env() {
  local var="$1" desc="$2"
  if [[ -z "${!var:-}" ]]; then
    echo "check-bindings: $var is not set. $desc" >&2
    ERRORS=$((ERRORS + 1))
  fi
}

check_file() {
  local path="$1" desc="$2"
  if [[ ! -f "$path" ]]; then
    echo "check-bindings: $desc not found at '$path'." >&2
    ERRORS=$((ERRORS + 1))
  fi
}

if [[ $RUN_R -eq 1 || $RUN_PY -eq 1 ]]; then
  check_env SCS_LIB_PATH   "Set it to the directory containing libscs_api.so or libscs_api.dylib."
  check_env SCS_INCLUDE_PATH "Set it to the directory containing scs_api.h."
fi

if [[ -n "${SCS_LIB_PATH:-}" ]]; then
  if [[ "$(uname)" == "Darwin" ]]; then
    check_file "$SCS_LIB_PATH/libscs_api.dylib" "libscs_api.dylib"
  else
    check_file "$SCS_LIB_PATH/libscs_api.so" "libscs_api.so"
  fi
fi

if [[ -n "${SCS_INCLUDE_PATH:-}" ]]; then
  check_file "$SCS_INCLUDE_PATH/scs_api.h" "scs_api.h"
fi

if [[ $ERRORS -gt 0 ]]; then
  echo "" >&2
  echo "Build libscs_api first:" >&2
  echo "  cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --target scs_api" >&2
  echo "Then set the required variables:" >&2
  echo "  export SCS_LIB_PATH=\$(pwd)/build" >&2
  echo "  export SCS_INCLUDE_PATH=\$(pwd)/include" >&2
  exit 1
fi

# ---------------------------------------------------------------------------
# R CMD check (no devtools required)
# ---------------------------------------------------------------------------

R_RC=0
if [[ $RUN_R -eq 1 ]]; then
  if ! command -v Rscript &>/dev/null; then
    echo "check-bindings: Rscript not found. Install R or skip with --py-only." >&2
    R_RC=1
  else
    echo "=== R CMD check ==="
    cd "$ROOT/r"
    # Build the source tarball, then run R CMD check.
    # _R_CHECK_FORCE_SUGGESTS_=false: don't fail when testthat isn't installed
    # (B8 tests are not yet written; we only guard against compilation errors,
    # namespace problems, and R code issues).
    if R CMD build . --no-build-vignettes 2>&1; then
      PKG=$(ls socialchoicelab_*.tar.gz 2>/dev/null | head -1)
      if [[ -n "$PKG" ]]; then
        if ! _R_CHECK_FORCE_SUGGESTS_=false \
             R CMD check "$PKG" --no-manual --no-build-vignettes --no-vignettes 2>&1; then
          R_RC=1
        fi
        rm -f "$PKG"
        rm -rf socialchoicelab.Rcheck
      else
        echo "check-bindings: R CMD build produced no tarball." >&2
        R_RC=1
      fi
    else
      R_RC=1
    fi
    cd "$ROOT"
  fi
fi

# ---------------------------------------------------------------------------
# pytest
# ---------------------------------------------------------------------------

PY_RC=0
if [[ $RUN_PY -eq 1 ]]; then
  if ! command -v pytest &>/dev/null && ! python3 -m pytest --version &>/dev/null 2>&1; then
    echo "check-bindings: pytest not found. Run: pip install './python[dev]'" >&2
    PY_RC=1
  else
    echo "=== pytest ==="
    PYTEST="pytest"
    command -v pytest &>/dev/null || PYTEST="python3 -m pytest"
    # Add python/src to PYTHONPATH so the package is importable without pip install.
    if ! PYTHONPATH="$ROOT/python/src${PYTHONPATH:+:$PYTHONPATH}" $PYTEST python/tests/ --tb=short; then
      PY_RC=1
    fi
  fi
fi

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------

echo ""
if [[ $R_RC -eq 0 && $PY_RC -eq 0 ]]; then
  echo "check-bindings: all checks passed."
  exit 0
else
  [[ $R_RC -ne 0 ]]  && echo "check-bindings: R CMD check FAILED." >&2
  [[ $PY_RC -ne 0 ]] && echo "check-bindings: pytest FAILED." >&2
  exit 1
fi
