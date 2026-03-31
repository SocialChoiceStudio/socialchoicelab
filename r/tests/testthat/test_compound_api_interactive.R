# test_compound_api_interactive.R — Payload + validation for interactive HTML demos

.com_int_env <- new.env(parent = asNamespace("socialchoicelab"))
sys.source(testthat::test_path("compound_api_interactive_common.R"), envir = .com_int_env)

test_that("validate_interactive_payload flags empty payload", {
  vc <- .com_int_env$validate_interactive_payload(list())
  expect_false(vc$all_ok)
  expect_true(any(!vapply(vc$checks, function(c) isTRUE(c$ok), logical(1L))))
})

test_that("format_interactive_validation_report includes summary line", {
  vc <- .com_int_env$validate_interactive_payload(list())
  txt <- .com_int_env$format_interactive_validation_report(vc)
  expect_true(length(txt) >= 2L)
  expect_match(paste(txt, collapse = "\n"), "SUMMARY")
})

test_that("build_interactive_payload passes all checks when lib available", {
  skip_without_lib()
  payload <- .com_int_env$build_interactive_payload()
  expect_identical(payload$meta$binding, "r")
  expect_true(isTRUE(payload$meta$build_all_ok))
  stripped <- payload[names(payload) != "meta"]
  vc2 <- .com_int_env$validate_interactive_payload(stripped)
  expect_true(vc2$all_ok)
  expect_identical(length(vc2$checks), length(payload$meta$build_checks))
})
