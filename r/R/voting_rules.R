# voting_rules.R — B3.7: Plurality, Borda, anti-plurality, scoring rule,
#                  approval voting (spatial and top-k ordinal)
#
# All winner indices are 1-based.
# tie_break: "smallest_index" (deterministic, default) or "random" (requires
#            stream_manager + stream_name).
# Scores are returned as integer or double vectors with alternative names.

# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

# Resolve tie-break arguments to the values expected by the C layer.
.tb_args <- function(tie_break, stream_manager, stream_name) {
  .check_tie_break_args(tie_break, stream_manager, stream_name)
  list(
    tb   = .resolve_tie_break(tie_break),
    mgr  = if (is.null(stream_manager)) NULL else .sm_ptr(stream_manager),
    sn   = if (is.null(stream_name)) character(0L) else as.character(stream_name)
  )
}

# ---------------------------------------------------------------------------
# C6.1 — Plurality
# ---------------------------------------------------------------------------

#' Compute plurality scores (first-place vote counts)
#'
#' @param profile A \code{\link{Profile}} object.
#' @return Integer vector of length n_alts. Names are \code{"alt1", "alt2", ...}.
#' @export
plurality_scores <- function(profile) {
  scores <- .Call("r_scs_plurality_scores", .prof_ptr(profile))
  names(scores) <- paste0("alt", seq_along(scores))
  scores
}

#' Return all plurality winners (alternatives tied for most first-place votes)
#'
#' @param profile A \code{\link{Profile}} object.
#' @return Integer vector of 1-based alternative indices.
#' @export
plurality_all_winners <- function(profile) {
  .Call("r_scs_plurality_all_winners", .prof_ptr(profile))
}

#' Return a single plurality winner with tie-breaking
#'
#' @param profile A \code{\link{Profile}} object.
#' @param tie_break \code{"smallest_index"} (default) or \code{"random"}.
#' @param stream_manager A \code{\link{StreamManager}} object; required when
#'   \code{tie_break = "random"}.
#' @param stream_name Character. Named stream for randomness; required when
#'   \code{tie_break = "random"}.
#' @return Integer scalar (1-based alternative index).
#' @export
plurality_one_winner <- function(profile,
                                  tie_break      = "smallest_index",
                                  stream_manager = NULL,
                                  stream_name    = NULL) {
  tba <- .tb_args(tie_break, stream_manager, stream_name)
  .Call("r_scs_plurality_one_winner", .prof_ptr(profile),
        tba$tb, tba$mgr, tba$sn)
}

# ---------------------------------------------------------------------------
# C6.2 — Borda Count
# ---------------------------------------------------------------------------

#' Compute Borda scores
#'
#' Each voter awards \code{n_alts - 1} points to their top alternative,
#' \code{n_alts - 2} to the second, and so on.
#'
#' @param profile A \code{\link{Profile}} object.
#' @return Integer vector of length n_alts. Names are \code{"alt1", "alt2", ...}.
#' @export
borda_scores <- function(profile) {
  scores <- .Call("r_scs_borda_scores", .prof_ptr(profile))
  names(scores) <- paste0("alt", seq_along(scores))
  scores
}

#' Return all Borda winners
#'
#' @param profile A \code{\link{Profile}} object.
#' @return Integer vector of 1-based alternative indices.
#' @export
borda_all_winners <- function(profile) {
  .Call("r_scs_borda_all_winners", .prof_ptr(profile))
}

#' Return a single Borda winner with tie-breaking
#'
#' @inheritParams plurality_one_winner
#' @return Integer scalar (1-based).
#' @export
borda_one_winner <- function(profile,
                              tie_break      = "smallest_index",
                              stream_manager = NULL,
                              stream_name    = NULL) {
  tba <- .tb_args(tie_break, stream_manager, stream_name)
  .Call("r_scs_borda_one_winner", .prof_ptr(profile),
        tba$tb, tba$mgr, tba$sn)
}

#' Full social ordering by descending Borda score
#'
#' @inheritParams plurality_one_winner
#' @return Integer vector of length n_alts. \code{result[1]} is the most
#'   preferred alternative (1-based).
#' @export
borda_ranking <- function(profile,
                           tie_break      = "smallest_index",
                           stream_manager = NULL,
                           stream_name    = NULL) {
  tba <- .tb_args(tie_break, stream_manager, stream_name)
  .Call("r_scs_borda_ranking", .prof_ptr(profile),
        tba$tb, tba$mgr, tba$sn)
}

# ---------------------------------------------------------------------------
# C6.3 — Anti-plurality
# ---------------------------------------------------------------------------

#' Compute anti-plurality scores
#'
#' \code{scores[j]} = number of voters for whom alternative \code{j} is NOT
#' ranked last. Higher is better.
#'
#' @param profile A \code{\link{Profile}} object.
#' @return Integer vector of length n_alts.
#' @export
antiplurality_scores <- function(profile) {
  scores <- .Call("r_scs_antiplurality_scores", .prof_ptr(profile))
  names(scores) <- paste0("alt", seq_along(scores))
  scores
}

#' Return all anti-plurality winners
#'
#' @param profile A \code{\link{Profile}} object.
#' @return Integer vector of 1-based alternative indices.
#' @export
antiplurality_all_winners <- function(profile) {
  .Call("r_scs_antiplurality_all_winners", .prof_ptr(profile))
}

#' Return a single anti-plurality winner with tie-breaking
#'
#' @inheritParams plurality_one_winner
#' @return Integer scalar (1-based).
#' @export
antiplurality_one_winner <- function(profile,
                                      tie_break      = "smallest_index",
                                      stream_manager = NULL,
                                      stream_name    = NULL) {
  tba <- .tb_args(tie_break, stream_manager, stream_name)
  .Call("r_scs_antiplurality_one_winner", .prof_ptr(profile),
        tba$tb, tba$mgr, tba$sn)
}

# ---------------------------------------------------------------------------
# C6.4 — Generic positional scoring rule
# ---------------------------------------------------------------------------

#' Compute scores under a positional scoring rule
#'
#' @param profile A \code{\link{Profile}} object.
#' @param score_weights Non-increasing numeric vector of length n_alts.
#'   \code{score_weights[r]} is awarded to the alternative at rank \code{r}
#'   (rank 1 = most preferred, 1-indexed).
#' @return Double vector of length n_alts.
#' @export
scoring_rule_scores <- function(profile, score_weights) {
  scores <- .Call("r_scs_scoring_rule_scores",
                  .prof_ptr(profile), as.double(score_weights))
  names(scores) <- paste0("alt", seq_along(scores))
  scores
}

#' Return all winners under a positional scoring rule
#'
#' @inheritParams scoring_rule_scores
#' @return Integer vector of 1-based alternative indices.
#' @export
scoring_rule_all_winners <- function(profile, score_weights) {
  .Call("r_scs_scoring_rule_all_winners",
        .prof_ptr(profile), as.double(score_weights))
}

#' Return a single winner under a positional scoring rule with tie-breaking
#'
#' @inheritParams scoring_rule_scores
#' @inheritParams plurality_one_winner
#' @return Integer scalar (1-based).
#' @export
scoring_rule_one_winner <- function(profile, score_weights,
                                     tie_break      = "smallest_index",
                                     stream_manager = NULL,
                                     stream_name    = NULL) {
  tba <- .tb_args(tie_break, stream_manager, stream_name)
  .Call("r_scs_scoring_rule_one_winner",
        .prof_ptr(profile), as.double(score_weights),
        tba$tb, tba$mgr, tba$sn)
}

# ---------------------------------------------------------------------------
# C6.5 — Approval voting (spatial threshold model)
# ---------------------------------------------------------------------------

#' Compute approval scores under the spatial threshold model
#'
#' A voter approves an alternative if its distance from the voter's ideal point
#' is at most \code{threshold}.
#'
#' @param alts Flat numeric vector \code{[x0, y0, ...]} of alternative
#'   coordinates (length = 2 * n_alts).
#' @param voter_ideals Flat numeric vector of voter ideal coordinates.
#' @param threshold Non-negative approval radius.
#' @param dist_config Distance configuration from \code{\link{make_dist_config}}.
#' @return Integer vector of length n_alts (approval counts).
#' @export
approval_scores_spatial <- function(alts, voter_ideals, threshold,
                                     dist_config = make_dist_config()) {
  scores <- .Call("r_scs_approval_scores_spatial",
                  as.double(alts), as.double(voter_ideals),
                  as.double(threshold), dist_config)
  names(scores) <- paste0("alt", seq_along(scores))
  scores
}

#' Return all winners under spatial approval voting
#'
#' @inheritParams approval_scores_spatial
#' @return Integer vector of 1-based alternative indices (those with the
#'   highest approval count, and at least one approver).
#' @export
approval_all_winners_spatial <- function(alts, voter_ideals, threshold,
                                          dist_config = make_dist_config()) {
  .Call("r_scs_approval_all_winners_spatial",
        as.double(alts), as.double(voter_ideals),
        as.double(threshold), dist_config)
}

# ---------------------------------------------------------------------------
# C6.5 — Approval voting (ordinal top-k variant)
# ---------------------------------------------------------------------------

#' Compute approval scores under the ordinal top-k model
#'
#' Each voter approves their top \code{k} ranked alternatives.
#'
#' @param profile A \code{\link{Profile}} object.
#' @param k Integer in \code{[1, n_alts]}.
#' @return Integer vector of length n_alts (approval counts).
#' @export
approval_scores_topk <- function(profile, k) {
  scores <- .Call("r_scs_approval_scores_topk",
                  .prof_ptr(profile), as.integer(k))
  names(scores) <- paste0("alt", seq_along(scores))
  scores
}

#' Return all winners under the top-k approval rule
#'
#' @param profile A \code{\link{Profile}} object.
#' @param k Integer in \code{[1, n_alts]}.
#' @return Integer vector of 1-based alternative indices.
#' @export
approval_all_winners_topk <- function(profile, k) {
  .Call("r_scs_approval_all_winners_topk",
        .prof_ptr(profile), as.integer(k))
}
