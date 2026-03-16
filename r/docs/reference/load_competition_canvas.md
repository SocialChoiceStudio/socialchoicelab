# Load a competition canvas from a .scscanvas file

Reads a file written by
[`save_competition_canvas`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/save_competition_canvas.md)
(or its Python equivalent) and returns an `htmlwidget` that can be
displayed in the RStudio Viewer, Shiny, or R Markdown, or saved to a
standalone HTML file with
[`htmlwidgets::saveWidget()`](https://rdrr.io/pkg/htmlwidgets/man/saveWidget.html).

## Usage

``` r
load_competition_canvas(path, width = NULL, height = NULL)
```

## Arguments

- path:

  Path to a `.scscanvas` JSON file.

- width, height:

  Widget dimensions in pixels. `NULL` uses the values stored in the file
  (or package defaults when those are also `NULL`).

## Value

An `htmlwidget` of class `competition_canvas`.
