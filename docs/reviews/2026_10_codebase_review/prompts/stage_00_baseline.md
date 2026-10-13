# Stage 0 prompt — Baseline

**Model:** Sonnet, medium effort. **Agents:** 1.

---

You are running Stage 0 of a staged code review of the socialchoicelab repository at
`/Users/robiragan/Dropbox/SocialChoiceStudioDev/socialchoicelab`. Your job is to collect
facts from tools so that later reviewers do not have to guess. You do **not** review code
by reading it, and you do **not** fix anything.

Read `docs/reviews/2026_10_codebase_review/review_rubric.md` first (the "Agent rules"
apply to you). Write your results to
`docs/reviews/2026_10_codebase_review/stage_00_baseline.md`.

## Hard constraints

- **No tracked file may change**, and no new files may appear in the repo other than your
  output file. Record `git status --porcelain` at the start and compare at the end; report
  any difference.
- All build directories, logs, and copies go under
  `SCR=/private/tmp/claude-501/-Users-robiragan-Dropbox-SocialChoiceStudioDev/2402f97e-221e-494b-88cb-fc63c41efc50/scratchpad/stage0`.
  Never use the repo's `build/` directory (it is the developer's working build).
- Do **not** run `./lint.sh format`, `./lint.sh both`, `./scripts/check-all.sh`, or
  `./scripts/check-bindings.sh` — they write into the repo. Run the underlying steps
  yourself as described below.
- Every tool below is installed and **required**. Do not install anything else. If a tool
  fails to run, stop that step, record the exact error, and report it — do not silently
  skip it or substitute a weaker check.
  - clang-tidy: `/opt/homebrew/opt/llvm/bin/clang-tidy` and
    `/opt/homebrew/opt/llvm/bin/run-clang-tidy` (Homebrew LLVM is keg-only; use full paths.
    Do not change the compiler used for builds — builds use Apple clang.)
  - ruff (on PATH), R packages lintr and covr, Python pytest-cov.
- Use `PYTHONDONTWRITEBYTECODE=1`, `pytest -p no:cacheprovider`, `ruff --no-cache`, and
  `COVERAGE_FILE=$SCR/logs/.coverage` so nothing writes caches into the repo.
- Keep raw logs in `$SCR/logs/`. Put only summaries in the output file; quote at most
  ~30 lines of any one log.
- Never read or print `.env` or `.github_token`.

## Steps

1. **Context.** Record `git rev-parse --short HEAD`, the branch, `git status --porcelain`,
   and versions of cmake, Apple clang, Python, R, pytest, cpplint.

2. **Release build + C++ tests.**
   `cmake -S . -B $SCR/build_release -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`
   (clang-tidy uses the compile database in step 8) then build, then
   `ctest --test-dir $SCR/build_release --output-on-failure`. Record pass/fail per test and
   wall time per test (`ctest` prints it). Flag any test taking >10 s.

3. **Warnings build.** Debug build in `$SCR/build_warn` with
   `-DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion"`.
   Summarize warnings by warning type and by file (project files only — ignore Eigen and
   other third-party headers). List every warning outside `-Wconversion`/`-Wsign-conversion`
   individually with `file:line`; give counts only for the conversion families.

4. **Sanitizer build.** Debug build in `$SCR/build_asan` with
   `-DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined"`
   and matching linker flags. Run `ctest`. Report every sanitizer error with the test name,
   error type, and the top project-code stack frame. If performance/speed tests time out
   under ASan, note it and move on.

5. **C++ coverage.** Build in `$SCR/build_cov` with
   `-fprofile-instr-generate -fcoverage-mapping` (compiler and linker). Run `ctest` with
   `LLVM_PROFILE_FILE=$SCR/build_cov/prof/%p-%m.profraw`, merge with
   `xcrun llvm-profdata merge`, then `xcrun llvm-cov report` over all test binaries,
   restricted to `include/socialchoicelab/` and `src/`. Report line and function coverage
   per file, sorted lowest first. List every file below 70% line coverage.

6. **Python tests.** With `SCS_LIB_PATH=$SCR/build_release`,
   `SCS_INCLUDE_PATH=<repo>/include`, `PYTHONPATH=<repo>/python/src`, run
   `python3 -m pytest -p no:cacheprovider python/tests -q -rA --cov=socialchoicelab --cov-report=term-missing`.
   Record pass/fail/skip counts, every failure or skip with its reason, and line coverage
   per module sorted lowest first (list modules below 70%).

7. **R tests.** Copy `r/` to `$SCR/r_copy` and work only there. With the same env vars,
   run `R CMD build . --no-build-vignettes` then
   `_R_CHECK_FORCE_SUGGESTS_=false R CMD check <tarball> --no-manual --no-build-vignettes --no-vignettes`.
   Record every ERROR, WARNING, and NOTE verbatim (they are short), and the testthat
   summary. Then, still in `$SCR/r_copy`, run
   `Rscript -e 'cov <- covr::package_coverage("."); print(cov, by = "line"); print(covr::coverage_to_list(cov))'`
   and report line coverage per R file, sorted lowest first (list files below 70%).

8. **Linters that do not write files.**
   - `cpplint --root=include` over `include/`, `src/`, `tests/unit/` (same as `./lint.sh lint`,
     which you may also run). Counts by category, then the list.
   - `clang-format --dry-run --Werror` over the same paths: list files that are not
     formatted (do not format them).
   - **clang-tidy**, two passes using `-p $SCR/build_release` over every project `.cpp`
     in `src/` and `tests/unit/`, with
     `--header-filter='.*/(include/socialchoicelab|src)/.*'` so project headers are
     analysed but Eigen/third-party are not:
     (a) the repo's own `.clang-tidy` config;
     (b) a broader pass:
     `--checks='-*,bugprone-*,clang-analyzer-*,performance-*,concurrency-*,cppcoreguidelines-pro-type-member-init,cppcoreguidelines-narrowing-conversions,misc-*,-misc-include-cleaner,-misc-non-private-member-variables-in-classes'`.
     Report each pass separately: counts by check name, then every diagnostic from
     `bugprone-*` and `clang-analyzer-*` individually with `file:line` (deduplicated across
     translation units); counts only for the rest.
   - **ruff** (no repo config exists, so be explicit):
     `ruff check --no-cache --select E,F,W,B,UP,SIM,PL,RUF python/src python/tests app`.
     Counts by rule, then every `F` (pyflakes) and `B` (bugbear) diagnostic individually.
   - **lintr**, in `$SCR/r_copy`: `Rscript -e 'print(lintr::lint_package("."))'` with
     default linters. Counts by linter, then the list for anything other than style
     linters (line length, indentation, quotes, spacing, object naming).

9. **Repo inventory.**
   - Tracked files that look like artifacts: `git ls-files` matches for `__pycache__`,
     `.pyc`, `.DS_Store`, `.Rcheck`, `*.o`, `*.so`, `*.dylib`, build outputs, and any tracked
     file >1 MB (list size).
   - Top-level directories and their tracked vs. untracked status and size
     (`build`, `build17`, `debug`, `scratch`, `temp`, `tmp`, `r/tmp`, `papers_and_presentations`,
     `agent_conversations`, `python/notebooks`, demo scripts). State whether each is in
     `.gitignore`. Do not judge whether they should exist; just report.
   - Count of `TODO`/`FIXME`/`XXX`/`HACK` comments per top-level area, with the list for
     `include/`, `src/`, `r/R/`, `python/src/`.

10. **Final check.** Re-run `git status --porcelain` and confirm it matches step 1.

## Output file structure

```
# Stage 00 — Baseline
Model / effort: Sonnet / medium   Commit: <sha>   Date: <date>

## Summary
Bullets: build status, test totals per layer, sanitizer errors, coverage headline,
lowest-coverage files, anything alarming.

## 1. Environment
## 2. C++ build and tests
## 3. Compiler warnings
## 4. Sanitizers
## 5. C++ coverage
## 6. Python tests
## 7. R CMD check
## 8. Linters
## 9. Repo inventory
## 10. Repo unchanged check
## Tool failures (if any)
```

Use rubric finding blocks (IDs `S00-nnn`) only for sanitizer errors and test failures;
everything else is a report, not a finding.
