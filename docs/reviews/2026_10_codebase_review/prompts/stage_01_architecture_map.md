# Stage 1 prompt — Architecture map

**Model:** Opus, high effort. **Agents:** 1.

---

You are running Stage 1 of a staged code review of socialchoicelab
(`/Users/robiragan/Dropbox/SocialChoiceStudioDev/socialchoicelab`) and its companion website
(`/Users/robiragan/Dropbox/SocialChoiceStudioDev/socialchoicestudiowebsite`).

Your job is to produce the **shared map** that every later review agent reads before
looking at its own slice. Later agents will see only the rubric, this map, and their
assigned files, so the map must give them enough context to judge their slice correctly
without re-reading the whole project. You are **not** hunting for bugs; deep review
happens in later stages.

Read first:
1. `docs/reviews/2026_10_codebase_review/review_rubric.md` (the agent rules apply to you).
2. `docs/reviews/2026_10_codebase_review/README.md` (the stage plan).
3. `docs/reviews/2026_10_codebase_review/stage_00_baseline.md` (tool results, coverage).

Write your output to `docs/reviews/2026_10_codebase_review/stage_01_architecture_map.md`.
Target length: 400–700 lines. Dense and factual; tables over prose.

## Hard constraints

- Do not modify any file other than your output. Do not commit. Do not build or run tests
  (Stage 0 already did).
- Never read or print `.env` or `.github_token`.
- Do not read `old_codebases/`, `agent_conversations/`, `docs/site/`, `docs/status/archive/`,
  `build*/`, `scratch/`, `temp/`, `tmp/`, or vendored code (Eigen, three.js).

## What to read

**Read in full** (these are the stated design; about 6,500 lines):
- `docs/README.md`, `docs/status/WHERE_WE_ARE.md`, `docs/status/ROADMAP.md`,
  `docs/status/MILESTONE_GATES.md`, `docs/status/follow_up_queue.md`, `.cursor/HANDOFF.md`
- Everything in `docs/architecture/` and `docs/development/DEVELOPMENT.md`
- `.cursor/rules/*.mdc` (the project's working rules)

**Skim for structure only** — includes, namespaces, public type and function
declarations, top-of-file comments, citations. Do **not** read implementations line by line:
- `include/socialchoicelab/**` and `src/**`
- `include/scs_api.h` (read its section headers, handle types, error codes, ownership
  comments; not every prototype)
- `r/R/*.R`, `r/src/*`, `r/DESCRIPTION`, `r/NAMESPACE`
- `python/src/socialchoicelab/*.py`, `python/pyproject.toml`
- `r/inst/htmlwidgets/*.js` (not `lib/`), `app/app.py`
- `socialchoicestudiowebsite/README.md`, `backend/app/*.py` (structure only)

## What to produce

Use this structure:

```
# Stage 01 — Architecture map
Model / effort: Opus / high   Commit: <socialchoicelab sha>   Website commit: <sha>

## 1. One-paragraph overview
What the project is, who uses it, and the layer stack.

## 2. Layer and module map
Table: module (path) | purpose | key types/functions | depends on | lines | Stage 0 coverage.
Cover C++ core (per subdirectory), C API, R package, Python package, JS widgets,
Shiny app, website backend.

## 3. Dependency graph
Which modules include/call which. Mermaid diagram plus a note on any layering
violations you notice (e.g. core depending on competition) — note them, don't
investigate.

## 4. Key data flows
For each main pipeline, the path from user call to result through every layer, naming
the function at each layer. At minimum:
 - spatial voters → distance/loss → preference profile → voting rule winner
 - geometry: winset / yolk / uncovered set / heart / core (2D, CGAL EPEC)
 - competition: setup → engine run → trace/experiment outputs
 - RNG: stream manager seeding and how streams reach each consumer
 - plotting: binding → overlay schema → JS canvas
 - website request → Python binding → result

## 5. Cross-cutting conventions
How each of these is done, with one representative `path:line` each:
 - error handling at each layer (C++ exceptions → C API codes + err_buf → R/Python errors)
 - memory ownership across the C API (handles, create/destroy, caller-provided buffers,
   two-call size/fill patterns, `_heap` variants)
 - numeric tolerances and exact vs. floating geometry (where CGAL EPEC is used vs. double)
 - randomness and reproducibility
 - R/Python parity mechanism (how the two packages stay in sync)

## 6. Stated invariants and contracts
Numbered list of every invariant or guarantee the docs state (units, coordinate
conventions, tie-breaking, ordering of outputs, determinism, thread-safety, ownership).
Each with its source doc and section. Mark any you can see contradicted by a header
declaration or comment as "⚠ possible mismatch" with both locations — do not dig further.

## 7. Published algorithms
Table: algorithm | file | cited source (as written in code/docs) | "faithful replica"
claimed? (yes/no/unclear). Stage 2 reviewers check correctness against these.

## 8. Known issues already on record
Everything open in follow_up_queue.md, HANDOFF.md, WHERE_WE_ARE.md, and MILESTONE_GATES.md,
one line each with source, so later stages do not re-report known items as new findings.

## 9. Slice plan check for Stages 2–9
Using actual line counts, confirm or adjust the slices in the README stage table so that
each agent gets roughly 1,500–3,000 lines of real source. For Stage 3, propose the exact
line ranges of `src/c_api/scs_api.cpp` for its three slices, split at function
boundaries along functional groups. List exact files per slice. Flag any file that does
not fit any slice.

## 10. Hotspots for later stages
Up to 15 bullets: areas that look risky from the structure alone (complexity, low
coverage from Stage 0, clang-tidy flags, duplicated logic between R and Python, recent
heavy churn per `git log --stat`), each naming which stage should look closely.

## Open questions
Things the docs leave ambiguous that later reviewers might need answered by the
project owner.
```

Do not use finding blocks in this stage, except: if while skimming you see something that is
unmistakably a P0 (wrong results or crash on normal input), record it as a finding block with
ID `S01-nnn` at the end, and keep going.
