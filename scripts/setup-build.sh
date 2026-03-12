#!/usr/bin/env bash
# setup-build.sh — ensure a valid build for this machine (multi-machine workflow).
#
# CMake caches absolute paths. A build/ created on another machine (e.g. different
# username or path) will fail here with "source directory does not exist".
# This script reconfigures and builds when needed.
#
# Usage (from repo root):
#   ./scripts/setup-build.sh
#
# Optional: build type (default Release)
#   ./scripts/setup-build.sh Debug

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

BUILD_TYPE="${1:-Release}"
BUILD_DIR="$ROOT/build"
CACHE="$BUILD_DIR/CMakeCache.txt"

needs_reconfigure() {
  if [[ ! -f "$CACHE" ]]; then
    return 0
  fi
  # CMake stores the source path that was used; if it differs, we must reconfigure.
  CACHED_SOURCE=$(grep -E '^CMAKE_HOME_DIRECTORY:INTERNAL=' "$CACHE" 2>/dev/null | cut -d= -f2-)
  if [[ -z "$CACHED_SOURCE" ]]; then
    return 0
  fi
  # Cached path from another machine (e.g. different username) won't exist here.
  if [[ ! -d "$CACHED_SOURCE" ]]; then
    return 0
  fi
  # Normalize for comparison (resolve symlinks).
  CACHED_NORM=$(cd "$CACHED_SOURCE" && pwd -P)
  ROOT_NORM=$(cd "$ROOT" && pwd -P)
  if [[ "$CACHED_NORM" != "$ROOT_NORM" ]]; then
    return 0
  fi
  return 1
}

if needs_reconfigure; then
  if [[ -d "$BUILD_DIR" ]]; then
    echo "Build directory was created on another machine or path. Reconfiguring."
    rm -rf "$BUILD_DIR"
  fi
  cmake -B build -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
fi

cmake --build build
