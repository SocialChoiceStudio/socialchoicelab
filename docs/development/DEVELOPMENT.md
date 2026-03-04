# Development Guide

This document covers development practices and tools for SocialChoiceLab.

## Code Style and Linting

### Tools Used

- **clang-format**: Automatic code formatting following Google C++ Style Guide
- **cpplint**: Static analysis and style checking based on Google C++ Style Guide
- **CMake**: Build system with integrated formatting and linting targets

### Quick Start

```bash
# Format all C++ code
./lint.sh format

# Lint all C++ code  
./lint.sh lint

# Format and lint all C++ code
./lint.sh both

# Format/lint specific file
./lint.sh format include/socialchoicelab/core/rng/prng.h
./lint.sh lint src/core/rng/stream_manager.cpp

# Format/lint specific directory
./lint.sh format include/
./lint.sh lint tests/
```

### CMake Targets

From a build directory, you can run:

```bash
# Build the project
mkdir build && cd build
cmake ..
make

# Format code (runs ./lint.sh format; requires clang-format)
make format

# Lint code (runs ./lint.sh lint; requires cpplint, permissive)
make lint
```

For CI, use `./lint.sh --strict lint` so the script exits with failure if cpplint reports any issues.

**GitHub Actions:** `.github/workflows/ci.yml` runs on every push and pull_request to `main`. It builds and tests on Ubuntu and macOS, runs a format check (code must be formatted), and runs lint in strict mode. Benchmark tests are excluded (`ctest -LE benchmark`) for speed.

### Style Guide

We follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) with the following key points:

- **Indentation**: 2 spaces
- **Line length**: 80 characters maximum
- **Naming**: 
  - Classes: `PascalCase` (e.g., `PRNG`, `StreamManager`)
  - Functions: `snake_case` (e.g., `minkowski_distance`, `linear_loss`)
  - Variables: `snake_case` (e.g., `point_distance`)
  - Constants: `kConstantName` (e.g., `kMaxDimensions`)
- **Headers**: Self-contained with include guards
- **Includes**: Ordered as: related header, C system, C++ standard, other libraries, project headers

### Common Issues and Fixes

1. **Trailing whitespace**: Use `clang-format` to fix automatically
2. **Indentation**: Use `clang-format` to fix automatically  
3. **Missing copyright**: Add copyright header to new files
4. **Include order**: Use `clang-format` to fix automatically
5. **Line length**: Break long lines or use `clang-format` to wrap

### Pre-commit Workflow

Before committing code:

```bash
# 1. Format code
./lint.sh format

# 2. Check for issues
./lint.sh lint

# 3. Fix any remaining issues manually
# 4. Test the build
cd build && make

# 5. Run tests
make test
```

### Configuration Files

- `.clang-format`: clang-format configuration (Google style)
- `CMakeLists.txt`: Build configuration with format/lint targets
- `lint.sh`: Convenience script for formatting and linting

### Installation

Required tools for building (GTest and Eigen are fetched by CMake via FetchContent):

```bash
# macOS
brew install clang-format
pip3 install cpplint

# Ubuntu/Debian
sudo apt-get install clang-format
pip3 install cpplint
```

## Project Structure

```
socialchoicelab/
├── include/socialchoicelab/     # Public headers
│   ├── core/rng/                # PRNG, StreamManager
│   ├── preference/distance/     # Distance functions (Eigen-based)
│   └── preference/loss/         # Loss and utility functions
├── src/                         # Implementation files
├── tests/unit/                  # Unit tests (GTest)
├── examples/                    # Reserved for future example programs
├── docs/                        # Documentation
└── build/                       # Build artifacts (ignored by git)
```

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Testing

```bash
cd build
make test
```

**Benchmark tests:** `DistanceSpeedTest` and `PerformanceComparisonTest` are labeled `benchmark`. They report timing (no strict pass/fail on speed). To run only benchmark tests: `ctest -L benchmark`. To run unit tests only (exclude benchmarks): `ctest -LE benchmark`.

## StreamManager and RNG Policy

**Decision doc:** `docs/architecture/StreamManager_Design.md`  
**Implemented:** Feb 14, 2026 (consensus plan Item 18)

### Single-owner contract

`StreamManager` uses a **single-owner policy**. Each instance must be used by exactly one thread. Do not share a `StreamManager` across concurrent threads. For multi-threaded programs, construct a separate `StreamManager` per thread (or per run) using `reset_for_run()`.

The global `get_global_stream_manager()` is safe for single-threaded programs and tests only.

### Standard stream names

All simulation randomness must come from a declared named stream. The standard names are:

| Stream name      | Purpose |
|-----------------|---------|
| `voters`        | Voter initialization (positions, attributes) |
| `candidates`    | Candidate initialization (positions, attributes) |
| `tiebreak`      | Preference and vote tie-breaking |
| `movement`      | Candidate movement strategies (Hunter, etc.) |
| `memory_update` | Agent memory updates |
| `analysis`      | Model output computations (sampling, analytical tie-breaking) |

Do not use `std::random_device`, `rand()`, or ad-hoc RNG in simulation logic.

### Per-run seeding

Use `reset_for_run(master_seed, run_index)` at the start of each run so runs are independently reproducible:

```cpp
StreamManager sm;
for (uint64_t run = 0; run < num_runs; ++run) {
    sm.reset_for_run(master_seed, run);
    // ... run simulation using sm.get_stream("voters") etc. ...
}
```

## Contributing

1. Follow the code style guidelines
2. Add tests for new functionality
3. Update documentation as needed
4. Run the full linting suite before committing
5. Ensure all tests pass

