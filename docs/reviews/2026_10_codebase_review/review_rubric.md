# Review rubric

Read this in full before reviewing anything.

## Agent rules

1. **Review only.** Do not edit, create, or delete any tracked file other than your own
   `stage_NN_<area>.md` output. Experiments and scratch tests go in the session scratchpad.
2. **Stay in your slice.** Read the files you were assigned, plus the minimum needed to
   understand a call into another layer. If you suspect a bug outside your slice, log it
   under "Out-of-slice notes" in one line; do not investigate it.
3. **Read whole files** in your slice, not excerpts. If your slice is too large to read
   completely, say so at the top of your output and list what you did not read.
4. **Every finding needs evidence**: `path:line`, what is wrong, and a concrete failure
   scenario (inputs or state → wrong result, crash, leak, or misleading output). No
   scenario → it is a question, not a finding; put it under "Open questions".
5. **Say how sure you are.** `confirmed` = you demonstrated it (ran code, wrote a failing
   scratch test, or the logic is unambiguous). `plausible` = reasoned but not demonstrated.
6. **Do not report what tools already catch** (formatting, cpplint style). Stage 0 owns that.
7. **Do not pad.** An honest "no P0/P1 found in this slice" is a useful result.
8. Never read or print `.env` or `.github_token`.

## Owner decisions

[owner_questions.md](owner_questions.md) records the owner's answers. They are the
contract for intended behaviour and override the code, the design docs, and your own
preferences.

1. **Decided work is not a new finding.** Anything already listed under an answer's
   "Implies" is known. Do not re-report it. Do report **additional instances** the
   "Implies" list missed (another `std::` distribution, another lowest-index tie in
   competition, another unscaled `pow`). Give them IDs as usual and add
   `Decision: Q<n>` to the finding's **Where** line.
2. **Withdrawn code is not reviewed.** Heart code (Q1) is being removed. Do not review it
   for correctness; list any remaining Heart code, tests, C API entries or doc claims once,
   as P3 "to be removed".
3. **Answers are rules.** Code or docs that contradict an answer are defects, usually P1.
   Each answer's "For reviewers" paragraph lists what to look for.
4. **"Flag both ways"** (e.g. the Predator default in Q8) means report the issue and the
   options, without deciding which is right.
5. **Unanswered questions** (blank **Answer:**) mean intended behaviour is unknown. Report
   the behaviour as an open question, not a defect.
6. **Cross-cutting principle — Laver & Sergenti as reference, not target.** Divergence from
   L&S is not a defect by itself. It is a P3 documentation finding if the difference is not
   documented.

## Severity

| Level | Meaning | Examples |
|---|---|---|
| **P0** | Wrong results, crash, memory corruption, or data loss on reachable input | Off-by-one in a voting rule; use-after-free across the C API; exception escaping a C boundary |
| **P1** | Real bug or contract break with narrower reach; R/Python behave differently | Unchecked null handle; error swallowed; argument default differs between R and Python |
| **P2** | Robustness, maintainability, or design problem that will cause bugs later | Duplicated logic that has started to drift; missing input validation on an edge case; untested branch |
| **P3** | Clarity, docs, naming, dead code | Stale comment; misleading name; unused function |

## Finding format

Use this exact block for each finding. IDs are `S<stage><slice>-<nnn>`, e.g. `S02b-004`.

```
### S02b-004 [P1, plausible] Short title
- **Where:** include/socialchoicelab/geometry/yolk.h:212
- **What:** One or two sentences.
- **Failure scenario:** Inputs/state → observed wrong behavior.
- **Suggested fix:** One or two sentences. (Do not implement.)
- **Also affects:** other layers/files, if any (e.g. C API wrapper, R, Python).
```

## Output file structure

```
# Stage NN<slice> — <area>
Model / effort: ...   Commit: <git short sha>   Files reviewed: <count, lines>
Not read (if any): ...

## Summary
3–6 bullets: counts by severity and the most important point.

## Contracts this layer provides
Short list of guarantees later stages may rely on (ownership, units, error behaviour,
invariants). Later agents read this instead of re-reading your code.

## Findings
(P0 first, then P1, P2, P3)

## Open questions
## Out-of-slice notes
```

## Checklist

Apply what is relevant to your slice.

**Correctness**
- Algorithm matches its cited source (paper/book); citation present for published algorithms.
- Edge cases: empty input, one voter/candidate/dimension, ties, duplicate points,
  collinear/degenerate geometry, NaN/Inf, very large/small magnitudes.
- Floating-point: tolerances explicit and consistent; no exact `==` on computed doubles
  where it matters.
- Randomness (Q4): every draw goes through `PRNG` methods using project-owned algorithms;
  no `std::*_distribution`, `std::shuffle`, or direct `engine()` use; transcendental
  calls in RNG code go through the single maths wrapper; named streams used consistently.
- Thread safety (Q9): no new global or mutable static state; no API that shares mutable
  state between handles; no doc calling concurrent use of one handle safe.
- Integer overflow, signed/unsigned mixing, index bounds.

**C API boundary**
- No C++ exception crosses the boundary; every entry point catches and returns an error.
- Ownership of every returned pointer/buffer is documented and matched by a free function.
- Null and invalid handle checks; buffer-size contracts; thread-safety stated.
- Error messages follow the end-user rule: what failed, what was entered, how to fix.

**Bindings (R / Python)**
- Same function names, argument names, defaults, return shapes, and errors in both.
- Input validated before reaching the C API, with end-user error messages.
- Resources freed (finalizers / context managers); no leaks on error paths.

**Tests**
- Each public function has tests; assertions check values, not just "no error".
- Edge cases above are covered; tests are deterministic.

**Design & hygiene**
- Duplicated logic, dead code, stale comments, misleading names.
- Backward-compatibility shims are removal candidates (project is pre-open), except
  faithful replicas of published algorithms and verbatim external code.
- Docs and status files match the code.
