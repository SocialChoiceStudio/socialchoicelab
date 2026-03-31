# compound_api_interactive_demo.R — Interactive HTML for compound C API paths
#
# Same UI as python/compound_api_interactive_demo.py (shell:
# python/compound_api_interactive_shell.html). Frames are computed in R via
# ic_interval_1d, r_scs_winset_2d_export_boundary, uncovered_set_boundary_2d,
# and r_scs_voronoi_cells_2d (heap-backed in C).
#
# Shared build/validation: r/tests/testthat/compound_api_interactive_common.R
#
# From repository root:
#   SCS_LIB_PATH=$(pwd)/build DYLD_LIBRARY_PATH=$(pwd)/build Rscript r/compound_api_interactive_demo.R
#
# Optional: COMPOUND_INTERACTIVE_STRICT=1 exits non-zero if data checks fail.
#
# Writes tmp/compound_api_interactive_r.html and opens it in the browser
# (set OPEN_BROWSER <- FALSE to skip).

OPEN_BROWSER <- TRUE
OUT_NAME <- "compound_api_interactive_r.html"

root <- getwd()
r_pkg <- file.path(root, "r", "DESCRIPTION")
here_pkg <- file.path(root, "DESCRIPTION")
if (file.exists(r_pkg)) {
  repo_root <- root
  pkg_root <- file.path(root, "r")
  devtools::load_all(pkg_root, quiet = TRUE)
} else if (file.exists(here_pkg)) {
  pkg_root <- root
  repo_root <- dirname(root)
  devtools::load_all(pkg_root, quiet = TRUE)
} else {
  stop("Run from repo root (r/DESCRIPTION) or from r/.")
}

common <- file.path(pkg_root, "tests", "testthat", "compound_api_interactive_common.R")
if (!file.exists(common)) {
  stop("Missing common helpers: ", common)
}
source(common, local = FALSE)

shell <- file.path(repo_root, "python", "compound_api_interactive_shell.html")
if (!file.exists(shell)) {
  stop("Missing shell template: ", shell)
}

payload <- build_interactive_payload()
vc <- list(checks = payload$meta$build_checks, all_ok = payload$meta$build_all_ok)
report_lines <- format_interactive_validation_report(vc)
message(paste(report_lines, collapse = "\n"))
strict <- identical(Sys.getenv("COMPOUND_INTERACTIVE_STRICT", ""), "1")
if (isTRUE(strict) && !isTRUE(vc$all_ok)) {
  quit(status = 1L)
}

json <- jsonlite::toJSON(
  payload,
  auto_unbox = TRUE,
  null = "null",
  digits = 10
)
tpl <- paste(readLines(shell, encoding = "UTF-8", warn = FALSE), collapse = "\n")
parts <- strsplit(tpl, "__DATA__", fixed = TRUE)[[1L]]
if (length(parts) != 2L) {
  stop("Shell template must contain exactly one __DATA__ placeholder.")
}
html <- paste0(parts[[1L]], json, parts[[2L]])
html <- gsub("__SOURCE_TAG__", "R bindings", html, fixed = TRUE)

out_dir <- file.path(repo_root, "tmp")
if (!dir.exists(out_dir)) dir.create(out_dir, recursive = TRUE)
out_path <- file.path(out_dir, OUT_NAME)
cat(html, file = out_path, sep = "")
message("Wrote ", normalizePath(out_path, winslash = "/"))

if (isTRUE(OPEN_BROWSER)) {
  utils::browseURL(out_path)
}
