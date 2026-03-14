# canvas_load_demo.R — Reload pre-computed competition canvases from .scscanvas files.
#
# Companion to canvas_save_demo.R. Reads the .scscanvas files saved by that
# script (written to the project root) and displays the canvases without
# recomputing any geometry (ICs, WinSet, Cutlines, etc.).
#
# Run canvas_save_demo.R first to generate the input files, then source this.
#
# HOW TO RUN IN RSTUDIO:
#   Session > Set Working Directory > To Source File Location
#   (picks up r/.Renviron which sets SCS_LIB_PATH / DYLD_LIBRARY_PATH)
#   Then source or step through with Cmd+Enter.
devtools::load_all("r/")

library(socialchoicelab)
library(htmlwidgets)

# Paths match those written by canvas_save_demo.R (project root).
PATH_1D <- "canvas_1d_demo.scscanvas"
PATH_2D <- "canvas_2d_demo.scscanvas"

# ===========================================================================
# Load and display the 1D canvas
# ===========================================================================
cat("Loading 1D canvas from:", PATH_1D, "\n")
w_1d <- load_competition_canvas(PATH_1D)

# Opens in RStudio Viewer — no computation, just layout and rendering.
w_1d

# Optionally save to a standalone HTML file and open in the browser:
# htmlwidgets::saveWidget(w_1d, "/tmp/canvas_1d_loaded.html", selfcontained = TRUE)
# browseURL("/tmp/canvas_1d_loaded.html")

# ===========================================================================
# Load and display the 2D canvas
# ===========================================================================
cat("Loading 2D canvas from:", PATH_2D, "\n")
w_2d <- load_competition_canvas(PATH_2D)

# Opens in RStudio Viewer
w_2d

# Optionally save to a standalone HTML file and open in the browser:
# htmlwidgets::saveWidget(w_2d, "/tmp/canvas_2d_loaded.html", selfcontained = TRUE)
# browseURL("/tmp/canvas_2d_loaded.html")

cat("Done. No recomputation was needed.\n")
