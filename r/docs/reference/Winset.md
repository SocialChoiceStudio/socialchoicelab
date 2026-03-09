# Winset: a 2D majority winset region

Winset: a 2D majority winset region

Winset: a 2D majority winset region

## Details

Wraps the `SCS_Winset` C handle. Create winset objects via the factory
functions
([`winset_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/winset_2d.md),
[`weighted_winset_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/weighted_winset_2d.md),
[`winset_with_veto_2d`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/winset_with_veto_2d.md))
rather than `Winset$new()` directly.

Boolean operations (`$union()`, `$intersection()`, `$difference()`,
`$symmetric_difference()`) return new `Winset` objects; the inputs are
never modified.

## Methods

### Public methods

- [`Winset$new()`](#method-Winset-new)

- [`Winset$is_empty()`](#method-Winset-is_empty)

- [`Winset$contains()`](#method-Winset-contains)

- [`Winset$bbox()`](#method-Winset-bbox)

- [`Winset$boundary()`](#method-Winset-boundary)

- [`Winset$clone_winset()`](#method-Winset-clone_winset)

- [`Winset$union()`](#method-Winset-union)

- [`Winset$intersection()`](#method-Winset-intersection)

- [`Winset$difference()`](#method-Winset-difference)

- [`Winset$symmetric_difference()`](#method-Winset-symmetric_difference)

- [`Winset$clone()`](#method-Winset-clone)

------------------------------------------------------------------------

### Method `new()`

Internal. Use factory functions instead.

#### Usage

    Winset$new(ptr)

#### Arguments

- `ptr`:

  External pointer returned by a C factory function.

------------------------------------------------------------------------

### Method `is_empty()`

Test whether the winset is empty (no policy beats the SQ).

#### Usage

    Winset$is_empty()

#### Returns

Logical scalar.

------------------------------------------------------------------------

### Method `contains()`

Test whether a point lies strictly inside the winset.

#### Usage

    Winset$contains(x, y)

#### Arguments

- `x, y`:

  Point coordinates.

#### Returns

Logical scalar.

------------------------------------------------------------------------

### Method `bbox()`

Axis-aligned bounding box of the winset.

#### Usage

    Winset$bbox()

#### Returns

Named list `list(min_x, min_y, max_x, max_y)`, or `NULL` if the winset
is empty.

------------------------------------------------------------------------

### Method `boundary()`

Export the boundary as a matrix of sampled vertices.

#### Usage

    Winset$boundary()

#### Returns

Named list with elements:

- xy:

  Numeric matrix (n_vertices × 2), columns `x` and `y`.

- path_starts:

  Integer vector of 1-based row indices where each boundary path starts.

- is_hole:

  Logical vector; `TRUE` if the path is a hole.

------------------------------------------------------------------------

### Method `clone_winset()`

Deep copy of this winset.

#### Usage

    Winset$clone_winset()

#### Returns

A new `Winset` object.

------------------------------------------------------------------------

### Method [`union()`](https://rdrr.io/r/base/sets.html)

Union with another winset (this ∪ other).

#### Usage

    Winset$union(other)

#### Arguments

- `other`:

  A `Winset` object.

#### Returns

A new `Winset`.

------------------------------------------------------------------------

### Method `intersection()`

Intersection with another winset (this ∩ other).

#### Usage

    Winset$intersection(other)

#### Arguments

- `other`:

  A `Winset` object.

#### Returns

A new `Winset`.

------------------------------------------------------------------------

### Method `difference()`

Set difference (this \\ other).

#### Usage

    Winset$difference(other)

#### Arguments

- `other`:

  A `Winset` object.

#### Returns

A new `Winset`.

------------------------------------------------------------------------

### Method `symmetric_difference()`

Symmetric difference (this △ other).

#### Usage

    Winset$symmetric_difference(other)

#### Arguments

- `other`:

  A `Winset` object.

#### Returns

A new `Winset`.

------------------------------------------------------------------------

### Method `clone()`

The objects of this class are cloneable with this method.

#### Usage

    Winset$clone(deep = FALSE)

#### Arguments

- `deep`:

  Whether to make a deep clone.

## Examples

``` r
if (FALSE) { # \dontrun{
# 3 voters; status quo at origin.
voters <- c(-1, -1, 1, -1, 1, 1)
ws <- winset_2d(c(0, 0), voters)
ws$is_empty()
ws$bbox()
bnd <- ws$boundary()
plot(bnd$xy, type = "l", asp = 1)
} # }
```
