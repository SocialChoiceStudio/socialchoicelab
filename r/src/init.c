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

/* Entry point table — populated in phases B2 and B3. */
static const R_CallMethodDef call_methods[] = {
    {NULL, NULL, 0}  /* sentinel */
};

void R_init_socialchoicelab(DllInfo* dll) {
    R_registerRoutines(dll, NULL, call_methods, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
    R_forceSymbols(dll, TRUE);
}
