/* voting_rules.c — .Call() wrappers for:
 *   B3.7  Plurality, Borda, anti-plurality, scoring rule, approval voting
 *
 * All "one_winner" functions return a 1-based integer.
 * All "all_winners" functions return a 1-based integer vector (size-query
 * pattern hidden from R callers).
 * "scores" functions return raw numeric/integer scores (no index shift).
 *
 * tie_break argument: 0 = random, 1 = smallest_index (SCS_TIEBREAK_*).
 * mgr_ptr: external pointer to SCS_StreamManager, or R_NilValue.
 * sname:   character(1) with stream name, or character(0) when unused.
 */

#include "scs_r_helpers.h"

/* Resolve the stream name argument: returns NULL when unused (character(0)
 * or when mgr == NULL). */
static const char *resolve_stream_name(SEXP sname, SCS_StreamManager *mgr) {
    if (!mgr) return NULL;
    if (XLENGTH(sname) < 1) return NULL;
    return CHAR(STRING_ELT(sname, 0));
}

/* Common preamble shared by profile-based wrappers. */
#define PROFILE_PREAMBLE(ptr_sexp)                                       \
    SCS_Profile *p = (SCS_Profile *)get_extptr(ptr_sexp);               \
    if (!p) Rf_error("Profile handle is NULL or has already been destroyed."); \
    char err[SCS_ERR_BUF_SIZE] = {0};                                   \
    int n_voters, n_alts;                                                \
    scs_check(scs_profile_dims(p, &n_voters, &n_alts, err, SCS_ERR_BUF_SIZE), err)

/* ---------------------------------------------------------------------------
 * C6.1 — Plurality
 * --------------------------------------------------------------------------- */

SEXP r_scs_plurality_scores(SEXP ptr) {
    PROFILE_PREAMBLE(ptr);
    SEXP result = PROTECT(allocVector(INTSXP, n_alts));
    scs_check(scs_plurality_scores(p, INTEGER(result), n_alts, err,
                                   SCS_ERR_BUF_SIZE),
              err);
    UNPROTECT(1);
    return result;
}

SEXP r_scs_plurality_all_winners(SEXP ptr) {
    PROFILE_PREAMBLE(ptr);
    int out_n;
    scs_check(scs_plurality_all_winners(p, NULL, 0, &out_n, err,
                                        SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(INTSXP, out_n));
    scs_check(scs_plurality_all_winners(p, INTEGER(result), out_n, &out_n,
                                        err, SCS_ERR_BUF_SIZE),
              err);
    for (int i = 0; i < out_n; i++) INTEGER(result)[i]++;
    UNPROTECT(1);
    return result;
}

SEXP r_scs_plurality_one_winner(SEXP ptr, SEXP tb, SEXP mgr_ptr, SEXP sname) {
    PROFILE_PREAMBLE(ptr);
    SCS_StreamManager *mgr = (SCS_StreamManager *)get_extptr(mgr_ptr);
    int out;
    scs_check(scs_plurality_one_winner(p, asInteger(tb), mgr,
                                       resolve_stream_name(sname, mgr), &out,
                                       err, SCS_ERR_BUF_SIZE),
              err);
    return ScalarInteger(out + 1);
}

/* ---------------------------------------------------------------------------
 * C6.2 — Borda Count
 * --------------------------------------------------------------------------- */

SEXP r_scs_borda_scores(SEXP ptr) {
    PROFILE_PREAMBLE(ptr);
    SEXP result = PROTECT(allocVector(INTSXP, n_alts));
    scs_check(scs_borda_scores(p, INTEGER(result), n_alts, err,
                               SCS_ERR_BUF_SIZE),
              err);
    UNPROTECT(1);
    return result;
}

SEXP r_scs_borda_all_winners(SEXP ptr) {
    PROFILE_PREAMBLE(ptr);
    int out_n;
    scs_check(scs_borda_all_winners(p, NULL, 0, &out_n, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(INTSXP, out_n));
    scs_check(scs_borda_all_winners(p, INTEGER(result), out_n, &out_n, err,
                                    SCS_ERR_BUF_SIZE),
              err);
    for (int i = 0; i < out_n; i++) INTEGER(result)[i]++;
    UNPROTECT(1);
    return result;
}

SEXP r_scs_borda_one_winner(SEXP ptr, SEXP tb, SEXP mgr_ptr, SEXP sname) {
    PROFILE_PREAMBLE(ptr);
    SCS_StreamManager *mgr = (SCS_StreamManager *)get_extptr(mgr_ptr);
    int out;
    scs_check(scs_borda_one_winner(p, asInteger(tb), mgr,
                                   resolve_stream_name(sname, mgr), &out,
                                   err, SCS_ERR_BUF_SIZE),
              err);
    return ScalarInteger(out + 1);
}

SEXP r_scs_borda_ranking(SEXP ptr, SEXP tb, SEXP mgr_ptr, SEXP sname) {
    PROFILE_PREAMBLE(ptr);
    SCS_StreamManager *mgr = (SCS_StreamManager *)get_extptr(mgr_ptr);
    SEXP result = PROTECT(allocVector(INTSXP, n_alts));
    scs_check(scs_borda_ranking(p, asInteger(tb), mgr,
                                resolve_stream_name(sname, mgr),
                                INTEGER(result), n_alts, err, SCS_ERR_BUF_SIZE),
              err);
    for (int i = 0; i < n_alts; i++) INTEGER(result)[i]++;
    UNPROTECT(1);
    return result;
}

/* ---------------------------------------------------------------------------
 * C6.3 — Anti-plurality
 * --------------------------------------------------------------------------- */

SEXP r_scs_antiplurality_scores(SEXP ptr) {
    PROFILE_PREAMBLE(ptr);
    SEXP result = PROTECT(allocVector(INTSXP, n_alts));
    scs_check(scs_antiplurality_scores(p, INTEGER(result), n_alts, err,
                                       SCS_ERR_BUF_SIZE),
              err);
    UNPROTECT(1);
    return result;
}

SEXP r_scs_antiplurality_all_winners(SEXP ptr) {
    PROFILE_PREAMBLE(ptr);
    int out_n;
    scs_check(scs_antiplurality_all_winners(p, NULL, 0, &out_n, err,
                                            SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(INTSXP, out_n));
    scs_check(scs_antiplurality_all_winners(p, INTEGER(result), out_n, &out_n,
                                            err, SCS_ERR_BUF_SIZE),
              err);
    for (int i = 0; i < out_n; i++) INTEGER(result)[i]++;
    UNPROTECT(1);
    return result;
}

SEXP r_scs_antiplurality_one_winner(SEXP ptr, SEXP tb, SEXP mgr_ptr,
                                     SEXP sname) {
    PROFILE_PREAMBLE(ptr);
    SCS_StreamManager *mgr = (SCS_StreamManager *)get_extptr(mgr_ptr);
    int out;
    scs_check(scs_antiplurality_one_winner(p, asInteger(tb), mgr,
                                           resolve_stream_name(sname, mgr),
                                           &out, err, SCS_ERR_BUF_SIZE),
              err);
    return ScalarInteger(out + 1);
}

/* ---------------------------------------------------------------------------
 * C6.4 — Generic positional scoring rule
 * --------------------------------------------------------------------------- */

SEXP r_scs_scoring_rule_scores(SEXP ptr, SEXP score_weights) {
    PROFILE_PREAMBLE(ptr);
    int nw = (int)XLENGTH(score_weights);
    SEXP result = PROTECT(allocVector(REALSXP, n_alts));
    scs_check(scs_scoring_rule_scores(p, REAL(score_weights), nw, REAL(result),
                                      n_alts, err, SCS_ERR_BUF_SIZE),
              err);
    UNPROTECT(1);
    return result;
}

SEXP r_scs_scoring_rule_all_winners(SEXP ptr, SEXP score_weights) {
    PROFILE_PREAMBLE(ptr);
    int nw = (int)XLENGTH(score_weights);
    int out_n;
    scs_check(scs_scoring_rule_all_winners(p, REAL(score_weights), nw, NULL, 0,
                                           &out_n, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(INTSXP, out_n));
    scs_check(scs_scoring_rule_all_winners(p, REAL(score_weights), nw,
                                           INTEGER(result), out_n, &out_n,
                                           err, SCS_ERR_BUF_SIZE),
              err);
    for (int i = 0; i < out_n; i++) INTEGER(result)[i]++;
    UNPROTECT(1);
    return result;
}

SEXP r_scs_scoring_rule_one_winner(SEXP ptr, SEXP score_weights, SEXP tb,
                                    SEXP mgr_ptr, SEXP sname) {
    PROFILE_PREAMBLE(ptr);
    int nw = (int)XLENGTH(score_weights);
    SCS_StreamManager *mgr = (SCS_StreamManager *)get_extptr(mgr_ptr);
    int out;
    scs_check(scs_scoring_rule_one_winner(p, REAL(score_weights), nw,
                                          asInteger(tb), mgr,
                                          resolve_stream_name(sname, mgr),
                                          &out, err, SCS_ERR_BUF_SIZE),
              err);
    return ScalarInteger(out + 1);
}

/* ---------------------------------------------------------------------------
 * C6.5 — Approval voting (spatial threshold model)
 * --------------------------------------------------------------------------- */

SEXP r_scs_approval_scores_spatial(SEXP alt_xy, SEXP voter_xy, SEXP threshold,
                                    SEXP dist_cfg) {
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    int n_alts   = (int)(XLENGTH(alt_xy) / 2);
    int n_voters = (int)(XLENGTH(voter_xy) / 2);
    char err[SCS_ERR_BUF_SIZE] = {0};
    SEXP result = PROTECT(allocVector(INTSXP, n_alts));
    scs_check(scs_approval_scores_spatial(REAL(alt_xy), n_alts, REAL(voter_xy),
                                          n_voters, asReal(threshold), &dc,
                                          INTEGER(result), n_alts, err,
                                          SCS_ERR_BUF_SIZE),
              err);
    UNPROTECT(1);
    return result;
}

SEXP r_scs_approval_all_winners_spatial(SEXP alt_xy, SEXP voter_xy,
                                         SEXP threshold, SEXP dist_cfg) {
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    int n_alts   = (int)(XLENGTH(alt_xy) / 2);
    int n_voters = (int)(XLENGTH(voter_xy) / 2);
    char err[SCS_ERR_BUF_SIZE] = {0};
    int out_n;
    scs_check(scs_approval_all_winners_spatial(REAL(alt_xy), n_alts,
                                               REAL(voter_xy), n_voters,
                                               asReal(threshold), &dc, NULL, 0,
                                               &out_n, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(INTSXP, out_n));
    scs_check(scs_approval_all_winners_spatial(REAL(alt_xy), n_alts,
                                               REAL(voter_xy), n_voters,
                                               asReal(threshold), &dc,
                                               INTEGER(result), out_n, &out_n,
                                               err, SCS_ERR_BUF_SIZE),
              err);
    for (int i = 0; i < out_n; i++) INTEGER(result)[i]++;
    UNPROTECT(1);
    return result;
}

/* ---------------------------------------------------------------------------
 * C6.5 — Approval voting (ordinal top-k variant)
 * --------------------------------------------------------------------------- */

SEXP r_scs_approval_scores_topk(SEXP ptr, SEXP k) {
    PROFILE_PREAMBLE(ptr);
    SEXP result = PROTECT(allocVector(INTSXP, n_alts));
    scs_check(scs_approval_scores_topk(p, asInteger(k), INTEGER(result),
                                       n_alts, err, SCS_ERR_BUF_SIZE),
              err);
    UNPROTECT(1);
    return result;
}

SEXP r_scs_approval_all_winners_topk(SEXP ptr, SEXP k) {
    PROFILE_PREAMBLE(ptr);
    int out_n;
    scs_check(scs_approval_all_winners_topk(p, asInteger(k), NULL, 0, &out_n,
                                            err, SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(INTSXP, out_n));
    scs_check(scs_approval_all_winners_topk(p, asInteger(k), INTEGER(result),
                                            out_n, &out_n, err,
                                            SCS_ERR_BUF_SIZE),
              err);
    for (int i = 0; i < out_n; i++) INTEGER(result)[i]++;
    UNPROTECT(1);
    return result;
}
