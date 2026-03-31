# compound_api_demo.R — Exercise compound C API entry points (R bindings)
#
# Demonstrates:
#   1. r_scs_ic_interval_1d              → ic_interval_1d()
#   2. r_scs_winset_2d_export_boundary   → .Call (no high-level wrapper)
#   3. r_scs_uncovered_set_boundary_2d   → uncovered_set_boundary_2d()
#      (R still uses size+fill in geometry.c; Python uses heap — same numerics.)
#   4. r_scs_voronoi_cells_2d            → .Call (implementation uses heap)
#
# From repository root:
#   Rscript r/compound_api_demo.R
#
# Interactive HTML (tabs + sliders): r/compound_api_interactive_demo.R
#
# Requires SCS_LIB_PATH (or DYLD_LIBRARY_PATH on macOS) pointing at build/;
# r/.Renviron can set this when working inside r/.

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

cat("compound_api_demo — R bindings for composite C API paths\n")

# --- 1. ic_interval_1d -----------------------------------------------------
cat("\n--- 1. ic_interval_1d (compound 1D IC) ---\n")
lc <- make_loss_config(loss_type = "linear", sensitivity = 1)
dc <- make_dist_config(n_dims = 1L, weights = 1)
ideal <- 0.5
ref   <- 1.2
p1 <- ic_interval_1d(ideal, ref, lc, dc)
d  <- calculate_distance(ideal, ref, dc)
ul <- distance_to_utility(d, lc)
p3 <- level_set_1d(ideal, weight = 1, utility_level = ul, loss_config = lc)
stopifnot(isTRUE(all.equal(as.numeric(p1), as.numeric(p3), tolerance = 1e-12)))
cat("  ideal=", ideal, ", ref=", ref, "  →  ", length(p1), " point(s)\n", sep = "")

# --- 2. winset export boundary (no Winset EXTPTR) ---------------------------
cat("\n--- 2. r_scs_winset_2d_export_boundary (no Winset handle) ---\n")
voters <- c(0, 0, 2, 0, 1, 2)
sq_x <- 1.0
sq_y <- 2 / 3
k <- -1L
num_samples <- 64L
dc2 <- make_dist_config(weights = c(1, 1))
bnd <- .Call(
  "r_scs_winset_2d_export_boundary",
  sq_x, sq_y, as.double(voters),
  k, num_samples, dc2,
  PACKAGE = "socialchoicelab"
)
stopifnot(isFALSE(bnd$empty))
n_paths <- length(bnd$paths)
xy_pairs <- sum(vapply(bnd$paths, length, integer(1))) / 2
cat("  status_quo=(", sq_x, ", ", sq_y, "), n_voters=", length(voters) / 2, "\n", sep = "")
cat("  boundary: ~", xy_pairs, " (x,y) pairs, ", n_paths, " path(s)\n", sep = "")

# --- 3. uncovered set boundary ---------------------------------------------
cat("\n--- 3. uncovered_set_boundary_2d ---\n")
vtr <- c(-1, 0, 0, 1, 1, 0, 0, -1)
bnd_u <- uncovered_set_boundary_2d(vtr, grid_resolution = 8L, k = "simple")
cat(
  "  n boundary vertices: ", nrow(bnd_u),
  "  (dim ", paste(dim(bnd_u), collapse = "×"), ")\n",
  sep = ""
)

# --- 4. Voronoi (heap inside C binding) ------------------------------------
cat("\n--- 4. r_scs_voronoi_cells_2d (heap-backed implementation) ---\n")
sites_xy <- c(0, 0, 1, 0, 0.5, 1)
n_sites <- 3L
bbox <- c(-0.5, -0.5, 1.5, 1.5)
cells <- .Call(
  "r_scs_voronoi_cells_2d",
  as.double(sites_xy), n_sites, as.double(bbox),
  PACKAGE = "socialchoicelab"
)
stopifnot(length(cells) == n_sites)
non_empty <- sum(vapply(cells, function(z) !is.null(z), logical(1)))
cat("  n_sites=", n_sites, ", bbox=(", paste(bbox, collapse = ", "), ")\n", sep = "")
cat("  non-empty cells:", non_empty, "/", n_sites, "\n", sep = "")

cat("\nDone.\n")
