# Create a StreamManager (convenience factory)

Equivalent to `StreamManager$new(master_seed, stream_names)`.

## Usage

``` r
stream_manager(master_seed, stream_names = NULL)
```

## Arguments

- master_seed:

  Integer or double. Master seed.

- stream_names:

  Optional character vector of stream names to register immediately.

## Value

A
[`StreamManager`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/StreamManager.md)
object.

## Examples

``` r
if (FALSE) { # \dontrun{
sm <- stream_manager(42, c("voters", "ties"))
sm$uniform_real("voters", 0, 1)
} # }
```
