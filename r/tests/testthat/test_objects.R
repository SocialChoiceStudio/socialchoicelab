# test_objects.R — Lifecycle, method, and index-translation tests for
#                  StreamManager, Winset, and Profile R6 classes.

# Pure-R data — no C calls at file-load time.
.voters <- c(-1, -1,  1, -1,  1,  1)
.alts   <- c( 0,  0,  2,  0, -2,  0)

# ---------------------------------------------------------------------------
# StreamManager
# ---------------------------------------------------------------------------

test_that("StreamManager creates without error", {
  skip_without_lib()
  sm <- StreamManager$new(master_seed = 42)
  expect_s3_class(sm, "StreamManager")
})

test_that("StreamManager draws are reproducible given the same seed", {
  skip_without_lib()
  sm1 <- StreamManager$new(42, "s")
  sm2 <- StreamManager$new(42, "s")
  expect_equal(sm1$uniform_real("s"), sm2$uniform_real("s"))
})

test_that("StreamManager uniform_real is in [min, max)", {
  skip_without_lib()
  sm  <- StreamManager$new(1, "s")
  val <- sm$uniform_real("s", 5.0, 10.0)
  expect_gte(val, 5.0)
  expect_lt(val,  10.0)
})

test_that("StreamManager bernoulli returns logical", {
  skip_without_lib()
  sm  <- StreamManager$new(1, "s")
  val <- sm$bernoulli("s", 0.5)
  expect_type(val, "logical")
})

test_that("StreamManager reset_all restores draws", {
  skip_without_lib()
  sm <- StreamManager$new(7, "s")
  v1 <- sm$uniform_real("s")
  sm$reset_all(7)
  v2 <- sm$uniform_real("s")
  expect_equal(v1, v2)
})

test_that("stream_manager() factory produces a StreamManager", {
  skip_without_lib()
  sm <- stream_manager(99)
  expect_s3_class(sm, "StreamManager")
})

test_that("stream PRNG functions error on wrong object type", {
  skip_without_lib()
  prof <- profile_from_utility_matrix(c(1, 2), n_voters = 1L, n_alts = 2L)
  expect_error(
    plurality_one_winner(prof, tie_break = "random",
                         stream_manager = list(), stream_name = "s"),
    "Expected a StreamManager"
  )
})

# ---------------------------------------------------------------------------
# Profile — lifecycle and index translation
# ---------------------------------------------------------------------------

test_that("profile_build_spatial returns a Profile", {
  skip_without_lib()
  prof <- profile_build_spatial(.alts, .voters)
  expect_s3_class(prof, "Profile")
})

test_that("Profile$dims returns correct n_voters and n_alts", {
  skip_without_lib()
  prof <- profile_build_spatial(.alts, .voters)
  d    <- prof$dims()
  expect_identical(d$n_voters, 3L)
  expect_identical(d$n_alts,   3L)
})

test_that("Profile$get_ranking returns a 1-based permutation", {
  skip_without_lib()
  prof    <- profile_build_spatial(.alts, .voters)
  ranking <- prof$get_ranking(1L)
  expect_type(ranking, "integer")
  expect_length(ranking, 3L)
  expect_identical(sort(ranking), 1:3)
})

test_that("Profile$get_ranking errors on out-of-range voter index", {
  skip_without_lib()
  prof <- profile_build_spatial(.alts, .voters)
  expect_error(prof$get_ranking(0L), "out of range")
  expect_error(prof$get_ranking(4L), "out of range")
})

test_that("Profile$export returns integer matrix with correct dimnames", {
  skip_without_lib()
  prof <- profile_build_spatial(.alts, .voters)
  mat  <- prof$export()
  expect_true(is.matrix(mat))
  expect_identical(dim(mat), c(3L, 3L))
  expect_identical(rownames(mat)[1], "voter1")
  expect_identical(colnames(mat)[1], "rank1")
  expect_true(all(mat %in% 1:3))
})

test_that("Profile$clone_profile creates an independent copy", {
  skip_without_lib()
  prof  <- profile_build_spatial(.alts, .voters)
  clone <- prof$clone_profile()
  expect_s3_class(clone, "Profile")
  expect_equal(prof$export(), clone$export())
  expect_false(identical(prof, clone))
})

test_that("profile_from_utility_matrix produces correct rankings", {
  skip_without_lib()
  # Voter 1: alt1 > alt2 > alt3; Voter 2: alt3 > alt2 > alt1.
  u    <- c(3, 2, 1,  1, 2, 3)
  prof <- profile_from_utility_matrix(u, n_voters = 2L, n_alts = 3L)
  r1   <- prof$get_ranking(1L)
  r2   <- prof$get_ranking(2L)
  expect_identical(r1[1], 1L)
  expect_identical(r2[1], 3L)
})

test_that("profile_impartial_culture returns Profile with correct dims", {
  skip_without_lib()
  sm   <- StreamManager$new(42, "ic")
  prof <- profile_impartial_culture(10L, 3L, sm, "ic")
  d    <- prof$dims()
  expect_identical(d$n_voters, 10L)
  expect_identical(d$n_alts,   3L)
})

test_that("profile functions error when a non-Profile is passed", {
  skip_without_lib()
  expect_error(plurality_scores(list()), "Expected a Profile")
})

# ---------------------------------------------------------------------------
# Winset — lifecycle and methods
# ---------------------------------------------------------------------------

test_that("winset_2d returns a Winset", {
  skip_without_lib()
  ws <- winset_2d(c(0, 0), .voters)
  expect_s3_class(ws, "Winset")
})

test_that("Winset$is_empty returns logical", {
  skip_without_lib()
  ws <- winset_2d(c(0, 0), .voters)
  expect_type(ws$is_empty(), "logical")
})

test_that("Winset$contains returns logical", {
  skip_without_lib()
  ws <- winset_2d(c(0.1, 0.1), .voters)
  expect_type(ws$contains(0.5, 0.5), "logical")
})

test_that("Winset$bbox returns NULL or a list with 4 named elements", {
  skip_without_lib()
  ws_empty <- winset_2d(c(10, 10), .voters)
  bb_empty <- ws_empty$bbox()
  expect_true(is.null(bb_empty) || is.list(bb_empty))

  ws <- winset_2d(c(0.1, 0.1), .voters)
  if (!ws$is_empty()) {
    bb <- ws$bbox()
    expect_true(is.list(bb))
    expect_true(all(c("min_x", "min_y", "max_x", "max_y") %in% names(bb)))
    expect_gte(bb$max_x, bb$min_x)
    expect_gte(bb$max_y, bb$min_y)
  }
})

test_that("Winset$boundary returns a list with xy matrix", {
  skip_without_lib()
  ws  <- winset_2d(c(0.1, 0.1), .voters)
  bnd <- ws$boundary()
  if (!ws$is_empty()) {
    expect_type(bnd, "list")
    expect_true(is.matrix(bnd$xy))
    expect_identical(ncol(bnd$xy), 2L)
    expect_type(bnd$path_starts, "integer")
    expect_type(bnd$is_hole, "logical")
  }
})

test_that("Winset$clone_winset produces an independent copy", {
  skip_without_lib()
  ws    <- winset_2d(c(0.1, 0.1), .voters)
  clone <- ws$clone_winset()
  expect_s3_class(clone, "Winset")
  expect_identical(ws$is_empty(), clone$is_empty())
})

test_that("Winset boolean ops return new Winset objects", {
  skip_without_lib()
  ws1 <- winset_2d(c(-0.5, 0), .voters)
  ws2 <- winset_2d(c( 0.5, 0), .voters)
  u   <- ws1$union(ws2)
  i   <- ws1$intersection(ws2)
  d   <- ws1$difference(ws2)
  sd  <- ws1$symmetric_difference(ws2)
  for (ws in list(u, i, d, sd)) {
    expect_s3_class(ws, "Winset")
  }
})

test_that("Winset boolean ops error when a non-Winset is passed as other", {
  skip_without_lib()
  ws <- winset_2d(c(0.1, 0.1), .voters)
  expect_error(ws$union(list()), "Expected a Winset")
})
