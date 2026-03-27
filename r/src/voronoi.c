/* voronoi.c — R wrapper for scs_voronoi_cells_2d (Euclidean Voronoi cells).
 *
 * Used by the competition canvas payload builder only. Returns one frame's
 * cells as a list of n_sites elements; each element is either NULL (empty cell)
 * or list(paths = list(flat_xy_vector), competitor_idx = 0-based int).
 */

#include <R.h>
#include <Rinternals.h>
#include "scs_r_helpers.h"

/* r_scs_voronoi_cells_2d(sites_xy, n_sites, bbox)
 *
 * sites_xy: numeric vector, length 2 * n_sites (interleaved x,y).
 * n_sites:  integer(1).
 * bbox:     numeric length 4: min_x, min_y, max_x, max_y.
 *
 * Returns: list of length n_sites. Element i is either R_NilValue or a list
 *   with "paths" (list of one numeric vector: x,y,x,y,...) and
 *   "competitor_idx" (0-based integer).
 */
SEXP r_scs_voronoi_cells_2d(SEXP sites_xy, SEXP n_sites, SEXP bbox) {
    if (!Rf_isReal(sites_xy) || !Rf_isInteger(n_sites) || !Rf_isReal(bbox))
        Rf_error("r_scs_voronoi_cells_2d: sites_xy and bbox must be numeric, n_sites integer.");
    int n = INTEGER(n_sites)[0];
    if (n < 1 || XLENGTH(sites_xy) < (R_xlen_t)(2 * n))
        Rf_error("r_scs_voronoi_cells_2d: n_sites must be >= 1 and length(sites_xy) >= 2*n_sites.");
    if (XLENGTH(bbox) != 4)
        Rf_error("r_scs_voronoi_cells_2d: bbox must have length 4.");
    double *xy = REAL(sites_xy);
    double bbox_min_x = REAL(bbox)[0];
    double bbox_min_y = REAL(bbox)[1];
    double bbox_max_x = REAL(bbox)[2];
    double bbox_max_y = REAL(bbox)[3];

    char err[SCS_ERR_BUF_SIZE] = {0};
    SCS_VoronoiCellsHeap heap = {0};
    int rc = scs_voronoi_cells_2d_heap(xy, n, bbox_min_x, bbox_min_y, bbox_max_x,
                                       bbox_max_y, &heap, err, SCS_ERR_BUF_SIZE);
    if (rc != SCS_OK) {
        scs_voronoi_cells_heap_destroy(&heap);
        scs_check(rc, err);
    }

    int n_cells = heap.cell_start_len > 0 ? heap.cell_start_len - 1 : 0;
    double *out_xy = heap.xy;
    int *cell_start = heap.cell_start;

    SEXP result = PROTECT(Rf_allocVector(VECSXP, n_cells));
    for (int i = 0; i < n_cells; i++) {
        int start = cell_start[i];
        int end   = cell_start[i + 1];
        if (start >= end) {
            SET_VECTOR_ELT(result, i, R_NilValue);
            continue;
        }
        int n_verts = end - start;
        SEXP path_xy = PROTECT(Rf_allocVector(REALSXP, n_verts * 2));
        double *dst = REAL(path_xy);
        for (int j = 0; j < n_verts; j++) {
            dst[2 * j]     = out_xy[2 * (start + j)];
            dst[2 * j + 1] = out_xy[2 * (start + j) + 1];
        }
        SEXP paths_list = PROTECT(Rf_allocVector(VECSXP, 1));
        SET_VECTOR_ELT(paths_list, 0, path_xy);

        SEXP cell_list = PROTECT(Rf_allocVector(VECSXP, 2));
        SEXP names = PROTECT(Rf_allocVector(STRSXP, 2));
        SET_STRING_ELT(names, 0, Rf_mkChar("paths"));
        SET_STRING_ELT(names, 1, Rf_mkChar("competitor_idx"));
        SET_VECTOR_ELT(cell_list, 0, paths_list);
        SET_VECTOR_ELT(cell_list, 1, Rf_ScalarInteger(i));
        Rf_setAttrib(cell_list, R_NamesSymbol, names);
        SET_VECTOR_ELT(result, i, cell_list);
        /* path_xy, paths_list, cell_list, names — all now reachable via result */
        UNPROTECT(4);
    }
    scs_voronoi_cells_heap_destroy(&heap);
    UNPROTECT(1);
    return result;
}
