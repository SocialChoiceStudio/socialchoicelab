#!/usr/bin/env bash
# End-of-session: update where_we_are.md, commit everything, push.
#
# Usage: ./scripts/end-of-session.sh [--no-push] ["optional commit message"]
#   --no-push   Commit but do not push to GitHub.

set -e
set -o pipefail

NO_PUSH=
MSG=
for arg in "$@"; do
  case "$arg" in
    --no-push) NO_PUSH=1 ;;
    *) MSG="$arg" ;;
  esac
done

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if ! git rev-parse --show-toplevel >/dev/null 2>&1; then
  echo "Error: not a git repository. Run from the repo root."
  exit 1
fi

today=$(date +%Y-%m-%d)

# --- Step 1: Update where_we_are.md pointer from Consensus Plan ---
echo "=== Updating where_we_are.md ==="
NEXT_LINE=$(./scripts/update-where-we-are.sh 2>/dev/null | tail -1)
if [ -z "$NEXT_LINE" ] || [ "$(echo "$NEXT_LINE" | awk -F'|' '{print NF}')" -lt 4 ]; then
  echo "Error: update-where-we-are.sh did not return expected phase|num|title|severity output."
  exit 1
fi
phase=$(echo "$NEXT_LINE" | cut -d'|' -f1)
num=$(echo "$NEXT_LINE" | cut -d'|' -f2)
title=$(echo "$NEXT_LINE" | cut -d'|' -f3)
echo "Next: $phase — Item $num: ${title%.}"

# --- Step 2: Append a dated session entry to WHERE_WE_ARE Recent Work (skip if today already present) ---
WHERE="$ROOT/docs/status/where_we_are.md"
if ! grep -q "### Session: $today" "$WHERE" 2>/dev/null; then
  active_plan=$(ls "$ROOT"/docs/status/consensus_plan*.md 2>/dev/null | sort -V | tail -1 | sed "s|$ROOT/||")
  printf '\n### Session: %s\n\nNext: %s — Item %s: %s.\nSee `%s` for details.\n' \
    "$today" "$phase" "$num" "${title%.}" "$active_plan" >> "$WHERE"
fi

# --- Step 3: Stage everything ---
echo ""
echo "=== Staging ==="
git add -A
if git diff --cached --quiet; then
  echo "Nothing to commit. Working tree is clean."
  exit 0
fi
git status --short

# --- Step 4: Commit ---
echo ""
echo "=== Committing ==="
if [ -z "$MSG" ]; then
  MSG="End of session $today: next is Item $num"
fi
git commit -m "$MSG"

# --- Step 5: Push ---
if [ -z "$NO_PUSH" ]; then
  echo ""
  echo "=== Pushing ==="
  if [ -f "$ROOT/.github_token" ] && [ -z "${GITHUB_TOKEN:-}" ]; then
    export GITHUB_TOKEN=$(head -1 "$ROOT/.github_token" | tr -d '\r\n')
  fi
  git push
  echo ""
  echo "Done. Committed and pushed."
  echo "Next: $phase — Item $num: ${title%.}."
else
  echo ""
  echo "Done. Committed (--no-push: not pushed)."
  echo "Next: $phase — Item $num: ${title%.}."
fi
