# Contributing to SocialChoiceLab

Thanks for your interest in contributing. This project is part of **SocialChoiceStudio**.

## How to contribute

1. **Check current position and backlog**  
   See [docs/status/where_we_are.md](docs/status/where_we_are.md) for what we're working on next and [docs/status/roadmap.md](docs/status/roadmap.md) for near-/mid-/long-term plans.

2. **Follow code style and tooling**  
   Use the style and tooling described in [docs/development/development.md](docs/development/development.md): clang-format (version 21 in CI), cpplint, and the project's `.clang-format` and `lint.sh`. Run `./lint.sh format` and `./lint.sh lint` before committing.

3. **Run tests**  
   Build and run tests (e.g. `cd build && ctest -LE benchmark`). CI runs on push/PR to `main`; keep the tree green.

4. **Add tests for new functionality**  
   New code should have tests; prefer `tests/unit/` and existing `test_*.cpp` patterns.

5. **Write clear end-user error messages**  
   When adding or changing user-facing errors, follow the guidelines in [docs/development/development.md](docs/development/development.md#end-user-error-messages): state what failed, include what the user entered that caused it, and suggest how to fix it; when in doubt, ask the maintainer.

6. **Update documentation as needed**  
   When you add or change behaviour, update the relevant docs. The docs index is [docs/README.md](docs/README.md); keep a single source of truth and avoid duplicating the same information in multiple files.

## First-time setup

For a full clone-to-workflow checklist (tools, build, pre-commit, Cursor), see [docs/development/development.md](docs/development/development.md) § After cloning.

**Pre-commit hook (optional):** Install the formatting hook once after cloning. It runs `./lint.sh format` automatically before every commit and blocks if anything is reformatted (so you can stage the changes and recommit):

```bash
cp scripts/pre-commit.sh .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

> **Note:** `.git/hooks/` is not tracked by Git, so this step is needed on each new clone or machine.

## Git workflow

For the solo/maintainer workflow (branching, committing, pushing), see [docs/development/git_reference.md](docs/development/git_reference.md).

## Binding development (R and Python)

The R and Python packages link against `libscs_api` at runtime — they do not
compile any C or C++ code themselves. You must build `libscs_api` first and
point the binding packages at it via the `SCS_LIB_PATH` environment variable.

### 1 — Build libscs_api

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target scs_api
```

The library lands at `build/libscs_api.dylib` (macOS) or `build/libscs_api.so`
(Linux).

### 2 — R package (local development)

```bash
export SCS_LIB_PATH=/absolute/path/to/socialchoicelab/build
export SCS_INCLUDE_PATH=/absolute/path/to/socialchoicelab/include

# Install the package from source (links against libscs_api):
R CMD INSTALL r/

# Run tests:
cd r && Rscript -e "devtools::test()"

# Or run R CMD check:
R CMD check r/
```

Alternatively, add both variables to `~/.Renviron` so they are set in every
R session without having to export each time:

```
SCS_LIB_PATH=/absolute/path/to/socialchoicelab/build
SCS_INCLUDE_PATH=/absolute/path/to/socialchoicelab/include
```

`SCS_INCLUDE_PATH` is required whenever R compiles the package outside the
source tree (e.g. `devtools::check()`, `R CMD check`), because the relative
path `../../../include` in `Makevars` only resolves correctly when building
directly from the repo with `R CMD INSTALL r/`.

### 3 — Python package (local development)

```bash
export SCS_LIB_PATH=/absolute/path/to/socialchoicelab/build

# Install in editable mode (no compilation — cffi links at import time):
pip install -e "./python[dev]"

# Run tests:
pytest python/tests/
```

### 4 — Type-mapping and error-handling conventions

See [docs/status/binding_plan.md](docs/status/binding_plan.md) §§ B1.2–B1.3
for the canonical type-mapping table and the error-propagation conventions
(R: `scs_check` → `Rf_error`; Python: `_check` → exception subclass).

## Questions

Open an issue on GitHub or contact the maintainers through the SocialChoiceStudio project.
