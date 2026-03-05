# Contributing to SocialChoiceLab

Thanks for your interest in contributing. This project is part of **SocialChoiceStudio**.

## How to contribute

1. **Check current position and backlog**  
   See [docs/status/where_we_are.md](docs/status/where_we_are.md) for what we're working on next and [docs/status/consensus_plan.md](docs/status/consensus_plan.md) for the full prioritized backlog.

2. **Follow code style and tooling**  
   Use the style and tooling described in [docs/development/development.md](docs/development/development.md): clang-format (version 21 in CI), cpplint, and the project's `.clang-format` and `lint.sh`. Run `./lint.sh format` and `./lint.sh lint` before committing.

3. **Run tests**  
   Build and run tests (e.g. `cd build && ctest -LE benchmark`). CI runs on push/PR to `main`; keep the tree green.

4. **Add tests for new functionality**  
   New code should have tests; prefer `tests/unit/` and existing `test_*.cpp` patterns.

5. **Update documentation as needed**  
   When you add or change behaviour, update the relevant docs. The docs index is [docs/README.md](docs/README.md); keep a single source of truth and avoid duplicating the same information in multiple files.

## Git workflow

For the solo/maintainer workflow (branching, committing, pushing), see [docs/development/git_reference.md](docs/development/git_reference.md).

## Questions

Open an issue on GitHub or contact the maintainers through the SocialChoiceStudio project.
