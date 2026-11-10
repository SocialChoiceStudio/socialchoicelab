# Stage 03c — C API: RNG and competition
Model / effort: Opus 5.5 / high   Commit: e6772eb   Files reviewed: `src/c_api/scs_api.cpp` lines 283–581, 3576–3786, 4132–4742 (1,121 lines), with 111–282 as context; matching declarations in `include/scs_api.h` (441–556, 1223–1294, 1889–2020)
Not read (if any): none in the slice.

Scratch programs: `SCR=/private/tmp/claude-501/-Users-robiragan-Dropbox-SocialChoiceStudioDev/2402f97e-221e-494b-88cb-fc63c41efc50/scratchpad/stage3c/`
(`$SCR/build.sh <name> [asan]`; links `libscs_api` from the Stage 3 builds, `$SCR/../stage3/build` or `build_asan`).

## Summary

- **0 P0, 5 P1, 5 P2, 1 P3** (11 findings). Every P1 and P2 was run in a scratch program,
  except S03c-010, which only runs out of memory and is confirmed by logic.
- **Most important:** the design doc says every non-finite double is rejected at the C
  boundary. That is untrue for competition: `scs_competition_run_experiment` checks none of
  its coordinates (S03c-001), and no step or termination double is checked in either run
  path, so an `Inf` step with `reflect` hangs forever (S03c-002). Both are reachable from
  R and Python, which pass these values through unchecked.
- **Other boundary breaks:** unknown seat-rule, boundary and objective values silently
  become the defaults (S03c-003); `n * dims` overflows before the check meant to protect
  the call, ending in a segfault (S03c-004, same pattern as SCP-003);
  `scs_competition_trace_strategy_kinds` dereferences a null buffer (S03c-005).
- **Owner decisions:** Q10's two known bounds sites are the only absolute-margin code in
  this slice (no extra instance). The absent `StreamManager` requirement is Q7's Implies;
  the C API still accepts `mgr = NULL` and silently drops jitter (shown, not re-reported).
  The slice has no `std::` distribution, `engine()` use or mutable static state (Q4, Q9).
- **Leak runs clean:** 0 leaks in `CApi_StreamManager*:CApi_Competition*` and in R
  `test_competition.R`.

## Contracts this layer provides

For Stage 5 (bindings). Line numbers are at e6772eb.

**Errors in general**
- Handle-returning functions return `NULL` on failure; `int` functions return
  `SCS_OK` (0) or a non-zero `SCS_ERROR_*`. `err_buf` is written only on failure, is
  NUL-terminated when `err_buf_len > 0`, and is never cleared on success. `err_buf` may be
  null.
- `SCS_ERROR_INVALID_ARGUMENT` for bad arguments (including core `std::invalid_argument`),
  `SCS_ERROR_BUFFER_TOO_SMALL` for short buffers, `SCS_ERROR_INTERNAL` for other
  `std::exception`. Competition `NULL`-returning entry points report every failure as a
  message only. No entry point in this slice has `catch (...)`; everything thrown here is
  a `std::` exception (see S03c-010 for the accessors that have no `try` at all).

**StreamManager (3580–3679)**
- `scs_stream_manager_create` → caller owns; free with `scs_stream_manager_destroy`
  (`NULL` is a no-op).
- `scs_register_streams`: non-null `mgr`; `names` non-null when `n_names > 0`; each name
  non-null and non-empty; replaces the allowlist; `n_names == 0` clears it. Negative
  `n_names` is not handled (S03c-008).
- `scs_reset_all(mgr, seed)`: keeps the allowlist, cannot fail for non-null `mgr`.
- `scs_reset_stream(mgr, name, seed)`: seeds the stream with `seed` directly; unknown name
  under an allowlist → `INVALID_ARGUMENT`.
- `scs_skip(mgr, name, n)`: discards **n raw engine outputs**, not n draws (Stage 2a
  contract); unknown name under an allowlist → `INVALID_ARGUMENT`.
- Thread safety (Q9): one thread per handle, including `get_stream`, which mutates.

**PRNG draws (3685–3786)**
- All need non-null `mgr`, `stream_name`, `out`; otherwise `INVALID_ARGUMENT`.
- Streams are created lazily; under an allowlist an unknown name is `INVALID_ARGUMENT`
  (`StreamManager: stream name not in registered set (typo?): "other"`).
- Parameters are validated by `PRNG`, not the C layer: `uniform_real` needs finite
  `min < max`; `normal` needs finite mean and finite `sd > 0`; `bernoulli` needs finite
  `p` in [0, 1]; `uniform_int` needs `min ≤ max` (inclusive, full int64 range works);
  `uniform_choice` needs `n > 0` (result in `[0, n)`).
- `*out` is written only on success.
- `scs_uniform_real` can return `Inf` when the width overflows (S03c-007).

**`scs_competition_run` (4132–4205)**
- Non-null required: positions, `strategy_kinds`, voters, `dist_cfg`, `engine_cfg`.
  `competitor_headings` may be null (zero headings). `mgr` may be null today (Q7 changes
  this): Hunters then error at the first heading draw, and jitter is silently dropped.
- `n_competitors`, `n_voters`, `n_dims` ≥ 1; `n_dims` must be 1 or 2, but only the core
  checks that, after the C layer has done arithmetic with it (S03c-004).
- Positions, headings and voters are checked for finiteness. Step and termination doubles
  are not (S03c-002).
- Strategy and step kinds are validated; seat rule, boundary policy and objective are not
  (S03c-003).
- Salience weights: finite (checked here); non-negative and correct length checked by the
  core. `order_p` is checked by the core for Minkowski only.
- Bounds: always inferred as bbox(competitors ∪ voters) ± 1.0 (Q10 changes this).
- `signature_resolution ≤ 0` or NaN is silently replaced by 1e-6 (S03c-006).
- Returns a handle owned by the caller; free with `scs_competition_trace_destroy` (`NULL`
  is a no-op).

**`scs_competition_run_experiment` (4436–4503)**
- As above, with `num_runs ≥ 1`, and with **no finiteness check** on positions, headings
  or voters (S03c-001). Takes `master_seed` and owns its own `StreamManager`. `retain_traces`
  is accepted but no accessor reads the traces (S03c-009).
- Returns a handle; free with `scs_competition_experiment_destroy`.

**Trace accessors (4211–4434)**
- `scs_competition_trace_dims` → `n_rounds`, `n_competitors`, `n_dims`. `n_rounds` is 0 only
  when `max_rounds = 0` (an early stop at round 0 still records that round).
- Positions are `out_len ≥ n_competitors × n_dims` doubles, row-major by competitor.
  Vote/seat shares and totals are `out_len ≥ n_competitors`. `round_index` in
  `[0, n_rounds)`, otherwise `INVALID_ARGUMENT`.
- Null output → `INVALID_ARGUMENT`; short buffer → `BUFFER_TOO_SMALL`. **Exception:**
  `scs_competition_trace_strategy_kinds` crashes on null and returns `INVALID_ARGUMENT` on
  a short buffer (S03c-005).
- `scs_competition_trace_dist_config`: null `out_weights` is a size query (writes
  `*out_n_weights` and returns `SCS_OK`); capacity below the count is `BUFFER_TOO_SMALL`.
- Final shares (`final_vote_shares`, `final_seat_shares`) are the last **evaluated**
  round's, not those at the final positions (S02d-008), and are 0 when `max_rounds = 0`.
- Strategy kinds come back as `static_cast<int>` of the C++ enum, equal to the
  `SCS_COMPETITION_STRATEGY_*` values today (S03c-005).
- Termination reason maps one-to-one; `kCount` is unreachable.

**Experiment accessors (4510–4742)**
- `dims` → `num_runs`, `n_competitors`, `n_dims`. `summary` → mean rounds, early
  termination rate. `mean_final_*` are length `n_competitors`.
- `run_round_counts`, `run_termination_reasons`, `run_terminated_early_flags` are length
  `num_runs`. `run_final_vote_shares` / `run_final_seat_shares` are `num_runs ×
  n_competitors`, row-major by run. `run_final_positions` is
  `num_runs × n_competitors × n_dims`, row-major by run, then competitor.
- Null output → `INVALID_ARGUMENT`; short buffer → `BUFFER_TOO_SMALL`. No size-query form.
  The products can overflow `int` for absurd sizes (S03c-004).

**Thread safety and determinism**
- No static or global mutable state in this slice. Determinism and platform limits are as
  in the Stage 2 contracts (Q4 pending); nothing here draws except through `PRNG` methods
  or the competition core.

## Findings

### S03c-001 [P1, confirmed] `scs_competition_run_experiment` does not check positions, headings or voters for finiteness
- **Where:** src/c_api/scs_api.cpp:4448–4453 (nothing between the size check and `try`;
  compare `scs_competition_run` :4150–4158). Contract: docs/architecture/c_api_design.md:87–92.
- **What:** The design doc says all doubles are validated at the boundary and the core
  need not tolerate non-finite input. The experiment entry point skips the
  `validate_finite_doubles` calls that `scs_competition_run` makes, so NaN/Inf go to the
  bounds code and core.
- **Failure scenario:** `$SCR/t02_experiment_nonfinite.c`: position NaN and 2 rounds →
  `ERROR alternative_point[0] is not finite` (no index, no name of the input, no fix),
  where `scs_competition_run` says `competitor_positions: non-finite value at index 0`.
  With 0 rounds it succeeds and returns the NaN/Inf: `OK … final=(nan,0.1),(1.9,1.9)`,
  and Inf voter → `OK` handle. A NaN heading on a Sticker → success, no error. NaN
  heading on a Hunter → `alternative_point[0] is not finite`. From Python
  (`$SCR/t09_py.py`): `competition_run_experiment` with a NaN position → `SCSError
  scs_competition_run_experiment failed: alternative_point[0] is not finite`.
- **Suggested fix:** Share one validation helper between run and experiment (this pairs
  with Q10's shared bounds helper).
- **Also affects:** R `competition_run_experiment` and Python `competition_run_experiment`
  pass values straight through (no finiteness check in `_competition.py` / `competition.R`).

### S03c-002 [P1, confirmed] Step and termination doubles are not checked for finiteness; `Inf` step with `reflect` hangs
- **Where:** src/c_api/scs_api.cpp:373–397 (`to_cpp_step_config`), :464–477
  (`to_cpp_termination_config`); contract c_api_design.md:87–92. Root cause in the core:
  S02d-003, S02d-010.
- **What:** No step double (`fixed_step_size`, `min_step_size`, `max_step_size`,
  `proportionality_constant`, `jitter`) or termination double (`position_epsilon`,
  `objective_epsilon`, `signature_resolution`) is checked for finiteness at the C boundary.
- **Failure scenario:** `$SCR/t04_step_term.c`:
  - `fixed_step_size=Inf`, `boundary_policy=reflect` → `KILLED by signal 14 (still running
    after 5 s: HANG)`. Also for `1e17`.
  - `fixed_step_size=NaN` → `ERROR alternative_point[0] is not finite` (an unrelated
    message).
  - `jitter=NaN` → run proceeds with no jitter applied (`final=(0,1)`), no error.
  - `position_epsilon=NaN`, `stop_on_convergence=1`, nobody moves → runs all 40 rounds,
    where 1e-9 stops after round 1 (`rounds=40 early=0` vs `rounds=1 early=1`): the stop
    rule is silently disabled.
  - `signature_resolution=Inf`, `stop_on_cycle` → `rounds=1 early=1 reason=2` (false cycle
    at round 0).
- **Suggested fix:** Validate each of these doubles at the boundary with an end-user
  message (value entered, valid range). Add the core fixes from S02d-003 and S02d-010; one
  shared check in the core `validate_*` functions also covers `run_competition_experiment`.
- **Also affects:** R `make_competition_step_config` / `make_competition_termination_config`,
  Python `CompetitionStepConfig` / `CompetitionTerminationConfig` (values pass through
  unchecked), and the website if it ever exposes these.

### S03c-003 [P1, confirmed] Unknown seat rule, boundary policy and objective values silently become defaults
- **Where:** src/c_api/scs_api.cpp:360–371 (`to_cpp_seat_rule`), :399–412
  (`to_cpp_boundary_policy`), :414–425 (`to_cpp_objective_kind`). Compare
  `to_cpp_step_config` and `to_cpp_comp_strategy` (:341–358, :387–389), which throw.
- **What:** An out-of-range enum value from C falls through to `default:` and runs as the
  first enumerator, with no error.
- **Failure scenario:** `$SCR/t03_enums.c`: `seat_rule=99`, `boundary_policy=99` and
  `objective_kind=99` each return a handle with `err=''` and final positions identical to
  a valid run (`final=(-0.207287,-0.0468676),(1.54645,1.54645)`); `step kind=99` and
  `strategy=99` return `ERROR unknown …`. A caller who passes a stale or wrong integer gets
  results under a different rule and is never told.
- **Suggested fix:** Throw `std::invalid_argument` in all three, naming the value received
  and the valid values, like the two siblings.
- **Also affects:** Direct C callers. R and Python validate the strings first.

### S03c-004 [P1, confirmed] `n_competitors * n_dims` and `n_voters * n_dims` overflow before any range check; the check is then skipped and the loops read out of bounds
- **Where:** src/c_api/scs_api.cpp:4150–4155 (the products passed to
  `validate_finite_doubles`); :4464–4476 (experiment bounds loops, no product check at
  all); :335, :514–515 (`build_points_nd`, `build_competitors`); :4659, :4690, :4726
  (`needed` in the `run_final_*` accessors). Same pattern as SCP-003.
- **What:** `n_dims` is only checked `≥ 1`, and products use `int`. A wrapped negative
  count makes `validate_finite_doubles` return "nothing to validate". The later loops then
  read `positions[i * n_dims + d]` past the buffer. `n_dims > 2` is rejected only by the
  core, after this arithmetic. In the accessors a wrapped `needed` passes the buffer check
  (`out_len < needed` is false) and the fill loop writes past the buffer.
- **Failure scenario:** `$SCR/t01_overflow.c`, `n_competitors = 1073741825`, `n_dims = 2`:
  - Debug build: `exit=139` (SIGSEGV) for both run and experiment.
  - ASan/UBSan: `scs_api.cpp:4150:68: runtime error: signed integer overflow: 1073741825 *
    2`; experiment: `AddressSanitizer: stack-buffer-overflow … in
    scs_competition_run_experiment scs_api.cpp:4468`.
  - Accessor overflow (`num_runs × n_competitors × n_dims > INT_MAX`) is by logic only; it
    needs more than 2^31 result values, so it was not run.
  Reachable by direct C calls; R and Python pass real lengths.
- **Suggested fix:** Before any arithmetic, reject `n_dims` outside {1, 2} and any count
  whose product with `n_dims` exceeds `INT_MAX`, with an end-user message. Compute
  accessor sizes in `int64_t`/`size_t` and reject sizes above `INT_MAX`.
- **Also affects:** Same check should live in the shared helper added for S03c-001.

### S03c-005 [P1, confirmed] `scs_competition_trace_strategy_kinds` dereferences a null output buffer and returns the wrong code for a short one
- **Where:** src/c_api/scs_api.cpp:4415–4434.
- **What:** Unlike every other trace accessor, there is no null check on `out_kinds`, and a
  short buffer returns `SCS_ERROR_INVALID_ARGUMENT` instead of `SCS_ERROR_BUFFER_TOO_SMALL`.
  The kinds are also emitted with `static_cast<int>` rather than a mapping, relying on the
  C++ and C enum orders matching (they do today; Q8's removal of `hunter_sticker` needs care).
- **Failure scenario:** `$SCR/t05_accessors.c`: `scs_competition_trace_strategy_kinds(t, NULL,
  2, …)` → `child killed by signal 11`. `out_len=1` → `rc=1` (BUFFER_TOO_SMALL is 3).
  `scs_competition_trace_round_vote_totals(t, 0, NULL, 2, …)` → `rc=1 … output buffer must
  not be null`, which is the expected behaviour.
- **Suggested fix:** Use `copy_int_vector` after mapping through an explicit
  `to_c_strategy_kind`.
- **Also affects:** Direct C callers; the bindings allocate the buffer themselves.

### S03c-006 [P2, confirmed] `signature_resolution` ≤ 0 or NaN is silently replaced by 1e-6 where the core would throw
- **Where:** src/c_api/scs_api.cpp:471–472; core check termination.h (`validate_termination_config`).
- **What:** `cfg.signature_resolution > 0.0 ? … : 1e-6` overrides invalid input before
  the core can reject it. Nothing in `scs_api.h` documents this.
- **Failure scenario:** `$SCR/t04_step_term.c`: `signature_resolution=-5`, `0` and `NaN`
  with `stop_on_cycle=1` all return `OK rounds=3 early=1 reason=2`: the run proceeds with a
  resolution the caller did not ask for. From Python (`$SCR/t09_py.py`):
  `signature_resolution=-1.0` runs with no error; R's `make_competition_termination_config`
  also passes the value through unchecked.
- **Suggested fix:** Pass the value through and let the core reject it (and add a
  finiteness check per S03c-002). If 0 is meant to be "use the default", document that and
  accept only 0.
- **Also affects:** R `make_competition_termination_config`, Python
  `CompetitionTerminationConfig`.

### S03c-007 [P2, confirmed] `scs_uniform_real` forwards ranges whose width overflows and returns `Inf`
- **Where:** src/c_api/scs_api.cpp:3685–3702 (forwards to `PRNG::uniform_real`; root cause
  is S02a-008).
- **What:** The header promises a draw in `[min, max)`. For finite bounds whose width
  overflows, the result is outside it and `rc` is `SCS_OK`.
- **Failure scenario:** `$SCR/t06_rng.c`: `scs_uniform_real(m, "s", -1.7e308, 1.7e308, &x,
  …)` → `rc=0 x=inf` on all 6 draws tried.
- **Suggested fix:** Fix once in `PRNG::uniform_real` (S02a-008): reject a non-finite
  width with an end-user error. No separate C API check is then needed.
- **Also affects:** R and Python RNG wrappers, any caller using this entry point.

### S03c-008 [P2, confirmed] `scs_register_streams` with negative `n_names` reports `SCS_ERROR_INTERNAL` with the message `vector`
- **Where:** src/c_api/scs_api.cpp:3602–3609.
- **What:** The null check is `n_names > 0 && !names`, so a negative count reaches
  `reserve(static_cast<size_t>(n_names))`, which throws `length_error`.
- **Failure scenario:** `$SCR/t06_rng.c`: `n_names=-1` → `rc=2 (INTERNAL) err='vector'`,
  with `names` null or not. Expected `INVALID_ARGUMENT` naming `n_names` and the value.
- **Suggested fix:** Reject `n_names < 0` at the start with an end-user message.
- **Also affects:** none.

### S03c-009 [P2, confirmed] `retain_traces` keeps every run's full trace with no way to read it
- **Where:** src/c_api/scs_api.cpp:4441, :4488; scs_api.h:1972; experiment_runner.h:24. No
  other mention of `traces` in `scs_api.cpp` or `scs_api.h`.
- **What:** The C API accepts `retain_traces` and stores the traces, but no accessor exposes
  them. The option only costs memory.
- **Failure scenario:** `$SCR/t08_retain.c` (2,000 voters, 3 competitors, 50 rounds, 40
  runs): `retain_traces=0` adds 12.0 MB of RSS, `retain_traces=1` adds 309.6 MB, and
  neither returns anything different to the caller. R documents it as "retain full per-run
  traces inside the C++ experiment result", which promises nothing usable.
- **Suggested fix:** Remove the parameter (no compatibility duty, Q12) or add a run-trace
  accessor.
- **Also affects:** R `competition_run_experiment`, Python `competition_run_experiment`.

### S03c-010 [P2, confirmed (logic)] Trace accessors and pre-`try` validation can let `bad_alloc` escape `extern "C"`
- **Where:** src/c_api/scs_api.cpp:4397, :4410 (`final_metric_shares_from_trace` builds a
  vector with no `try`); :4324, :4344, :4364, :4384, :4555, :4570 and siblings
  (`copy_*_vector` builds `std::string` on its error path outside any `try`); :4150–4155
  (`validate_finite_doubles` builds a `std::string` on its error path, before the `try`).
- **What:** These paths allocate outside a `try`. A `bad_alloc` would leave the `extern
  "C"` function and terminate the process. Only out-of-memory conditions reach it, so this
  is not a P0 despite the rubric's "exception escaping a C boundary" example. Not run.
- **Failure scenario:** Under memory pressure, `scs_competition_trace_final_vote_shares`
  (one small allocation) terminates the host R or Python process instead of returning an
  error code. The file's header comment says no exception escapes this translation unit.
- **Suggested fix:** Wrap each accessor in the same `try`/`catch` pattern, or make the
  shared helpers `noexcept` and fall back to static messages.
- **Also affects:** Other slices (3a, 3b) if the same helpers are called outside `try`.

### S03c-011 [P3, confirmed] Several error messages do not say what was entered or how to fix it
- **Where:** src/c_api/scs_api.cpp:4141 (`null pointer argument`, which pointer?), :4146,
  :4450 (`invalid dimensions or num_runs`, no values), :3599, :3647–3648, :3666, :3689,
  :3772; core messages passed through unchanged (e.g. `alternative_point[0] is not finite`
  from S03c-001, `uniform_real: min must be < max`).
- **What:** The end-user rule (what failed, what was entered, how to fix it) is not met.
  The numeric errors in this slice do not report the values given.
- **Failure scenario:** `$SCR/t06_rng.c` and `t03_enums.c` output: `scs_uniform_choice: n
  must be positive` for `n=0`, `uniform_real: min must be < max` for `min=max=1`, with no
  entered value or remedy.
- **Suggested fix:** Name the argument and include its value; reuse the checks added for
  S03c-001, -002 and -004 to do this once.
- **Also affects:** R and Python surface these strings.

## Tool leads checked

- Stage 2: `scs_competition_run_experiment` skips `validate_finite_doubles` → S03c-001.
- Stage 2: step and termination doubles not checked for finiteness → S03c-002.
- Stage 2: `to_cpp_termination_config` replaces `signature_resolution <= 0` with 1e-6 → S03c-006.
- Stage 2: bounds inference duplicated (Q10 Implies) → known, not re-reported. The two
  sites at :4165–4186 and :4460–4479 are the only absolute-margin code in the slice.
- Stage 2 / S02a-008: `scs_uniform_real` width overflow → S03c-007.
- S02d-006 (engine without a StreamManager) → confirmed reachable through the C API and
  already covered by Q7 Implies for `scs_competition_run`: `$SCR/t07_null_mgr.c`,
  Aggregators with `jitter=0.5`, `mgr=NULL` → `OK final=(0.000000,0.000000)`, against
  `(-0.032486,-0.164819)` with a manager; Hunters error at the first heading draw.
- SCP-003 pattern (other `n * dim` instances) → S03c-004. Not a re-report of Voronoi.
- SCP-004 → out of this slice (3b); nothing in this slice draws in an argument list.
- SCP-001, SCP-002 → geometry only; not reachable from this slice.
- Stage 0 never-called accessors → in this slice `test_c_api` does not call
  `scs_competition_trace_final_positions`, `scs_competition_trace_round_seat_shares` or
  `scs_competition_experiment_mean_final_seat_shares` (three by grep; the Stage 3 prompt
  says two). All three are used by R and Python. `trace_final_positions` also ran correctly
  in scratch tests. Correct but untested at C level; for Stage 4.
- Stage 0 clang-tidy `implicit-widening` / `sign-conversion` in this slice → covered by
  S03c-004 where it matters; the other sites index after a range check and are harmless.
- Stage 1 hotspot "only one `catch (...)`" → no non-`std::` exception is thrown in this
  slice; the real gap is accessors with no `try` (S03c-010).
- Q4 greps of `scs_api.cpp` (`engine()`, `std::*_distribution`, `std::shuffle`) → none.
  RNG-version constant is Q4 Implies. Q9 grep for mutable statics → only `static` helper
  functions.
- Heart → no Heart code in this slice.

## Leak runs

Build: `$SCR/../stage3/build` (Debug, no sanitizers).

```
cd $SCR
MallocStackLogging=1 leaks --atExit -- $B/test_c_api \
    --gtest_filter='CApi_StreamManager*:CApi_Competition*'
```
Result: 16 tests passed. `Process 58591: 187 nodes malloced for 15 KB` /
`Process 58591: 0 leaks for 0 total leaked bytes.` Output in `$SCR/leaks_gtest.txt`.

R, from a copy of `r/` (`$SCR/rcopy`, installed to `$SCR/rlib` with
`SCS_LIB_PATH=$B SCS_INCLUDE_PATH=<repo>/include R CMD INSTALL --no-docs`), running
`test_competition.R` through `$SCR/run_r.sh leaks`:
```
MallocStackLogging=1 leaks --atExit -- $R_HOME/bin/exec/R \
    --no-save --no-restore --vanilla --slave -f $SCR/run_r_comp.R
```
Result: `tests: 5 failed: 0 errors: 0 warnings: 0 skipped: 0 nb expectations: 31`;
`Process 59065: 17894 nodes malloced for 80178 KB` /
`Process 59065: 0 leaks for 0 total leaked bytes.` No `libscs_api` or `scs_` frames.
Output in `$SCR/leaks_r.txt`.

Not covered by the leak runs: error paths where validation returns before any handle is
allocated. Reading the slice, every handle is created last (`new … {std::move(...)}`), so
no error path holds one.

## Open questions

None new in this slice. Behaviour at the Q7/Q8/Q10 boundary is decided and tracked in
those answers; Q29, Q30 and Q33 (Predator tie, reflect heading, Hunter step reuse) are core
questions and do not change this layer's contract.

## Out-of-slice notes

- Bindings pass non-finite step and termination values through to the C API (S03c-002,
  S03c-006); Stage 5 should note this if they add their own validation.
- `r/tests/testthat/test_competition.R` has 5 `test_that` blocks with 31 expectations and
  uses only finite inputs; Stage 4 may want NaN/Inf and experiment-path cases.
