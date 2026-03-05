# StreamManager: Combined Implementation Guidelines

**Authors:** Claude Opus 4.6 + GPT 5.3 (merged)  
**Date:** 2026-02-14  
**Status:** Adopted (see project_log.md and where_we_are.md for history)  
**Sources:** `GPTProposedCombinedStreamManagerImplementation.md`, `OpusProposedCombinedStreamManagerImplementation.md`

---

## Purpose

Define the rules, conventions, and code changes for StreamManager so that simulations built on SocialChoiceLab are:

- **Reproducible** — same seed + configuration → same results, always.
- **Simple to reason about** — one owner per StreamManager, no hidden shared state.
- **Safe under parallelism** — parallelization never corrupts or couples random sequences.
- **Consistent across docs and code** — every simulation spec states its stream map and seeding policy.

---

## Part I: Rules

### R1. Single-owner StreamManager

Each StreamManager instance is owned and used by exactly one execution path (one thread). No StreamManager instance is shared across concurrent threads.

### R2. No global singleton in multi-threaded code

The global `get_global_stream_manager()` is acceptable for single-threaded programs and tests. In multi-threaded programs, each thread (or each run) should construct its own local StreamManager rather than using the global one.

### R3. Explicit seed hierarchy

Always define and document seed derivation:

- `master_seed` — global experiment seed.
- `run_seed = f(master_seed, run_index)` — deterministic per-run seed.
- Stream seeds — derived automatically by StreamManager from `run_seed` + stream name.

Use deterministic derivation only. No runtime entropy (`std::random_device`, `rand()`, etc.) for production experiments.

### R4. Portable stream seed derivation

StreamManager derives stream seeds from the master seed and stream name using FNV-1a (64-bit) plus a SplitMix64-style finalizer for better avalanche (consensus Items 32–33). This is deterministic across compilers and platforms. Do not replace FNV-1a with `std::hash` or any implementation-defined hash.

### R5. Fixed stream naming contract

Define a stable stream-name set and keep it versioned in docs and code. Suggested stream names (different simulations may adjust):

| Stream name      | Purpose |
|-----------------|---------|
| `voters`        | Voter initialization (positions, attributes) |
| `candidates`    | Candidate initialization (positions, attributes) |
| `tiebreak`      | Breaking preference ties and vote ties |
| `movement`      | Candidate movement strategies (Hunter, etc.) |
| `memory_update` | Agent memory updates (random components in stored information) |
| `analysis`      | Model output computations (sampling, tie-breaking in analysis) |

Do not overload one stream for unrelated purposes unless explicitly justified.

**Optional allowlist (Item 30):** Call `StreamManager::register_streams(names)` to lock the allowed set; thereafter `get_stream(name)` and `reset_stream(name, seed)` throw for unknown names (catches typos). Call `register_streams({})` to clear the allowlist. See design_document.md § c_api design inputs.

### R6. One responsibility per stream

Each stochastic concern draws from a dedicated stream. This prevents randomness coupling — changing one subsystem (e.g., adding a voter) must not shift the random sequence consumed by an unrelated subsystem (e.g., candidate movement).

### R7. Separate behavioral from analytical streams

Streams used by agents (voters, candidates) to make decisions within the simulation must be separate from streams used to compute model output for analysis. Enabling or disabling optional analysis computations must not alter the simulation's behavior.

### R8. Run-level independence

Each run is reproducible as a self-contained unit:

- A run can be re-executed alone (by index) and reproduce the same result.
- Run N must not depend on random draws consumed by run N−1.

### R9. No hidden fallback randomness

All random draws must come from declared streams. Do not use `std::random_device`, global `rand()`, unnamed generators, or any ad-hoc RNG inside simulation logic.

### R10. Tie-break policy is explicit

For all tie-breaks (preference ranking, vote winner, etc.):

- Document the tie-break rule.
- Document the stream used.
- Ensure deterministic behavior under fixed seed.

### R11. Replay metadata

Persist enough metadata to replay any result:

- Master seed.
- Run index and run seed.
- Stream naming/version.
- Key simulation parameters (dimensions, distance metric, loss function, number of voters/candidates, etc.).

Without replay metadata, results are analysis-only, not reproducible artifacts.

---

## Part II: Parallelism

### P1. Parallelize at the run level

The natural unit of parallelism is the run. Each run is independent and self-contained when R3 (deterministic per-run seeds) and R8 (run-level independence) are followed.

### P2. Multi-process parallelism (simplest)

Distribute runs across OS processes (job scheduler, `subprocess`, `multiprocessing`, etc.). Each process constructs its own StreamManager. No coordination needed beyond assigning run indices.

### P3. Multi-thread parallelism

If runs are distributed across threads within a single process, each thread constructs its own StreamManager (one per run, or one per thread reseeded per run). No thread accesses another thread's StreamManager.

### P4. Coordinator-thread pattern (alternative)

If a single thread needs to control seeding centrally, it may own the "master" StreamManager, derive per-run seeds from it, and distribute those seeds to worker threads. Workers construct their own local PRNGs from the received seeds. The coordinator thread is the only thread that touches the master StreamManager.

### P5. Parallel-safe by isolation

When parallelizing:

- Assign each worker isolated RNG ownership.
- Avoid concurrent mutation of shared stream state.
- Aggregate results after worker completion.

---

## Part III: Simulation Specification Requirements

Any simulation spec must explicitly state:

1. Which entities move per round (e.g., candidates only; voters are fixed).
2. Which steps consume randomness.
3. Which stream each random step uses.
4. How seeds are derived per run.
5. Whether workers share RNG state (expected answer: no).

If these are missing, the simulation is under-specified for reproducibility.

---

## Part IV: Validation Checklist

- [ ] Same config + same seed reproduces same outputs.
- [ ] Re-running a single run by index reproduces that run.
- [ ] Parallel and serial execution produce equivalent outcomes (same deterministic stream assignment → same results).
- [ ] Adding a new stochastic subsystem does not change existing subsystems' random sequences.
- [ ] Tie-break behavior is deterministic and documented.
- [ ] StreamManager docs match actual ownership contract.

---

## Part V: Code Changes Required

These changes bring `stream_manager.h` / `stream_manager.cpp` into alignment with the rules above. (Exact line numbers in the source may have changed since this document was written; refer to the current code for implementation details.)

### C1. Update the class-level Doxygen comment

The class-level comment currently implies thread-safety for concurrent access. Replace with:

> "Single-owner: each StreamManager instance should be used by exactly one thread. For multi-threaded programs, construct a separate StreamManager per thread or per run rather than sharing one instance."

### C2. Remove the mutex *(agreed by both agents; one option noted)*

Remove the `std::mutex` and all `std::lock_guard` calls from `StreamManager`. The mutex currently provides false safety: `get_stream()` returns a reference that outlives the lock, so the lock does not actually prevent data races on the returned PRNG. Removing it eliminates per-instance overhead and the false sense of thread-safety.

**Both agents agree removal is preferred.** GPT noted an alternative (keep as a limited guard with accurate docs); Opus rejected this as it still misleads. **Recommendation: remove.**

The global `get_global_stream_manager()` retains its Meyers' singleton initialization guard (sufficient for lazy construction). Document that the returned reference must only be used from one thread.

### C3. Fix the `const` overload of `get_stream()` *(agreed; one option noted)*

The current `const` overload silently creates new streams via `mutable` members, violating semantic `const`.

- **Opus recommendation (adopted):** Remove the `const` overload entirely. A `const StreamManager&` that silently mutates is worse than a compile error.
- **GPT alternative:** Make the `const` overload throw if the stream doesn't exist. Both agents agree this is acceptable; Opus prefers full removal.

**Recommendation: remove the `const` overload.**

### C4. Add `reset_for_run()` convenience method

Add a method that makes the per-run reseeding pattern (R3, R8) explicit and hard to get wrong:

```cpp
void reset_for_run(uint64_t global_master_seed, uint64_t run_index) {
    uint64_t run_seed = combine_seed(global_master_seed, run_index);
    reset_all(run_seed);
}
```

Where `combine_seed` uses FNV-1a or a similar deterministic combiner consistent with `generate_stream_seed`.

### C5. Remove `mutable` from member declarations

After removing the `const` overload (C3) and the mutex (C2), `streams_` and `mutex_` no longer need to be `mutable`. Remove the `mutable` qualifier:

```cpp
// before:
mutable std::unordered_map<std::string, std::unique_ptr<PRNG>> streams_;
mutable std::mutex mutex_;  // removed entirely per C2

// after:
std::unordered_map<std::string, std::unique_ptr<PRNG>> streams_;
// mutex_ removed entirely
```

---

## Part VI: Documentation Updates

### D1. Update `sample_simulation_description.md`

- In Step 7 ("Next run"), specify that StreamManager is reset with a deterministic per-run seed derived from `(master_seed, run_index)`.
- Add a stream assignment table mapping the six RNG use-points to named streams.

### D2. Update `stream_manager.h` Doxygen

Per C1 above.

### D3. Update `project_log.md`

Mark Item 18 as decided: single-owner StreamManager, no cross-thread sharing. Reference this document.

### D4. Update status docs

Once code changes C1–C5 are implemented, update where_we_are.md (and project_log.md) as needed.

---

## Part VII: When to Revisit

These guidelines should be revisited only if one of the following becomes true:

- **Binding callbacks:** R or Python bindings (via c_api) need to call StreamManager from threads the binding runtime controls (e.g., async callbacks in Shiny or Jupyter). In this case, the c_api layer should serialize access rather than making StreamManager itself thread-safe.
- **Shared cooperative streams:** A future simulation requires multiple threads to draw from the *same* named stream in a coordinated order. This would require a thread-safe stream container or a different RNG architecture.
- **Performance bottleneck:** Profiling proves that process-spawning overhead or per-thread StreamManager construction is a bottleneck. In this (unlikely) case, a shared StreamManager with proper ownership semantics could be considered.

If any of these triggers occurs, treat the change as a deliberate architecture revision with new invariants, tests, and documentation. Do not introduce shared concurrent RNG access implicitly.

---

## Summary of choices / points of difference

Both agents agreed on everything except these two points:

| Point | Opus | GPT | Adopted |
|-------|------|-----|---------|
| Mutex | Remove entirely | Preferred: remove; Alternative: keep as limited guard | **Remove** |
| `const get_stream()` | Remove overload entirely | Remove or make it throw | **Remove** |

No fundamental disagreements were found.

---

## Implementation status (Item 18)

All code changes C1–C5 were implemented on 2026-02-14. This document is the adopted reference for StreamManager ownership rules.
