/* scs_r_helpers.h — internal C helpers shared by all R binding translation units.
 *
 * Include this header (instead of scs_api.h directly) in every .c file in
 * r/src/ that calls C API functions.  It provides:
 *
 *   SCS_ERR_BUF_SIZE   — standard error buffer length
 *   scs_check()        — map C API return codes to Rf_error()
 *   build_dist_config() — extract SCS_DistanceConfig from an R list
 *   build_loss_config() — extract SCS_LossConfig from an R list
 *   get_extptr()        — safely extract a C pointer from an EXTPTR SEXP
 */

#pragma once

#include <R.h>
#include <Rinternals.h>
#include "scs_api.h"

/* Standard error buffer size used by every .Call() wrapper. */
#define SCS_ERR_BUF_SIZE 512

/* scs_check — raise an R error if rc != SCS_OK.
 *
 * Defined static inline so that every translation unit that includes this
 * header compiles its own copy without linker conflicts.
 *
 * This function either returns (when rc == SCS_OK) or longjmps (via
 * Rf_error) and never returns to the caller when rc != SCS_OK. */
static inline void scs_check(int rc, const char *err_buf) {
    if (rc == SCS_OK) return;
    Rf_error("%s",
             (err_buf && err_buf[0]) ? err_buf : "(no error message from C API)");
}

/* build_dist_config — build SCS_DistanceConfig from the R list produced by
 * make_dist_config() in R/utils.R.
 *
 * List element order is fixed by contract between make_dist_config() and this
 * function; the list must NOT be reordered in R:
 *   [[1]] distance_type  integer(1)
 *   [[2]] weights        numeric vector  (pointer borrowed — keep list PROTECT'd)
 *   [[3]] n_weights      integer(1)      (redundant; derived from weights length)
 *   [[4]] order_p        numeric(1)
 */
static inline SCS_DistanceConfig build_dist_config(SEXP dcfg) {
    SCS_DistanceConfig cfg;
    cfg.distance_type    = (SCS_DistanceType)INTEGER(VECTOR_ELT(dcfg, 0))[0];
    SEXP w               = VECTOR_ELT(dcfg, 1);
    cfg.salience_weights = REAL(w);
    cfg.n_weights        = (int)XLENGTH(w);
    cfg.order_p          = REAL(VECTOR_ELT(dcfg, 3))[0];
    return cfg;
}

/* build_loss_config — build SCS_LossConfig from the R list produced by
 * make_loss_config() in R/utils.R.
 *
 * List element order is fixed by contract:
 *   [[1]] loss_type    integer(1)
 *   [[2]] sensitivity  numeric(1)
 *   [[3]] max_loss     numeric(1)
 *   [[4]] steepness    numeric(1)
 *   [[5]] threshold    numeric(1)
 */
static inline SCS_LossConfig build_loss_config(SEXP lcfg) {
    SCS_LossConfig cfg;
    cfg.loss_type   = (SCS_LossType)INTEGER(VECTOR_ELT(lcfg, 0))[0];
    cfg.sensitivity = REAL(VECTOR_ELT(lcfg, 1))[0];
    cfg.max_loss    = REAL(VECTOR_ELT(lcfg, 2))[0];
    cfg.steepness   = REAL(VECTOR_ELT(lcfg, 3))[0];
    cfg.threshold   = REAL(VECTOR_ELT(lcfg, 4))[0];
    return cfg;
}

/* get_extptr — extract the C pointer from an EXTPTR SEXP.
 * Returns NULL if ptr has been cleared (R_ClearExternalPtr) or is R_NilValue. */
static inline void *get_extptr(SEXP ptr) {
    if (ptr == R_NilValue) return NULL;
    return R_ExternalPtrAddr(ptr);
}
