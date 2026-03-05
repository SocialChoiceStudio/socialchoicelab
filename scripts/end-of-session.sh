#!/usr/bin/env bash
# End-of-session: commit all changes and push.
#
# Before running, make sure you (or an agent) have:
#   1. Updated docs/status/where_we_are.md with what was done and what is next.
#   2. Marked any completed plan steps as ✅ Done in the relevant plan doc.
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

# --- Step 1: Warn if where_we_are.md has not been updated today ---
WHERE="$ROOT/docs/status/where_we_are.md"
if ! grep -q "Last updated: $today" "$WHERE" 2>/dev/null; then
  echo "Warning: docs/status/where_we_are.md does not show today's date ($today)."
  echo "         Update it before committing if the session made progress."
  echo ""
fi

# --- Step 2: Stage everything ---
echo "=== Staging ==="
git add -A
if git diff --cached --quiet; then
  echo "Nothing to commit. Working tree is clean."
  exit 0
fi
git status --short

# --- Step 3: Commit ---
echo ""
echo "=== Committing ==="
if [ -z "$MSG" ]; then
  MSG="End of session $today"
fi
git commit -m "$MSG"

# --- Step 4: Push ---
if [ -z "$NO_PUSH" ]; then
  echo ""
  echo "=== Pushing ==="
  git push
  echo ""
  echo "Done. Committed and pushed."
else
  echo ""
  echo "Done. Committed (--no-push: not pushed)."
fi
