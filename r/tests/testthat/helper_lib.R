# helper_lib.R — Shared test helpers loaded by testthat before any test files.

# Whether the C library linked correctly in this R CMD check environment.
# If scs_version() throws, libscs_api couldn't be called at runtime.
.lib_available <- tryCatch({ scs_version(); TRUE }, error = function(e) FALSE)

# Call at the start of any test_that block that needs the C library.
# In plain devtools::test() or local runs where libscs_api is on the path,
# .lib_available is TRUE and the skip is a no-op.
skip_without_lib <- function() {
  testthat::skip_if(!.lib_available, "libscs_api not available at runtime")
}
