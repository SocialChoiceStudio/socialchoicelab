/* init.c — DLL initialisation for the socialchoicelab R package.
 *
 * Registers all .Call() entry points and disables dynamic symbol lookup so
 * that R requires every symbol to be explicitly listed here.
 *
 * scs_check() and SCS_ERR_BUF_SIZE are defined in scs_r_helpers.h.
 * This file includes that header to validate the definition compiles; the
 * static inline copy here is unused (each translation unit gets its own).
 */

#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>
#include "scs_r_helpers.h"

/* ---------------------------------------------------------------------------
 * Forward declarations for all .Call() entry points.
 * One line per function; keep in alphabetical order within each group.
 * --------------------------------------------------------------------------- */

/* StreamManager */
extern SEXP r_scs_bernoulli(SEXP, SEXP, SEXP);
extern SEXP r_scs_normal(SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_register_streams(SEXP, SEXP);
extern SEXP r_scs_reset_all(SEXP, SEXP);
extern SEXP r_scs_reset_stream(SEXP, SEXP, SEXP);
extern SEXP r_scs_skip(SEXP, SEXP, SEXP);
extern SEXP r_scs_stream_manager_create(SEXP);
extern SEXP r_scs_stream_manager_destroy(SEXP);
extern SEXP r_scs_uniform_choice(SEXP, SEXP, SEXP);
extern SEXP r_scs_uniform_int(SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_uniform_real(SEXP, SEXP, SEXP, SEXP);

/* Winset */
extern SEXP r_scs_winset_2d(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_winset_bbox_2d(SEXP);
extern SEXP r_scs_winset_boundary_2d(SEXP);
extern SEXP r_scs_winset_clone(SEXP);
extern SEXP r_scs_winset_contains_point_2d(SEXP, SEXP, SEXP);
extern SEXP r_scs_winset_destroy(SEXP);
extern SEXP r_scs_winset_difference(SEXP, SEXP);
extern SEXP r_scs_winset_intersection(SEXP, SEXP);
extern SEXP r_scs_winset_is_empty(SEXP);
extern SEXP r_scs_winset_symmetric_difference(SEXP, SEXP);
extern SEXP r_scs_winset_union(SEXP, SEXP);
extern SEXP r_scs_winset_with_veto_2d(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_weighted_winset_2d(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);

/* Profile */
extern SEXP r_scs_profile_build_spatial(SEXP, SEXP, SEXP);
extern SEXP r_scs_profile_clone(SEXP);
extern SEXP r_scs_profile_destroy(SEXP);
extern SEXP r_scs_profile_dims(SEXP);
extern SEXP r_scs_profile_export_rankings(SEXP);
extern SEXP r_scs_profile_from_utility_matrix(SEXP, SEXP, SEXP);
extern SEXP r_scs_profile_gaussian_spatial(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_profile_get_ranking(SEXP, SEXP);
extern SEXP r_scs_profile_impartial_culture(SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_profile_uniform_spatial(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);

/* ---------------------------------------------------------------------------
 * .Call() entry point table — one entry per function.
 * The third field is the number of SEXP arguments.
 * --------------------------------------------------------------------------- */
static const R_CallMethodDef call_methods[] = {
    /* StreamManager */
    {"r_scs_bernoulli",              (DL_FUNC)&r_scs_bernoulli,              3},
    {"r_scs_normal",                 (DL_FUNC)&r_scs_normal,                 4},
    {"r_scs_register_streams",       (DL_FUNC)&r_scs_register_streams,       2},
    {"r_scs_reset_all",              (DL_FUNC)&r_scs_reset_all,              2},
    {"r_scs_reset_stream",           (DL_FUNC)&r_scs_reset_stream,           3},
    {"r_scs_skip",                   (DL_FUNC)&r_scs_skip,                   3},
    {"r_scs_stream_manager_create",  (DL_FUNC)&r_scs_stream_manager_create,  1},
    {"r_scs_stream_manager_destroy", (DL_FUNC)&r_scs_stream_manager_destroy, 1},
    {"r_scs_uniform_choice",         (DL_FUNC)&r_scs_uniform_choice,         3},
    {"r_scs_uniform_int",            (DL_FUNC)&r_scs_uniform_int,            4},
    {"r_scs_uniform_real",           (DL_FUNC)&r_scs_uniform_real,           4},
    /* Winset */
    {"r_scs_weighted_winset_2d",          (DL_FUNC)&r_scs_weighted_winset_2d,          7},
    {"r_scs_winset_2d",                   (DL_FUNC)&r_scs_winset_2d,                   6},
    {"r_scs_winset_bbox_2d",              (DL_FUNC)&r_scs_winset_bbox_2d,              1},
    {"r_scs_winset_boundary_2d",          (DL_FUNC)&r_scs_winset_boundary_2d,          1},
    {"r_scs_winset_clone",                (DL_FUNC)&r_scs_winset_clone,                1},
    {"r_scs_winset_contains_point_2d",    (DL_FUNC)&r_scs_winset_contains_point_2d,    3},
    {"r_scs_winset_destroy",              (DL_FUNC)&r_scs_winset_destroy,              1},
    {"r_scs_winset_difference",           (DL_FUNC)&r_scs_winset_difference,           2},
    {"r_scs_winset_intersection",         (DL_FUNC)&r_scs_winset_intersection,         2},
    {"r_scs_winset_is_empty",             (DL_FUNC)&r_scs_winset_is_empty,             1},
    {"r_scs_winset_symmetric_difference", (DL_FUNC)&r_scs_winset_symmetric_difference, 2},
    {"r_scs_winset_union",                (DL_FUNC)&r_scs_winset_union,                2},
    {"r_scs_winset_with_veto_2d",         (DL_FUNC)&r_scs_winset_with_veto_2d,         7},
    /* Profile */
    {"r_scs_profile_build_spatial",       (DL_FUNC)&r_scs_profile_build_spatial,       3},
    {"r_scs_profile_clone",               (DL_FUNC)&r_scs_profile_clone,               1},
    {"r_scs_profile_destroy",             (DL_FUNC)&r_scs_profile_destroy,             1},
    {"r_scs_profile_dims",                (DL_FUNC)&r_scs_profile_dims,                1},
    {"r_scs_profile_export_rankings",     (DL_FUNC)&r_scs_profile_export_rankings,     1},
    {"r_scs_profile_from_utility_matrix", (DL_FUNC)&r_scs_profile_from_utility_matrix, 3},
    {"r_scs_profile_gaussian_spatial",    (DL_FUNC)&r_scs_profile_gaussian_spatial,    7},
    {"r_scs_profile_get_ranking",         (DL_FUNC)&r_scs_profile_get_ranking,         2},
    {"r_scs_profile_impartial_culture",   (DL_FUNC)&r_scs_profile_impartial_culture,   4},
    {"r_scs_profile_uniform_spatial",     (DL_FUNC)&r_scs_profile_uniform_spatial,     7},
    {NULL, NULL, 0} /* sentinel */
};

void R_init_socialchoicelab(DllInfo *dll) {
    R_registerRoutines(dll, NULL, call_methods, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
    R_forceSymbols(dll, TRUE);
}
