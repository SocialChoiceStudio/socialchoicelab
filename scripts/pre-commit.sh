#!/usr/bin/env bash
# Pre-commit hook: ensure C++ code is formatted before commit.
# Install: cp scripts/pre-commit.sh .git/hooks/pre-commit && chmod +x .git/hooks/pre-commit
#
# Runs ./lint.sh format on include/, src/, tests/unit/, then fails if any of those
# files have uncommitted changes (so you must add the formatted result and recommit).

set -e

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

if ! command -v clang-format &>/dev/null; then
  echo "pre-commit: clang-format not found. Install LLVM/clang-format or skip with --no-verify."
  exit 1
fi

./lint.sh format

if ! git diff --exit-code -- include/ src/ tests/unit/ 2>/dev/null; then
  echo ""
  echo "pre-commit: Code was reformatted. Stage the changes and commit again:"
  echo "  git add include/ src/ tests/unit/"
  echo "  git commit"
  exit 1
fi

exit 0
