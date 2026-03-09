# CompetitionTrace: one competition simulation trace

CompetitionTrace: one competition simulation trace

CompetitionTrace: one competition simulation trace

## Details

Wraps the `SCS_CompetitionTrace` C handle. Create traces with
[`competition_run`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/competition_run.md).

## Methods

### Public methods

- [`CompetitionTrace$new()`](#method-CompetitionTrace-new)

- [`CompetitionTrace$dims()`](#method-CompetitionTrace-dims)

- [`CompetitionTrace$termination()`](#method-CompetitionTrace-termination)

- [`CompetitionTrace$round_positions()`](#method-CompetitionTrace-round_positions)

- [`CompetitionTrace$final_positions()`](#method-CompetitionTrace-final_positions)

- [`CompetitionTrace$round_vote_shares()`](#method-CompetitionTrace-round_vote_shares)

- [`CompetitionTrace$round_seat_shares()`](#method-CompetitionTrace-round_seat_shares)

- [`CompetitionTrace$round_vote_totals()`](#method-CompetitionTrace-round_vote_totals)

- [`CompetitionTrace$round_seat_totals()`](#method-CompetitionTrace-round_seat_totals)

- [`CompetitionTrace$final_vote_shares()`](#method-CompetitionTrace-final_vote_shares)

- [`CompetitionTrace$final_seat_shares()`](#method-CompetitionTrace-final_seat_shares)

- [`CompetitionTrace$clone()`](#method-CompetitionTrace-clone)

------------------------------------------------------------------------

### Method `new()`

Internal. Use
[`competition_run`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/competition_run.md)
instead.

#### Usage

    CompetitionTrace$new(ptr)

#### Arguments

- `ptr`:

  External pointer returned by a C factory function.

------------------------------------------------------------------------

### Method `dims()`

Return basic trace dimensions.

#### Usage

    CompetitionTrace$dims()

#### Returns

Named list with `n_rounds`, `n_competitors`, and `n_dims`.

------------------------------------------------------------------------

### Method `termination()`

Return the trace termination status.

#### Usage

    CompetitionTrace$termination()

#### Returns

Named list with `terminated_early` and `reason`.

------------------------------------------------------------------------

### Method `round_positions()`

Return competitor positions for a given round.

#### Usage

    CompetitionTrace$round_positions(round)

#### Arguments

- `round`:

  1-based round index.

#### Returns

Numeric matrix with one row per competitor and one column per dimension.

------------------------------------------------------------------------

### Method `final_positions()`

Return final competitor positions after the last move.

#### Usage

    CompetitionTrace$final_positions()

#### Returns

Numeric matrix with one row per competitor and one column per dimension.

------------------------------------------------------------------------

### Method `round_vote_shares()`

Return vote shares for a given round.

#### Usage

    CompetitionTrace$round_vote_shares(round)

#### Arguments

- `round`:

  1-based round index.

#### Returns

Named numeric vector of vote shares by competitor.

------------------------------------------------------------------------

### Method `round_seat_shares()`

Return seat shares for a given round.

#### Usage

    CompetitionTrace$round_seat_shares(round)

#### Arguments

- `round`:

  1-based round index.

#### Returns

Named numeric vector of seat shares by competitor.

------------------------------------------------------------------------

### Method `round_vote_totals()`

Return integer vote totals for a given round.

#### Usage

    CompetitionTrace$round_vote_totals(round)

#### Arguments

- `round`:

  1-based round index.

#### Returns

Named integer vector of vote totals by competitor.

------------------------------------------------------------------------

### Method `round_seat_totals()`

Return integer seat totals for a given round.

#### Usage

    CompetitionTrace$round_seat_totals(round)

#### Arguments

- `round`:

  1-based round index.

#### Returns

Named integer vector of seat totals by competitor.

------------------------------------------------------------------------

### Method `final_vote_shares()`

Return final vote shares.

#### Usage

    CompetitionTrace$final_vote_shares()

#### Returns

Named numeric vector of vote shares by competitor.

------------------------------------------------------------------------

### Method `final_seat_shares()`

Return final seat shares.

#### Usage

    CompetitionTrace$final_seat_shares()

#### Returns

Named numeric vector of seat shares by competitor.

------------------------------------------------------------------------

### Method `clone()`

The objects of this class are cloneable with this method.

#### Usage

    CompetitionTrace$clone(deep = FALSE)

#### Arguments

- `deep`:

  Whether to make a deep clone.
