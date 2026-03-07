/* winset.c — .Call() wrappers for SCS_Winset.
 *
 * Factory functions return a new EXTPTR that owns a heap-allocated SCS_Winset.
 * The GC finalizer (winset_finalizer) is the primary owner; the R6 finalize()
 * method calls r_scs_winset_destroy which is a safe no-op if already freed.
 */

#include "scs_r_helpers.h"

/* ---------------------------------------------------------------------------
 * GC finalizer and EXTPTR wrapper helper
 * --------------------------------------------------------------------------- */

static void winset_finalizer(SEXP ptr) {
    SCS_Winset *ws = (SCS_Winset *)R_ExternalPtrAddr(ptr);
    if (ws) {
        scs_winset_destroy(ws);
        R_ClearExternalPtr(ptr);
    }
}

/* Wrap a freshly created SCS_Winset* into a protected EXTPTR and return it.
 * Calls Rf_error if ws == NULL (factory returned NULL on error). */
static SEXP wrap_winset(SCS_Winset *ws, const char *err) {
    if (!ws)
        Rf_error("%s", err[0] ? err : "winset factory returned NULL");
    SEXP ptr = PROTECT(R_MakeExternalPtr(ws, R_NilValue, R_NilValue));
    R_RegisterCFinalizerEx(ptr, winset_finalizer, FALSE);
    UNPROTECT(1);
    return ptr;
}

/* ---------------------------------------------------------------------------
 * Factory functions
 * --------------------------------------------------------------------------- */

SEXP r_scs_winset_2d(SEXP sqx, SEXP sqy, SEXP voter_xy, SEXP k_s,
                     SEXP samples_s, SEXP dist_cfg) {
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    char err[SCS_ERR_BUF_SIZE] = {0};
    int n_voters = (int)(XLENGTH(voter_xy) / 2);
    SCS_Winset *ws =
        scs_winset_2d(asReal(sqx), asReal(sqy), REAL(voter_xy), n_voters, &dc,
                      asInteger(k_s), asInteger(samples_s), err,
                      SCS_ERR_BUF_SIZE);
    return wrap_winset(ws, err);
}

SEXP r_scs_weighted_winset_2d(SEXP sqx, SEXP sqy, SEXP voter_xy, SEXP weights,
                               SEXP threshold_s, SEXP samples_s,
                               SEXP dist_cfg) {
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    char err[SCS_ERR_BUF_SIZE] = {0};
    int n_voters = (int)(XLENGTH(voter_xy) / 2);
    SCS_Winset *ws = scs_weighted_winset_2d(
        asReal(sqx), asReal(sqy), REAL(voter_xy), n_voters, REAL(weights), &dc,
        asReal(threshold_s), asInteger(samples_s), err, SCS_ERR_BUF_SIZE);
    return wrap_winset(ws, err);
}

SEXP r_scs_winset_with_veto_2d(SEXP sqx, SEXP sqy, SEXP voter_xy,
                                SEXP veto_xy, SEXP k_s, SEXP samples_s,
                                SEXP dist_cfg) {
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    char err[SCS_ERR_BUF_SIZE] = {0};
    int n_voters = (int)(XLENGTH(voter_xy) / 2);
    int n_veto = (int)(XLENGTH(veto_xy) / 2);
    const double *veto_ptr = (n_veto > 0) ? REAL(veto_xy) : NULL;
    SCS_Winset *ws = scs_winset_with_veto_2d(
        asReal(sqx), asReal(sqy), REAL(voter_xy), n_voters, &dc, veto_ptr,
        n_veto, asInteger(k_s), asInteger(samples_s), err, SCS_ERR_BUF_SIZE);
    return wrap_winset(ws, err);
}

/* ---------------------------------------------------------------------------
 * Lifecycle
 * --------------------------------------------------------------------------- */

SEXP r_scs_winset_destroy(SEXP ptr) {
    winset_finalizer(ptr);
    return R_NilValue;
}

SEXP r_scs_winset_clone(SEXP ptr) {
    SCS_Winset *ws = (SCS_Winset *)get_extptr(ptr);
    if (!ws) Rf_error("Winset handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    SCS_Winset *clone = scs_winset_clone(ws, err, SCS_ERR_BUF_SIZE);
    return wrap_winset(clone, err);
}

/* ---------------------------------------------------------------------------
 * Query functions
 * --------------------------------------------------------------------------- */

SEXP r_scs_winset_is_empty(SEXP ptr) {
    SCS_Winset *ws = (SCS_Winset *)get_extptr(ptr);
    if (!ws) Rf_error("Winset handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    int out;
    scs_check(scs_winset_is_empty(ws, &out, err, SCS_ERR_BUF_SIZE), err);
    return ScalarLogical(out);
}

SEXP r_scs_winset_contains_point_2d(SEXP ptr, SEXP x, SEXP y) {
    SCS_Winset *ws = (SCS_Winset *)get_extptr(ptr);
    if (!ws) Rf_error("Winset handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    int out;
    scs_check(scs_winset_contains_point_2d(ws, asReal(x), asReal(y), &out, err,
                                           SCS_ERR_BUF_SIZE),
              err);
    return ScalarLogical(out);
}

SEXP r_scs_winset_bbox_2d(SEXP ptr) {
    SCS_Winset *ws = (SCS_Winset *)get_extptr(ptr);
    if (!ws) Rf_error("Winset handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    int found;
    double min_x, min_y, max_x, max_y;
    scs_check(scs_winset_bbox_2d(ws, &found, &min_x, &min_y, &max_x, &max_y,
                                 err, SCS_ERR_BUF_SIZE),
              err);
    if (!found) return R_NilValue;
    SEXP result = PROTECT(allocVector(VECSXP, 4));
    SEXP names = PROTECT(allocVector(STRSXP, 4));
    SET_VECTOR_ELT(result, 0, ScalarReal(min_x));
    SET_STRING_ELT(names, 0, mkChar("min_x"));
    SET_VECTOR_ELT(result, 1, ScalarReal(min_y));
    SET_STRING_ELT(names, 1, mkChar("min_y"));
    SET_VECTOR_ELT(result, 2, ScalarReal(max_x));
    SET_STRING_ELT(names, 2, mkChar("max_x"));
    SET_VECTOR_ELT(result, 3, ScalarReal(max_y));
    SET_STRING_ELT(names, 3, mkChar("max_y"));
    setAttrib(result, R_NamesSymbol, names);
    UNPROTECT(2);
    return result;
}

/* r_scs_winset_boundary_2d — two-call pattern hidden from the user.
 *
 * Returns a named list:
 *   $xy           numeric matrix (n_vertices × 2), columns named "x" and "y"
 *   $path_starts  integer vector of 1-based row offsets where each path starts
 *   $is_hole      logical vector; TRUE if the path is a hole
 */
SEXP r_scs_winset_boundary_2d(SEXP ptr) {
    SCS_Winset *ws = (SCS_Winset *)get_extptr(ptr);
    if (!ws) Rf_error("Winset handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};

    /* 1. Query sizes. */
    int n_xy_pairs, n_paths;
    scs_check(scs_winset_boundary_size_2d(ws, &n_xy_pairs, &n_paths, err,
                                          SCS_ERR_BUF_SIZE),
              err);

    /* 2. Allocate temporary C buffers (R_alloc — freed after .Call returns). */
    double *tmp_xy = (double *)R_alloc((size_t)n_xy_pairs * 2, sizeof(double));
    int *tmp_starts = (int *)R_alloc((size_t)n_paths, sizeof(int));
    int *tmp_holes = (int *)R_alloc((size_t)n_paths, sizeof(int));

    int out_xy_n, out_n_paths;
    scs_check(scs_winset_sample_boundary_2d(ws, tmp_xy, n_xy_pairs, &out_xy_n,
                                            tmp_starts, n_paths, tmp_holes,
                                            &out_n_paths, err, SCS_ERR_BUF_SIZE),
              err);

    /* 3. De-interleave [x0,y0,x1,y1,...] into an out_xy_n × 2 column-major
     *    matrix (column 1 = x coords, column 2 = y coords). */
    SEXP xy_mat = PROTECT(allocMatrix(REALSXP, out_xy_n, 2));
    double *dst = REAL(xy_mat);
    for (int i = 0; i < out_xy_n; i++) {
        dst[i]             = tmp_xy[2 * i];      /* column 1: x */
        dst[out_xy_n + i]  = tmp_xy[2 * i + 1]; /* column 2: y */
    }
    SEXP colnames = PROTECT(allocVector(STRSXP, 2));
    SET_STRING_ELT(colnames, 0, mkChar("x"));
    SET_STRING_ELT(colnames, 1, mkChar("y"));
    SEXP dimnames = PROTECT(allocVector(VECSXP, 2));
    SET_VECTOR_ELT(dimnames, 0, R_NilValue);
    SET_VECTOR_ELT(dimnames, 1, colnames);
    setAttrib(xy_mat, R_DimNamesSymbol, dimnames);
    UNPROTECT(2); /* colnames, dimnames (now referenced by xy_mat) */

    /* 4. Convert path_starts from 0-based to 1-based. */
    SEXP starts_sexp = PROTECT(allocVector(INTSXP, out_n_paths));
    for (int i = 0; i < out_n_paths; i++)
        INTEGER(starts_sexp)[i] = tmp_starts[i] + 1;

    SEXP is_hole_sexp = PROTECT(allocVector(LGLSXP, out_n_paths));
    for (int i = 0; i < out_n_paths; i++)
        LOGICAL(is_hole_sexp)[i] = tmp_holes[i];

    /* 5. Build and return the named list. */
    SEXP result = PROTECT(allocVector(VECSXP, 3));
    SEXP rnames = PROTECT(allocVector(STRSXP, 3));
    SET_VECTOR_ELT(result, 0, xy_mat);
    SET_STRING_ELT(rnames, 0, mkChar("xy"));
    SET_VECTOR_ELT(result, 1, starts_sexp);
    SET_STRING_ELT(rnames, 1, mkChar("path_starts"));
    SET_VECTOR_ELT(result, 2, is_hole_sexp);
    SET_STRING_ELT(rnames, 2, mkChar("is_hole"));
    setAttrib(result, R_NamesSymbol, rnames);
    UNPROTECT(6); /* xy_mat, starts_sexp, is_hole_sexp, result, rnames */
    return result;
}

/* ---------------------------------------------------------------------------
 * Boolean set operations — each returns a new owned EXTPTR
 * --------------------------------------------------------------------------- */

typedef SCS_Winset *(*winset_bool_fn)(const SCS_Winset *, const SCS_Winset *,
                                      char *, int);

static SEXP winset_bool_op(SEXP a, SEXP b, winset_bool_fn op,
                           const char *opname) {
    SCS_Winset *wa = (SCS_Winset *)get_extptr(a);
    SCS_Winset *wb = (SCS_Winset *)get_extptr(b);
    if (!wa) Rf_error("%s: first Winset handle is NULL.", opname);
    if (!wb) Rf_error("%s: second Winset handle is NULL.", opname);
    char err[SCS_ERR_BUF_SIZE] = {0};
    SCS_Winset *result = op(wa, wb, err, SCS_ERR_BUF_SIZE);
    return wrap_winset(result, err);
}

SEXP r_scs_winset_union(SEXP a, SEXP b) {
    return winset_bool_op(a, b, scs_winset_union, "winset_union");
}
SEXP r_scs_winset_intersection(SEXP a, SEXP b) {
    return winset_bool_op(a, b, scs_winset_intersection, "winset_intersection");
}
SEXP r_scs_winset_difference(SEXP a, SEXP b) {
    return winset_bool_op(a, b, scs_winset_difference, "winset_difference");
}
SEXP r_scs_winset_symmetric_difference(SEXP a, SEXP b) {
    return winset_bool_op(a, b, scs_winset_symmetric_difference,
                          "winset_symmetric_difference");
}
