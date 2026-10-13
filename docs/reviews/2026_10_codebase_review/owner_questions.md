# Owner questions

Design questions raised by review agents. The owner's answers define intended behaviour:
review agents treat an answered question as the contract and report code that disagrees
with it. "Don't know — flag both ways" is a valid answer; the reviewer then reports the
issue without deciding which side is wrong.

Write the answer under each question. Questions are grouped by the first stage that needs
the answer. Source: [stage_01_architecture_map.md](stage_01_architecture_map.md),
"Open questions".

---

## Needed before Stage 2 (C++ core)

### Q1. Heart — which concept?
The code cites Laffond, Laslier & Le Breton (1993), which defines the *bipartisan set*. It
also describes the result as the "smallest" set, but it iterates downward from the
uncovered set, which yields the *largest* fixed point. Is Schofield's Heart intended, or a
refinement of the uncovered set? Which citation is correct?
Where: `include/socialchoicelab/geometry/heart.h`.

**Answer:**

### Q2. Minkowski at order p ≥ 100
For p ≥ 100, Minkowski distance is replaced by Chebyshev (L∞) to avoid overflow
(`distance_functions.h:28–31, 166–168`). The relative error can reach n^(1/p) − 1
(about 0.7% for n = 2 dimensions at p = 100). Is this an accepted contract? If so, what error
bound is acceptable, and should it be documented to users? (This decides whether failing
test S00-001 is a test bug or a library bug.)

**Answer:**

### Q3. Default tie-break
The C++ default is `kRandom` with a null PRNG, which throws on any tie. Both the R and
Python bindings default to `"smallest_index"`. `aggregation_design.md` says random is the
user default. Which is intended for end users?

**Answer:**

### Q4. Reproducibility scope
Seeds are portable, but `std::*_distribution` and `std::shuffle` produce different
results on different standard libraries (libc++ on macOS vs. libstdc++ on Linux). Must the
same seed give identical results across platforms, or only on the same toolchain?

**Answer:**

### Q5. Winset "empty"
The header says the winset is empty if and only if a Condorcet winner exists, but the
winset is built from sampled polygons (`num_samples`). Should "empty" mean exactly empty,
or empty at the sampling resolution? Are the sampled polygons meant to be inscribed or
circumscribed?

**Answer:**

### Q6. Weighted winset approximation
Weighted winsets expand each voter into copies, rounding weights to multiples of
min_weight/100. The expansion grows with the weight ratio × `weight_scale`. Is this an
accepted approximation, and what expansion size is acceptable?

**Answer:**

### Q7. Voronoi with coincident candidates
When two candidates are at the same point (common when competitors converge), the
bisector is degenerate and their cells overlap instead of splitting. What output is
expected — shared cell, error, or something else?

**Answer:**

### Q8. `hunter_sticker` strategy
This competition strategy exists in code but not in the design docs. What is its published
source (if any)? Should it be added to `competition_design.md`?

**Answer:**

### Q9. Thread safety
Docs claim the geometry functions are thread-safe. Is thread safety a requirement (e.g.
for parallel experiment runs or the website), and has it been checked under CGAL EPEC?

**Answer:**

---

## Needed before Stage 3 (C API)

### Q10. Competition bounds
The C API silently sets the competition space to the bounding box of competitors and
voters ± 1.0 in absolute units (`scs_api.cpp:4083–4103`). R and Python cannot set bounds.
Is this intended? Should the bindings expose bounds?

**Answer:**

### Q11. Version numbers
CMake says 0.1.0, `SCS_API_VERSION` says 0.2.0, and the packages are 0.3.0. Which is
authoritative? Should the C API version track the package version?

**Answer:**

### Q12. Deprecation language
`c_api_design.md` describes a deprecation period for API changes. Does that apply before
the project opens, given the no-backward-compatibility rule?

**Answer:**

---

## Needed before Stages 5–7 (bindings, parity, plotting)

### Q13. Visualization "renderers only" exceptions
The rule is that plotting layers only render what the core computes. Exceptions found:
R/Python draw Euclidean indifference circles themselves when no distance config is given;
the JS competition stats panel computes Euclidean distances regardless of metric; the JS
utility surface computes contours with marching squares. Are any of these acceptable?

**Answer:**

### Q14. Yolk and Heart in the bindings
`scs_yolk_2d` and the three Heart functions are in the C API but not bound in Python (and
not exposed in R). Should they stay unbound until replaced? Should `layer_yolk` (which
takes a user-supplied centre and radius) stay?

**Answer:**

---

## Needed before Stage 8 (app and website)

### Q15. Website integration
The website imports the library by inserting the sibling repo into `sys.path`, and
partly duplicates the plot payload builder. Is that the intended long-term design? Is the
website in scope for parity updates when the library API changes?

**Answer:**

---

## Needed before Stage 9 (docs and hygiene)

### Q16. Stale status docs
`WHERE_WE_ARE.md` and `.cursor/HANDOFF.md` say the October work is uncommitted, but it was
committed (fbc52da…aa19b05). Treat these as stale and report them?

**Answer:**

### Q17. Doc file names
Links point to `design_document.md` and `development.md`, but the files are
`Design_Document.md` and `DEVELOPMENT.md`. Which names are correct under the naming
convention?

**Answer:**
