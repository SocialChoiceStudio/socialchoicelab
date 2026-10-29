# Codebase review — October 2026

Top-to-bottom review of socialchoicelab (and, in Stage 8, socialchoicestudiowebsite).
Review only: no source changes are made during the review. Fixes come afterwards from
the ranked plan produced in Stage 11.

Every review agent reads [review_rubric.md](review_rubric.md) first and writes its output
to `stage_NN_<area>.md` in this folder. Agent prompts live in [prompts/](prompts/).

Excluded: `old_codebases/`, `agent_conversations/`, vendored third-party code
(e.g. `r/inst/htmlwidgets/lib/three/`, Eigen).

## Stages

| # | Stage | Agents | Model / effort | Status |
|---|---|---|---|---|
| 0 | Baseline: builds, tests, sanitizers, warnings, coverage, lint, repo inventory | 1 | Sonnet / medium | done (stage_00_baseline.md) |
| 1 | Architecture map and stated invariants | 1 | Opus / high | done (stage_01_architecture_map.md) |
| 2 | C++ core: 2a core+rng+preference, 2b geometry, 2c aggregation, 2d competition | 4 | Opus / high | — |
| 3 | C API: 3h header contract; `scs_api.cpp` slices 3a/3b/3c (line ranges in stage_01 §9); run `test_c_api` and the Python/R tests under macOS `leaks --atExit` (LeakSanitizer is unavailable with Apple clang, per Stage 0) | 4 | Opus / high | — |
| 4 | C++ tests: 4a core/preference, 4b geometry I, 4c geometry II, 4d aggregation+competition, 4e/4f `test_c_api.cpp` halves | 6 | Sonnet / high | — |
| 5 | Bindings: 5a R code, 5b R C glue, 5c Python infra (incl. cdef vs. `scs_api.h` diff), 5d Python objects | 4 | Sonnet / high | — |
| 6 | Parity: 6a signature inventory; 6b non-plot behaviour + binding tests; 6c static plot tests; 6d competition canvas tests | 4 | 6a Haiku; 6b–6d Sonnet / high | — |
| 7 | Plotting + JS: 7a static plot bindings, 7b competition/surface bindings, 7c static JS, 7d competition JS | 4 | Sonnet / medium | — |
| 8 | Shiny app + `service.py` + website backend | 1 | Sonnet / high | — |
| 9 | 9a docs consistency; 9b repo hygiene (build/CI, demos, duplicated assets, generated docs) | 2 | Haiku / medium | — |
| 10 | Adversarial verification of every P0/P1 | per finding | Opus (C++/C API), Sonnet (rest) | — |
| 11 | Synthesis: dedupe, cross-cutting themes, ranked fix plan | 1 | Opus / high | — |

Exact files and line ranges for every slice are in
[stage_01_architecture_map.md](stage_01_architecture_map.md) §9.

Every agent from Stage 2 on also reads [owner_questions.md](owner_questions.md); the
owner's answers there define intended behaviour.

Stages 0 and 1 run alone. Agents within a later stage may run in parallel; stages run one
at a time so the rubric can be adjusted between them.

## Where to run sessions

- **Review (Stages 0–11):** run from the workspace folder
  (`SocialChoiceStudioDev/`), because the review spans both projects.
- **Before Stage 8:** `socialchoicestudiowebsite/` must be its own git repository.
  Done 2026-10-13: private repo SocialChoiceStudio/socialchoicestudiowebsite, initial
  commit `03d9c52`. Stage 8 reviews that commit.
- **Fix phase (after Stage 11): switch workspaces.** Start each fix session directly in
  `socialchoicelab/` (or `socialchoicestudiowebsite/` for website fixes), not the workspace
  folder, so the Claude app's diff pane, worktrees, and PR/CI tracking work.

## Files

- `review_rubric.md` — severity scale, finding format, checklist, agent rules.
- `owner_questions.md` — design questions raised by reviewers, with the owner's answers.
- `prompts/stage_NN_*.md` — the exact prompt given to each agent.
- `stage_NN_<area>.md` — each agent's findings.
- `findings_ledger.md` — merged, deduplicated findings (built in Stage 11).
