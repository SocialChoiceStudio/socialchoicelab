# Aggregation Layer Design Document

Covers Layers 4–5 of SocialChoiceLab: preference profiles, positional voting
rules, approval voting, social rankings, Pareto efficiency, and Condorcet/
majority consistency.

**Implementation status:** complete.  All headers in
`include/socialchoicelab/aggregation/`.  Tests in `tests/unit/test_profile.cpp`,
`test_voting_rules.cpp`, `test_aggregation_properties.cpp`,
`test_aggregation_integration.cpp`.

---

## 1. Profile type and construction

### Header: `aggregation/profile.h`

```cpp
struct Profile {
  int n_voters = 0;
  int n_alts   = 0;
  std::vector<std::vector<int>> rankings;  // rankings[voter][rank] = alt index
};
```

**Indexing convention:** 0-based throughout the C++ core.
`rankings[i][0]` = voter i's most-preferred alternative.
`rankings[i][m-1]` = voter i's least-preferred alternative.
For R bindings, increment all alternative indices by 1 at the binding boundary.
Where possible, return the *named object* rather than its position index.

### `build_spatial_profile(alternatives, voter_ideals, cfg) → Profile`

For each voter, alternatives are sorted by ascending Lp-distance from
`voter_ideals[i]` to `alternatives[j]`, using `DistConfig` (default: Euclidean).
Ties at exactly equal distance are broken by ascending alternative index
(deterministic, no PRNG needed).

**Dependency:** uses `geometry::detail::voter_distance` (internal helper in
`geometry/majority.h`).

### `profile_from_utility_matrix(Eigen::MatrixXd utilities) → Profile`

`utilities(i, j)` = voter i's utility for alternative j (higher = better).
Each row is sorted in descending utility order. Ties in utility are broken
by ascending alternative index.

### `is_well_formed(profile) → bool`

Validates that:
- `rankings.size() == n_voters`
- Each `rankings[i].size() == n_alts`
- Each `rankings[i]` is a permutation of 0 … n_alts−1.

---

## 2. Profile generators

### Header: `aggregation/profile_generators.h`

All generators are seeded via `core::rng::PRNG&`.

| Function | Description |
|---|---|
| `uniform_spatial_profile(n, alts, xmin, xmax, ymin, ymax, cfg, prng)` | Voter ideals drawn from U([xmin,xmax]×[ymin,ymax]); then `build_spatial_profile`. |
| `gaussian_spatial_profile(n, alts, center, std_dev, cfg, prng)` | Voter ideals drawn from N(center, std_dev² I₂); then `build_spatial_profile`. |
| `impartial_culture(n_voters, n_alts, prng)` | Each voter's ranking drawn uniformly from all m! orderings via Fisher-Yates shuffle. No spatial model. |

---

## 3. Tie-breaking design

### Header: `aggregation/tie_break.h`

```cpp
enum class TieBreak {
  kRandom,         // Uniform random among tied winners. Requires PRNG.
  kSmallestIndex,  // Lowest index (deterministic; use in tests).
};

int break_tie(const std::vector<int>& tied, TieBreak tie_break, PRNG* prng);
```

**Default is `kRandom`** for all `*_one_winner()` functions — this is
appropriate for simulations and user-facing code.  Tests pass `kSmallestIndex`
explicitly when reproducibility is needed.  The burden is on the test author,
not the user.

### Category system

| Category | Rule behaviour | Function pattern |
|---|---|---|
| **1** | Rule natively returns a set (Approval, Pareto). | `*_all_winners()` only; no `*_one_winner()`. |
| **2** | Rule returns optional (Condorcet winner). | Returns `std::optional<int>`; no tie-breaking. |
| **3** | Rule doesn't specify ties (Plurality, Borda, Anti-plurality, generic). | `*_scores()`, `*_all_winners()`, `*_one_winner(profile, tie_break, prng)`. |

---

## 4. Positional voting rules

### Plurality — `aggregation/plurality.h`

**Rule:** score[j] = number of voters who rank j first.

```cpp
std::vector<int> plurality_scores(const Profile&);
std::vector<int> plurality_all_winners(const Profile&);          // Category 3
int              plurality_one_winner(const Profile&,
                   TieBreak = kRandom, PRNG* = nullptr);
```

**Reference:** Self-evident; Dummett (1984) gives modern treatment.

### Borda count — `aggregation/borda.h`

**Rule:** voter i contributes (m−1−rank) to the alternative at position rank.
Total Borda score = sum over all voters.

```cpp
std::vector<int> borda_scores(const Profile&);
std::vector<int> borda_all_winners(const Profile&);
int              borda_one_winner(const Profile&,
                   TieBreak = kRandom, PRNG* = nullptr);
std::vector<int> borda_ranking(const Profile&,
                   TieBreak = kRandom, PRNG* = nullptr);
```

Score identity: sum over all alternatives = n × m(m−1)/2.

**Reference:** Borda (1784; presented 1781).

### Anti-plurality — `aggregation/antiplurality.h`

**Rule:** score[j] = number of voters who do NOT rank j last.

```cpp
std::vector<int> antiplurality_scores(const Profile&);
std::vector<int> antiplurality_all_winners(const Profile&);
int              antiplurality_one_winner(const Profile&,
                   TieBreak = kRandom, PRNG* = nullptr);
```

Score identity: sum over all alternatives = n × (m−1).

**Reference:** Fishburn (1977).

### Generic positional scoring rule — `aggregation/scoring_rule.h`

**Rule:** voter i contributes `score_vector[rank]` to the alternative at
position rank.  `score_vector` must be non-increasing with length m.

```cpp
std::vector<double> scoring_rule_scores(const Profile&,
                      const std::vector<double>& score_vector);
std::vector<int>    scoring_rule_all_winners(const Profile&,
                      const std::vector<double>&);
int                 scoring_rule_one_winner(const Profile&,
                      const std::vector<double>&,
                      TieBreak = kRandom, PRNG* = nullptr);
```

Special cases: `(1,0,…,0)` = plurality; `(m-1,…,0)` = Borda;
`(1,…,1,0)` = anti-plurality.

**Reference:** Young (1975).

---

## 5. Approval voting

### Header: `aggregation/approval.h`

Category 1: no `_one_winner()` variant.

**Spatial variant** — voter i approves j iff dist(voter_ideals[i], alternatives[j]) ≤ threshold:

```cpp
std::vector<int> approval_scores_spatial(alts, voters, cfg, threshold);
std::vector<int> approval_all_winners_spatial(alts, voters, cfg, threshold);
```

Returns empty if no voter approves any alternative.

**Ordinal top-k variant** — voter i approves their top-k alternatives:

```cpp
std::vector<int> approval_scores_topk(const Profile&, int k);
std::vector<int> approval_all_winners_topk(const Profile&, int k);
```

Special cases: k=1 → same as plurality_all_winners; k=m → all approved.

**Reference:** Brams & Fishburn (1978, 1983).

---

## 6. Social rankings

### Header: `aggregation/social_ranking.h`

```cpp
std::vector<int> rank_by_scores(const std::vector<double>& scores,
                                 TieBreak = kRandom, PRNG* = nullptr);

std::vector<int> pairwise_ranking(const Eigen::MatrixXi& pairwise,
                                   TieBreak = kRandom, PRNG* = nullptr);
```

`rank_by_scores` returns alternative indices sorted by descending score.
Ties within a score group are ordered by `tie_break`.

`pairwise_ranking` computes Copeland scores from the geometry-layer pairwise
matrix (`pairwise(i,j) > 0` → i beats j) and delegates to `rank_by_scores`.
The pairwise matrix is obtained from `geometry::pairwise_matrix()`.

---

## 7. Pareto efficiency

### Header: `aggregation/pareto.h`

```cpp
bool             pareto_dominates(const Profile&, int a, int b);
std::vector<int> pareto_set(const Profile&);
bool             is_pareto_efficient(const Profile&, int winner);
```

`a` Pareto-dominates `b` iff every voter weakly prefers a and at least one
strictly prefers a.  The Pareto set is always non-empty.

**Properties:**
- Condorcet winner (when one exists) is always Pareto-efficient.
- In a Condorcet cycle all alternatives are Pareto-efficient.

**References:** Pareto (1906); Arrow (1951).

---

## 8. Condorcet and majority consistency

### Header: `aggregation/condorcet_consistency.h`

All functions operate on ordinal profiles (rank comparisons only); no spatial
geometry.

```cpp
bool              has_condorcet_winner_profile(const Profile&);
std::optional<int> condorcet_winner_profile(const Profile&);  // Category 2
bool              is_condorcet_consistent(const Profile&, int winner);
bool              is_majority_consistent(const Profile&, int winner);
```

**Strict majority threshold:** alternative a beats b iff more than n/2 voters
rank a ahead of b (integer division, equivalent to count > floor(n/2)).

**`is_condorcet_consistent(p, w)`:** true when (a) no Condorcet winner exists
(vacuously true), or (b) a Condorcet winner exists and w is that winner.

**`is_majority_consistent(p, w)`:** true iff w beats every other alternative
by strict majority.  Stronger than Condorcet consistency; implies it.

**References:** Condorcet (1785); Arrow (1951).

---

## 9. Known limitations

1. **Weak preferences:** ties at exactly equal spatial distance are broken by
   alternative index.  If the caller requires a different tie-breaking for
   profile construction, a `TieBreak` parameter could be added to
   `build_spatial_profile` in a future update.

2. **Floating-point equality in `scoring_rule_all_winners`:** when comparing
   scores from `scoring_rule_scores`, equality is checked with `==`.  For
   exact integer-equivalent score vectors (e.g. Borda with integer weights)
   this is exact.  For non-integer weights, floating-point rounding may cause
   spurious ties or missed ties in extreme cases.

3. **N-dimensional profiles:** `build_spatial_profile` works for any `Point2d`
   alternatives; extension to `PointNd` requires N-D distance support.

4. **Weighted voters:** not supported.  Follow-on work (voter expansion
   analogous to `weighted_winset_2d`).

---

## 10. Citation table

| Concept | Primary reference | Notes |
|---|---|---|
| Preference profile (ordinal) | Sen (1970) *Collective Choice and Social Welfare* | Standard reference for ordinal profiles. |
| Positional scoring rule | Young (1975) "Social choice scoring functions" | Axiomatic characterisation. |
| Plurality rule | Dummett (1984) *Voting Procedures* | Oldest electoral method. |
| Borda count | Borda (1784) | Published in *Histoire de l'Académie Royale des Sciences* 1784; presented to the Académie 1781. Standard citation year is 1784. |
| Anti-plurality | Fishburn (1977) "Condorcet Social Choice Functions" | Names and surveys the rule. |
| Approval voting | Brams & Fishburn (1978, 1983) | 1978 paper introduces the term; 1983 book gives full treatment. |
| Condorcet winner is also Borda winner (3 alternatives) | Fishburn (1977) | When a Condorcet winner exists among 3 alternatives, it is also the Borda winner. The converse does not hold in general. Verified in integration tests. |
| Pareto efficiency (ordinal) | Pareto (1906); Arrow (1951) | Pareto (1906) original; Arrow (1951) formalises in social choice setting. |
| Condorcet consistency | Condorcet (1785); Arrow (1951) | Condorcet (1785) proposes the concept. |
| Condorcet paradox / cycling | Condorcet (1785) | Demonstrates the majority cycle. |
| Impartial culture | Guilbaud ([1952] 1966); Gehrlein & Lepelley (2011) | English translation in Lazarsfeld & Henry (eds.) 1966, MIT Press. |

---

## 11. Full reference list

- **Arrow, K. J. (1951).** *Social Choice and Individual Values.* Yale University
  Press. (2nd ed. 1963.)
- **Borda, J.-C. de (1784).** "Mémoire sur les élections au scrutin."
  *Histoire de l'Académie Royale des Sciences.* (Presented 1781.)
- **Brams, S. J. & Fishburn, P. C. (1978).** "Approval voting." *American
  Political Science Review* 72(3), 831–847.
- **Brams, S. J. & Fishburn, P. C. (1983).** *Approval Voting.* Birkhäuser.
- **Condorcet, M. J. A. N. de (1785).** *Essai sur l'Application de l'Analyse
  à la Probabilité des Décisions Rendues à la Pluralité des Voix.* Paris.
- **Dummett, M. (1984).** *Voting Procedures.* Oxford University Press.
- **Fishburn, P. C. (1977).** "Condorcet social choice functions." *SIAM Journal
  on Applied Mathematics* 33(3), 469–489.
- **Gehrlein, W. V. & Lepelley, D. (2011).** *Voting Paradoxes and Group
  Coherence.* Springer.
- **Guilbaud, G. T. ([1952] 1966).** "Theories of the General Interest, and the
  Logical Problem of Aggregation." In P. F. Lazarsfeld and N. W. Henry (eds.),
  *Readings in Mathematical Social Science.* Cambridge, MA: MIT Press, pp. 262–307.
  (Originally published in French as "Les théories de l'intérêt général et le problème
  logique de l'agrégation," *Économie Appliquée* 5 (1952), 501–584.)
- **Pareto, V. (1906).** *Manuale di economia politica.* Società Editrice Libraria,
  Milan. (English translation: *Manual of Political Economy*, 1971.)
- **Sen, A. K. (1970).** *Collective Choice and Social Welfare.* Holden-Day.
- **Young, H. P. (1975).** "Social choice scoring functions." *SIAM Journal on
  Applied Mathematics* 28(4), 824–838.
