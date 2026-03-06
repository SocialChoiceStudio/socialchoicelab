# Binding Decisions — Open Questions

This document records decisions that are **deliberately deferred** until the
binding plan milestone. Do not implement R or Python bindings until these have
been revisited and resolved with the user.

**When to read this:** At the start of the binding plan session, read this file
and raise each open question with the user before writing any code or plan.

---

## Already decided (do not re-open without user request)

| Decision | Resolution |
|----------|------------|
| R binding mechanism | cpp11 (not Rcpp) |
| Python binding mechanism | pybind11 (not ctypes, cffi, or nanobind) |
| What bindings call | `scs_api.h` C API only — never C++ or CGAL directly |
| R indexing | Translate 0-indexed C → 1-indexed R at the binding boundary only |
| Python indexing | Keep 0-indexed (matches C and Python convention) |

---

## Open — revisit with user before writing binding code

| # | Question | Notes |
|---|----------|-------|
| 1 | **Package names** | What should the R package and Python package be called? (e.g. `socialchoicelab`, `scslab`, something else?) |
| 2 | **Scope of first release** | Expose everything in the c_api at once, or a curated subset first (e.g. geometry only, or core + geometry, then voting rules in a follow-up)? |
| 3 | **Opaque handle strategy in R** | `SCS_Winset*` and `SCS_Profile*` are heap-allocated C handles. In R: R6 class with `$finalize()` calling destroy? External pointer with finalizer? Something else? User to decide. |
| 4 | **Opaque handle strategy in Python** | Same handles in Python: Python class with `__del__`? Context manager (`with` statement)? User to decide. |
| 5 | **R or Python first, or both simultaneously?** | User has not decided. Ask at start of binding plan session. |
| 6 | **CI for bindings** | Add R/Python build+test jobs to the existing GitHub Actions matrix, or separate workflow? |
| 7 | **Documentation format** | R: roxygen2 + pkgdown? Python: sphinx + autodoc? Both? User to decide. |
| 8 | **Vignette / notebook** | Should the first release include a worked example (R vignette or Python Jupyter notebook)? Scope question for user. |

---

## How this fits the roadmap

```
c_api extensions  ← current milestone
       ↓
binding plan      ← read this file, resolve open questions, then write the plan
       ↓
R and/or Python binding implementation
```

Do not skip the "binding plan" step. The plan document for bindings should
record resolutions to all items in the table above before any code is written.
