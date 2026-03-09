# Profile: an ordinal preference profile

Profile: an ordinal preference profile

Profile: an ordinal preference profile

## Details

Wraps the `SCS_Profile` C handle. Create profiles via the factory
functions
([`profile_build_spatial`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/profile_build_spatial.md),
[`profile_from_utility_matrix`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/profile_from_utility_matrix.md),
etc.) rather than `Profile$new()` directly.

## Methods

### Public methods

- [`Profile$new()`](#method-Profile-new)

- [`Profile$dims()`](#method-Profile-dims)

- [`Profile$get_ranking()`](#method-Profile-get_ranking)

- [`Profile$export()`](#method-Profile-export)

- [`Profile$clone_profile()`](#method-Profile-clone_profile)

- [`Profile$clone()`](#method-Profile-clone)

------------------------------------------------------------------------

### Method `new()`

Internal. Use factory functions instead.

#### Usage

    Profile$new(ptr)

#### Arguments

- `ptr`:

  External pointer returned by a C factory function.

------------------------------------------------------------------------

### Method `dims()`

Query profile dimensions.

#### Usage

    Profile$dims()

#### Returns

Named list `list(n_voters, n_alts)`.

------------------------------------------------------------------------

### Method `get_ranking()`

Get one voter's ranking as a 1-based integer vector.

#### Usage

    Profile$get_ranking(voter)

#### Arguments

- `voter`:

  1-based voter index.

#### Returns

Integer vector of length n_alts. `result[r]` is the 1-based alternative
index at rank `r` (rank 1 = most preferred).

------------------------------------------------------------------------

### Method `export()`

Export all rankings as an integer matrix.

#### Usage

    Profile$export()

#### Returns

Integer matrix (n_voters × n_alts). Entry `[v, r]` is the 1-based
alternative index at rank `r` for voter `v`. Row names are
`"voter1", "voter2", ...`; column names are `"rank1", "rank2", ...`.

------------------------------------------------------------------------

### Method `clone_profile()`

Deep copy of this profile.

#### Usage

    Profile$clone_profile()

#### Returns

A new `Profile` object.

------------------------------------------------------------------------

### Method `clone()`

The objects of this class are cloneable with this method.

#### Usage

    Profile$clone(deep = FALSE)

#### Arguments

- `deep`:

  Whether to make a deep clone.

## Examples

``` r
if (FALSE) { # \dontrun{
alts   <- c(0, 0, 2, 0, -2, 0)   # 3 alternatives in 2D
voters <- c(-1, -1, 1, -1, 1, 1) # 3 voters
prof   <- profile_build_spatial(alts, voters)
prof$dims()        # list(n_voters = 3, n_alts = 3)
prof$get_ranking(1L)  # ranking of voter 1 (1-based)
prof$export()      # full 3×3 ranking matrix
} # }
```
