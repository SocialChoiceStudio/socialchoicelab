# Setup checklist (new or synced machine)

Use this when you clone the repo for the first time or when you land on a machine where the project was synced (e.g. via Dropbox). It ensures the build and tooling work on **this** computer.

## 1. Prerequisites (first time on this machine)

You need these before `./scripts/setup-build.sh` or any CMake build will succeed:

| Requirement | macOS (Homebrew) | Ubuntu/Debian |
|-------------|------------------|---------------|
| **CMake** (3.20+) | `brew install cmake` | `sudo apt-get install -y cmake` |
| **C++20 compiler** | Xcode CLI or `xcode-select --install` | `sudo apt-get install -y build-essential` |
| **CGAL** | `brew install cgal` | `sudo apt-get install -y libcgal-dev libgmp-dev libmpfr-dev |

GTest and Eigen are fetched by CMake; no system install needed.

**Ensure build tools are on PATH.** On macOS with Homebrew, add to `~/.zshrc` (or equivalent):

```bash
eval "$(/opt/homebrew/bin/brew shellenv)"
# or: export PATH="/opt/homebrew/bin:$PATH"
```

Then open a new terminal or `source ~/.zshrc`.

## 2. When you land on a machine (every time)

1. **Pull** so this clone is up to date:
   ```bash
   git pull
   ```
2. **Reconfigure and build** (handles synced `build/` from another machine or path):
   ```bash
   ./scripts/setup-build.sh
   ```
   Use `./scripts/setup-build.sh Debug` for a Debug build.
3. **Run tests** (optional but recommended):
   ```bash
   cd build && ctest --output-on-failure -LE benchmark
   ```

## 3. Optional: linting (format and lint)

Required only if you run `./lint.sh format` or `./lint.sh lint` (or CI-style checks):

| Tool | Purpose | macOS | Ubuntu/Debian |
|------|---------|--------|----------------|
| **clang-format 21** | Match CI formatting | `brew install llvm@21` then `export PATH="$(brew --prefix llvm@21)/bin:$PATH"` | [LLVM 21 install](https://apt.llvm.org/) then `sudo apt-get install -y clang-format-21` |
| **cpplint** | Style checks | `pip3 install cpplint` | `pip3 install --user cpplint` |

See [DEVELOPMENT.md](DEVELOPMENT.md#installation) for details.

## 4. R and Python bindings (after C++ build works)

The R and Python packages use the built `libscs_api` shared library; they do not compile C++.

1. **Build the library** (if not already built):
   ```bash
   ./scripts/setup-build.sh
   ```
   Library path: `build/libscs_api.dylib` (macOS) or `build/libscs_api.so` (Linux).

2. **Set environment** (use your actual repo path):
   ```bash
   export SCS_LIB_PATH=/absolute/path/to/socialchoicelab/build
   export SCS_INCLUDE_PATH=/absolute/path/to/socialchoicelab/include   # needed for R CMD check / devtools
   ```
   You can add these to `~/.Renviron` for R or your shell profile.

3. **R**: `R CMD INSTALL r/` then e.g. `cd r && Rscript -e "devtools::test()"` or `R CMD check r/`.
4. **Python**: `pip install -e "./python[dev]"` then `pytest python/tests/`.

See [CONTRIBUTING.md](../../CONTRIBUTING.md#binding-development-r-and-python) for full binding workflow.

## Quick summary

- **First time on this machine:** Install CMake + CGAL (and compiler if needed), put them on PATH, then run step 2.
- **Every time you land:** `git pull`, then `./scripts/setup-build.sh`, then optionally `cd build && ctest -LE benchmark`.

**One-command check:** From the repo root, run `./scripts/check-setup.sh`. It verifies prerequisites (cmake, compiler, CGAL), reports what’s missing if any, and if all are present runs the build and tests.
