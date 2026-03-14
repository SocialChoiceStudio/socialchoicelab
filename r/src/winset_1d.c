/* winset_1d.c — R wrapper for scs_winset_interval_1d.
 *
 * Used by the 1D competition canvas payload builder. Given a numeric voter
 * vector and a scalar seat position, returns a numeric(2) of [lo, hi], or
 * numeric(2) of [NaN, NaN] if the WinSet is empty.
 */

#include <math.h>
#include <R.h>
#include <Rinternals.h>
#include "scs_r_helpers.h"

/* r_scs_winset_interval_1d(voter_x, seat_x)
 *
 * voter_x: numeric vector of voter ideal points (length >= 1).
 * seat_x:  numeric(1), seat holder position.
 *
 * Returns: numeric(2): c(lo, hi), or c(NaN, NaN) if WinSet is empty.
 */
SEXP r_scs_winset_interval_1d(SEXP voter_x, SEXP seat_x) {
    if (!Rf_isReal(voter_x))
        Rf_error("r_scs_winset_interval_1d: voter_x must be a numeric vector.");
    if (!Rf_isReal(seat_x) || XLENGTH(seat_x) != 1)
        Rf_error("r_scs_winset_interval_1d: seat_x must be numeric(1).");

    int n = (int)XLENGTH(voter_x);
    if (n < 1)
        Rf_error("r_scs_winset_interval_1d: voter_x must have length >= 1.");

    char err_buf[512] = {0};
    double lo = R_NaN, hi = R_NaN;

    int rc = scs_winset_interval_1d(REAL(voter_x), n, REAL(seat_x)[0],
                                    &lo, &hi, err_buf, (int)sizeof(err_buf));
    if (rc != SCS_OK)
        Rf_error("scs_winset_interval_1d: %s", err_buf[0] ? err_buf : "unknown error");

    SEXP result = PROTECT(Rf_allocVector(REALSXP, 2));
    REAL(result)[0] = lo;
    REAL(result)[1] = hi;
    UNPROTECT(1);
    return result;
}
