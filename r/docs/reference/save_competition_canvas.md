# Save a competition canvas widget to a .scscanvas file

Serialises the pre-computed animation payload (positions, ICs, WinSet,
Cutlines, etc.) to a JSON file with metadata. The file can be reloaded
later with
[`load_competition_canvas`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/load_competition_canvas.md)
without recomputing anything. R and Python packages can read each
other's files.

## Usage

``` r
save_competition_canvas(widget, path)
```

## Arguments

- widget:

  An `htmlwidget` returned by
  [`animate_competition_canvas`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/animate_competition_canvas.md).

- path:

  Path to the output file. By convention use the `.scscanvas` extension.

## Value

`path`, invisibly.
