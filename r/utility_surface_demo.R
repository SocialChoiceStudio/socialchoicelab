# utility_surface_demo.R — Interactive 3D utility surface (plot_utility_surface)
#
# Shows how the utility tent changes across different distance metrics and loss
# functions.  Each call saves a self-contained HTML file and opens it in the
# browser via browseURL().
#
# From repository root:
#   Rscript r/utility_surface_demo.R
#
# Requires SCS_LIB_PATH (or DYLD_LIBRARY_PATH on macOS) pointing at build/.

root <- getwd()
r_pkg <- file.path(root, "r", "DESCRIPTION")
here_pkg <- file.path(root, "DESCRIPTION")
if (file.exists(r_pkg)) {
  devtools::load_all(file.path(root, "r"), quiet = TRUE)
} else if (file.exists(here_pkg)) {
  devtools::load_all(root, quiet = TRUE)
} else {
  stop("Run from repo root (so r/DESCRIPTION exists) or from r/ with DESCRIPTION in getwd().")
}

outdir <- normalizePath(tempdir(), mustWork = TRUE)
cat("utility_surface_demo — writing HTML files to", outdir, "\n")

# ---------------------------------------------------------------------------
# 1. Euclidean distance + quadratic loss — circular tent (textbook Fig. I-12)
# ---------------------------------------------------------------------------
cat("\n--- 1. Euclidean + quadratic (circular tent) ---\n")
w1 <- plot_utility_surface(
  ideal_x     = 1.0,
  ideal_y     = 0.5,
  dist_config = make_dist_config("euclidean"),
  loss_config = make_loss_config("quadratic"),
  xlim        = c(-3, 3),
  ylim        = c(-3, 3),
  resolution  = 60L,
  n_levels    = 5L,
  axis_labels = c("Guns", "Butter"),
  title       = "Voter C — Euclidean / quadratic"
)
p1 <- file.path(outdir, "utility_surface_euclidean_quadratic.html")
htmlwidgets::saveWidget(w1, p1, selfcontained = TRUE)
cat("  saved:", p1, "\n")
browseURL(p1)

# ---------------------------------------------------------------------------
# 2. Manhattan distance + linear loss — diamond-shaped tent
# ---------------------------------------------------------------------------
cat("\n--- 2. Manhattan + linear (diamond tent) ---\n")
w2 <- plot_utility_surface(
  ideal_x     = 0.0,
  ideal_y     = 0.0,
  dist_config = make_dist_config("manhattan"),
  loss_config = make_loss_config("linear"),
  xlim        = c(-3, 3),
  ylim        = c(-3, 3),
  resolution  = 60L,
  n_levels    = 5L,
  axis_labels = c("Dimension 1", "Dimension 2"),
  title       = "Voter — Manhattan / linear"
)
p2 <- file.path(outdir, "utility_surface_manhattan_linear.html")
htmlwidgets::saveWidget(w2, p2, selfcontained = TRUE)
cat("  saved:", p2, "\n")
browseURL(p2)

# ---------------------------------------------------------------------------
# 3. Euclidean + Gaussian loss — soft bell curve
# ---------------------------------------------------------------------------
cat("\n--- 3. Euclidean + Gaussian (bell curve) ---\n")
w3 <- plot_utility_surface(
  ideal_x     = -0.5,
  ideal_y     = 0.8,
  dist_config = make_dist_config("euclidean"),
  loss_config = make_loss_config("gaussian", steepness = 1.5),
  xlim        = c(-3, 3),
  ylim        = c(-3, 3),
  resolution  = 60L,
  n_levels    = 5L,
  axis_labels = c("Dimension 1", "Dimension 2"),
  title       = "Voter — Euclidean / Gaussian loss"
)
p3 <- file.path(outdir, "utility_surface_euclidean_gaussian.html")
htmlwidgets::saveWidget(w3, p3, selfcontained = TRUE)
cat("  saved:", p3, "\n")
browseURL(p3)

cat("\nDone. Drag each plot to rotate.\n")

# Keep the session alive long enough for the browser to open all three files
# before R cleans up its temp directory on exit.
Sys.sleep(5)
