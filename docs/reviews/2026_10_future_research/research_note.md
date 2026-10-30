# Research note: AI breakthroughs and computational social choice

**Filed:** 2026-10-29. **Status:** brief for a future assessment; not on the active roadmap.
Roadmap pointer: [ROADMAP.md § Possible future extensions](../../status/ROADMAP.md#possible-future-extensions).

**Next step:** after Stage 11 of the [October 2026 codebase review](../2026_10_codebase_review/README.md),
write the assessment requested in §4–5 of the note as `assessment.md` in this folder, then move any
accepted items into `ROADMAP.md`. The review's
[Stage 1 architecture map](../2026_10_codebase_review/stage_01_architecture_map.md) already lists the
modules the assessment needs to map against.

"SocialChoiceStudio" in the note means the project as a whole: the `socialchoicelab` library and
its R/Python bindings, plus the website.

## Source check (added 2026-10-29)

Added when filing; not part of the original note. All three links in the note resolve.

- **OpenAI, "The Exponential Time Hypothesis is False"** (dated 17 October 2026, revised
  25 October 2026; announced 26 October). Claims a deterministic, polynomial-space algorithm that
  exactly counts the satisfying assignments of any fixed-width k-CNF in time
  2^O(n log log n / √log n), which would refute ETH, SETH and #ETH. The paper itself does not claim
  a Lean or other machine-checked proof, although some news reports say it does.
- **Anthropic, `formal-math/eth`** ("A 2^o(n) Time Algorithm for 3-SAT", credited to Claude
  (Anthropic); undated). Claims 3-SAT in 2^o(n) time and #3-SAT in 2^O(n log log n / √log n). It
  includes a Lean 4 formalization. The README reports two `sorry` warnings from `Challenge.lean`, and
  notes that no tool checks the Lean statements against the paper's statements. It also cautions that
  the result is asymptotic only and gives no practical speedup.
- **arXiv:2608.25444** is Koutecký, Melissinos, Vu & Sabater, "Continuous Computational Social
  Choice: A Case Study in Bribery" (August 2026, cs.CC). It studies bribery and control when the
  electorate is a continuum of infinitesimal voters, and finds that several attacks that are NP-hard
  for finite electorates become polynomial-time. It is not about spatial voting or preference
  restrictions, so it supports the note's point in §2B about continuous electorates rather than the
  spatial-structure points.
- Neither ETH result has been independently verified. An independent review of the OpenAI result is
  reportedly under way.

**What this means here:** the speedup is asymptotic only. SocialChoiceLab's main algorithms (2D
winsets, yolk, uncovered set) are polynomial-time geometry and do not reduce to SAT, so the ETH
claims are background context, not a reason to change current work.

---

## Original note

Wording unchanged; Markdown headings and list formatting added.

### Research Context and Future Development Opportunities: AI Breakthroughs and Computational Social Choice

#### Purpose

This is a research and development note for the SocialChoiceStudio project. Its purpose is to preserve potentially important developments in computational complexity, AI-assisted mathematical research, and computational social choice so they can inform future extensions, algorithmic improvements, and architectural decisions.

This is not a request to implement new functionality immediately or change the current development priorities. Instead, review the implications below, identify connections to the existing architecture and roadmap, and document promising opportunities for future investigation.

#### 1. Background: Recent Computational Complexity Breakthrough

In October 2026, OpenAI and Anthropic independently announced algorithms that reportedly refute the Exponential Time Hypothesis (ETH), a longstanding conjecture asserting that 3-SAT requires exponential time in the worst case.

The reported algorithms achieve deterministic subexponential running times for fixed-width satisfiability and related counting problems.

Primary sources:

- OpenAI: https://github.com/openai/math/blob/main/preprints/The-Exponential-Time-Hypothesis-is-False-October-25-2026/build/main_full_version.tex
- Anthropic: https://github.com/anthropics/formal-math/blob/main/eth/README.md

These results are recent and should be treated as provisional pending independent verification.

Importantly, disproving ETH does not establish P = NP or eliminate NP-hardness. However, it potentially invalidates ETH-dependent fine-grained lower bounds and creates opportunities for improved exact algorithms.

For computational social choice, this may affect the computational analysis of preference aggregation, voting-rule evaluation, manipulation, control, bribery, possible-winner problems, and counting problems.

#### 2. Relevance to SocialChoiceStudio

SocialChoiceStudio is intended to support multidimensional spatial voting models, preference generation, voting rules, solution concepts, geometric analysis, and candidate dynamics.

The following areas deserve consideration for future extensions.

##### A. Computational complexity and algorithm selection

Investigate whether new theoretical developments suggest better algorithms for existing or planned functionality, particularly:

- Computing winsets and majority dominance relations.
- Identifying uncovered sets, Copeland winners, strong points, hearts, and yolks.
- Evaluating Condorcet-based rules and other preference aggregation procedures.
- Finding candidate equilibria and analyzing strategic positioning.
- Computing possible outcomes under uncertainty or incomplete preference information.
- Scaling computations to larger electorates, more candidates, and higher-dimensional spatial models.

Potential improvements include SAT/SMT formulations, integer programming, constraint programming, specialized computational geometry algorithms, and hybrid approaches.

Evaluate asymptotic complexity, parameter dependence, numerical stability, and empirical performance. Do not assume that a theoretically subexponential SAT algorithm will outperform specialized algorithms for spatial voting.

##### B. Exploiting spatial preference structure

Spatial voting models impose restrictions on preference profiles that general-purpose social choice algorithms may not exploit.

Investigate whether these restrictions permit more efficient algorithms for:

- Euclidean and other Minkowski distance-based preferences.
- Majority dominance and pairwise comparisons.
- Geometric representations of winsets and uncovered sets.
- Computing and characterizing spatial solution concepts.
- Detecting equilibrium existence or nonexistence.
- Identifying equivalences between voting rules under restricted preference domains.

Consider whether alternative representations of electorates, including continuous distributions or weighted voter types, can substantially reduce computational requirements.

Relevant research:
https://arxiv.org/abs/2608.25444

##### C. Counting, enumeration, and uncertainty

Because the reported breakthrough concerns exact counting as well as satisfiability, consider future functionality for:

- Counting admissible preference profiles under specified restrictions.
- Enumerating possible winners and election outcomes.
- Computing probabilities of different outcomes under stochastic electorate models.
- Measuring the prevalence of Condorcet cycles and other voting phenomena.
- Evaluating the robustness of solution concepts to perturbations in voter or candidate positions.
- Systematically searching for counterexamples to conjectured properties.

Consider when exact counting, approximate counting, Monte Carlo simulation, or geometric integration is most appropriate.

##### D. AI-assisted mathematical and algorithmic discovery

A potentially more significant long-term opportunity is using AI systems to assist with mathematical research.

Possible capabilities include:

- Generating conjectures about spatial voting properties.
- Constructing counterexamples to proposed theorems.
- Discovering more efficient algorithms for existing solution concepts.
- Identifying relationships between voting rules and solution concepts.
- Producing candidate proofs for subsequent formal verification.
- Automatically testing proposed properties over generated preference profiles.
- Searching for pathological examples and boundary cases.
- Generating benchmark instances designed to stress particular algorithms.

Consider how SocialChoiceStudio's profile generator, property checker, geometric engine, and voting-rule implementations could eventually support these workflows.

The distinction between AI-generated conjectures and mathematically verified results must remain explicit.

#### 3. Architectural Implications

Review the existing design for opportunities to preserve flexibility without introducing unnecessary complexity now.

Potential considerations include:

- **Algorithm interchangeability:** Support multiple implementations of the same operation so that future algorithms can be benchmarked and substituted without modifying higher-level interfaces.
- **Independent verification:** Allow competing algorithms or formulations to compute the same result, enabling consistency checks and regression testing.
- **Property-testing infrastructure:** Design the property checker to support systematic searches, generated counterexamples, and potentially AI-assisted conjecture testing.
- **Reproducible computational experiments:** Preserve sufficient metadata to reproduce generated voter profiles, candidate configurations, algorithm settings, and numerical results.
- **Solver integration:** Consider whether future SAT, SMT, optimization, or formal-verification backends should be accessible through modular interfaces rather than embedded in the core voting logic.
- **Scalability:** Preserve the ability to distinguish exact algorithms, approximations, heuristics, and probabilistic methods, including their accuracy guarantees and failure conditions.
- **Research-oriented APIs:** Consider how the C++ core and planned Python, R, REST, and other interfaces could support automated experiments and external AI research agents.

These are architectural considerations, not immediate implementation requirements.

#### 4. Requested Project Review

Please review this note against the actual SocialChoiceStudio codebase, architecture, existing documentation, and development roadmap.

Specifically:

1. Identify existing components relevant to these research directions, distinguishing implemented functionality from planned or speculative functionality.
2. Identify promising extensions that would directly improve SocialChoiceStudio's scientific usefulness, computational performance, or extensibility.
3. Identify architectural decisions worth preserving now because changing them later could be expensive. Avoid recommending premature abstractions without a concrete justification.
4. Separate near-term opportunities from longer-term research ideas, including those dependent on unverified theoretical developments.
5. Identify relevant open research questions in spatial voting, computational social choice, computational geometry, and AI-assisted mathematical discovery that SocialChoiceStudio could help investigate.
6. Suggest an appropriate place in the project's existing documentation or backlog to preserve these findings for future development.
7. Provide references to the relevant academic literature where possible, distinguishing established results, recent claims, and original suggestions.

#### 5. Expected Deliverable

Produce a concise but substantive Future Research and Extensions assessment that includes:

- A mapping from these ideas to existing SocialChoiceStudio modules.
- A prioritized list of potential future features and algorithmic improvements.
- Specific architectural considerations, if any, that should influence current development.
- Research questions that the software could eventually help answer.
- Dependencies, technical risks, and unresolved theoretical questions.
- Recommendations for where and how to record these opportunities in the project roadmap.

Use the following priority categories:

- P1: Preserve architectural compatibility now. Low-cost design decisions that prevent substantial future rework.
- P2: Revisit after core functionality stabilizes. Concrete improvements with plausible research or performance benefits.
- P3: Long-term research extensions. Speculative, experimental, or theory-dependent capabilities.

Do not modify source code, dependencies, or the implementation roadmap without explicit approval. Documentation additions may be proposed, but first explain where they belong and how they relate to existing plans.

The primary objective is to ensure that SocialChoiceStudio remains a flexible computational research platform capable of incorporating future theoretical breakthroughs, improved algorithms, and AI-assisted mathematical discovery without distracting from its current development milestones.
