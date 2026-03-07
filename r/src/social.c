/* social.c — .Call() wrappers for:
 *   B3.8  Social rankings and social-choice properties (C7)
 *
 * Alternative indices: 0-based in C API, 1-based returned to R.
 * tie_break: 0 = random, 1 = smallest_index.
 * mgr_ptr / sname: see voting_rules.c for the convention.
 */

#include "scs_r_helpers.h"
#include <string.h>

static const char *resolve_sn(SEXP sname, SCS_StreamManager *mgr) {
    if (!mgr) return NULL;
    if (XLENGTH(sname) < 1) return NULL;
    return CHAR(STRING_ELT(sname, 0));
}

/* ---------------------------------------------------------------------------
 * C7.1 — Rank by scores
 * --------------------------------------------------------------------------- */

/* scores: double vector length n_alts. Returns 1-based ranking vector. */
SEXP r_scs_rank_by_scores(SEXP scores, SEXP tb, SEXP mgr_ptr, SEXP sname) {
    int n_alts = (int)XLENGTH(scores);
    SCS_StreamManager *mgr = (SCS_StreamManager *)get_extptr(mgr_ptr);
    char err[SCS_ERR_BUF_SIZE] = {0};
    SEXP result = PROTECT(allocVector(INTSXP, n_alts));
    scs_check(scs_rank_by_scores(REAL(scores), n_alts, asInteger(tb), mgr,
                                 resolve_sn(sname, mgr), INTEGER(result),
                                 n_alts, err, SCS_ERR_BUF_SIZE),
              err);
    for (int i = 0; i < n_alts; i++) INTEGER(result)[i]++;
    UNPROTECT(1);
    return result;
}

/* ---------------------------------------------------------------------------
 * C7.2 — Pairwise ranking from matrix
 * --------------------------------------------------------------------------- */

/* mat: n_alts × n_alts integer matrix (column-major from R).
 * The C API expects row-major SCS_PairwiseResult[]. We transpose here. */
SEXP r_scs_pairwise_ranking_from_matrix(SEXP mat, SEXP n_alts_s, SEXP tb,
                                         SEXP mgr_ptr, SEXP sname) {
    int n_alts = asInteger(n_alts_s);
    SCS_StreamManager *mgr = (SCS_StreamManager *)get_extptr(mgr_ptr);
    char err[SCS_ERR_BUF_SIZE] = {0};
    /* Column-major R mat[r + c*n] → row-major tmp[r*n + c]. */
    SCS_PairwiseResult *tmp =
        (SCS_PairwiseResult *)R_alloc((size_t)n_alts * n_alts,
                                      sizeof(SCS_PairwiseResult));
    int *src = INTEGER(mat);
    for (int r = 0; r < n_alts; r++)
        for (int c = 0; c < n_alts; c++)
            tmp[r * n_alts + c] = (SCS_PairwiseResult)src[r + c * n_alts];
    SEXP result = PROTECT(allocVector(INTSXP, n_alts));
    scs_check(scs_pairwise_ranking_from_matrix(tmp, n_alts, asInteger(tb),
                                               mgr, resolve_sn(sname, mgr),
                                               INTEGER(result), n_alts, err,
                                               SCS_ERR_BUF_SIZE),
              err);
    for (int i = 0; i < n_alts; i++) INTEGER(result)[i]++;
    UNPROTECT(1);
    return result;
}

/* ---------------------------------------------------------------------------
 * C7.3 — Pareto efficiency
 * --------------------------------------------------------------------------- */

/* Returns 1-based integer vector of Pareto-efficient alternative indices. */
SEXP r_scs_pareto_set(SEXP ptr) {
    SCS_Profile *p = (SCS_Profile *)get_extptr(ptr);
    if (!p) Rf_error("Profile handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    int out_n;
    scs_check(scs_pareto_set(p, NULL, 0, &out_n, err, SCS_ERR_BUF_SIZE), err);
    SEXP result = PROTECT(allocVector(INTSXP, out_n));
    scs_check(scs_pareto_set(p, INTEGER(result), out_n, &out_n, err,
                             SCS_ERR_BUF_SIZE),
              err);
    for (int i = 0; i < out_n; i++) INTEGER(result)[i]++;
    UNPROTECT(1);
    return result;
}

/* alt_0based: 0-based alternative index (the R wrapper translates 1→0). */
SEXP r_scs_is_pareto_efficient(SEXP ptr, SEXP alt_0based) {
    SCS_Profile *p = (SCS_Profile *)get_extptr(ptr);
    if (!p) Rf_error("Profile handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    int out;
    scs_check(scs_is_pareto_efficient(p, asInteger(alt_0based), &out, err,
                                      SCS_ERR_BUF_SIZE),
              err);
    return ScalarLogical(out);
}

/* ---------------------------------------------------------------------------
 * C7.4 — Condorcet and majority-selection predicates
 * --------------------------------------------------------------------------- */

SEXP r_scs_has_condorcet_winner_profile(SEXP ptr) {
    SCS_Profile *p = (SCS_Profile *)get_extptr(ptr);
    if (!p) Rf_error("Profile handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    int found;
    scs_check(scs_has_condorcet_winner_profile(p, &found, err,
                                               SCS_ERR_BUF_SIZE),
              err);
    return ScalarLogical(found);
}

/* Returns 1-based winner index or NA_integer if no Condorcet winner. */
SEXP r_scs_condorcet_winner_profile(SEXP ptr) {
    SCS_Profile *p = (SCS_Profile *)get_extptr(ptr);
    if (!p) Rf_error("Profile handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    int found, winner;
    scs_check(scs_condorcet_winner_profile(p, &found, &winner, err,
                                           SCS_ERR_BUF_SIZE),
              err);
    return found ? ScalarInteger(winner + 1) : ScalarInteger(NA_INTEGER);
}

/* alt_0based: 0-based index. Returns logical. */
SEXP r_scs_is_condorcet_consistent(SEXP ptr, SEXP alt_0based) {
    SCS_Profile *p = (SCS_Profile *)get_extptr(ptr);
    if (!p) Rf_error("Profile handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    int out;
    scs_check(scs_is_selected_by_condorcet_consistent_rules(
                  p, asInteger(alt_0based), &out, err, SCS_ERR_BUF_SIZE),
              err);
    return ScalarLogical(out);
}

/* alt_0based: 0-based index. Returns logical. */
SEXP r_scs_is_majority_consistent(SEXP ptr, SEXP alt_0based) {
    SCS_Profile *p = (SCS_Profile *)get_extptr(ptr);
    if (!p) Rf_error("Profile handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    int out;
    scs_check(scs_is_selected_by_majority_consistent_rules(
                  p, asInteger(alt_0based), &out, err, SCS_ERR_BUF_SIZE),
              err);
    return ScalarLogical(out);
}
