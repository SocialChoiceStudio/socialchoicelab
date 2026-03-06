# Profiles and Aggregation Plan

Short-to-medium-term plan to deliver **Layers 4–5 (Profiles, Voting Rules, and
Aggregation Properties)** of the SocialChoiceLab C++ core.

Covers preference profile construction (spatial and ordinal), positional voting
rules (plurality, Borda, approval, anti-plurality, generic scoring rules),
social welfare functions (rankings from rule outputs), and aggregation property
checks (Pareto efficiency, Condorcet consistency).

**Authority:** This plan is the source for "what's next" for aggregation work.
When a step is done, mark it ✅ Done and update [where_we_are.md](where_we_are.md).

**References:**
- [Design Document](../architecture/design_document.md) — Layers 4–5
- [Implementation Priority Guide](../references/implementation_priority.md) — Phase 2
- [Roadmap](ROADMAP.md) — mid-term priorities

---

## Scope

**In scope:**

- Preference profile data structure (`Profile`) — stores n voter rankings over m
  alternatives as a collection of total orderings (indices into the alternatives
  vector, preferred first).
- Profile construction from a spatial model (voter ideal points + distance config
  → ranked preferences).
- Profile construction from a raw utility matrix.
- Synthetic profile generators: uniform and Gaussian spatial profiles, impartial
  culture (purely ordinal).
- Positional voting rules: plurality, Borda count, anti-plurality, generic
  positional scoring rule.
- Approval voting with a distance threshold (spatial) and top-k threshold
  (ordinal).
- Social rankings: sort alternatives by voting-rule scores to produce a full
  social ordering.
- Aggregation properties: Pareto efficiency, Condorcet consistency, majority
  consistency — checks for a given winner and profile.
- Integration tests chaining the aggregation layer with the geometry layer
  (spatial profiles → voting rule → Condorcet check → comparison with geometry
  layer winner).
- Design document `docs/architecture/aggregation_design.md` with full API
  surface and citations.

**Out of scope for this plan:**

- Strategic (insincere) voting models.
- Multi-winner voting (approval-based committees, proportional representation).
- Sequential voting procedures (agendas, runoff), tournament solutions beyond
  Copeland (Banks set, Bipartisan set, Schattschneider set — see Roadmap §
  Possible future extensions).
- Social choice impossibility theorems (Arrow, Gibbard-Satterthwaite) as
  theorems — they inform design but are not computational outputs.
- N-D / 3D spatial profiles.
- c_api exposure of aggregation services (follow-on, requires c_api geometry
  extensions first).
- R / Python bindings.

---

## Background: key concepts

**Preference profile:** A profile `P = (≻₁, ≻₂, …, ≻ₙ)` is a tuple of n
complete, transitive, antisymmetric (strict) preference orderings over a finite
set of m alternatives. In a spatial model, voter i's ordering is induced by their
Lp-distance to each alternative: a ≻ᵢ b iff dist(vᵢ, a) < dist(vᵢ, b). Ties
(equal distances) are treated as strict indifference and handled consistently with
the geometry layer.

**Positional scoring rule:** A social choice function that assigns each
alternative a score based on its rank positions across all voters. The score
vector **s** = (s₀ ≥ s₁ ≥ … ≥ s_{m-1}) defines the rule:
- **Plurality:** s = (1, 0, …, 0) — only first-place votes count.
- **Borda:** s = (m−1, m−2, …, 0) — linear position scores.
- **Anti-plurality:** s = (1, 1, …, 1, 0) — everyone gets 1 except last place.

**Approval voting:** Voters approve all alternatives within a distance threshold
(spatial approval) or their top-k alternatives (ordinal approval). Winner =
most approvals.

**Pareto efficiency (ordinal):** Alternative a Pareto-dominates b if every voter
weakly prefers a to b and at least one voter strictly prefers a to b. A social
choice is Pareto efficient if no alternative Pareto-dominates the winner.

**Condorcet consistency:** A voting rule is Condorcet consistent if it always
selects the Condorcet winner (the alternative that beats all others in pairwise
majority) whenever one exists.

**Condorcet paradox:** The majority relation need not be transitive. A profile
can have A ≻_majority B, B ≻_majority C, C ≻_majority A — the Condorcet cycle.
Positional rules resolve this by aggregating ranks rather than pairwise
comparisons.

---

## Implementation notes

### New header directory

All new headers go in `include/socialchoicelab/aggregation/`. Each header is
inline/header-only (no separate `.cpp`), consistent with the existing geometry
layer.

### Relationship to geometry layer

The aggregation layer depends on Layer 3 only for:
- `DistConfig` from `geometry/majority.h` (for spatial profile construction).
- `majority_prefers` / `pairwise_matrix` (for Condorcet consistency checks).
- `copeland_scores` / `condorcet_winner` (for Condorcet comparison).

These are one-directional dependencies: Layer 3 does not depend on Layer 4.

### Profile indexing convention

**The C++ core uses 0-based indexing throughout.** All alternative indices and
voter indices run from 0 to (count − 1), consistent with C++ and Python
conventions. Rankings are ordered **most preferred first**:

```
rankings[i][0]   = index of voter i's first choice  (most preferred)
rankings[i][1]   = index of voter i's second choice
rankings[i][m-1] = index of voter i's last choice   (least preferred)

scores[j]        = total score for alternative j
```

For example, with alternatives {A=0, B=1, C=2} and a voter who prefers B > C > A:
```
rankings[voter] = [1, 2, 0]
                   ^  ^  ^
                   B  C  A  (B is best, A is worst)
```

The rank of alternative j for voter i is the position at which j appears:
`rank = std::find(rankings[i].begin(), rankings[i].end(), j) - rankings[i].begin()`.
Borda assigns score `(m − 1 − rank)` to alternative j for voter i.

#### Binding implications

**R (1-indexed):** R vectors are 1-indexed. When the c_api and R binding are
built, every alternative index returned to R must be incremented by 1.
`rankings[i][0] = 1` in R means the first alternative (`alternatives[[1]]`).
Failure to shift will silently produce off-by-one errors. The R binding layer
is responsible for this translation; the C++ core always uses 0-based indices.

**Python (0-indexed):** Python lists and NumPy arrays are 0-indexed, matching
the C++ convention directly. No index shift is needed. A `Profile` exposed to
Python can use the raw C++ indices without translation.

**Binding rule:** Convert indices at the binding boundary only, never inside
the C++ core. Document the shift explicitly in every R helper that returns an
alternative index (winner, ranking, Pareto set, etc.).

**R surface API design note (defer to R binding plan):** Where possible, prefer
returning the *named object* rather than its index. For example, `plurality_winner()`
in R should ideally return the winning alternative's coordinates (or a named row
from a data frame) rather than a raw position integer. This sidesteps the indexing
question entirely for most user-facing functions — the 1-indexed shift only surfaces
when the user explicitly needs to slice by position. When both an index and a value
are useful, provide both (e.g. `list(winner = point, index = 1L)`). Document index
values in R with parallel C++ (0-based) and R (1-based) examples wherever indexing
appears in user-facing documentation.

---

## Phases

### Phase P — Preference Profiles

| Step | Task | Deliverable | Status |
|------|------|-------------|--------|
| **P1** | **Core `Profile` type and spatial construction** | `include/socialchoicelab/aggregation/profile.h`: `struct Profile { int n_voters; int n_alts; std::vector<std::vector<int>> rankings; };` — rankings[i] is a permutation of 0..m-1 (most preferred first). Constructor validates completeness and no duplicates. `build_spatial_profile(alternatives, voter_ideals, cfg) → Profile`: for each voter, sort alternatives by distance (ascending), breaking strict ties by index (deterministic). `profile_from_utility_matrix(Eigen::MatrixXd utilities) → Profile`: sort each row descending by utility. `is_well_formed(profile) → bool`: validates rankings are permutations of 0..n_alts-1. Tests: empty alternatives; single alternative; Condorcet winner configuration (voter nearest first-place matches nearest alternative); collinear voters (median voter's most-preferred = alternative nearest median); utility matrix → correct ranking order; ill-formed profile detection. | ⬜ |
| **P2** | **Synthetic profile generators** | `include/socialchoicelab/aggregation/profile_generators.h`: `uniform_spatial_profile(n_voters, n_alts, alternatives, xmin, xmax, ymin, ymax, cfg, prng) → Profile` — voter ideals drawn from `U([xmin,xmax] × [ymin,ymax])`; then `build_spatial_profile`. `gaussian_spatial_profile(n_voters, n_alts, alternatives, center, std_dev, cfg, prng) → Profile` — voter ideals drawn from bivariate Gaussian with equal σ per dimension. `impartial_culture(n_voters, n_alts, prng) → Profile` — each voter's ranking drawn uniformly from all m! orderings using a Fisher-Yates shuffle (no spatial model). Tests: generated profile is always well-formed; uniform spatial profile has correct alternative count; impartial culture produces every ordering with equal probability over many draws (chi-square test, n=10000, m=3, tolerance 5%); reproducibility under same seed. | ⬜ |

---

### Phase V — Voting Rules

| Step | Task | Deliverable | Status |
|------|------|-------------|--------|
| **V1** | **Plurality** | `include/socialchoicelab/aggregation/plurality.h`: `plurality_scores(profile) → std::vector<int>` — scores[j] = number of voters for whom alternative j is ranked first. `plurality_winner(profile) → int` — index of alternative with the most first-place votes; ties broken by smallest index. Tests: Condorcet winner need not be plurality winner (explicit counterexample); tie-breaking by index; single voter; single alternative; Condorcet cycle — plurality picks the alternative with the most first-place votes. | ⬜ |
| **V2** | **Borda count** | `include/socialchoicelab/aggregation/borda.h`: `borda_scores(profile) → std::vector<int>` — standard Borda: voter i gives score (m−1−r) to the alternative it ranks at position r. `borda_winner(profile) → int` — index of highest Borda score; ties by smallest index. `borda_ranking(profile) → std::vector<int>` — alternatives sorted by descending Borda score (full social ordering). Tests: Condorcet winner is always the Borda winner for 3 alternatives (Fishburn 1977, Thm 1); scores sum to n×m(m−1)/2 across all alternatives; single voter; single alternative; Condorcet cycle — verify Borda produces a complete social ranking despite cycle. | ⬜ |
| **V3** | **Approval voting** | `include/socialchoicelab/aggregation/approval.h`: `approval_scores_spatial(alternatives, voter_ideals, cfg, threshold_distance) → std::vector<int>` — voter i approves alternative j iff `dist(voter_ideals[i], alternatives[j]) ≤ threshold_distance`; score = total approvals. `approval_scores_topk(profile, k) → std::vector<int>` — voter i approves their top-k alternatives. `approval_winner_spatial(alternatives, voter_ideals, cfg, threshold_distance) → int`. `approval_winner_topk(profile, k) → int`. Tests: at threshold=0, no approvals (all scores 0 → return 0 by convention or throw?  → return -1 for no winner); at threshold=∞ (very large), all approved → scores equal → tie-broken by smallest index; top-k=m → all approved (tie); top-k=1 → same as plurality; distance threshold selects alternatives within Euclidean radius; invalid k (k<1 or k>m) throws. | ⬜ |
| **V4** | **Anti-plurality** | `include/socialchoicelab/aggregation/antiplurality.h`: `antiplurality_scores(profile) → std::vector<int>` — scores[j] = number of voters for whom alternative j is NOT ranked last (equivalently, all voters cast one "veto" against their last-ranked alternative). `antiplurality_winner(profile) → int` — index of alternative vetoed least; ties by smallest index. Tests: single alternative wins trivially; winner of plurality can lose anti-plurality (explicit counterexample); scores sum to n×(m−1); tie-breaking. | ⬜ |
| **V5** | **Generic positional scoring rule** | `include/socialchoicelab/aggregation/scoring_rule.h`: `scoring_rule_scores(profile, score_vector) → std::vector<double>` — voter i contributes `score_vector[r]` to the score of the alternative it ranks at position r. `score_vector` must be non-increasing and have length m (= n_alts). `scoring_rule_winner(profile, score_vector) → int`. Verify: plurality = `(1,0,…,0)`, Borda = `(m-1,…,0)`, anti-plurality = `(1,…,1,0)` all match their dedicated functions. Tests: explicit recovery of plurality/Borda/anti-plurality; invalid score_vector (wrong length, not non-increasing) throws; single voter; single alternative. | ⬜ |

---

### Phase W — Social Welfare and Aggregation Properties

| Step | Task | Deliverable | Status |
|------|------|-------------|--------|
| **W1** | **Social rankings (full orderings from rule outputs)** | `include/socialchoicelab/aggregation/social_ranking.h`: `rank_by_scores(scores) → std::vector<int>` — returns alternative indices sorted by descending score, ties broken by smallest index (can be applied to any score vector from V1–V5 or from geometry/copeland). `pairwise_ranking(pairwise_matrix) → std::vector<int>` — sort by Copeland score derived from the geometry-layer pairwise matrix (calls `copeland_scores`; no new computation). Tests: consistent with individual voting rule winners; ties resolved by index; pairwise_ranking on Condorcet winner configuration places winner first. | ⬜ |
| **W2** | **Pareto efficiency** | `include/socialchoicelab/aggregation/pareto.h`: `pareto_dominates(profile, a, b) → bool` — alternative a (index) Pareto-dominates alternative b (index): every voter weakly prefers a to b, and at least one strictly prefers a to b. `pareto_set(profile) → std::vector<int>` — indices of alternatives not Pareto-dominated by any other. `is_pareto_efficient(profile, winner) → bool` — checks the given winner is in the Pareto set. Tests: Condorcet winner is always Pareto efficient; all alternatives Pareto-efficient in a Condorcet cycle; a clearly dominated alternative (e.g. uniformly last-ranked) is Pareto-dominated; Pareto set is non-empty. | ⬜ |
| **W3** | **Condorcet and majority consistency** | `include/socialchoicelab/aggregation/condorcet_consistency.h`: `has_condorcet_winner_profile(profile) → bool` — checks whether the majority relation derived from the profile has a Condorcet winner (using pairwise majority counts, no geometry). `condorcet_winner_profile(profile) → std::optional<int>` — returns the Condorcet winner index, or nullopt. `is_condorcet_consistent(profile, winner) → bool` — returns true if no Condorcet winner exists in the profile, or the given winner IS the Condorcet winner. `is_majority_consistent(profile, winner) → bool` — returns true if the winner beats every other alternative by strict majority. Note: these functions operate purely on ordinal profiles (rank comparisons), not on spatial geometry. Tests: Condorcet winner case; Condorcet cycle case; is_condorcet_consistent true when no winner exists; majority_consistent implies condorcet_consistent; a plurality winner need not be majority consistent (counterexample). | ⬜ |

---

### Phase I — Integration tests and documentation

| Step | Task | Deliverable | Status |
|------|------|-------------|--------|
| **I1** | **Integration tests** | `tests/unit/test_aggregation_integration.cpp`: end-to-end tests chaining spatial voter configuration → `build_spatial_profile` → voting rules → property checks → comparison with geometry layer (Phase B/F). Specific chains: (a) Condorcet winner configuration → all positional rules agree on winner → `is_condorcet_consistent = true` for all; (b) Condorcet cycle → majority relation has no winner → `is_condorcet_consistent = true` for any winner (vacuously); (c) spatial profile → Borda winner matches Copeland winner for 3-alternative case (Fishburn 1977 result); (d) impartial culture → all alternatives have equal expected Borda score (statistical test); (e) Pareto set from profile ⊆ convex hull of alternatives (spatial Pareto). | ⬜ |
| **I2** | **Documentation and citation verification** | `docs/architecture/aggregation_design.md`: full API surface, implementation notes, known limitations, citation table. Verify all citations below. Update `design_document.md` Layers 4–5 to "implemented". Update `where_we_are.md`. | ⬜ |

---

## Definition of done

- Steps P1–P2, V1–V5, W1–W3, I1–I2 all ✅ Done.
- All new functions are header-only in `include/socialchoicelab/aggregation/`.
- Tests added under `tests/unit/test_*.cpp`; CI green on all platforms.
- `aggregation_design.md` complete with API, limitations, and citations.
- `design_document.md` Layers 4–5 updated to "implemented".

---

## Open questions and future extensions

1. **Tie-breaking policy:** All rules break ties by smallest alternative index.
   An alternative tie-breaking (e.g. by Borda score, or random) could be added
   as a parameter. Defer unless a concrete use case arises.

2. **Weak preferences / ties in spatial model:** Two alternatives at exactly
   equal distance from a voter create a tie. Current policy (geometry layer):
   ties contribute to neither side of a strict preference. The profile layer
   should propagate this consistently. Consider adding a tie-breaking parameter
   (e.g. random, by index) to `build_spatial_profile`.

3. **Kemeny rule (rank aggregation):** Produce the consensus ranking that
   minimises the total number of pairwise disagreements (Kendall tau distance)
   with all voter rankings. NP-hard for m ≥ 4; tractable for small m. Possible
   future Phase V6.

4. **Copeland social ranking as a full ordering:** `pairwise_ranking` (W1) uses
   Copeland scores; ties are resolved by index. A more principled tie-breaking
   (e.g. by Borda score within tied Copeland groups) is possible but deferred.

5. **N-dimensional profiles:** `build_spatial_profile` works for any `Point2d`
   alternatives; extending to `PointNd` would generalise to N-D. Depends on
   N-D distance support in the preference layer (already present via Minkowski
   with arbitrary dimension, but `alternatives` and `voter_ideals` are 2D types).
   Defer to a future plan.

6. **Weighted profiles:** Voters with weights > 1. Could be handled by voter
   expansion (same approach as `weighted_winset_2d`) or by directly multiplying
   scores. Defer.

---

## Citation plan

Every concept introduced in this layer must have a verified primary citation
before Phase I2 is marked ✅ Done.

| Concept | Planned primary reference | Confidence | Notes |
|---|---|---|---|
| Preference profile (ordinal) | Sen (1970) *Collective Choice and Social Welfare* | ✅ High | Standard reference for ordinal profiles. |
| Positional scoring rule | Young (1975) "Social choice scoring functions" | ✅ High | Axiomatic characterisation of positional rules. |
| Plurality rule | Self-evident / Dummett (1984) | ✅ High | Oldest electoral method; Dummett (1984) gives a modern treatment. |
| Borda count | Borda (1781/1784) | ✅ High | Original paper; de Borda read to the Académie des Sciences in 1781. |
| Anti-plurality | Fishburn (1977) "Condorcet Social Choice Functions" | ✅ High | Surveys and names the anti-plurality rule. |
| Approval voting | Brams & Fishburn (1978/1983) | ✅ High | 1978 paper introduces the term; 1983 book gives full treatment. |
| Borda → Condorcet winner (3 alternatives) | Fishburn (1977) Thm 1 | ✅ High | Proved here: for m=3 the Borda winner always agrees with the Condorcet winner when one exists. Verify theorem statement. |
| Pareto efficiency (ordinal) | Pareto (1906); Arrow (1951) | ✅ High | Pareto (1906) original; Arrow (1951) formalises in social choice setting. |
| Condorcet consistency | Condorcet (1785); Arrow (1951) | ✅ High | Condorcet (1785) proposes the concept; Arrow (1951) situates it in modern framework. |
| Condorcet paradox / cycling | Condorcet (1785) | ✅ High | Condorcet (1785) demonstrates the paradox. |
| Impartial culture | Guilbaud (1952); Gehrlein & Lepelley (2011) | ✅ Medium | Guilbaud (1952) first formalises the model; Gehrlein & Lepelley (2011) *Voting Paradoxes and Group Coherence* is the modern reference. Verify Guilbaud (1952) is accessible. |
| Kemeny rule (future) | Kemeny (1959) "Mathematics without numbers" | ✅ High | Original paper defines the distance-based ranking. |

**Citations to verify before I2 is complete:**
- Borda (1781): confirm year (1781 presentation vs 1784 publication).
- Fishburn (1977): verify that Theorem 1 says Borda winner = Condorcet winner for m=3.
- Guilbaud (1952): confirm the citation is correct and accessible (in French; English translation may be needed).

---

## Full reference list (planned)

- **Arrow, K. J. (1951).** *Social Choice and Individual Values.* Yale University Press. (2nd ed. 1963.)
- **Borda, J.-C. de (1784).** "Mémoire sur les élections au scrutin." *Histoire de l'Académie Royale des Sciences.* (Presented 1781.)
- **Brams, S. J. & Fishburn, P. C. (1978).** "Approval voting." *American Political Science Review* 72(3), 831–847.
- **Brams, S. J. & Fishburn, P. C. (1983).** *Approval Voting.* Birkhäuser.
- **Condorcet, M. J. A. N. de (1785).** *Essai sur l'Application de l'Analyse à la Probabilité des Décisions Rendues à la Pluralité des Voix.* Paris.
- **Dummett, M. (1984).** *Voting Procedures.* Oxford University Press.
- **Fishburn, P. C. (1977).** "Condorcet social choice functions." *SIAM Journal on Applied Mathematics* 33(3), 469–489.
- **Gehrlein, W. V. & Lepelley, D. (2011).** *Voting Paradoxes and Group Coherence.* Springer.
- **Guilbaud, G. Th. (1952).** "Les théories de l'intérêt général et le problème logique de l'agrégation." *Économie Appliquée* 5, 501–584.
- **Kemeny, J. G. (1959).** "Mathematics without numbers." *Daedalus* 88(4), 577–591.
- **Merrill, S. & Grofman, B. (1999).** *A Unified Theory of Voting.* Cambridge University Press.
- **Sen, A. K. (1970).** *Collective Choice and Social Welfare.* Holden-Day.
- **Young, H. P. (1975).** "Social choice scoring functions." *SIAM Journal on Applied Mathematics* 28(4), 824–838.
