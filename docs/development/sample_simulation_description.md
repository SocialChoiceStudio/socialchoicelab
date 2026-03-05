# Sample Simulation Description

**Note:** This is an illustrative description of one possible simulation (2D, plurality, Hunter, 10,000 runs × 1000 rounds). As we develop, it may need to be revised with more detail as things arise. It should **not** be considered a governing document.

---

## Setup

2D space. 5 candidates, 100 voters. The whole thing is repeated 10,000 times. Each of those 10,000 runs has 1000 rounds. We use Minkowski distance with p = 2. Voting rule: **plurality**. Candidates move each round using the **Hunter** rule. Candidates and voters can **keep information about where everyone was in earlier rounds** (stored information). Each round we may also compute and store **model output** (e.g. indifference, curves, win-sets, uncovered set); that is not agent memory — it is results we record for analysis.

---

## For each of the 10,000 runs

1. **Set up this run.**  
   In 2D space, give every candidate and every voter a random (x, y) position. Set any place that will hold "where people were before" to empty or to this starting setup.

2. **Compute distances.**  
   For each voter, compute the Minkowski-2 distance to each of the 5 candidates.

3. **Distances → utilities.**  
   Turn each distance into a utility (e.g. closer = higher utility). Each voter ends up with a utility for each candidate.

4. **Build the preference profile.**  
   For each voter, rank the 5 candidates by utility. When a voter has the same utility for two or more candidates, break ties at random to get a strict ranking. Each voter's ranking is a **preference ordering**. The set of these 100 preference orderings (one per voter) is the **preference profile** for this round.

5. **Run 1000 rounds: plurality, Hunter, stored information, and model output.**  
   For each round:
   - **Vote:** Apply **plurality** to the current preference profile (each voter's top candidate gets one vote; most votes wins). If two or more candidates tie for most votes, break the tie at random. Record who won.
   - **Update stored information:** Candidates and voters can update what they remember: where everyone was, this round and earlier. In some versions of the model, this use or update of memory can have a random component. This is **agent memory** — used by Hunter and by whatever uses "past positions."
   - **Move candidates:** Use the **Hunter** rule (or another candidate strategy) to update the 5 candidates' positions. These strategies will almost certainly use random numbers (e.g. choice among moves, perturbations). They can use current positions and the stored information about past positions.
   - **Compute and store model output (optional):** Each round we may compute and store things like indifference (e.g. indifference curves), win-sets, uncovered set, and similar. These are **model output** — results we record for analysis, not data that candidates or voters "remember" or use to adapt. Some of these computations may use random sampling or tie-breaking.
   - **Recompute for next round:** With the new candidate positions, recompute distances, utilities, and the preference profile (with random tie-breaking when utilities tie) for the next round.

6. **Record this run.**  
   From the 1000 round outcomes (and optionally from final positions, stored information, and the model output from each round), compute and save the summary for this run.

7. **Next run.**  
   Call `reset_for_run(master_seed, run_index)` (or equivalent) so the next run uses a deterministic, independent RNG state. Then repeat from step 1 with new random positions and reset stored information until all 10,000 runs are done.

---

## After all 10,000 runs

8. **Summarize across runs.**  
   Combine the 10,000 run summaries (e.g. averages, distributions).

---

## Where we draw random numbers (explicit list)

1. **Step 1 – Set up this run.** Random (x, y) for each candidate and each voter.
2. **Step 4 (and "recompute for next round") – Build the preference profile.** Random tie-breaking when a voter has the same utility for two or more candidates.
3. **Step 5 – Vote (plurality).** Random tie-breaking when two or more candidates tie for most votes.
4. **Step 5 – Move candidates (Hunter or other strategies).** Candidate strategies almost certainly use random numbers.
5. **Step 5 – Update stored information (agent memory).** In some versions, how agents/candidates use or update memory has a random component.
6. **Step 5 – Model output.** Some computations (indifference, win-sets, uncovered set, etc.) may use random sampling or tie-breaking.

---

## Terms used (plain language)

- **Preference ordering:** A single voter's ranking of the candidates (e.g. A ≻ B ≻ C ≻ D ≻ E, or equivalently u_i(A) > u_i(B) > u_i(C) > u_i(D) > u_i(E)).
- **Preference profile:** The set of preference orderings (one per voter).
- **Stored information / agent memory:** What candidates and voters keep about where everyone was in earlier rounds; used for adaptation (e.g. Hunter). May have a random component in some versions.
- **Model output:** Things we compute and store each round for analysis (indifference, curves, win-sets, uncovered set, etc.), not part of any agent's memory. Some of it may use randomness.

---

## Jargon mapping: process, thread, stream, memory, etc.

This section maps the technical terms (command, process, thread, module, stream, StreamManager, program, execution, memory space) onto the simulation above, and notes **alternative ways** to implement the same logical flow (nested and branching steps). The same "series of nested and branching function calls" can be implemented with one process, many processes, or one process with many threads; that choice affects where random numbers and memory live.

---

### Command

**Definition:** A **command** is the concrete request that tells the **operating system** to **start a new program** (and thus create a new process). When you type something in a terminal (e.g. `python run_sim.py`) and press Enter, that is one command — the terminal asked the OS to start that. One command → one new process.

**The actual difference:** In both cases script A causes script B to run. The **only** thing that differs is **how many processes (memory spaces) exist after**:

- **Way 1 — After B runs, there are TWO processes.**  
  A does something that asks the OS to start a new program (e.g. `subprocess.run(["python", "B.py"])`). The OS creates a **second** process and runs B there. So: **two processes**, **two separate memory spaces**. A's data is in one; B's data is in the other. They cannot see each other's variables. If B needs something from A, it has to be passed (e.g. files, command-line args). That request to the OS is a **command** → **new process**.

- **Way 2 — After B runs, there is still ONE process.**  
  A loads B's code and runs it (e.g. `import B; B.main()`). No second process is created. B runs **inside the same process** as A. So: **one process**, **one memory space**. A and B share it. They can see the same variables, the same StreamManager, the same everything. No request to the OS to start a new program → **no command** → **same process**.

So: "A runs B" in both cases. The difference is **one process vs two**. One memory space (shared) vs two (separate). We call the thing that creates a second process a **command**.

**Two minimal Python examples (same outcome, different mechanism):**

Both examples do the same thing: compute 2 + 3 and print the result. One uses Way 1 (two processes), one uses Way 2 (one process).

---

**Way 1 — two processes (A asks the OS to start B):**

You run: `python caller_way1.py`

`caller_way1.py`:
```python
import subprocess

# We are in process 1. We ask the OS to start a new program (Python running do_math.py).
# The OS creates process 2, runs do_math.py there, then process 2 exits.
# We are still in process 1. We never see do_math.py's variables.
subprocess.run(["python", "do_math.py"])

# Still process 1. We cannot see any variables that were inside do_math.py.
print("Caller done.")
```

`do_math.py` (runs in a separate process when started by the OS):
```python
# This file runs in process 2. It has its own memory. Nothing from caller_way1.py is here.
x = 2
y = 3
result = x + y
print(f"Result: {result}")  # prints Result: 5
# Process 2 ends when this file finishes.
```

Output: `Result: 5` then `Caller done.` — but the addition happened in another process. Two processes existed (caller + do_math).

---

**Way 2 — one process (A loads B's code and runs it):**

You run: `python caller_way2.py`

`caller_way2.py`:
```python
# We are in process 1. We load do_math's code into this same process and run it.
# No second process is created. No request to the OS to start a new program.
import do_math

# do_math's code runs here, in process 1. We could even see do_math's variables
# if we used do_math.x, do_math.result, etc.
do_math.run()

# Still process 1. Same memory space as do_math.run() used.
print("Caller done.")
```

`do_math.py` (same logic as in Way 1, but used as a module):
```python
# This code runs inside the same process as caller_way2. Same memory space.
def run():
    x = 2
    y = 3
    result = x + y
    print(f"Result: {result}")  # prints Result: 5
```

Output: `Result: 5` then `Caller done.` — same output. One process did everything.

---

Same printed result. Way 1: two processes (caller + do_math as a separate program). Way 2: one process (caller loads do_math and runs it).

---

### Module

**Definition:** A **module** is code that is **loaded into** the current process and run there (e.g. in Python, a `.py` file that you `import`). The module does not run as a separate program; it becomes part of whatever process did the import. So: **module** = code used the Way 2 way — same process, same memory space. The word is language-specific (Python says "module"; other languages have similar ideas like "library" or "package"), but the idea is: "code we load and run here, not code we ask the OS to start as a new program."

**In the examples above:** In Way 1, `do_math.py` is run by the OS as a **separate program** (not a module in that run). In Way 2, `do_math.py` is **imported** by `caller_way2.py`, so it is used as a **module** — same process, no new command.

**Library:** Often used to mean the same thing as "module" — code you load and use in the same process (Way 2). Sometimes "library" implies a collection of modules or code meant to be reused. For our purposes: **library** = code used the Way 2 way (load into this process, don't start a new program). We won't keep adding near-synonyms unless one becomes important for the simulation.

---

### Process

**Definition:** A process is one memory space created by the operating system when something is **launched** — i.e. when a **command** is run (you type a command in a terminal, or code asks the OS to run another command). Everything that runs as a result of that single launch shares that memory space until something explicitly asks the OS to start another launch (then a new process is created).

**Mapping onto this simulation:**

- The **whole simulation** (all 10,000 runs, steps 1–8) could be done inside **one process**: you launch once, and that one launch does all 10,000 runs (e.g. in a loop). One memory space holds all the data (positions, stored information, model output, run summaries) for whatever runs that process is doing.
- **Alternative:** The 10,000 runs could be split across **many processes**. Something (you, a script, a job scheduler) runs a **command** many times (or runs many commands). Each time the OS runs a command, it creates a separate process. Each process has its **own** memory space. Run 1's positions and stored information live in process 1's memory; run 2's live in process 2's memory; they do not share.

So: **one process** = one memory space doing some or all of the work. **Many processes** = many memory spaces, each doing a subset of the work. The **logical** structure (steps 1–8, 10,000 runs, 1000 rounds) is the same; only how many OS launches there are, and how many memory spaces, changes.

---

### Thread

**Definition:** A thread is one of several concurrent execution paths **inside** a single process. All threads in a process share the same memory space (same positions, same stored information, same run summaries — and, if the code is written that way, the same random-number manager).

**Mapping onto this simulation:**

- If we use **one process** to do the 10,000 runs, we can still do it in two ways:
  - **One thread:** That one process has a single execution path. It does run 1, then run 2, then run 3, … in sequence. All the steps (set up, distances, utilities, profile, 1000 rounds, record) are just nested/branching logic in that one path.
  - **Many threads:** That same process creates several threads. For example, thread A does runs 1–2500, thread B does runs 2501–5000, etc., at the same time. They all share the **same** memory space. So if they all use one shared "random-number manager" (StreamManager), that one object is accessed by multiple threads — which is the case we had to decide about (Option A vs B for StreamManager).

So: **thread** = one execution path inside a process. The simulation steps (1–7 per run) are the same whether one thread does them or many threads do different runs in parallel; the difference is whether multiple threads **share** the same memory (and same RNG) or not.

---

### Stream and StreamManager

**Definition:** A **stream** is a named sequence of random numbers (like a deck of cards you draw from in order). **StreamManager** is the object that creates and hands out streams so that (e.g.) "positions" and "tie-breaking" and "Hunter" can each have their own reproducible sequence. Whenever we "draw random numbers" in the simulation, that draw comes from some stream, and streams are typically obtained from a StreamManager.

**Mapping onto this simulation:**

- Every place in **"Where we draw random numbers"** (steps 1, 4, 5 vote, 5 Hunter, 5 stored information, 5 model output) needs random numbers. In code, those draws usually come from one or more **streams** (e.g. a stream for "initial positions," one for "profile tie-breaking," one for "Hunter"). The **StreamManager** is the single object that owns and doles out those streams so runs are reproducible.
- **Where StreamManager lives:** It lives in **one memory space** — the process that runs the code that uses it. So:
  - **One process, one thread:** One StreamManager in that process; only that thread uses it. No sharing.
  - **One process, many threads:** One StreamManager in that process; **multiple threads** may call it. Then we must either design it for that (Option A: thread-safe) or document that only one thread may use it and give each thread its own RNG elsewhere (Option B-style: single-threaded StreamManager).
  - **Many processes:** Each process has its **own** StreamManager (or its own RNG setup). No sharing across processes. Each process's streams are independent; reproducibility is per process (e.g. per run) unless we explicitly coordinate seeds.

So: **stream** = the thing that supplies the random numbers at each of the six places we listed. **StreamManager** = the thing that owns streams and is either used by one execution path (one thread) or by many (multiple threads in one process). Alternatives: one shared StreamManager for all threads (Option A) vs one StreamManager per process with only one thread using it (Option B), or many processes each with their own StreamManager.

---

### Option A and Option B (StreamManager thread-safety) — historical

**Adopted policy: Option B (single-threaded StreamManager).** The following describes the two options that were considered; Option B was adopted (see consensus plan Item 18).

These were two design choices for how StreamManager behaves when **multiple threads** in the same process might use it:

- **Option A (thread-safe StreamManager):** We design and implement StreamManager so that **multiple threads** in one process can call it at the same time without corrupting state or crashing. That means proper locking (or equivalent) so that one thread’s use does not interfere with another’s. We **document** that it is safe to use from many threads.

- **Option B (single-threaded StreamManager):** We **do not** support multiple threads calling the same StreamManager. We document that StreamManager is for use by **only one thread** per process. If a process has multiple threads, either only one thread uses StreamManager (and other threads use their own RNG or don’t need one), or we remove any misleading thread-safety claims and the mutex. Each process may still have one StreamManager, but only one thread in that process may touch it.

**In short:** Option A = multiple threads may share one StreamManager in a process. Option B = only one thread per process may use StreamManager (or we don’t claim thread-safety).

---

### Program / execution (disambiguation)

**Definition:** These terms are often used loosely. Here we tie them to the mapping above:

- **Program:** The code (scripts, libraries, executables) that implements the simulation. The same program can be **launched** once or many times. Each **launch** that asks the OS to run it creates one **process**.
- **Execution:** The act of running that code. "One execution" often means one process (one launch). So "10,000 executions" could mean 10,000 processes (10,000 launches), or one execution (one process) that internally performs 10,000 runs.

So for this simulation: the **program** is whatever implements steps 1–8. An **execution** is one run of that program by the OS (= one process, unless the program itself starts more processes). The **runs** (10,000 runs) are the logical repeat count; they can be implemented as 10,000 executions (10,000 processes) or as 1 execution (1 process) that loops 10,000 times, or 1 execution with many threads each doing some of the 10,000 runs.

---

### Memory space and what lives where

**Definition:** A **memory space** is the chunk of memory that one process has. All data that that process uses (positions, preference profiles, stored information, model output, run summaries, StreamManager, streams) lives in that process's memory space. Another process has a **different** memory space and cannot see the first process's data unless we explicitly send it (e.g. via files, pipes, or message passing).

**Mapping onto this simulation:**

- **Stored information (agent memory):** For a given run, this is the data that "candidates and voters remember" (past positions, etc.). It lives in the memory space of the **process** that is performing that run. If one process does one run, that run's stored information lives only in that process. If one process does all 10,000 runs (one thread), all 10,000 runs' stored information can live in that one memory space (e.g. one at a time, or in structures keyed by run id).
- **Model output:** Same idea — it lives in the memory space of the process that computed it. One process ⇒ one place; many processes ⇒ each process holds the model output for the runs it did.
- **StreamManager and streams:** They live in the process that uses them. One process ⇒ one StreamManager (and its streams) in that process. Many processes ⇒ each process has its own StreamManager (and its own streams), unless we explicitly build something shared (advanced and rare).

So the **core distinction** for "one process vs many processes" in terms of memory: with **one process**, one memory space holds everything for the work that process does (and if multiple threads run in that process, they **share** that memory). With **many processes**, each process has **discrete** memory; no process sees another's stored information or StreamManager unless we pass data between them.

---

### Worker (optional term)

**Definition:** "Worker" is sometimes used to mean "thread" — one of several execution paths doing work in parallel inside one process. We do not use it elsewhere in this doc; when we say "thread" we mean that.

---

### Preference ordering vs preference profile (recap)

**Preference ordering:** One voter's ranking of the candidates (e.g. A ≻ B ≻ C ≻ D ≻ E).  
**Preference profile:** The set of all preference orderings in a round (one per voter). So in this simulation, each round has one preference profile made of 100 preference orderings.

---

### Summary: alternatives for implementing the same simulation

| Choice | What it means for this simulation | RNG / StreamManager | Memory |
|--------|-----------------------------------|---------------------|--------|
| **One process, one thread** | One OS launch; one execution path; does all 10,000 runs in sequence (steps 1–7 in a loop). | One StreamManager in that process; only one thread uses it. Option B (single-threaded StreamManager) is enough. | One memory space; stored information and model output for each run live there (e.g. one run at a time). |
| **One process, many threads** | One OS launch; several execution paths; e.g. each thread does a chunk of the 10,000 runs in parallel. | One StreamManager in that process; **multiple threads** may call it. Need Option A (thread-safe) or give each thread its own RNG and do not share StreamManager (Option B: only one thread touches StreamManager). | One memory space **shared** by all threads; must be careful not to corrupt shared data (e.g. run summaries, or the StreamManager). |
| **Many processes** | Many OS launches; each process does one or more runs (e.g. process 1 does runs 1–1000, process 2 does 1001–2000, …). | Each process has its **own** StreamManager (or RNG). No sharing. Option B is enough per process. | **Discrete** memory per process; each process holds only the stored information and model output for its runs. |

The **logical** simulation (10,000 runs × 1000 rounds × steps 1–7, with randomness at the six places we listed) is the same in all cases. The **implementation** choice (one process vs many, one thread vs many) determines how many memory spaces there are and whether the random-number manager is used by one execution path or many, which is what we needed the jargon for.
