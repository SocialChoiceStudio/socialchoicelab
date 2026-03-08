/* geometry.c — .Call() wrappers for:
 *   B3.5  Copeland scores/winner, Condorcet winner, core, uncovered set
 *   B3.6  Centrality: marginal median, centroid, geometric mean
 *
 * All alternative indices are translated 0-based (C) ↔ 1-based (R) here.
 * The size-query pattern for uncovered_set_2d is hidden from R callers.
 */

#include "scs_r_helpers.h"

/* ---------------------------------------------------------------------------
 * Copeland
 * --------------------------------------------------------------------------- */

SEXP r_scs_copeland_scores_2d(SEXP alt_xy, SEXP voter_xy, SEXP dist_cfg,
                               SEXP k) {
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    int n_alts   = (int)(XLENGTH(alt_xy) / 2);
    int n_voters = (int)(XLENGTH(voter_xy) / 2);
    char err[SCS_ERR_BUF_SIZE] = {0};
    SEXP result = PROTECT(allocVector(INTSXP, n_alts));
    scs_check(scs_copeland_scores_2d(REAL(alt_xy), n_alts, REAL(voter_xy),
                                     n_voters, &dc, asInteger(k),
                                     INTEGER(result), n_alts, err,
                                     SCS_ERR_BUF_SIZE),
              err);
    UNPROTECT(1);
    return result;
}

/* Returns the 1-based index of the Copeland winner. */
SEXP r_scs_copeland_winner_2d(SEXP alt_xy, SEXP voter_xy, SEXP dist_cfg,
                               SEXP k) {
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    int n_alts   = (int)(XLENGTH(alt_xy) / 2);
    int n_voters = (int)(XLENGTH(voter_xy) / 2);
    char err[SCS_ERR_BUF_SIZE] = {0};
    int out;
    scs_check(scs_copeland_winner_2d(REAL(alt_xy), n_alts, REAL(voter_xy),
                                     n_voters, &dc, asInteger(k), &out, err,
                                     SCS_ERR_BUF_SIZE),
              err);
    return ScalarInteger(out + 1); /* 0→1-based */
}

/* ---------------------------------------------------------------------------
 * Condorcet winner (discrete)
 * --------------------------------------------------------------------------- */

SEXP r_scs_has_condorcet_winner_2d(SEXP alt_xy, SEXP voter_xy, SEXP dist_cfg,
                                    SEXP k) {
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    int n_alts   = (int)(XLENGTH(alt_xy) / 2);
    int n_voters = (int)(XLENGTH(voter_xy) / 2);
    char err[SCS_ERR_BUF_SIZE] = {0};
    int found;
    scs_check(scs_has_condorcet_winner_2d(REAL(alt_xy), n_alts, REAL(voter_xy),
                                          n_voters, &dc, asInteger(k), &found,
                                          err, SCS_ERR_BUF_SIZE),
              err);
    return ScalarLogical(found);
}

/* Returns the 1-based index of the Condorcet winner, or NA_integer if none. */
SEXP r_scs_condorcet_winner_2d(SEXP alt_xy, SEXP voter_xy, SEXP dist_cfg,
                                SEXP k) {
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    int n_alts   = (int)(XLENGTH(alt_xy) / 2);
    int n_voters = (int)(XLENGTH(voter_xy) / 2);
    char err[SCS_ERR_BUF_SIZE] = {0};
    int found, winner;
    scs_check(scs_condorcet_winner_2d(REAL(alt_xy), n_alts, REAL(voter_xy),
                                      n_voters, &dc, asInteger(k), &found,
                                      &winner, err, SCS_ERR_BUF_SIZE),
              err);
    return found ? ScalarInteger(winner + 1) : ScalarInteger(NA_INTEGER);
}

/* ---------------------------------------------------------------------------
 * Core (continuous)
 * --------------------------------------------------------------------------- */

/* Returns list(found = logical, x = numeric or NA, y = numeric or NA). */
SEXP r_scs_core_2d(SEXP voter_xy, SEXP dist_cfg, SEXP k) {
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    int n_voters = (int)(XLENGTH(voter_xy) / 2);
    char err[SCS_ERR_BUF_SIZE] = {0};
    int found;
    double cx, cy;
    scs_check(scs_core_2d(REAL(voter_xy), n_voters, &dc, asInteger(k), &found,
                          &cx, &cy, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(VECSXP, 3));
    SEXP names = PROTECT(allocVector(STRSXP, 3));
    SET_VECTOR_ELT(result, 0, ScalarLogical(found));
    SET_STRING_ELT(names, 0, mkChar("found"));
    SET_VECTOR_ELT(result, 1, found ? ScalarReal(cx) : ScalarReal(NA_REAL));
    SET_STRING_ELT(names, 1, mkChar("x"));
    SET_VECTOR_ELT(result, 2, found ? ScalarReal(cy) : ScalarReal(NA_REAL));
    SET_STRING_ELT(names, 2, mkChar("y"));
    setAttrib(result, R_NamesSymbol, names);
    UNPROTECT(2);
    return result;
}

/* ---------------------------------------------------------------------------
 * Uncovered set (discrete and continuous boundary)
 * --------------------------------------------------------------------------- */

/* Returns 1-based integer vector of uncovered alternative indices. */
SEXP r_scs_uncovered_set_2d(SEXP alt_xy, SEXP voter_xy, SEXP dist_cfg,
                             SEXP k) {
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    int n_alts   = (int)(XLENGTH(alt_xy) / 2);
    int n_voters = (int)(XLENGTH(voter_xy) / 2);
    char err[SCS_ERR_BUF_SIZE] = {0};
    /* Size query. */
    int out_n;
    scs_check(scs_uncovered_set_2d(REAL(alt_xy), n_alts, REAL(voter_xy),
                                   n_voters, &dc, asInteger(k), NULL, 0,
                                   &out_n, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(INTSXP, out_n));
    scs_check(scs_uncovered_set_2d(REAL(alt_xy), n_alts, REAL(voter_xy),
                                   n_voters, &dc, asInteger(k),
                                   INTEGER(result), out_n, &out_n, err,
                                   SCS_ERR_BUF_SIZE),
              err);
    for (int i = 0; i < out_n; i++) INTEGER(result)[i]++; /* 0→1-based */
    UNPROTECT(1);
    return result;
}

/* Returns an n_pts × 2 numeric matrix (columns x, y) of the approximate
 * continuous uncovered-set boundary polygon. */
SEXP r_scs_uncovered_set_boundary_2d(SEXP voter_xy, SEXP dist_cfg,
                                      SEXP grid_res, SEXP k) {
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    int n_voters = (int)(XLENGTH(voter_xy) / 2);
    char err[SCS_ERR_BUF_SIZE] = {0};
    int out_n;
    scs_check(scs_uncovered_set_boundary_size_2d(
                  REAL(voter_xy), n_voters, &dc, asInteger(grid_res),
                  asInteger(k), &out_n, err, SCS_ERR_BUF_SIZE),
              err);
    double *tmp = (double *)R_alloc((size_t)out_n * 2, sizeof(double));
    scs_check(scs_uncovered_set_boundary_2d(
                  REAL(voter_xy), n_voters, &dc, asInteger(grid_res),
                  asInteger(k), tmp, out_n, &out_n, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP mat = PROTECT(allocMatrix(REALSXP, out_n, 2));
    double *dst = REAL(mat);
    for (int i = 0; i < out_n; i++) {
        dst[i]         = tmp[2 * i];
        dst[out_n + i] = tmp[2 * i + 1];
    }
    SEXP cn = PROTECT(allocVector(STRSXP, 2));
    SET_STRING_ELT(cn, 0, mkChar("x"));
    SET_STRING_ELT(cn, 1, mkChar("y"));
    SEXP dn = PROTECT(allocVector(VECSXP, 2));
    SET_VECTOR_ELT(dn, 0, R_NilValue);
    SET_VECTOR_ELT(dn, 1, cn);
    setAttrib(mat, R_DimNamesSymbol, dn);
    UNPROTECT(3);
    return mat;
}

/* ---------------------------------------------------------------------------
 * Centrality measures — marginal median, centroid, geometric mean
 *
 * All three return list(x = numeric, y = numeric).
 * --------------------------------------------------------------------------- */

static SEXP make_xy_list(double x, double y) {
    SEXP result = PROTECT(allocVector(VECSXP, 2));
    SEXP names  = PROTECT(allocVector(STRSXP, 2));
    SET_VECTOR_ELT(result, 0, ScalarReal(x));
    SET_STRING_ELT(names,  0, mkChar("x"));
    SET_VECTOR_ELT(result, 1, ScalarReal(y));
    SET_STRING_ELT(names,  1, mkChar("y"));
    setAttrib(result, R_NamesSymbol, names);
    UNPROTECT(2);
    return result;
}

SEXP r_scs_marginal_median_2d(SEXP voter_xy) {
    int n_voters = (int)(XLENGTH(voter_xy) / 2);
    char err[SCS_ERR_BUF_SIZE] = {0};
    double cx, cy;
    scs_check(scs_marginal_median_2d(REAL(voter_xy), n_voters, &cx, &cy,
                                     err, SCS_ERR_BUF_SIZE), err);
    return make_xy_list(cx, cy);
}

SEXP r_scs_centroid_2d(SEXP voter_xy) {
    int n_voters = (int)(XLENGTH(voter_xy) / 2);
    char err[SCS_ERR_BUF_SIZE] = {0};
    double cx, cy;
    scs_check(scs_centroid_2d(REAL(voter_xy), n_voters, &cx, &cy,
                              err, SCS_ERR_BUF_SIZE), err);
    return make_xy_list(cx, cy);
}

SEXP r_scs_geometric_mean_2d(SEXP voter_xy) {
    int n_voters = (int)(XLENGTH(voter_xy) / 2);
    char err[SCS_ERR_BUF_SIZE] = {0};
    double cx, cy;
    scs_check(scs_geometric_mean_2d(REAL(voter_xy), n_voters, &cx, &cy,
                                    err, SCS_ERR_BUF_SIZE), err);
    return make_xy_list(cx, cy);
}
