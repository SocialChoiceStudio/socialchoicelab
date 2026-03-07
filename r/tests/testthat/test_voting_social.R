# test_voting_social.R — Tests for voting rules and social rankings/properties.

# Pure-R data — no C calls at file-load time.
.voters <- c(-1, -1,  1, -1,  1,  1)
.alts   <- c( 0,  0,  2,  0, -2,  0)

# Internal helper: build a fresh profile each time (avoids top-level C call).
.prof <- function() profile_build_spatial(.alts, .voters)

# ---------------------------------------------------------------------------
# Plurality
# ---------------------------------------------------------------------------

test_that("plurality_scores sums to n_voters", {
  skip_without_lib()
  scores <- plurality_scores(.prof())
  expect_identical(sum(scores), 3L)
  expect_identical(names(scores), c("alt1", "alt2", "alt3"))
})

test_that("plurality_all_winners returns non-empty 1-based vector", {
  skip_without_lib()
  winners <- plurality_all_winners(.prof())
  expect_type(winners, "integer")
  expect_gte(length(winners), 1L)
  expect_true(all(winners >= 1L & winners <= 3L))
})

test_that("plurality_one_winner is in plurality_all_winners", {
  skip_without_lib()
  w     <- plurality_one_winner(.prof())
  all_w <- plurality_all_winners(.prof())
  expect_true(w %in% all_w)
})

test_that("plurality_one_winner with random tie-break requires stream args", {
  skip_without_lib()
  expect_error(
    plurality_one_winner(.prof(), tie_break = "random"),
    "requires both"
  )
})

# ---------------------------------------------------------------------------
# Borda
# ---------------------------------------------------------------------------

test_that("borda_scores are non-negative integers", {
  skip_without_lib()
  scores <- borda_scores(.prof())
  expect_type(scores, "integer")
  expect_true(all(scores >= 0L))
  expect_identical(names(scores), c("alt1", "alt2", "alt3"))
})

test_that("borda_ranking is a permutation of 1:n_alts", {
  skip_without_lib()
  ranking <- borda_ranking(.prof())
  expect_identical(sort(ranking), 1:3)
})

test_that("borda_one_winner is in borda_all_winners", {
  skip_without_lib()
  w     <- borda_one_winner(.prof())
  all_w <- borda_all_winners(.prof())
  expect_true(w %in% all_w)
})

# ---------------------------------------------------------------------------
# Anti-plurality
# ---------------------------------------------------------------------------

test_that("antiplurality_scores are non-negative", {
  skip_without_lib()
  scores <- antiplurality_scores(.prof())
  expect_type(scores, "integer")
  expect_true(all(scores >= 0L))
})

test_that("antiplurality_one_winner is in antiplurality_all_winners", {
  skip_without_lib()
  w     <- antiplurality_one_winner(.prof())
  all_w <- antiplurality_all_winners(.prof())
  expect_true(w %in% all_w)
})

# ---------------------------------------------------------------------------
# Scoring rule (Borda weights equivalence)
# ---------------------------------------------------------------------------

test_that("scoring_rule_scores with Borda weights matches borda_scores", {
  skip_without_lib()
  sw    <- c(2.0, 1.0, 0.0)
  sr    <- scoring_rule_scores(.prof(), sw)
  borda <- borda_scores(.prof())
  expect_equal(sr, as.double(borda), tolerance = 1e-10)
})

test_that("scoring_rule_one_winner is in scoring_rule_all_winners", {
  skip_without_lib()
  sw    <- c(2.0, 1.0, 0.0)
  w     <- scoring_rule_one_winner(.prof(), sw)
  all_w <- scoring_rule_all_winners(.prof(), sw)
  expect_true(w %in% all_w)
})

# ---------------------------------------------------------------------------
# Approval voting — spatial
# ---------------------------------------------------------------------------

test_that("approval_scores_spatial returns non-negative integer vector", {
  skip_without_lib()
  scores <- approval_scores_spatial(.alts, .voters, threshold = 1.5)
  expect_type(scores, "integer")
  expect_true(all(scores >= 0L))
})

test_that("approval_all_winners_spatial entries are 1-based and in range", {
  skip_without_lib()
  winners <- approval_all_winners_spatial(.alts, .voters, threshold = 1.5)
  expect_true(all(winners >= 1L & winners <= 3L))
})

# ---------------------------------------------------------------------------
# Approval voting — top-k
# ---------------------------------------------------------------------------

test_that("approval_scores_topk with k=n_alts gives equal scores", {
  skip_without_lib()
  scores <- approval_scores_topk(.prof(), k = 3L)
  expect_true(all(scores == scores[1]))
})

test_that("approval_scores_topk with k=1 equals plurality_scores", {
  skip_without_lib()
  topk <- approval_scores_topk(.prof(), k = 1L)
  plu  <- plurality_scores(.prof())
  expect_equal(topk, plu)
})

# ---------------------------------------------------------------------------
# rank_by_scores
# ---------------------------------------------------------------------------

test_that("rank_by_scores produces a permutation of 1:n", {
  skip_without_lib()
  scores  <- c(1.5, 0.5, 2.0)
  ranking <- rank_by_scores(scores)
  expect_identical(sort(ranking), 1:3)
  expect_identical(ranking[1], 3L)
})

# ---------------------------------------------------------------------------
# pairwise_ranking_from_matrix
# ---------------------------------------------------------------------------

test_that("pairwise_ranking_from_matrix produces a permutation", {
  skip_without_lib()
  mat     <- pairwise_matrix_2d(.alts, .voters)
  ranking <- pairwise_ranking_from_matrix(mat)
  expect_identical(sort(ranking), 1:3)
})

# ---------------------------------------------------------------------------
# Pareto
# ---------------------------------------------------------------------------

test_that("pareto_set returns non-empty 1-based vector", {
  skip_without_lib()
  ps <- pareto_set(.prof())
  expect_type(ps, "integer")
  expect_gte(length(ps), 1L)
  expect_true(all(ps >= 1L & ps <= 3L))
})

test_that("is_pareto_efficient returns logical", {
  skip_without_lib()
  expect_type(is_pareto_efficient(.prof(), 1L), "logical")
})

test_that("is_pareto_efficient errors on out-of-range alt", {
  skip_without_lib()
  expect_error(is_pareto_efficient(.prof(), 0L), "out of range")
  expect_error(is_pareto_efficient(.prof(), 4L), "out of range")
})

test_that("pareto_set and is_pareto_efficient agree", {
  skip_without_lib()
  prof <- .prof()
  ps   <- pareto_set(prof)
  for (i in seq_len(3L)) {
    if (i %in% ps) {
      expect_true(is_pareto_efficient(prof, i))
    } else {
      expect_false(is_pareto_efficient(prof, i))
    }
  }
})

# ---------------------------------------------------------------------------
# Condorcet and majority-selection predicates
# ---------------------------------------------------------------------------

test_that("has_condorcet_winner_profile returns logical", {
  skip_without_lib()
  expect_type(has_condorcet_winner_profile(.prof()), "logical")
})

test_that("condorcet_winner_profile returns 1-based int or NA", {
  skip_without_lib()
  w <- condorcet_winner_profile(.prof())
  expect_true(is.na(w) || (is.integer(w) && w >= 1L && w <= 3L))
})

test_that("has/condorcet_winner_profile agree", {
  skip_without_lib()
  prof  <- .prof()
  has_w <- has_condorcet_winner_profile(prof)
  w     <- condorcet_winner_profile(prof)
  if (has_w) {
    expect_false(is.na(w))
  } else {
    expect_true(is.na(w))
  }
})

test_that("is_condorcet_consistent errors on out-of-range alt", {
  skip_without_lib()
  expect_error(is_condorcet_consistent(.prof(), 0L), "out of range")
  expect_error(is_condorcet_consistent(.prof(), 4L), "out of range")
})

test_that("is_majority_consistent implies is_condorcet_consistent", {
  skip_without_lib()
  prof <- .prof()
  for (i in 1:3) {
    if (is_majority_consistent(prof, i)) {
      expect_true(is_condorcet_consistent(prof, i))
    }
  }
})

test_that("if Condorcet winner exists, it is majority-consistent", {
  skip_without_lib()
  prof <- .prof()
  w    <- condorcet_winner_profile(prof)
  if (!is.na(w)) {
    expect_true(is_majority_consistent(prof, w))
  }
})
