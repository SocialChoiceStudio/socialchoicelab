#!/usr/bin/env bash
# End-of-milestone: clang-tidy check, CHANGELOG reminder, commit+push, then create and push a tag.
#
# Usage: ./scripts/end-of-milestone.sh <tag-name> [--no-push]
#   tag-name    Git tag to create, e.g. phase-3 or v0.1.0
#   --no-push   Commit and tag locally but do not push either to GitHub.

set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if ! git rev-parse --show-toplevel >/dev/null 2>&1; then
  echo "Error: not a git repository. Run from the repo root."
  exit 1
fi

# --- Parse args ---
TAG=
NO_PUSH_FLAG=
for arg in "$@"; do
  case "$arg" in
    --no-push) NO_PUSH_FLAG="--no-push" ;;
    *) TAG="$arg" ;;
  esac
done

if [ -z "$TAG" ]; then
  echo "Error: provide a tag name."
  echo "Usage: ./scripts/end-of-milestone.sh <tag-name> [--no-push]"
  echo "Examples: phase-3   v0.1.0   phase-3-complete"
  exit 1
fi

# Refuse to overwrite an existing tag
if git tag | grep -qx "$TAG"; then
  echo "Error: tag '$TAG' already exists. Choose a different name."
  exit 1
fi

# --- Step 1: clang-tidy (advisory) ---
echo "=== clang-tidy ==="
if ! command -v clang-tidy >/dev/null 2>&1; then
  echo "clang-tidy not found (install with: brew install llvm). Skipping."
else
  if [ ! -f "$ROOT/build/compile_commands.json" ]; then
    echo "Generating build/compile_commands.json..."
    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build -DCMAKE_BUILD_TYPE=Release >/dev/null
  fi
  n=0
  clang_tidy_exit=0
  while IFS= read -r -d '' f; do
    clang-tidy "$f" -p build 2>&1 || clang_tidy_exit=1
    n=$((n + 1))
  done < <(find include src tests/unit -type f \( -name '*.h' -o -name '*.cpp' \) -print0 2>/dev/null)
  echo "Ran clang-tidy on $n file(s)."
  if [ "$clang_tidy_exit" -ne 0 ]; then
    echo ""
    echo "clang-tidy reported issues above. Review them before proceeding."
    read -r -p "Continue anyway? [y/N] " ans
    case "$ans" in [yY]*) ;; *) echo "Fix issues and re-run."; exit 1 ;; esac
  fi
fi

# --- Step 2: CHANGELOG reminder ---
echo ""
echo "=== Reminder ==="
echo "Before tagging '$TAG': CHANGELOG.md should have a [$TAG] section under [Unreleased]."
read -r -p "Is CHANGELOG.md up to date? [y/N] " ans
case "$ans" in
  [yY]*) ;;
  *) echo "Update CHANGELOG.md first, then re-run."; exit 1 ;;
esac

# --- Step 3: Commit + push all changes via end-of-session ---
echo ""
echo "=== Running end-of-session ==="
"$ROOT/scripts/end-of-session.sh" $NO_PUSH_FLAG "End of milestone $TAG"

# --- Step 4: Create and push the tag ---
echo ""
echo "=== Tagging $TAG ==="
git tag -a "$TAG" -m "Milestone: $TAG"

if [ -z "$NO_PUSH_FLAG" ]; then
  if [ -f "$ROOT/.github_token" ] && [ -z "${GITHUB_TOKEN:-}" ]; then
    export GITHUB_TOKEN=$(head -1 "$ROOT/.github_token" | tr -d '\r\n')
  fi
  git push origin "$TAG"
  echo "Tag '$TAG' pushed to GitHub."
else
  echo "Tag '$TAG' created locally (--no-push: not pushed to GitHub)."
fi

echo ""
echo "=== Milestone $TAG complete ==="
