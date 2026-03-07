/* stream_manager.c — .Call() wrappers for SCS_StreamManager.
 *
 * All entry points are registered in init.c.  Every function either returns
 * R_NilValue or a scalar/vector SEXP.  Error propagation is via scs_check()
 * → Rf_error() → R longjmp; no C++ exceptions cross this boundary.
 */

#include "scs_r_helpers.h"
#include <stdint.h>

/* ---------------------------------------------------------------------------
 * GC finalizer — called when the EXTPTR is collected by R's GC, and also
 * called explicitly by r_scs_stream_manager_destroy (R6 finalize).
 * R_ClearExternalPtr ensures the second call is a no-op.
 * --------------------------------------------------------------------------- */
static void stream_manager_finalizer(SEXP ptr) {
    SCS_StreamManager *mgr = (SCS_StreamManager *)R_ExternalPtrAddr(ptr);
    if (mgr) {
        scs_stream_manager_destroy(mgr);
        R_ClearExternalPtr(ptr);
    }
}

/* ---------------------------------------------------------------------------
 * Lifecycle
 * --------------------------------------------------------------------------- */

SEXP r_scs_stream_manager_create(SEXP seed_sexp) {
    char err[SCS_ERR_BUF_SIZE] = {0};
    /* R has no native uint64; represent seeds as double (exact up to 2^53). */
    uint64_t seed = (uint64_t)asReal(seed_sexp);
    SCS_StreamManager *mgr =
        scs_stream_manager_create(seed, err, SCS_ERR_BUF_SIZE);
    if (!mgr)
        Rf_error("%s", err[0] ? err : "scs_stream_manager_create returned NULL");
    SEXP ptr = PROTECT(R_MakeExternalPtr(mgr, R_NilValue, R_NilValue));
    R_RegisterCFinalizerEx(ptr, stream_manager_finalizer, FALSE);
    UNPROTECT(1);
    return ptr;
}

SEXP r_scs_stream_manager_destroy(SEXP ptr) {
    stream_manager_finalizer(ptr);
    return R_NilValue;
}

SEXP r_scs_register_streams(SEXP ptr, SEXP names_sexp) {
    SCS_StreamManager *mgr = (SCS_StreamManager *)get_extptr(ptr);
    if (!mgr)
        Rf_error("StreamManager handle is NULL or has already been destroyed.");
    int n = (int)XLENGTH(names_sexp);
    const char **names = (const char **)R_alloc(n, sizeof(char *));
    for (int i = 0; i < n; i++) names[i] = CHAR(STRING_ELT(names_sexp, i));
    char err[SCS_ERR_BUF_SIZE] = {0};
    scs_check(scs_register_streams(mgr, names, n, err, SCS_ERR_BUF_SIZE), err);
    return R_NilValue;
}

SEXP r_scs_reset_all(SEXP ptr, SEXP seed_sexp) {
    SCS_StreamManager *mgr = (SCS_StreamManager *)get_extptr(ptr);
    if (!mgr)
        Rf_error("StreamManager handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    scs_check(
        scs_reset_all(mgr, (uint64_t)asReal(seed_sexp), err, SCS_ERR_BUF_SIZE),
        err);
    return R_NilValue;
}

SEXP r_scs_reset_stream(SEXP ptr, SEXP name_sexp, SEXP seed_sexp) {
    SCS_StreamManager *mgr = (SCS_StreamManager *)get_extptr(ptr);
    if (!mgr)
        Rf_error("StreamManager handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    scs_check(scs_reset_stream(mgr, CHAR(STRING_ELT(name_sexp, 0)),
                               (uint64_t)asReal(seed_sexp), err,
                               SCS_ERR_BUF_SIZE),
              err);
    return R_NilValue;
}

SEXP r_scs_skip(SEXP ptr, SEXP name_sexp, SEXP n_sexp) {
    SCS_StreamManager *mgr = (SCS_StreamManager *)get_extptr(ptr);
    if (!mgr)
        Rf_error("StreamManager handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    scs_check(scs_skip(mgr, CHAR(STRING_ELT(name_sexp, 0)),
                       (uint64_t)asReal(n_sexp), err, SCS_ERR_BUF_SIZE),
              err);
    return R_NilValue;
}

/* ---------------------------------------------------------------------------
 * PRNG draws — each returns a scalar SEXP
 * --------------------------------------------------------------------------- */

SEXP r_scs_uniform_real(SEXP ptr, SEXP name, SEXP min_s, SEXP max_s) {
    SCS_StreamManager *mgr = (SCS_StreamManager *)get_extptr(ptr);
    if (!mgr)
        Rf_error("StreamManager handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    double out;
    scs_check(scs_uniform_real(mgr, CHAR(STRING_ELT(name, 0)), asReal(min_s),
                               asReal(max_s), &out, err, SCS_ERR_BUF_SIZE),
              err);
    return ScalarReal(out);
}

SEXP r_scs_normal(SEXP ptr, SEXP name, SEXP mean_s, SEXP sd_s) {
    SCS_StreamManager *mgr = (SCS_StreamManager *)get_extptr(ptr);
    if (!mgr)
        Rf_error("StreamManager handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    double out;
    scs_check(scs_normal(mgr, CHAR(STRING_ELT(name, 0)), asReal(mean_s),
                         asReal(sd_s), &out, err, SCS_ERR_BUF_SIZE),
              err);
    return ScalarReal(out);
}

SEXP r_scs_bernoulli(SEXP ptr, SEXP name, SEXP prob_s) {
    SCS_StreamManager *mgr = (SCS_StreamManager *)get_extptr(ptr);
    if (!mgr)
        Rf_error("StreamManager handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    int out;
    scs_check(scs_bernoulli(mgr, CHAR(STRING_ELT(name, 0)), asReal(prob_s),
                            &out, err, SCS_ERR_BUF_SIZE),
              err);
    return ScalarLogical(out);
}

SEXP r_scs_uniform_int(SEXP ptr, SEXP name, SEXP min_s, SEXP max_s) {
    SCS_StreamManager *mgr = (SCS_StreamManager *)get_extptr(ptr);
    if (!mgr)
        Rf_error("StreamManager handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    int64_t out;
    scs_check(scs_uniform_int(mgr, CHAR(STRING_ELT(name, 0)),
                              (int64_t)asReal(min_s), (int64_t)asReal(max_s),
                              &out, err, SCS_ERR_BUF_SIZE),
              err);
    /* Return as double to avoid int64 truncation (R integers are 32-bit). */
    return ScalarReal((double)out);
}

SEXP r_scs_uniform_choice(SEXP ptr, SEXP name, SEXP n_s) {
    SCS_StreamManager *mgr = (SCS_StreamManager *)get_extptr(ptr);
    if (!mgr)
        Rf_error("StreamManager handle is NULL or has already been destroyed.");
    char err[SCS_ERR_BUF_SIZE] = {0};
    int64_t out;
    scs_check(scs_uniform_choice(mgr, CHAR(STRING_ELT(name, 0)),
                                 (int64_t)asReal(n_s), &out, err,
                                 SCS_ERR_BUF_SIZE),
              err);
    /* Return 0-based index; the R wrapper translates to 1-based as needed. */
    return ScalarInteger((int)out);
}
