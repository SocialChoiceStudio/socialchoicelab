# Stage 02c — Aggregation
Model / effort: Opus 5.5 / high   Commit: 3f19579   Files reviewed: 13, 1,559 lines
(`include/socialchoicelab/aggregation/{profile,profile_generators,voter_sampler,tie_break,plurality,borda,antiplurality,scoring_rule,approval,social_ranking,pareto,condorcet_consistency,seat_allocation}.h`)
Not read: none. All 13 files read in full.

Scratch programs: `SCR=/private/tmp/claude-501/-Users-robiragan-Dropbox-SocialChoiceStudioDev/2402f97e-221e-494b-88cb-fc63c41efc50/scratchpad/stage2c/`.
Built with Apple clang 21, `-std=c++20 -O3 -DNDEBUG`, `-I<repo>/include`, Homebrew Eigen
(3.5.0, not the 3.4.0 the project fetches; the aggregation code uses no Eigen feature that
differs between them). The Stage 0 build trees had been cleaned, so no `compile_commands.json`
was available. Flags were taken from `CMakeLists.txt` (Release: `-O3 -DNDEBUG`). Each `.out`
file next to a `.cpp` holds that program's output.

Sources checked: Zwicker (2016), "Introduction to the Theory of Voting", *Handbook of
Computational Social Choice* ch. 2 (`references/social_choice/core/Handbook of Computational Social Choice.pdf`).
Felsenthal (2012), "Review of Paradoxes Afflicting Procedures for Electing a Single
Candidate", in Felsenthal & Machover (eds.), *Electoral Systems*
(`references/social_choice/core/Felsenthal_Machover_ElectoralSystems_2012.pdf`).
**Not in `references/`:** any apportionment source (Hare–Niemeyer, Balinski & Young,
Pukelsheim), Young (1975), Fishburn (1977), Brams & Fishburn (1978/1983), Borda (1784),
Guilbaud (1952) and Fisher–Yates/Durstenfeld. So the seat-allocation check below compares
the code with its own comparator and the textbook rule as the header states it. It does not
compare against a cited text.

## Summary
- **4 P1, 5 P2, 4 P3; no P0.** Every finding was confirmed with a scratch program or by
  unambiguous logic.
- **Most important: Hare largest remainder breaks its own tie rule (S02c-001).** When two
  parties have exactly equal remainders, floating-point rounding picks the winner of the
  last seat. The comparator's intended order (more votes, then lower index) is skipped.
  Measured: 10,731 of 216,498 exact-tie cases (about 5%) gave the wrong allocation; every
  mismatch was a tie. This feeds competition seat outputs.
- **Two property functions use non-literature definitions.**
  - `is_majority_consistent` tests "is the Condorcet winner". The literature's majority
    criterion (an absolute-majority first choice must win) is a special case of the
    Condorcet criterion, not stronger than it (S02c-003).
  - `pairwise_ranking` is documented as Copeland, but it counts wins only and uses
    margin > 0. Its rankings differ from `geometry::copeland_scores` on ordinary spatial
    inputs (S02c-002).
- **Unchecked indices give wrong answers.** `is_majority_consistent` returns `true` for any
  out-of-range index. `is_condorcet_consistent` does the same when there is no Condorcet
  winner. R checks the range; Python does not, so the bindings differ (S02c-004).
- **Q4:** every RNG bypass in the slice is already in Q4's Implies list (`tie_break.h:66`,
  `borda.h:126`, `social_ranking.h:69`, `profile_generators.h:65–151`). No additional
  instances. `voter_sampler.h` draws only through `PRNG` methods.
- **Q9:** no global or static mutable state in the slice. **Q3:** two additional
  doc instances of "kRandom is the default" (S02c-010).

## Contracts this layer provides
For Stage 3. Everything is header-only, in namespace `socialchoicelab::aggregation`, and
0-based.

**Profile**
- `Profile{n_voters, n_alts, rankings}` is a plain value type. `rankings[i]` is a
  permutation of `0..n_alts-1`, best first. The constructors (`build_spatial_profile`,
  `profile_from_utility_matrix`, the three generators) always return well-formed profiles.
  Exception: `n_alts == 0` is currently allowed (S02c-007).
- No rule calls `is_well_formed`. Every rule assumes a well-formed profile. A malformed one
  can index out of bounds (`plurality_scores`, `borda_scores`) or give wrong counts
  (`pairwise_count`).
- `build_spatial_profile(alts, voters, cfg)`:
  - Throws `std::invalid_argument` if `voters` is empty.
  - Ranks by ascending double distance (`geometry::detail::voter_distance`).
  - Exact `==` distance ties go to ascending alternative index. The result is
    deterministic, uses no RNG and costs O(n·m log m).
  - Needs finite inputs: NaN gives an invalid sort comparator. The C API validates this.
- `profile_from_utility_matrix(U)`: throws if `U` has 0 rows. Ranks by descending utility;
  exact-equality ties go to ascending index. Finite input needed (the C API validates it).

**Generators and voter sampler**
- `uniform_spatial_profile` / `gaussian_spatial_profile` draw x then y for each voter, then
  call `build_spatial_profile`. `impartial_culture` does one shuffle per voter, starting
  from the identity.
  - They throw on `n_voters < 1`, `xmin >= xmax` / `ymin >= ymax`, `std_dev <= 0` and
    `n_alts < 1`. They do not validate `alternatives` (it may be empty) or finiteness.
  - Draws currently use `std::` distributions and `std::shuffle` on `prng.engine()`. They
    do not count in `draw_count_` and are not portable across standard libraries (Q4,
    known).
- `draw_voters(n, d, cfg, prng)`:
  - Returns a flat row-major vector of length `n*d`, drawn through
    `PRNG::uniform_real` / `PRNG::normal`.
  - Throws `invalid_argument` on `n < 1`, `d < 1` or an unknown kind.
  - Parameter errors come from `PRNG` with PRNG-prefixed messages. `VoterSamplerConfig`
    is a public aggregate, so the `make_*` validators can be bypassed.

**Tie-breaking**
- `break_tie(tied, tb, prng)` throws `invalid_argument` if `tied` is empty.
- A single tied element returns at once with no draw.
- `kSmallestIndex` returns the minimum.
- `kRandom` needs a non-null `prng` (otherwise it throws) and makes exactly one draw per
  tie.

**Positional and approval rules**
- `*_scores` returns length `n_alts`:
  - `int` for plurality, Borda, antiplurality and approval;
  - `double` for `scoring_rule_scores`.
- `*_all_winners` returns ascending indices. It is empty iff `n_alts == 0`.
  `approval_all_winners_spatial` is also empty when nobody approves anything.
- `*_one_winner` = `break_tie(*_all_winners(...))`. The current C++ default `kRandom` with
  `prng = nullptr` throws on any tie (Q3 changes this to `kSmallestIndex`).
- `borda_ranking` / `rank_by_scores`:
  - Return a permutation of `0..m-1` by descending score.
  - Within a tied group, order is ascending index (`kSmallestIndex`) or one
    `std::shuffle` per group (`kRandom`, which needs a PRNG).
  - `rank_by_scores({})` returns `{}`; it does not throw, despite its doc.
  - `rank_by_scores` needs finite scores: NaN loops forever (S02c-006). The C API
    validates this.
- `pairwise_ranking(M)`:
  - Needs a square `M`; this is not checked.
  - Score = number of j with `M(i,j) > 0`. This is wins only, not wins − losses
    (S02c-002).
- `scoring_rule_scores`:
  - Throws if the length is not `n_alts` or the vector is increasing. NaN weights pass the
    check; the C API rejects them.
  - Sums in voter order, so results are not exactly anonymous (S02c-005).
- `approval_scores_spatial`:
  - A voter approves iff `d <= threshold`. Throws if `threshold < 0`.
  - A NaN threshold approves nothing; the C API rejects it.
  - Empty `voters` is allowed and gives zero scores.
- `approval_scores_topk`: throws unless `1 <= k <= n_alts`.

**Properties**
- `pareto_dominates(a,b)` / `is_pareto_efficient(w)` throw `invalid_argument` on
  out-of-range indices.
- `pareto_set` returns ascending indices. It is non-empty iff `n_alts >= 1`.
- Condorcet predicates use a strict majority over all voters: count > `n_voters/2`
  (integer division).
  - `condorcet_winner_profile` returns `nullopt` when there is none.
  - `is_condorcet_consistent` and `is_majority_consistent` do **not** validate the index
    (S02c-004).

**Seat allocation**
- Output has the same length as `vote_totals`.
- Throws `invalid_argument` on negative `seat_count` or negative votes.
- `plurality_top_k_seat_allocation` requires `seat_count <= n`; Hare LR does not.
- Ties go to the lower index: top-k boundary ties, equal remainder plus equal votes, and
  the all-zero-votes fallback.
- Hare LR remainders are computed in double (S02c-001).
- The sum of votes must fit in `int`, which competition callers guarantee (sum = number of
  voters).

**Errors and exceptions**
- Only `std::invalid_argument` is thrown on purpose. `std::bad_alloc` and `length_error`
  are possible from allocation. No other exception types.

**Threads and determinism**
- No global or static mutable state. All functions are pure apart from advancing the
  `PRNG` they are given, so one `PRNG` must not be shared across threads (Q9).
- Results depend only on the inputs and the PRNG state. Platform portability is limited
  as described under Q4.

**Integer range**
- Scores are `int32`. The largest Borda score is n(m−1). Overflow would need a profile
  of at least 8.6 GB, and it is not checked.

## Findings

### S02c-001 [P1, confirmed] Hare largest remainder: floating-point remainders decide exact ties against the documented tie order
- **Where:** include/socialchoicelab/aggregation/seat_allocation.h:97–115
- **What:** Remainders are computed as `v_i / (V/S) − floor(...)` in double. When two
  parties' exact remainders are equal (`v_i·S mod V == v_j·S mod V`), rounding makes one
  look larger. The comparator's next keys (more votes, then lower index) are then never
  reached. Every remainder-tie case is affected, not only extreme ones.
- **Failure scenario:**
  - `hare_largest_remainder_allocation({0, 7, 35}, 9)`: exact quotients are 1.5 and 7.5,
    so the remainders tie at 0.5. The documented order gives the last seat to the
    35-vote party, `(0,1,8)`. The code returns `(0,2,7)`.
  - Also mismatched: `({8980, 57298, 84238}, 160)`, where the exact remainders are both
    82156/V; the code gives `(10,61,89)` against `(9,61,90)`.
  - Exhaustive check over 3 parties, votes 0–60, seats 1–12: 10,731 of 2,723,760
    allocations differ. All 10,731 are exact-remainder ties, out of 216,498 tie cases
    (about 5% of ties).
  - Reproduce with `$SCR/demo_seats.cpp` and `$SCR/demo_seats_classify.cpp`; outputs in
    `demo_seats.out` and `demo_seats_classify.out`.
- **Suggested fix:**
  - Use exact integer arithmetic: `seats_i = (int64)v_i*S / V`, `rem_i = (int64)v_i*S % V`,
    and compare `rem` as integers.
  - Add a test with equal remainders and unequal votes.
- **Also affects:** `competition/election_feedback.h:147` (`kHareLargestRemainder`), and so
  the C API / R / Python competition seat totals and shares, and experiment mean seat
  shares.

### S02c-002 [P1, confirmed] `pairwise_ranking` is documented as Copeland but computes a different score than the library's Copeland
- **Where:** include/socialchoicelab/aggregation/social_ranking.h:12–13, 77–102; docs/architecture/aggregation_design.md:212–214
- **What:** There are two differences from `geometry::copeland_scores` (copeland.h:59,
  wins − losses under k-majority):
  - **Wins only.** The score is the number of j with `M(i,j) > 0`. Zwicker (2016, ch. 2,
    eq. 2.2 and fn. 12, book p. 28) defines Copeland as wins − losses. He notes that the
    wins-only score ("Cass.") gives different winners when pairwise ties exist.
  - **Margin > 0 instead of a k-majority of all voters.** When some voters are
    equidistant, "i beats j" differs between the two functions.
  - The header also says it "delegates to copeland_scores()" (line 13), which it does not.
    Line 81 says "beats by strict majority", but the code uses margin > 0.
- **Failure scenario:**
  - Hand case: margins a>b and b>c with a and c tied. `pairwise_ranking` returns `0 1 2`
    with a and b tied on 1 win, so the order depends on index. Copeland gives a=1, b=0,
    c=−1.
  - Spatial case: 8 voters `(0,-2) (-2,2) (-3,1) (-3,-3) (-3,3) (1,-2) (-2,2) (-2,0)` and
    alternatives `(0,2) (0,0) (2,2) (2,3) (1,-1)`, Euclidean.
    `pairwise_ranking(pairwise_matrix(...))` puts alternative 1 first. Ranking by
    `copeland_scores` puts alternative 0 first (scores 3, 3, −2, −4, 0).
  - A random search found the first two discrepant cases in 21 trials.
  - Reproduce with `$SCR/demo_ranking.cpp`; output in `demo_ranking.out`.
  - The existing test `Integration_PairwiseRanking.ConsistentWithCopelandWinner` uses only
    a profile with a Condorcet winner, where the two functions agree.
- **Suggested fix:**
  - Score wins − losses (symmetric Copeland).
  - Decide which majority relation the pairwise path uses (see Open questions) and make it
    match `geometry::copeland_scores`, or rename the function and its documentation.
  - Add a test with pairwise ties.
- **Also affects:** `scs_pairwise_ranking_from_matrix` (scs_api.cpp:3760); R and Python
  `pairwise_ranking_from_matrix` (`_social.py:79`, "Copeland score").

### S02c-003 [P1, confirmed] `is_majority_consistent` uses a non-standard definition and states the logical relation backwards
- **Where:** include/socialchoicelab/aggregation/condorcet_consistency.h:122–142; docs/architecture/aggregation_design.md:259–260
- **What:** The literature's majority criterion is Felsenthal's (2012, §3.2.1.3, p. 21)
  "absolute majority paradox": an alternative ranked first by an absolute majority must be
  elected. Felsenthal calls this a special case of the Condorcet winner criterion. So
  Condorcet consistency implies majority consistency, not the other way round.
  - The code returns true iff `winner` beats every other alternative by strict majority,
    which is the same as being the Condorcet winner.
  - The doc says this is "stronger than Condorcet consistency".
  - The C API name `scs_is_selected_by_majority_consistent_rules` claims more than the
    code checks.
- **Failure scenario:**
  - Profile: 5 voters, 2×B>A>C, 2×C>A>B, 1×A>B>C. A is the Condorcet winner, and no
    alternative is ranked first by a majority.
  - `is_majority_consistent(p, A)` returns 1, which says A is selected by
    majority-consistent rules.
  - Plurality satisfies the majority criterion but picks {B, C} here (scores 1, 2, 2).
  - Reproduce with `$SCR/demo_rules.cpp`, section D3; output in `demo_rules.out`.
- **Suggested fix:** Either:
  - Implement the majority criterion: if some alternative is first for more than n/2
    voters, return `winner == that alternative`; otherwise return true (vacuous, like
    `is_condorcet_consistent`). Or:
  - Rename the current function to "is the Condorcet winner" and drop the "majority
    consistent" naming.

  In both cases fix the "stronger than" claims and cite Felsenthal (2012).
- **Also affects:**
  - `scs_is_selected_by_majority_consistent_rules` (scs_api.cpp:3920) and its doc at
    `scs_api.h:1816–1828`;
  - R `is_majority_consistent` (social.R:183–208) and Python `is_majority_consistent`
    (_social.py:241–247);
  - tests `test_aggregation_properties.cpp:204–246` (the test at `:219`,
    `MajorityConsistency_ImpliesCondorcetConsistency`, encodes the non-standard direction).

### S02c-004 [P1, confirmed] Condorcet and majority predicates accept out-of-range indices and return `true`; Python passes them through, R rejects them
- **Where:** include/socialchoicelab/aggregation/condorcet_consistency.h:39–55, 115–142
- **What:** `pairwise_count` leaves `pos_a = -1` when `a` is not in the ranking, so
  `pos_a < pos_b` holds for every voter.
  - `is_majority_consistent(p, w)` with any out-of-range `w` therefore returns true.
  - `is_condorcet_consistent(p, w)` returns true whenever no Condorcet winner exists.
  - `is_pareto_efficient` validates and throws. These two do not.
- **Failure scenario:**
  - Outputs on a 3-alternative profile: `is_majority_consistent(cw, -1) = 1`,
    `(cw, 3) = 1`, `(cw, 99) = 1`; `is_condorcet_consistent(cycle, 99) = 1`.
    `is_pareto_efficient(cw, 99)` throws, as it should.
  - Reproduce with `$SCR/demo_rules.cpp`, section D2.
  - Python `is_majority_consistent(profile, 5)` (_social.py:241–268, no range check)
    returns `True`. R checks the range first (social.R:172–178, 198–204) and errors. Same
    input, different behaviour across the bindings.
- **Suggested fix:**
  - Validate `winner` in both C++ functions with an end-user `invalid_argument` (the
    `is_pareto_efficient` message is the model).
  - Python then matches R through the C API.
  - Add tests for out-of-range indices.
- **Also affects:** `scs_is_selected_by_condorcet_consistent_rules` and
  `scs_is_selected_by_majority_consistent_rules` (scs_api.cpp:3896, 3920); Python
  `_social.py`.

### S02c-005 [P2, confirmed] `scoring_rule_*` is not anonymous: voter order changes the winner set for ordinary weights
- **Where:** include/socialchoicelab/aggregation/scoring_rule.h:66–72, 89–93
- **What:** Scores are summed in voter order and compared with `==`. With weights that are
  not exactly representable in binary, alternatives with identical rank counts get
  different sums depending on the order of addition.
  - aggregation_design.md §9.2 records this as a known limitation (K10), but says it
    happens only "in extreme cases".
- **Failure scenario:**
  - 3-voter cycle `{0,1,2},{1,2,0},{2,0,1}` with `s = (0.3, 0.2, 0.1)`: each alternative
    is ranked once in every position, so all three are exactly tied.
  - The code gives scores 0.6000000000000001, 0.6, 0.6000000000000001, so `all_winners`
    returns {0, 2}.
  - Reordering the same three ballots gives {0, 1}.
  - Reproduce with `$SCR/demo_rules.cpp`, section D1. Dowdall weights (1, 1/2, 1/3) happen
    to tie exactly on this profile (`demo_dowdall.out`).
- **Suggested fix:**
  - First count how often each alternative appears at each rank (integers). Then compute
    each score as a fixed-order sum over ranks. This makes the result depend only on the
    counts.
  - Compare scores with a relative tolerance (`k_near_zero_rel`).
  - Correct the §9.2 wording.
- **Also affects:** `scs_scoring_rule_*`, and the R and Python scoring-rule functions.

### S02c-006 [P2, confirmed] `rank_by_scores` loops forever on a NaN score
- **Where:** include/socialchoicelab/aggregation/social_ranking.h:57–73
- **What:** The grouping loop advances while `scores[order[j]] == scores[order[i]]`. For a
  NaN at `order[i]`, `NaN == NaN` is false, so `j == i`, the group is empty and `i = j`
  never moves. The NaN also breaks the `stable_sort` comparator.
- **Failure scenario:** `rank_by_scores({1.0, NaN, 0.5}, kSmallestIndex)` never returns.
  The run was killed by alarm after 5 s, exit 142. Reproduce with
  `$SCR/demo_nan_rank.cpp`.
  - The C API rejects non-finite scores (scs_api.cpp:3734), so R and Python are safe
    today. Any future C++ caller that passes computed scores (for example a ratio rule) is
    not.
- **Suggested fix:**
  - Throw `invalid_argument` on non-finite scores in `rank_by_scores`, and document the
    precondition.
  - `borda_ranking` uses the same loop on `int` scores and is safe.

### S02c-007 [P2, confirmed] Profiles with zero alternatives are accepted; downstream functions break their contracts or give internal errors
- **Where:** include/socialchoicelab/aggregation/profile.h:96–110; profile_generators.h:45–73, 91–115
- **What:** `build_spatial_profile` and the spatial generators check voters but not
  alternatives.
- **Failure scenario:** `build_spatial_profile({}, {(0,0),(1,1)})` returns a "well-formed"
  profile with `n_alts = 0`. Then:
  - `pareto_set` returns `[]`, although the doc says it is always non-empty;
  - `plurality_one_winner` throws "break_tie: tied set must not be empty.", which tells the
    user nothing about the cause.
  - Reproduce with `$SCR/demo_rules.cpp`, section D5.
  - Reachable from Python with `profile_build_spatial([], voters)`. The binding passes
    `n_alts = 0`, and `scs_profile_build_spatial` (scs_api.cpp:2551) does not check
    `n_alts >= 1`. `scs_profile_from_utility_matrix` does check it.
- **Suggested fix:** Require `alternatives` to be non-empty in `build_spatial_profile` and
  the generators, with an end-user message. Do the same in `profile_from_utility_matrix`,
  which currently checks only rows.
- **Also affects:** C API `scs_profile_build_spatial`; Python/R `profile_build_spatial`.

### S02c-008 [P2, confirmed] Profile-based Condorcet and plurality results disagree with the geometry layer when voters are equidistant, and this is not documented
- **Where:** include/socialchoicelab/aggregation/profile.h:83–95; docs/architecture/aggregation_design.md:32–37, 268–271. Decision: Q7 (index ties in `build_spatial_profile` stay, "Out of scope").
- **What:** The index tie rule is documented in C++, R and Python. Its consequence is not:
  - an equidistant voter becomes a strict supporter of the lower-index alternative in
    every ordinal function;
  - the geometry layer (`majority_prefers`, `copeland_scores`, `condorcet_winner`) counts
    such voters for neither side.

  So the same spatial input gets different answers depending on the path.
- **Failure scenario:**
  - Alternatives A(−1,0), B(1,0); voters (0,0), (0,5) (both equidistant) and (0.5,0)
    (prefers B).
  - Profile path: `condorcet_winner_profile` = A, plurality scores (2, 1).
  - Geometry path: neither alternative is majority-preferred, and Copeland scores are
    (0, 0).
  - So A is called the Condorcet winner although no voter strictly prefers it.
  - Reproduce with `$SCR/demo_rules.cpp`, section D6.
- **Suggested fix:** Keep the rule (Q7). Document the consequence, with this example, in
  `profile.h`, aggregation_design §1/§9, and the R/Python `profile_build_spatial` docs.
  Optionally add a diagnostic that reports how many voters had exact distance ties.
- **Also affects:** R `profile.R:82–85`; Python `_profile.py:160–163`.

### S02c-009 [P2, confirmed] Seat-allocation tests miss every error branch and every remainder tie
- **Where:** tests/unit/test_seat_allocation.cpp (5 tests); include/socialchoicelab/aggregation/seat_allocation.h:31–45, 68–77, 112–114
- **What:** Line coverage from `test_seat_allocation` alone, measured with llvm-cov in
  `$SCR/cov/`:
  - all five `throw` branches are unexecuted;
  - the index tie-break `return a < b` (line 114) is never reached;
  - no test has equal remainders.

  This is why S02c-001 went unnoticed.
- **Failure scenario:** S02c-001 passes the whole suite.
- **Suggested fix:** Add tests for:
  - negative seats and negative votes;
  - top-k with `seat_count > n`;
  - top-k ties at the boundary;
  - equal remainders with unequal votes, and with equal votes;
  - the all-zero fallback with `seat_count > n`.

  For Stage 4.

### S02c-010 [P3, confirmed] Two more places still call `kRandom` the default
- **Where:** include/socialchoicelab/aggregation/tie_break.h:11–20; docs/architecture/aggregation_design.md:108–109, 122–125, 139–140, 157–159, 202–206. Decision: Q3
- **What:** These are not in Q3's Implies list:
  - The `tie_break.h` file comment says `kRandom` "is the DEFAULT for all *_one_winner()"
    and "the burden of passing kSmallestIndex is on the test author". Q3 reverses both.
  - The design doc's §4 and §6 signature blocks show `TieBreak = kRandom`. Implies lists
    only §3 lines 84–87.
- **Failure scenario:** After the Q3 code change, a reader of `tie_break.h` or of design doc
  §4/§6 is told the opposite of the actual default.
- **Suggested fix:** Update these together with the Q3 change, adding the bias warning for
  simulations.

### S02c-011 [P3, confirmed] Design doc claims every alternative in a Condorcet cycle is Pareto-efficient; false for four or more alternatives
- **Where:** docs/architecture/aggregation_design.md:233
- **What:** The claim holds for three alternatives, which is all the test
  `Pareto.Set_CycleProfile_AllEfficient` checks. With four or more, a Pareto-dominated
  alternative can sit inside a majority cycle.
- **Failure scenario:**
  - 3 voters: a>d>b>c, b>c>a>d, c>a>d>b.
  - Everyone ranks a over d, so d is Pareto-dominated.
  - Majority gives a>d, d>b, b>c and c>a, so d is in the cycle and there is no Condorcet
    winner.
  - `pareto_set` = {a, b, c}.
  - Reproduce with `$SCR/demo_rules.cpp`, section D4.
- **Suggested fix:** Limit the claim to three alternatives, or delete it. Zwicker (2016,
  Def. 2.6, book p. 32) is the definition source for Pareto dominance under linear orders,
  which the code matches.

### S02c-012 [P3, confirmed] `social_ranking.h` contract text does not match the code
- **Where:** include/socialchoicelab/aggregation/social_ranking.h:37–38, 44, 86, 94–98
- **What:**
  - `rank_by_scores` says it throws "if scores is empty", but it returns `{}`.
  - `pairwise_ranking` requires a square matrix but reads `pairwise(i, j)` for
    `j < rows()` without checking `cols()`. With `cols < rows` that is an out-of-bounds
    read in release builds (Eigen asserts are off).

  The C API always builds a square matrix, so only C++ callers are exposed.
- **Failure scenario:** `pairwise_ranking(Eigen::MatrixXi(3, 2))` reads past the column
  storage (by inspection; not run).
- **Suggested fix:** Correct the `@throws` text. Check `rows() == cols()` and throw
  `invalid_argument`.

### S02c-013 [P3, confirmed] `seat_allocation.h` has no documentation or citation, and the design doc does not cover it or `voter_sampler.h`
- **Where:** include/socialchoicelab/aggregation/seat_allocation.h:1–124; docs/architecture/aggregation_design.md (whole file)
- **What:** The header has no Doxygen and no reference for largest remainder
  (Hare–Niemeyer). Its tie rules are not stated:
  - top-k boundary ties go to the lower index;
  - Hare LR ties go by remainder, then votes, then index;
  - the all-zero-votes fallback goes to the lowest indices.

  Its error messages do not state the value entered (end-user rule). The design doc says
  "Implementation status: complete" but has no section on `seat_allocation.h` or
  `voter_sampler.h`. `competition_design.md:179–184` documents only the zero-vote fallback.
- **Failure scenario:**
  - `plurality_top_k_seat_allocation({5,5,3}, 1)` gives the seat to competitor 0, and
    `hare_largest_remainder_allocation({50,50}, 1)` gives `(1,0)`. A reader has no way to
    know this from the header.
  - `seat_count must be non-negative.` does not echo the value.
  - Reproduce with `$SCR/demo_seats.out`, edge-case lines.
- **Suggested fix:** Add doc comments with the tie rules and a citation to an
  apportionment source. Add the source to `references/REFERENCE_INDEX.md`; none is there
  now. Add the entered values to the messages, and add the two headers to the design doc.

## Tool leads checked
- Coverage `voter_sampler.h` 51% → **false positive.** It is an artifact of the merged
  multi-binary llvm-cov report, which warned about "mismatched data". Measured from
  `test_voter_sampler` alone: 95.6% lines (43/45). Only the unreachable `default:` branch
  (lines 96–97) is unexecuted. Correct but untested for that branch.
- Coverage `seat_allocation.h` 80% → S02c-009 (error branches and index tie path untested).
- Coverage `borda.h` 87% → correct but untested at C++ level: the `borda_ranking` `kRandom`
  path (lines 121–126) is not run by the C++ unit tests. For Stage 4.
- Coverage `tie_break.h` 88% → correct but untested: the empty-`tied` throw (line 51).
  S02c-007 shows it is reachable.
- Coverage `profile_generators.h` 88% → correct but untested: the `n_voters < 1` throws
  (lines 50, 96).
- Coverage `pareto.h` 94% → correct but untested: `rank_of` malformed-profile throw (line
  36), unreachable for constructor-built profiles.
- Coverage `social_ranking.h` 95% → correct but untested: `rank_by_scores` `kRandom` shuffle
  (line 69).
- `-Wsign-conversion` in `seat_allocation.h` (14), `profile.h` (12), `borda.h` (7),
  `approval.h` (7), `social_ranking.h` (6), `scoring_rule.h` (6),
  `profile_generators.h` (4), `plurality.h` (3), `antiplurality.h` (3),
  `condorcet_consistency.h` (2), `pareto.h` (1) → **false positive.** All are `int`
  indices used as `vector` subscripts or sizes, and they are non-negative under the
  documented preconditions (well-formed profile, validated counts). There is no
  signed/unsigned arithmetic mixing.
- clang-tidy `bugprone-easily-swappable-parameters` `condorcet_consistency.h:39`
  (`pairwise_count(profile, a, b)`) → false positive. It is a style warning; argument order
  is consistent at all call sites.
- Stage 1 hotspot "C++ default `kRandom` with null PRNG throws on ties" → known (Q3); only
  the additional doc instances are reported (S02c-010).
- Stage 1 hotspot "direct `engine()` use / `std::shuffle`" (`tie_break.h:66`,
  `profile_generators.h:151`) → known (Q4 Implies); no further instances in the slice.
- Prompt lead "profiles from spatial voters: tie handling documented?" → the rule is
  documented in C++ (`profile.h:87–88`), the design doc (§1, §9.1), R (`profile.R:84`) and
  Python (`_profile.py:163`). Its downstream consequence is not → S02c-008.
- Prompt lead "seat allocation vs published definitions" → only plurality top-k and Hare
  largest remainder are implemented; there is no D'Hondt or Sainte-Laguë.
  - Hare LR matches the textbook rule (quota V/S, floors, largest remainders), except at
    exact ties (S02c-001).
  - Zero-vote parties get no seats unless remainder seats run out, which is correct.
  - The all-zero case uses a deterministic fallback.
- Prompt lead "properties in `condorcet_consistency.h` and `pareto.h` vs literature":
  - Pareto dominance matches Zwicker (2016, Def. 2.6) for linear orders.
  - `is_condorcet_consistent` matches the per-profile reading of a Condorcet extension
    (Zwicker 2016, book pp. 35–36).
  - `is_majority_consistent` does not match → S02c-003.

## Open questions
1. **Seat ties in competition** (flag both ways). All seat ties go to the lowest
   competitor index: top-k boundary ties, equal remainders with equal votes, and the
   all-zero fallback.
   - Q7 chose random tie-breaking for voter assignment and Predator targeting so that
     earlier-listed competitors are not favoured.
   - Should seat ties also be broken at random from a named stream? Option A: yes,
     consistent with Q7 and unbiased. Option B: no, they are deterministic and documented,
     and they are rare with many voters.
2. **Which majority relation should the ordinal/pairwise "Copeland" use** (S02c-002)?
   - Option 1: net margin > 0, Zwicker's `NetP(x>y) > 0`, where indifferent voters do not
     count.
   - Option 2: a k-majority of all voters, as in `geometry::copeland_scores`.
   - Either way the two functions should agree, and the score should be wins − losses
     unless the asymmetric variant is wanted and named.
3. **Rename or redefine `is_majority_consistent`** (S02c-003)? The C API name
   `scs_is_selected_by_*_consistent_rules` suggests the rule-property reading
   (redefine). The current behaviour is "is the Condorcet winner" (rename).

## Out-of-slice notes
- `scs_profile_build_spatial` / `scs_approval_scores_spatial`: a negative `n_alts` or
  `n_voters` skips `validate_finite_doubles` (count ≤ 0 returns true). `build_voters` then
  calls `reserve(size_t(negative))`, so the user gets a `length_error` text instead of an
  end-user message (plausible, not run). Stage 3.
- `scs_profile_gaussian_spatial` / `_uniform_spatial` draw alternatives with `PRNG`
  methods but voters through `std::` distributions, mixing two draw paths on one stream.
  This is covered by Q4; noted for Stage 3 golden-value tests.
- Website impact: S02c-001 changes competition seat outputs in tie cases, and S02c-003/004
  change Python-visible predicates. The website uses competition seat shares only if it
  displays them; Stage 8 should check.
