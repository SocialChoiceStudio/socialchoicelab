/* profile.c — .Call() wrappers for SCS_Profile.
 * snprintf() requires <stdio.h> which may not be pulled in transitively. */

#include <stdio.h>
 *
 * All factory functions return a new EXTPTR owning a heap-allocated profile.
 * Index translation: the C API is 0-based; R wrappers accept/return 1-based
 * voter and alternative indices.
 */

#include "scs_r_helpers.h"

/* ---------------------------------------------------------------------------
 * GC finalizer and EXTPTR wrapper helper
 * --------------------------------------------------------------------------- */

static void profile_finalizer(SEXP ptr) {
    SCS_Profile *p = (SCS_Profile *)R_ExternalPtrAddr(ptr);
    if (p) {
        scs_profile_destroy(p);
        R_ClearExternalPtr(ptr);
    }
}

static SEXP wrap_profile(SCS_Profile *p, const char *err) {
    if (!p)
        Rf_error("%s", err[0] ? err : "profile factory returned NULL");
    SEXP ptr = PROTECT(R_MakeExternalPtr(p, R_NilValue, R_NilValue));
    R_RegisterCFinalizerEx(ptr, profile_finalizer, FALSE);
    UNPROTECT(1);
    return ptr;
}

/* ---------------------------------------------------------------------------
 * Factory functions
 * --------------------------------------------------------------------------- */

SEXP r_scs_profile_build_spatial(SEXP alt_xy, SEXP voter_xy, SEXP dist_cfg) {
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    char err[SCS_ERR_BUF_SIZE] = {0};
    SCS_Profile *p = scs_profile_build_spatial(
        REAL(alt_xy), (int)(XLENGTH(alt_xy) / 2), REAL(voter_xy),
        (int)(XLENGTH(voter_xy) / 2), &dc, err, SCS_ERR_BUF_SIZE);
    return wrap_profile(p, err);
}

SEXP r_scs_profile_from_utility_matrix(SEXP utilities, SEXP n_voters_s,
                                        SEXP n_alts_s) {
    char err[SCS_ERR_BUF_SIZE] = {0};
    SCS_Profile *p = scs_profile_from_utility_matrix(
        REAL(utilities), asInteger(n_voters_s), asInteger(n_alts_s), err,
        SCS_ERR_BUF_SIZE);
    return wrap_profile(p, err);
}

SEXP r_scs_profile_impartial_culture(SEXP n_voters_s, SEXP n_alts_s,
                                      SEXP mgr_ptr, SEXP stream_name) {
    SCS_StreamManager *mgr = (SCS_StreamManager *)get_extptr(mgr_ptr);
    if (!mgr)
        Rf_error("StreamManager handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    SCS_Profile *p = scs_profile_impartial_culture(
        asInteger(n_voters_s), asInteger(n_alts_s), mgr,
        CHAR(STRING_ELT(stream_name, 0)), err, SCS_ERR_BUF_SIZE);
    return wrap_profile(p, err);
}

SEXP r_scs_profile_uniform_spatial(SEXP n_voters_s, SEXP n_alts_s, SEXP lo_s,
                                    SEXP hi_s, SEXP dist_cfg, SEXP mgr_ptr,
                                    SEXP stream_name) {
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    SCS_StreamManager *mgr = (SCS_StreamManager *)get_extptr(mgr_ptr);
    if (!mgr)
        Rf_error("StreamManager handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    /* Only 2D is supported in the current binding scope. */
    SCS_Profile *p = scs_profile_uniform_spatial(
        asInteger(n_voters_s), asInteger(n_alts_s), 2, asReal(lo_s),
        asReal(hi_s), &dc, mgr, CHAR(STRING_ELT(stream_name, 0)), err,
        SCS_ERR_BUF_SIZE);
    return wrap_profile(p, err);
}

SEXP r_scs_profile_gaussian_spatial(SEXP n_voters_s, SEXP n_alts_s,
                                     SEXP mean_s, SEXP stddev_s,
                                     SEXP dist_cfg, SEXP mgr_ptr,
                                     SEXP stream_name) {
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    SCS_StreamManager *mgr = (SCS_StreamManager *)get_extptr(mgr_ptr);
    if (!mgr)
        Rf_error("StreamManager handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    SCS_Profile *p = scs_profile_gaussian_spatial(
        asInteger(n_voters_s), asInteger(n_alts_s), 2, asReal(mean_s),
        asReal(stddev_s), &dc, mgr, CHAR(STRING_ELT(stream_name, 0)), err,
        SCS_ERR_BUF_SIZE);
    return wrap_profile(p, err);
}

/* ---------------------------------------------------------------------------
 * Lifecycle
 * --------------------------------------------------------------------------- */

SEXP r_scs_profile_destroy(SEXP ptr) {
    profile_finalizer(ptr);
    return R_NilValue;
}

SEXP r_scs_profile_clone(SEXP ptr) {
    SCS_Profile *p = (SCS_Profile *)get_extptr(ptr);
    if (!p) Rf_error("Profile handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    SCS_Profile *clone = scs_profile_clone(p, err, SCS_ERR_BUF_SIZE);
    return wrap_profile(clone, err);
}

/* ---------------------------------------------------------------------------
 * Inspection
 * --------------------------------------------------------------------------- */

SEXP r_scs_profile_dims(SEXP ptr) {
    SCS_Profile *p = (SCS_Profile *)get_extptr(ptr);
    if (!p) Rf_error("Profile handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    int n_voters, n_alts;
    scs_check(scs_profile_dims(p, &n_voters, &n_alts, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(VECSXP, 2));
    SEXP names = PROTECT(allocVector(STRSXP, 2));
    SET_VECTOR_ELT(result, 0, ScalarInteger(n_voters));
    SET_STRING_ELT(names, 0, mkChar("n_voters"));
    SET_VECTOR_ELT(result, 1, ScalarInteger(n_alts));
    SET_STRING_ELT(names, 1, mkChar("n_alts"));
    setAttrib(result, R_NamesSymbol, names);
    UNPROTECT(2);
    return result;
}

/* r_scs_profile_get_ranking — returns one voter's ranking as a 1-based integer
 * vector.  voter_0based is the 0-based voter index (the R wrapper does the
 * 1→0 translation before calling). */
SEXP r_scs_profile_get_ranking(SEXP ptr, SEXP voter_0based) {
    SCS_Profile *p = (SCS_Profile *)get_extptr(ptr);
    if (!p) Rf_error("Profile handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    int n_voters, n_alts;
    scs_check(scs_profile_dims(p, &n_voters, &n_alts, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(INTSXP, n_alts));
    scs_check(scs_profile_get_ranking(p, asInteger(voter_0based),
                                      INTEGER(result), n_alts, err,
                                      SCS_ERR_BUF_SIZE),
              err);
    /* Translate 0-based alternative indices to 1-based for R. */
    for (int i = 0; i < n_alts; i++) INTEGER(result)[i]++;
    UNPROTECT(1);
    return result;
}

/* r_scs_profile_export_rankings — returns all rankings as an n_voters × n_alts
 * integer matrix (1-based alternative indices).
 *
 * C API layout (row-major): out[voter * n_alts + rank] = alt_idx (0-based).
 * R matrix layout (column-major): mat[voter, rank] at offset voter + rank*n_voters.
 */
SEXP r_scs_profile_export_rankings(SEXP ptr) {
    SCS_Profile *p = (SCS_Profile *)get_extptr(ptr);
    if (!p) Rf_error("Profile handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    int n_voters, n_alts;
    scs_check(scs_profile_dims(p, &n_voters, &n_alts, err, SCS_ERR_BUF_SIZE),
              err);
    int *tmp = (int *)R_alloc((size_t)n_voters * n_alts, sizeof(int));
    scs_check(
        scs_profile_export_rankings(p, tmp, n_voters * n_alts, err,
                                    SCS_ERR_BUF_SIZE),
        err);
    SEXP result = PROTECT(allocMatrix(INTSXP, n_voters, n_alts));
    int *dst = INTEGER(result);
    for (int v = 0; v < n_voters; v++) {
        for (int r = 0; r < n_alts; r++) {
            /* column-major: [v, r] → v + r*n_voters */
            dst[v + r * n_voters] = tmp[v * n_alts + r] + 1; /* 0→1-based */
        }
    }
    /* Set column names to rank numbers and row names to voter numbers. */
    SEXP colnames = PROTECT(allocVector(STRSXP, n_alts));
    SEXP rownames = PROTECT(allocVector(STRSXP, n_voters));
    char buf[32];
    for (int r = 0; r < n_alts; r++) {
        snprintf(buf, sizeof(buf), "rank%d", r + 1);
        SET_STRING_ELT(colnames, r, mkChar(buf));
    }
    for (int v = 0; v < n_voters; v++) {
        snprintf(buf, sizeof(buf), "voter%d", v + 1);
        SET_STRING_ELT(rownames, v, mkChar(buf));
    }
    SEXP dimnames = PROTECT(allocVector(VECSXP, 2));
    SET_VECTOR_ELT(dimnames, 0, rownames);
    SET_VECTOR_ELT(dimnames, 1, colnames);
    setAttrib(result, R_DimNamesSymbol, dimnames);
    UNPROTECT(4); /* result, colnames, rownames, dimnames */
    return result;
}
