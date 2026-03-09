# Return n RGBA colour strings from a named palette

A convenience utility for retrieving coordinated colours from the same
palettes used by the `layer_*` plotting functions. Useful when adding
custom Plotly traces that should remain visually consistent with the
rest of the plot.

## Usage

``` r
scl_palette(name = "auto", n = 8L, alpha = 1)
```

## Arguments

- name:

  Palette name: `"dark2"` (ColorBrewer Dark2, 8 colours,
  colorblind-safe), `"set2"` (ColorBrewer Set2, 8 colours, softer),
  `"okabe_ito"` (Okabe-Ito, 7 colours, print-safe), `"paired"`
  (ColorBrewer Paired, 12 colours), or `"auto"` (selects the best
  palette for `n`: Okabe-Ito for n ≤ 7, Dark2 for n ≤ 8, Paired for n \>
  8).

- n:

  Number of colours to return. If `n` exceeds the palette size, colours
  are cycled.

- alpha:

  Opacity applied uniformly to all returned colours (0–1).

## Value

Character vector of `n` RGBA strings.

## Examples

``` r
scl_palette("dark2", n = 4L)
#> [1] "rgba(27,158,119,1)"  "rgba(217,95,2,1)"    "rgba(117,112,179,1)"
#> [4] "rgba(231,41,138,1)" 
scl_palette("auto",  n = 5L, alpha = 0.7)
#> [1] "rgba(0,114,178,0.7)"  "rgba(213,94,0,0.7)"   "rgba(0,158,115,0.7)" 
#> [4] "rgba(230,159,0,0.7)"  "rgba(86,180,233,0.7)"
```
