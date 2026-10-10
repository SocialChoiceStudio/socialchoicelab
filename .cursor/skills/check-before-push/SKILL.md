---
name: check-before-push
description: >-
  Runs ./scripts/check-all.sh before a git push. Use when the user asks to
  push, or before pushing to origin. Does not push or merge by itself.
---

# Check before push

From the repo root, run this and push only if it exits 0:

```bash
export SCS_LIB_PATH=$(pwd)/build
export SCS_INCLUDE_PATH=$(pwd)/include
./scripts/check-all.sh
```

`check-all.sh` runs these in order and stops at the first failure:

1. Format check (`./lint.sh format` + `git diff --exit-code`)
2. C++ build
3. C++ tests (`ctest -LE benchmark`)
4. Strict lint (`cpplint`)
5. Binding checks (`R CMD check` + `pytest`)
6. R pkgdown site build
7. Python mkdocs site build

- Push only after `./scripts/check-all.sh` exits 0.
- Run all seven steps.
- A missing tool (mkdocs, Rscript) gets installed, then the script is re-run.
- GitHub CI (`.github/workflows/ci.yml`) covers the core, R, and Python binding jobs. This script also builds the two doc sites.
- The pre-push hook (`scripts/pre-push.sh`, installed as `.git/hooks/pre-push`) calls `check-all.sh` when `r/` or `python/` files change. After success it restores `r/docs` and `docs/site` from the index so the doc builds do not leave a dirty tree.
- Reinstall the hook after a fresh clone: `cp scripts/pre-push.sh .git/hooks/pre-push && chmod +x .git/hooks/pre-push`
