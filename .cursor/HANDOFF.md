# Handoff for next agent

**Context:** The user will reopen **this directory** (`socialchoicelab`) as the Cursor workspace root. All paths below are relative to this repo root.

## What’s already set up

- **Repo:** Cloned from `git@github.com:SocialChoiceStudio/socialchoicelab.git`. Lives at `SocialChoiceStudioGitDev/socialchoicelab/` on this machine. Git push works (SSH, user has write access).
- **Git:** Commit email set to `ragan_ra@mercer.edu` (global). Name: Robi Ragan.
- **C++ build:** CMake + CGAL (Homebrew). Built with `./scripts/setup-build.sh`. `build/` contains `libscs_api.dylib`. C++ tests: `cd build && ctest -LE benchmark` (34 tests, passing).
- **R package:** Installed and working (R CMD INSTALL r/ was run; `SCS_LIB_PATH` / `SCS_INCLUDE_PATH` used during install).
- **Python:** Venv at `.venv/` (Python 3.12 from Homebrew). Package installed in editable mode with `[dev]`. `SCS_LIB_PATH` and `SCS_INCLUDE_PATH` are set in `.venv/bin/activate` and in `.vscode/settings.json` → `terminal.integrated.env.osx`. Interpreter: `.venv/bin/python`. Run tests: `pytest python/tests/` (279 tests).
- **Pre-commit hook:** `.git/hooks/pre-commit` runs `./lint.sh format` and blocks commit if C++ is reformatted.
- **VS Code/Cursor:** `.vscode/settings.json` sets default Python to `${workspaceFolder}/.venv/bin/python` and `python.terminal.activateEnvironment: true`. Env vars for the terminal point at `build/` and `include/`. Works when this directory is the workspace root.

## If something breaks

- **Python can’t find libscs_api:** Ensure `SCS_LIB_PATH` points at `build/` (e.g. `export SCS_LIB_PATH=$(pwd)/build` from repo root, or use the venv / integrated terminal which set it).
- **Build from scratch:** `./scripts/setup-build.sh`. Full check: `./scripts/check-setup.sh`.
- **R/Python bindings:** See `CONTRIBUTING.md` § Binding development.

User preference: fix at the cause, not the symptom; propose solutions and ask before implementing; no silent fallbacks—use errors or propose fallbacks for approval.
