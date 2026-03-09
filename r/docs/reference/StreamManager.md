# StreamManager: reproducible pseudo-random number streams

StreamManager: reproducible pseudo-random number streams

StreamManager: reproducible pseudo-random number streams

## Details

Wraps the `SCS_StreamManager` C handle. All PRNG calls go through named
streams derived from a single master seed, enabling full
reproducibility. Register stream names before drawing from them.

## Methods

### Public methods

- [`StreamManager$new()`](#method-StreamManager-new)

- [`StreamManager$register()`](#method-StreamManager-register)

- [`StreamManager$reset_all()`](#method-StreamManager-reset_all)

- [`StreamManager$reset_stream()`](#method-StreamManager-reset_stream)

- [`StreamManager$skip()`](#method-StreamManager-skip)

- [`StreamManager$uniform_real()`](#method-StreamManager-uniform_real)

- [`StreamManager$normal()`](#method-StreamManager-normal)

- [`StreamManager$bernoulli()`](#method-StreamManager-bernoulli)

- [`StreamManager$uniform_int()`](#method-StreamManager-uniform_int)

- [`StreamManager$uniform_choice()`](#method-StreamManager-uniform_choice)

- [`StreamManager$clone()`](#method-StreamManager-clone)

------------------------------------------------------------------------

### Method `new()`

Create a new StreamManager.

#### Usage

    StreamManager$new(master_seed, stream_names = NULL)

#### Arguments

- `master_seed`:

  Integer or double. Master seed (values up to 2^53 are represented
  exactly).

- `stream_names`:

  Optional character vector. Stream names to register immediately after
  creation.

------------------------------------------------------------------------

### Method `register()`

Register allowed stream names. Only registered names may be used with
PRNG draw functions.

#### Usage

    StreamManager$register(stream_names)

#### Arguments

- `stream_names`:

  Character vector of stream names.

#### Returns

`self` invisibly.

------------------------------------------------------------------------

### Method `reset_all()`

Reset all streams with a new master seed.

#### Usage

    StreamManager$reset_all(master_seed)

#### Arguments

- `master_seed`:

  New master seed.

#### Returns

`self` invisibly.

------------------------------------------------------------------------

### Method `reset_stream()`

Reset a single named stream to a new seed.

#### Usage

    StreamManager$reset_stream(stream_name, seed)

#### Arguments

- `stream_name`:

  Character. The stream to reset.

- `seed`:

  New seed for this stream.

#### Returns

`self` invisibly.

------------------------------------------------------------------------

### Method `skip()`

Skip (discard) `n` values in a named stream.

#### Usage

    StreamManager$skip(stream_name, n)

#### Arguments

- `stream_name`:

  Character.

- `n`:

  Number of values to skip.

#### Returns

`self` invisibly.

------------------------------------------------------------------------

### Method `uniform_real()`

Draw a uniform real in `[min, max)`.

#### Usage

    StreamManager$uniform_real(stream_name, min = 0, max = 1)

#### Arguments

- `stream_name`:

  Character.

- `min, max`:

  Range boundaries.

#### Returns

Scalar double.

------------------------------------------------------------------------

### Method `normal()`

Draw a normal variate N(mean, sd^2).

#### Usage

    StreamManager$normal(stream_name, mean = 0, sd = 1)

#### Arguments

- `stream_name`:

  Character.

- `mean, sd`:

  Distribution parameters.

#### Returns

Scalar double.

------------------------------------------------------------------------

### Method `bernoulli()`

Draw a Bernoulli variate.

#### Usage

    StreamManager$bernoulli(stream_name, prob)

#### Arguments

- `stream_name`:

  Character.

- `prob`:

  Probability of `TRUE`.

#### Returns

Logical scalar.

------------------------------------------------------------------------

### Method `uniform_int()`

Draw a uniform integer in `[min, max]`.

#### Usage

    StreamManager$uniform_int(stream_name, min, max)

#### Arguments

- `stream_name`:

  Character.

- `min, max`:

  Integer range (inclusive).

#### Returns

Scalar double (to accommodate values beyond R's 32-bit integer).

------------------------------------------------------------------------

### Method `uniform_choice()`

Draw a uniform choice index in `[0, n)` (0-based).

#### Usage

    StreamManager$uniform_choice(stream_name, n)

#### Arguments

- `stream_name`:

  Character.

- `n`:

  Number of choices.

#### Returns

Integer scalar (0-based index).

------------------------------------------------------------------------

### Method `clone()`

The objects of this class are cloneable with this method.

#### Usage

    StreamManager$clone(deep = FALSE)

#### Arguments

- `deep`:

  Whether to make a deep clone.

## Examples

``` r
if (FALSE) { # \dontrun{
  sm <- StreamManager$new(master_seed = 42, stream_names = "sim")
  sm$uniform_real("sim", 0, 1)
  sm$reset_all(42)
} # }
```
