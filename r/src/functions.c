/* functions.c — .Call() wrappers for:
 *   B3.1  API version
 *   B3.2  Distance, loss, and level-set functions
 *   B3.3  Convex hull and majority-preference functions
 */

#include <string.h>
#include "scs_r_helpers.h"

/* ---------------------------------------------------------------------------
 * B3.1 — API version
 * --------------------------------------------------------------------------- */

SEXP r_scs_api_version(SEXP ignored) {
    (void)ignored;
    char err[SCS_ERR_BUF_SIZE] = {0};
    int major, minor, patch;
    scs_check(scs_api_version(&major, &minor, &patch, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(VECSXP, 3));
    SEXP names = PROTECT(allocVector(STRSXP, 3));
    SET_VECTOR_ELT(result, 0, ScalarInteger(major));
    SET_STRING_ELT(names, 0, mkChar("major"));
    SET_VECTOR_ELT(result, 1, ScalarInteger(minor));
    SET_STRING_ELT(names, 1, mkChar("minor"));
    SET_VECTOR_ELT(result, 2, ScalarInteger(patch));
    SET_STRING_ELT(names, 2, mkChar("patch"));
    setAttrib(result, R_NamesSymbol, names);
    UNPROTECT(2);
    return result;
}

/* ---------------------------------------------------------------------------
 * B3.2 — Distance functions
 * --------------------------------------------------------------------------- */

SEXP r_scs_calculate_distance(SEXP ideal, SEXP alt, SEXP dist_cfg) {
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    char err[SCS_ERR_BUF_SIZE] = {0};
    double out;
    scs_check(scs_calculate_distance(REAL(ideal), REAL(alt), dc.n_weights, &dc,
                                     &out, err, SCS_ERR_BUF_SIZE),
              err);
    return ScalarReal(out);
}

SEXP r_scs_distance_to_utility(SEXP distance, SEXP loss_cfg) {
    SCS_LossConfig lc = build_loss_config(loss_cfg);
    char err[SCS_ERR_BUF_SIZE] = {0};
    double out;
    scs_check(scs_distance_to_utility(asReal(distance), &lc, &out, err,
                                      SCS_ERR_BUF_SIZE),
              err);
    return ScalarReal(out);
}

SEXP r_scs_normalize_utility(SEXP utility, SEXP max_distance, SEXP loss_cfg) {
    SCS_LossConfig lc = build_loss_config(loss_cfg);
    char err[SCS_ERR_BUF_SIZE] = {0};
    double out;
    scs_check(scs_normalize_utility(asReal(utility), asReal(max_distance), &lc,
                                    &out, err, SCS_ERR_BUF_SIZE),
              err);
    return ScalarReal(out);
}

/* ---------------------------------------------------------------------------
 * B3.2 — Level-set functions
 * --------------------------------------------------------------------------- */

SEXP r_scs_level_set_1d(SEXP ideal, SEXP weight, SEXP utility_level,
                         SEXP loss_cfg) {
    SCS_LossConfig lc = build_loss_config(loss_cfg);
    char err[SCS_ERR_BUF_SIZE] = {0};
    double pts[2];
    int n;
    scs_check(scs_level_set_1d(asReal(ideal), asReal(weight),
                               asReal(utility_level), &lc, pts, &n, err,
                               SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(REALSXP, n));
    for (int i = 0; i < n; i++) REAL(result)[i] = pts[i];
    UNPROTECT(1);
    return result;
}

static const char *level_set_type_name(SCS_LevelSetType t) {
    switch (t) {
    case SCS_LEVEL_SET_CIRCLE:      return "circle";
    case SCS_LEVEL_SET_ELLIPSE:     return "ellipse";
    case SCS_LEVEL_SET_SUPERELLIPSE: return "superellipse";
    case SCS_LEVEL_SET_POLYGON:     return "polygon";
    default:                        return "unknown";
    }
}

/* r_scs_level_set_2d — returns a 7-element named list representing
 * an SCS_LevelSet2d:
 *   type (character), center_x, center_y, param0, param1,
 *   exponent_p (numerics), vertices (n×2 matrix or NULL). */
SEXP r_scs_level_set_2d(SEXP ideal_x, SEXP ideal_y, SEXP util, SEXP loss_cfg,
                         SEXP dist_cfg) {
    SCS_LossConfig lc = build_loss_config(loss_cfg);
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    SCS_LevelSet2d ls;
    memset(&ls, 0, sizeof(ls));
    char err[SCS_ERR_BUF_SIZE] = {0};
    scs_check(scs_level_set_2d(asReal(ideal_x), asReal(ideal_y), asReal(util),
                               &lc, &dc, &ls, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(VECSXP, 7));
    SEXP names = PROTECT(allocVector(STRSXP, 7));
    SET_VECTOR_ELT(result, 0, mkString(level_set_type_name(ls.type)));
    SET_STRING_ELT(names, 0, mkChar("type"));
    SET_VECTOR_ELT(result, 1, ScalarReal(ls.center_x));
    SET_STRING_ELT(names, 1, mkChar("center_x"));
    SET_VECTOR_ELT(result, 2, ScalarReal(ls.center_y));
    SET_STRING_ELT(names, 2, mkChar("center_y"));
    SET_VECTOR_ELT(result, 3, ScalarReal(ls.param0));
    SET_STRING_ELT(names, 3, mkChar("param0"));
    SET_VECTOR_ELT(result, 4, ScalarReal(ls.param1));
    SET_STRING_ELT(names, 4, mkChar("param1"));
    SET_VECTOR_ELT(result, 5, ScalarReal(ls.exponent_p));
    SET_STRING_ELT(names, 5, mkChar("exponent_p"));
    if (ls.type == SCS_LEVEL_SET_POLYGON && ls.n_vertices > 0) {
        /* De-interleave [x0,y0,...] into n_vertices × 2 column-major matrix. */
        SEXP vmat = PROTECT(allocMatrix(REALSXP, ls.n_vertices, 2));
        double *dst = REAL(vmat);
        for (int i = 0; i < ls.n_vertices; i++) {
            dst[i]                 = ls.vertices[2 * i];
            dst[ls.n_vertices + i] = ls.vertices[2 * i + 1];
        }
        SET_VECTOR_ELT(result, 6, vmat);
        UNPROTECT(1);
    } else {
        SET_VECTOR_ELT(result, 6, R_NilValue);
    }
    SET_STRING_ELT(names, 6, mkChar("vertices"));
    setAttrib(result, R_NamesSymbol, names);
    UNPROTECT(2);
    return result;
}

/* r_scs_level_set_to_polygon — reconstruct SCS_LevelSet2d from the R list
 * produced by r_scs_level_set_2d, then sample it as an n×2 vertex matrix.
 *
 * List element order matches r_scs_level_set_2d:
 *   [[1]] type (character)
 *   [[2]] center_x  [[3]] center_y  [[4]] param0  [[5]] param1
 *   [[6]] exponent_p  [[7]] vertices (matrix or NULL)
 */
SEXP r_scs_level_set_to_polygon(SEXP ls_list, SEXP n_samples_s) {
    SCS_LevelSet2d ls;
    memset(&ls, 0, sizeof(ls));
    const char *tstr = CHAR(STRING_ELT(VECTOR_ELT(ls_list, 0), 0));
    if      (strcmp(tstr, "circle")      == 0) ls.type = SCS_LEVEL_SET_CIRCLE;
    else if (strcmp(tstr, "ellipse")     == 0) ls.type = SCS_LEVEL_SET_ELLIPSE;
    else if (strcmp(tstr, "superellipse")== 0) ls.type = SCS_LEVEL_SET_SUPERELLIPSE;
    else if (strcmp(tstr, "polygon")     == 0) ls.type = SCS_LEVEL_SET_POLYGON;
    else Rf_error("level_set_to_polygon: unknown type '%s'.", tstr);
    ls.center_x  = REAL(VECTOR_ELT(ls_list, 1))[0];
    ls.center_y  = REAL(VECTOR_ELT(ls_list, 2))[0];
    ls.param0    = REAL(VECTOR_ELT(ls_list, 3))[0];
    ls.param1    = REAL(VECTOR_ELT(ls_list, 4))[0];
    ls.exponent_p= REAL(VECTOR_ELT(ls_list, 5))[0];
    SEXP vmat = VECTOR_ELT(ls_list, 6);
    if (ls.type == SCS_LEVEL_SET_POLYGON && vmat != R_NilValue) {
        int nv = nrows(vmat);
        ls.n_vertices = nv;
        double *src = REAL(vmat);
        for (int i = 0; i < nv && i < 4; i++) {
            ls.vertices[2*i]   = src[i];       /* column 1: x */
            ls.vertices[2*i+1] = src[nv + i];  /* column 2: y */
        }
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    /* Size query. */
    int out_n;
    scs_check(scs_level_set_to_polygon(&ls, asInteger(n_samples_s), NULL, 0,
                                       &out_n, err, SCS_ERR_BUF_SIZE),
              err);
    /* Fill into temporary buffer, then de-interleave. */
    double *tmp = (double *)R_alloc((size_t)out_n * 2, sizeof(double));
    scs_check(scs_level_set_to_polygon(&ls, asInteger(n_samples_s), tmp, out_n,
                                       &out_n, err, SCS_ERR_BUF_SIZE),
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

/* r_scs_ic_polygon_2d — compound: distance → utility → level set → polygon.
 * Arguments: ideal_x, ideal_y, sq_x, sq_y, loss_cfg, dist_cfg, n_samples.
 * Returns an n×2 column-major matrix (x, y columns). */
SEXP r_scs_ic_polygon_2d(SEXP ideal_x, SEXP ideal_y, SEXP sq_x, SEXP sq_y,
                          SEXP loss_cfg, SEXP dist_cfg, SEXP n_samples_s) {
    SCS_LossConfig lc = build_loss_config(loss_cfg);
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    int ns = asInteger(n_samples_s);
    char err[SCS_ERR_BUF_SIZE] = {0};
    /* Size query. */
    int out_n;
    scs_check(scs_ic_polygon_2d(asReal(ideal_x), asReal(ideal_y),
                                asReal(sq_x), asReal(sq_y),
                                &lc, &dc, ns, NULL, 0, &out_n,
                                err, SCS_ERR_BUF_SIZE),
              err);
    /* Fill into temporary buffer, then de-interleave into column-major matrix. */
    double *tmp = (double *)R_alloc((size_t)out_n * 2, sizeof(double));
    scs_check(scs_ic_polygon_2d(asReal(ideal_x), asReal(ideal_y),
                                asReal(sq_x), asReal(sq_y),
                                &lc, &dc, ns, tmp, out_n, &out_n,
                                err, SCS_ERR_BUF_SIZE),
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
 * B3.3 — Convex hull
 * --------------------------------------------------------------------------- */

/* r_scs_convex_hull_2d — points: flat [x0,y0,x1,y1,...] vector.
 * Returns an n_hull × 2 column-major matrix (CCW order). */
SEXP r_scs_convex_hull_2d(SEXP points) {
    int n = (int)(XLENGTH(points) / 2);
    char err[SCS_ERR_BUF_SIZE] = {0};
    double *out_xy = (double *)R_alloc((size_t)n * 2, sizeof(double));
    int out_n;
    scs_check(scs_convex_hull_2d(REAL(points), n, out_xy, &out_n, err,
                                 SCS_ERR_BUF_SIZE),
              err);
    SEXP mat = PROTECT(allocMatrix(REALSXP, out_n, 2));
    double *dst = REAL(mat);
    for (int i = 0; i < out_n; i++) {
        dst[i]         = out_xy[2 * i];
        dst[out_n + i] = out_xy[2 * i + 1];
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
 * B3.3 — Majority preference
 * --------------------------------------------------------------------------- */

SEXP r_scs_majority_prefers_2d(SEXP ax, SEXP ay, SEXP bx, SEXP by,
                                SEXP voter_xy, SEXP dist_cfg, SEXP k) {
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    int n_voters = (int)(XLENGTH(voter_xy) / 2);
    char err[SCS_ERR_BUF_SIZE] = {0};
    int out;
    scs_check(scs_majority_prefers_2d(asReal(ax), asReal(ay), asReal(bx),
                                      asReal(by), REAL(voter_xy), n_voters,
                                      &dc, asInteger(k), &out, err,
                                      SCS_ERR_BUF_SIZE),
              err);
    return ScalarLogical(out);
}

/* r_scs_pairwise_matrix_2d — returns an n_alts × n_alts integer matrix.
 * C API returns row-major; R stores column-major. Transposition is done here.
 * Values: SCS_PAIRWISE_WIN=1, SCS_PAIRWISE_TIE=0, SCS_PAIRWISE_LOSS=-1. */
SEXP r_scs_pairwise_matrix_2d(SEXP alt_xy, SEXP voter_xy, SEXP dist_cfg,
                               SEXP k) {
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    int n_alts   = (int)(XLENGTH(alt_xy) / 2);
    int n_voters = (int)(XLENGTH(voter_xy) / 2);
    char err[SCS_ERR_BUF_SIZE] = {0};
    SCS_PairwiseResult *tmp =
        (SCS_PairwiseResult *)R_alloc((size_t)n_alts * n_alts,
                                      sizeof(SCS_PairwiseResult));
    scs_check(scs_pairwise_matrix_2d(REAL(alt_xy), n_alts, REAL(voter_xy),
                                     n_voters, &dc, asInteger(k), tmp,
                                     n_alts * n_alts, err, SCS_ERR_BUF_SIZE),
              err);
    /* Row-major tmp[r*n+c] → column-major dst[r + c*n]. */
    SEXP mat = PROTECT(allocMatrix(INTSXP, n_alts, n_alts));
    int *dst = INTEGER(mat);
    for (int r = 0; r < n_alts; r++)
        for (int c = 0; c < n_alts; c++)
            dst[r + c * n_alts] = (int)tmp[r * n_alts + c];
    UNPROTECT(1);
    return mat;
}

SEXP r_scs_weighted_majority_prefers_2d(SEXP ax, SEXP ay, SEXP bx, SEXP by,
                                         SEXP voter_xy, SEXP weights,
                                         SEXP dist_cfg, SEXP threshold) {
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    int n_voters = (int)(XLENGTH(voter_xy) / 2);
    char err[SCS_ERR_BUF_SIZE] = {0};
    int out;
    scs_check(scs_weighted_majority_prefers_2d(
                  asReal(ax), asReal(ay), asReal(bx), asReal(by),
                  REAL(voter_xy), n_voters, REAL(weights), &dc,
                  asReal(threshold), &out, err, SCS_ERR_BUF_SIZE),
              err);
    return ScalarLogical(out);
}
