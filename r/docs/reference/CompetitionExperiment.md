# CompetitionExperiment: replicated competition runs

CompetitionExperiment: replicated competition runs

CompetitionExperiment: replicated competition runs

## Details

Wraps the `SCS_CompetitionExperiment` C handle. Create experiments with
[`competition_run_experiment`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/competition_run_experiment.md).

## Methods

### Public methods

- [`CompetitionExperiment$new()`](#method-CompetitionExperiment-new)

- [`CompetitionExperiment$dims()`](#method-CompetitionExperiment-dims)

- [`CompetitionExperiment$summary()`](#method-CompetitionExperiment-summary)

- [`CompetitionExperiment$mean_final_vote_shares()`](#method-CompetitionExperiment-mean_final_vote_shares)

- [`CompetitionExperiment$mean_final_seat_shares()`](#method-CompetitionExperiment-mean_final_seat_shares)

- [`CompetitionExperiment$run_round_counts()`](#method-CompetitionExperiment-run_round_counts)

- [`CompetitionExperiment$run_termination_reasons()`](#method-CompetitionExperiment-run_termination_reasons)

- [`CompetitionExperiment$run_terminated_early_flags()`](#method-CompetitionExperiment-run_terminated_early_flags)

- [`CompetitionExperiment$run_final_vote_shares()`](#method-CompetitionExperiment-run_final_vote_shares)

- [`CompetitionExperiment$run_final_seat_shares()`](#method-CompetitionExperiment-run_final_seat_shares)

- [`CompetitionExperiment$run_final_positions()`](#method-CompetitionExperiment-run_final_positions)

- [`CompetitionExperiment$clone()`](#method-CompetitionExperiment-clone)

------------------------------------------------------------------------

### Method `new()`

Internal. Use
[`competition_run_experiment`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/competition_run_experiment.md)
instead.

#### Usage

    CompetitionExperiment$new(ptr)

#### Arguments

- `ptr`:

  External pointer returned by a C factory function.

------------------------------------------------------------------------

### Method `dims()`

Return basic experiment dimensions.

#### Usage

    CompetitionExperiment$dims()

#### Returns

Named list with `n_runs`, `n_competitors`, and `n_dims`.

------------------------------------------------------------------------

### Method [`summary()`](https://rdrr.io/r/base/summary.html)

Return summary statistics across experiment runs.

#### Usage

    CompetitionExperiment$summary()

#### Returns

Named list with summary metrics such as mean rounds and early
termination rate.

------------------------------------------------------------------------

### Method `mean_final_vote_shares()`

Return mean final vote shares across runs.

#### Usage

    CompetitionExperiment$mean_final_vote_shares()

#### Returns

Named numeric vector of mean final vote shares by competitor.

------------------------------------------------------------------------

### Method `mean_final_seat_shares()`

Return mean final seat shares across runs.

#### Usage

    CompetitionExperiment$mean_final_seat_shares()

#### Returns

Named numeric vector of mean final seat shares by competitor.

------------------------------------------------------------------------

### Method `run_round_counts()`

Return the realized round count for each run.

#### Usage

    CompetitionExperiment$run_round_counts()

#### Returns

Integer vector of round counts.

------------------------------------------------------------------------

### Method `run_termination_reasons()`

Return the termination reason for each run.

#### Usage

    CompetitionExperiment$run_termination_reasons()

#### Returns

Character vector of termination reason labels.

------------------------------------------------------------------------

### Method `run_terminated_early_flags()`

Return whether each run terminated early.

#### Usage

    CompetitionExperiment$run_terminated_early_flags()

#### Returns

Logical vector.

------------------------------------------------------------------------

### Method `run_final_vote_shares()`

Return final vote shares for each run.

#### Usage

    CompetitionExperiment$run_final_vote_shares()

#### Returns

Numeric matrix with rows as runs and columns as competitors.

------------------------------------------------------------------------

### Method `run_final_seat_shares()`

Return final seat shares for each run.

#### Usage

    CompetitionExperiment$run_final_seat_shares()

#### Returns

Numeric matrix with rows as runs and columns as competitors.

------------------------------------------------------------------------

### Method `run_final_positions()`

Return final competitor positions for each run.

#### Usage

    CompetitionExperiment$run_final_positions()

#### Returns

Numeric 3D array with dimensions run x competitor x dimension.

------------------------------------------------------------------------

### Method `clone()`

The objects of this class are cloneable with this method.

#### Usage

    CompetitionExperiment$clone(deep = FALSE)

#### Arguments

- `deep`:

  Whether to make a deep clone.
