/* init.c — DLL initialisation for socialchoicelab R package.
 *
 * Registers all .Call() entry points and disables dynamic symbol lookup so
 * that R requires every symbol to be explicitly registered here.
 *
 * Add one R_CallMethodDef entry per .Call() wrapper as functions are
 * implemented in subsequent phases (B2, B3).
 */

#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>
#include "scs_api.h"

/* ---------------------------------------------------------------------------
 * Error-handling convention (B1.2)
 *
 * Every .Call() wrapper allocates a char err_buf[SCS_ERR_BUF_SIZE] on the
 * stack, passes it to the C API call, then calls scs_check().  If the return
 * code is anything other than SCS_OK, scs_check() calls Rf_error(), which
 * longjmps to R's error handler — the wrapper never sees control again.
 *
 * Usage pattern in a wrapper:
 *
 *   char err[SCS_ERR_BUF_SIZE] = {0};
 *   int rc = scs_some_function(..., err, SCS_ERR_BUF_SIZE);
 *   scs_check(rc, err);
 *
 * SCS_ERR_BUF_SIZE is defined here rather than in scs_api.h because it is
 * a binding-layer convention, not part of the public C API contract.
 * --------------------------------------------------------------------------- */

#define SCS_ERR_BUF_SIZE 512

/* scs_check — raise an R error if rc != SCS_OK.
 *
 * This function either returns (when rc == SCS_OK) or longjmps (via
 * Rf_error) and never returns to the caller. */
static void scs_check(int rc, const char* err_buf) {
    if (rc == SCS_OK) return;
    Rf_error("%s", (err_buf && err_buf[0]) ? err_buf
                                           : "(no error message from C API)");
}

/* ---------------------------------------------------------------------------
 * .Call() entry point table — populated in phases B2 and B3.
 * --------------------------------------------------------------------------- */

static const R_CallMethodDef call_methods[] = {
    {NULL, NULL, 0}  /* sentinel */
};

void R_init_socialchoicelab(DllInfo* dll) {
    R_registerRoutines(dll, NULL, call_methods, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
    R_forceSymbols(dll, TRUE);
}
