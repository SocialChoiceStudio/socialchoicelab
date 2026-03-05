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

## First-time setup: pre-commit hook

Install the formatting hook once after cloning. It runs `./lint.sh format` automatically before every commit and blocks if anything is reformatted (so you can stage the changes and recommit):

```bash
cp scripts/pre-commit.sh .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

> **Note:** `.git/hooks/` is not tracked by Git, so this step is needed on each new clone or machine.

## Git workflow

For the solo/maintainer workflow (branching, committing, pushing), see [docs/development/git_reference.md](docs/development/git_reference.md).

## Questions

Open an issue on GitHub or contact the maintainers through the SocialChoiceStudio project.
