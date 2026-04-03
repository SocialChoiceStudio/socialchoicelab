#!/usr/bin/env bash
# pre-push.sh — pre-push hook that runs the full check suite when r/ or python/ changed.
#
# Install:
#   cp scripts/pre-push.sh .git/hooks/pre-push && chmod +x .git/hooks/pre-push
#
# Skip for a single push (use sparingly):
#   git push --no-verify
#
# The check is skipped automatically when neither r/ nor python/ files were
# changed in the commits being pushed.

set -euo pipefail

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

# Collect all commits being pushed (stdin format: <local-ref> <local-sha> <remote-ref> <remote-sha>)
CHANGED_BINDINGS=0
while read -r local_ref local_sha remote_ref remote_sha; do
  # remote_sha is all-zeros when the remote branch doesn't exist yet.
  if [[ "$remote_sha" == "0000000000000000000000000000000000000000" ]]; then
    BASE="$(git rev-list --max-parents=0 "$local_sha")"
  else
    BASE="$remote_sha"
  fi

  if git diff --name-only "$BASE" "$local_sha" 2>/dev/null | grep -qE '^(r|python)/'; then
    CHANGED_BINDINGS=1
    break
  fi
done

if [[ $CHANGED_BINDINGS -eq 0 ]]; then
  exit 0  # no binding files changed; skip the check
fi

echo "pre-push: r/ or python/ files changed — running full check suite."
echo "pre-push: (skip with: git push --no-verify)"
echo ""

if ! "$ROOT/scripts/check-all.sh"; then
  echo "" >&2
  echo "pre-push: checks FAILED. Fix the errors above before pushing." >&2
  echo "pre-push: to bypass (use sparingly): git push --no-verify" >&2
  exit 1
fi

# check-all runs pkgdown and mkdocs, which rewrite tracked trees under r/docs and
# docs/site. That would leave the working tree dirty after an otherwise good
# push. Restore those paths from the index and drop stray untracked outputs
# there so the hook only *verifies* site builds. To commit regenerated docs,
# run ./scripts/check-all.sh, commit r/docs and/or docs/site, then push.
echo ""
echo "pre-push: restoring r/docs and docs/site to match index (sites were verified)."
git restore r/docs docs/site
git clean -fdq -- r/docs docs/site 2>/dev/null || true
