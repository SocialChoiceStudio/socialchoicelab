# 2026-03-09: The Last Problem

This note is for the next agent who picks up the competition-animation work.

## What is implemented now

Layer 7 competition is substantially present in the local codebase:

- C++ competition core
- experiment runner
- competition C API
- R and Python bindings
- static competition plotting
- animated competition trajectories in R and Python

The current open problem is **not** the engine math. The underlying round
positions are correct.

## The specific remaining problem

In **R only**, the default animated competition plot still shows a visually
incorrect behavior:

- the first two animated jumps are too large;
- after those first two jumps, the later movement looks like the correct step
  size;
- Python does **not** show this problem.

This is currently most visible in:

- `r/TestScript_17_Hunter_Duel_100x100.R`

Scenario used for diagnosis:

- 100x100 space
- 101 voters
- 2 candidates
- both Hunter
- fixed step size

## What has already been checked

The following are **not** the problem:

- The competition engine itself:
  - round positions are correct when printed directly.
- Initial visible candidate coordinates:
  - the visible base traces now start at the correct first-round coordinates.
- Stale frame seeding from the final state:
  - visible base competitor traces are now unframed and start at the first
    round.
- Candidate trace disappearance:
  - earlier disappearance issues were improved by moving to native frame-based
    animation in R for the default no-trail path.

The problem appears to be in **R Plotly animation interpolation / state
application**, not in the competition engine.

## Important implementation context

Current design direction:

- Keep R and Python as parallel as their libraries reasonably allow.
- The default animation path (`trail = "none"`) is meant to be parallel across
  both bindings.
- The remaining R issue should be solved in a principled way, not with
  increasingly ad hoc tweaks.

The user explicitly asked that we stop driving further off the road and check
known Plotly behavior when the fixes start to get too creative. That was the
right call.

## Most likely next step

If the oversized first two jumps still persist after the latest unframed-start
fix, the next step should be:

- switch the R default animation to **stepwise frame changes with no tweened
  interpolation between rounds**, rather than trying to preserve smooth
  interpolation.

Why:

- this is a discrete round-based simulation;
- exact round-to-round stepping is defensible;
- it avoids relying on Plotly R’s interpolation/object-constancy behavior,
  which is the part that still appears unstable.

## After this problem is solved

The next work item should be **animation refinement**, not more core-engine
work. Expected next refinements:

- refine the animation controls/layout
- add trail toggles
- improve fade-trail behavior
- continue R/Python visual parity work

