/* competition.c — .Call() wrappers for SCS_CompetitionTrace and
 * SCS_CompetitionExperiment.
 *
 * The C API uses 0-based round indices; the R layer translates user-facing
 * 1-based round indices before calling these wrappers.
 */

#include <stdio.h>
#include <string.h>

#include "scs_r_helpers.h"

static void competition_trace_finalizer(SEXP ptr) {
    SCS_CompetitionTrace *trace = (SCS_CompetitionTrace *)R_ExternalPtrAddr(ptr);
    if (trace) {
        scs_competition_trace_destroy(trace);
        R_ClearExternalPtr(ptr);
    }
}

static void competition_experiment_finalizer(SEXP ptr) {
    SCS_CompetitionExperiment *experiment =
        (SCS_CompetitionExperiment *)R_ExternalPtrAddr(ptr);
    if (experiment) {
        scs_competition_experiment_destroy(experiment);
        R_ClearExternalPtr(ptr);
    }
}

static SEXP wrap_competition_trace(SCS_CompetitionTrace *trace, const char *err) {
    if (!trace) {
        Rf_error("%s", err[0] ? err : "competition trace factory returned NULL");
    }
    SEXP ptr = PROTECT(R_MakeExternalPtr(trace, R_NilValue, R_NilValue));
    R_RegisterCFinalizerEx(ptr, competition_trace_finalizer, FALSE);
    UNPROTECT(1);
    return ptr;
}

static SEXP wrap_competition_experiment(SCS_CompetitionExperiment *experiment,
                                        const char *err) {
    if (!experiment) {
        Rf_error("%s",
                 err[0] ? err : "competition experiment factory returned NULL");
    }
    SEXP ptr = PROTECT(R_MakeExternalPtr(experiment, R_NilValue, R_NilValue));
    R_RegisterCFinalizerEx(ptr, competition_experiment_finalizer, FALSE);
    UNPROTECT(1);
    return ptr;
}

static SCS_CompetitionStepConfig build_competition_step_config(SEXP cfg_s) {
    SCS_CompetitionStepConfig cfg;
    cfg.kind = (SCS_CompetitionStepPolicyKind)INTEGER(VECTOR_ELT(cfg_s, 0))[0];
    cfg.fixed_step_size = REAL(VECTOR_ELT(cfg_s, 1))[0];
    cfg.min_step_size = REAL(VECTOR_ELT(cfg_s, 2))[0];
    cfg.max_step_size = REAL(VECTOR_ELT(cfg_s, 3))[0];
    cfg.proportionality_constant = REAL(VECTOR_ELT(cfg_s, 4))[0];
    cfg.jitter = REAL(VECTOR_ELT(cfg_s, 5))[0];
    return cfg;
}

static SCS_CompetitionTerminationConfig build_competition_termination_config(
    SEXP cfg_s) {
    SCS_CompetitionTerminationConfig cfg;
    cfg.stop_on_convergence = asLogical(VECTOR_ELT(cfg_s, 0));
    cfg.position_epsilon = REAL(VECTOR_ELT(cfg_s, 1))[0];
    cfg.stop_on_cycle = asLogical(VECTOR_ELT(cfg_s, 2));
    cfg.cycle_memory = asInteger(VECTOR_ELT(cfg_s, 3));
    cfg.signature_resolution = REAL(VECTOR_ELT(cfg_s, 4))[0];
    cfg.stop_on_no_improvement = asLogical(VECTOR_ELT(cfg_s, 5));
    cfg.no_improvement_window = asInteger(VECTOR_ELT(cfg_s, 6));
    cfg.objective_epsilon = REAL(VECTOR_ELT(cfg_s, 7))[0];
    return cfg;
}

static SCS_CompetitionEngineConfig build_competition_engine_config(SEXP cfg_s) {
    SCS_CompetitionEngineConfig cfg;
    cfg.seat_count = asInteger(VECTOR_ELT(cfg_s, 0));
    cfg.seat_rule = (SCS_CompetitionSeatRule)INTEGER(VECTOR_ELT(cfg_s, 1))[0];
    cfg.step_config = build_competition_step_config(VECTOR_ELT(cfg_s, 2));
    cfg.boundary_policy =
        (SCS_CompetitionBoundaryPolicy)INTEGER(VECTOR_ELT(cfg_s, 3))[0];
    cfg.objective_kind =
        (SCS_CompetitionObjectiveKind)INTEGER(VECTOR_ELT(cfg_s, 4))[0];
    cfg.max_rounds = asInteger(VECTOR_ELT(cfg_s, 5));
    cfg.termination =
        build_competition_termination_config(VECTOR_ELT(cfg_s, 6));
    return cfg;
}

static SEXP build_position_matrix(const double *values, int n_competitors,
                                  int n_dims) {
    SEXP result = PROTECT(allocMatrix(REALSXP, n_competitors, n_dims));
    double *dst = REAL(result);
    for (int c = 0; c < n_competitors; ++c) {
        for (int d = 0; d < n_dims; ++d) {
            dst[c + d * n_competitors] = values[c * n_dims + d];
        }
    }
    SEXP rownames = PROTECT(allocVector(STRSXP, n_competitors));
    SEXP colnames = PROTECT(allocVector(STRSXP, n_dims));
    SEXP dimnames = PROTECT(allocVector(VECSXP, 2));
    char buf[32];
    for (int c = 0; c < n_competitors; ++c) {
        snprintf(buf, sizeof(buf), "competitor%d", c + 1);
        SET_STRING_ELT(rownames, c, mkChar(buf));
    }
    for (int d = 0; d < n_dims; ++d) {
        snprintf(buf, sizeof(buf), "dim%d", d + 1);
        SET_STRING_ELT(colnames, d, mkChar(buf));
    }
    SET_VECTOR_ELT(dimnames, 0, rownames);
    SET_VECTOR_ELT(dimnames, 1, colnames);
    setAttrib(result, R_DimNamesSymbol, dimnames);
    UNPROTECT(4);
    return result;
}

static SEXP build_named_share_vector(const double *values, int n_competitors) {
    SEXP result = PROTECT(allocVector(REALSXP, n_competitors));
    memcpy(REAL(result), values, (size_t)n_competitors * sizeof(double));
    SEXP names = PROTECT(allocVector(STRSXP, n_competitors));
    char buf[32];
    for (int i = 0; i < n_competitors; ++i) {
        snprintf(buf, sizeof(buf), "competitor%d", i + 1);
        SET_STRING_ELT(names, i, mkChar(buf));
    }
    setAttrib(result, R_NamesSymbol, names);
    UNPROTECT(2);
    return result;
}

SEXP r_scs_competition_run(SEXP competitor_positions, SEXP competitor_headings,
                           SEXP strategy_kinds, SEXP voter_ideals,
                           SEXP dist_cfg, SEXP engine_cfg, SEXP mgr_ptr) {
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    SCS_CompetitionEngineConfig cfg = build_competition_engine_config(engine_cfg);
    SCS_StreamManager *mgr = Rf_isNull(mgr_ptr)
                                 ? NULL
                                 : (SCS_StreamManager *)get_extptr(mgr_ptr);
    if (!Rf_isNull(mgr_ptr) && !mgr) {
        Rf_error("StreamManager handle is NULL or has already been destroyed.");
    }
    const int n_competitors = (int)XLENGTH(strategy_kinds);
    const int n_voters = (int)(XLENGTH(voter_ideals) / dc.n_weights);
    const int n_dims = dc.n_weights;
    const double *heading_ptr =
        Rf_isNull(competitor_headings) ? NULL : REAL(competitor_headings);
    char err[SCS_ERR_BUF_SIZE] = {0};
    SCS_CompetitionTrace *trace = scs_competition_run(
        REAL(competitor_positions), heading_ptr, INTEGER(strategy_kinds),
        n_competitors, REAL(voter_ideals), n_voters, n_dims, &dc, &cfg, mgr,
        err, SCS_ERR_BUF_SIZE);
    return wrap_competition_trace(trace, err);
}

SEXP r_scs_competition_trace_destroy(SEXP ptr) {
    competition_trace_finalizer(ptr);
    return R_NilValue;
}

SEXP r_scs_competition_trace_dims(SEXP ptr) {
    SCS_CompetitionTrace *trace = (SCS_CompetitionTrace *)get_extptr(ptr);
    if (!trace) {
        Rf_error("CompetitionTrace handle is NULL or has already been destroyed.");
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    int n_rounds = 0;
    int n_competitors = 0;
    int n_dims = 0;
    scs_check(scs_competition_trace_dims(trace, &n_rounds, &n_competitors,
                                         &n_dims, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(VECSXP, 3));
    SEXP names = PROTECT(allocVector(STRSXP, 3));
    SET_VECTOR_ELT(result, 0, ScalarInteger(n_rounds));
    SET_STRING_ELT(names, 0, mkChar("n_rounds"));
    SET_VECTOR_ELT(result, 1, ScalarInteger(n_competitors));
    SET_STRING_ELT(names, 1, mkChar("n_competitors"));
    SET_VECTOR_ELT(result, 2, ScalarInteger(n_dims));
    SET_STRING_ELT(names, 2, mkChar("n_dims"));
    setAttrib(result, R_NamesSymbol, names);
    UNPROTECT(2);
    return result;
}

SEXP r_scs_competition_trace_termination(SEXP ptr) {
    SCS_CompetitionTrace *trace = (SCS_CompetitionTrace *)get_extptr(ptr);
    if (!trace) {
        Rf_error("CompetitionTrace handle is NULL or has already been destroyed.");
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    int terminated_early = 0;
    SCS_CompetitionTerminationReason reason;
    scs_check(scs_competition_trace_termination(trace, &terminated_early, &reason,
                                                err, SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(VECSXP, 2));
    SEXP names = PROTECT(allocVector(STRSXP, 2));
    SET_VECTOR_ELT(result, 0, ScalarLogical(terminated_early));
    SET_STRING_ELT(names, 0, mkChar("terminated_early"));
    SET_VECTOR_ELT(result, 1, ScalarInteger((int)reason));
    SET_STRING_ELT(names, 1, mkChar("reason"));
    setAttrib(result, R_NamesSymbol, names);
    UNPROTECT(2);
    return result;
}

SEXP r_scs_competition_trace_round_positions(SEXP ptr, SEXP round_0based) {
    SCS_CompetitionTrace *trace = (SCS_CompetitionTrace *)get_extptr(ptr);
    if (!trace) {
        Rf_error("CompetitionTrace handle is NULL or has already been destroyed.");
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    int n_rounds = 0;
    int n_competitors = 0;
    int n_dims = 0;
    scs_check(scs_competition_trace_dims(trace, &n_rounds, &n_competitors,
                                         &n_dims, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP buffer = PROTECT(allocVector(REALSXP, n_competitors * n_dims));
    scs_check(scs_competition_trace_round_positions(
                  trace, asInteger(round_0based), REAL(buffer),
                  n_competitors * n_dims, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP result = build_position_matrix(REAL(buffer), n_competitors, n_dims);
    UNPROTECT(1);
    return result;
}

SEXP r_scs_competition_trace_final_positions(SEXP ptr) {
    SCS_CompetitionTrace *trace = (SCS_CompetitionTrace *)get_extptr(ptr);
    if (!trace) {
        Rf_error("CompetitionTrace handle is NULL or has already been destroyed.");
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    int n_rounds = 0;
    int n_competitors = 0;
    int n_dims = 0;
    scs_check(scs_competition_trace_dims(trace, &n_rounds, &n_competitors,
                                         &n_dims, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP buffer = PROTECT(allocVector(REALSXP, n_competitors * n_dims));
    scs_check(scs_competition_trace_final_positions(
                  trace, REAL(buffer), n_competitors * n_dims, err,
                  SCS_ERR_BUF_SIZE),
              err);
    SEXP result = build_position_matrix(REAL(buffer), n_competitors, n_dims);
    UNPROTECT(1);
    return result;
}

SEXP r_scs_competition_trace_round_vote_shares(SEXP ptr, SEXP round_0based) {
    SCS_CompetitionTrace *trace = (SCS_CompetitionTrace *)get_extptr(ptr);
    if (!trace) {
        Rf_error("CompetitionTrace handle is NULL or has already been destroyed.");
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    int n_rounds = 0;
    int n_competitors = 0;
    int n_dims = 0;
    scs_check(scs_competition_trace_dims(trace, &n_rounds, &n_competitors,
                                         &n_dims, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP buffer = PROTECT(allocVector(REALSXP, n_competitors));
    scs_check(scs_competition_trace_round_vote_shares(
                  trace, asInteger(round_0based), REAL(buffer), n_competitors,
                  err, SCS_ERR_BUF_SIZE),
              err);
    SEXP result = build_named_share_vector(REAL(buffer), n_competitors);
  UNPROTECT(1);
  return result;
}

SEXP r_scs_competition_trace_round_seat_shares(SEXP ptr, SEXP round_0based) {
    SCS_CompetitionTrace *trace = (SCS_CompetitionTrace *)get_extptr(ptr);
    if (!trace) {
        Rf_error("CompetitionTrace handle is NULL or has already been destroyed.");
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    int n_rounds = 0;
    int n_competitors = 0;
    int n_dims = 0;
    scs_check(scs_competition_trace_dims(trace, &n_rounds, &n_competitors,
                                         &n_dims, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP buffer = PROTECT(allocVector(REALSXP, n_competitors));
    scs_check(scs_competition_trace_round_seat_shares(
                  trace, asInteger(round_0based), REAL(buffer), n_competitors,
                  err, SCS_ERR_BUF_SIZE),
              err);
    SEXP result = build_named_share_vector(REAL(buffer), n_competitors);
  UNPROTECT(1);
  return result;
}

SEXP r_scs_competition_trace_round_vote_totals(SEXP ptr, SEXP round_0based) {
    SCS_CompetitionTrace *trace = (SCS_CompetitionTrace *)get_extptr(ptr);
    if (!trace) {
        Rf_error("CompetitionTrace handle is NULL or has already been destroyed.");
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    int n_rounds = 0;
    int n_competitors = 0;
    int n_dims = 0;
    scs_check(scs_competition_trace_dims(trace, &n_rounds, &n_competitors,
                                         &n_dims, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(INTSXP, n_competitors));
    scs_check(scs_competition_trace_round_vote_totals(
                  trace, asInteger(round_0based), INTEGER(result), n_competitors,
                  err, SCS_ERR_BUF_SIZE),
              err);
    SEXP names = PROTECT(allocVector(STRSXP, n_competitors));
    char buf[32];
    for (int i = 0; i < n_competitors; ++i) {
        snprintf(buf, sizeof(buf), "competitor%d", i + 1);
        SET_STRING_ELT(names, i, mkChar(buf));
    }
    setAttrib(result, R_NamesSymbol, names);
    UNPROTECT(2);
    return result;
}

SEXP r_scs_competition_trace_round_seat_totals(SEXP ptr, SEXP round_0based) {
    SCS_CompetitionTrace *trace = (SCS_CompetitionTrace *)get_extptr(ptr);
    if (!trace) {
        Rf_error("CompetitionTrace handle is NULL or has already been destroyed.");
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    int n_rounds = 0;
    int n_competitors = 0;
    int n_dims = 0;
    scs_check(scs_competition_trace_dims(trace, &n_rounds, &n_competitors,
                                         &n_dims, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(INTSXP, n_competitors));
    scs_check(scs_competition_trace_round_seat_totals(
                  trace, asInteger(round_0based), INTEGER(result), n_competitors,
                  err, SCS_ERR_BUF_SIZE),
              err);
    SEXP names = PROTECT(allocVector(STRSXP, n_competitors));
    char buf[32];
    for (int i = 0; i < n_competitors; ++i) {
        snprintf(buf, sizeof(buf), "competitor%d", i + 1);
        SET_STRING_ELT(names, i, mkChar(buf));
    }
    setAttrib(result, R_NamesSymbol, names);
    UNPROTECT(2);
    return result;
}

SEXP r_scs_competition_trace_final_vote_shares(SEXP ptr) {
    SCS_CompetitionTrace *trace = (SCS_CompetitionTrace *)get_extptr(ptr);
    if (!trace) {
        Rf_error("CompetitionTrace handle is NULL or has already been destroyed.");
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    int n_rounds = 0;
    int n_competitors = 0;
    int n_dims = 0;
    scs_check(scs_competition_trace_dims(trace, &n_rounds, &n_competitors,
                                         &n_dims, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP buffer = PROTECT(allocVector(REALSXP, n_competitors));
    scs_check(scs_competition_trace_final_vote_shares(
                  trace, REAL(buffer), n_competitors, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP result = build_named_share_vector(REAL(buffer), n_competitors);
    UNPROTECT(1);
    return result;
}

SEXP r_scs_competition_trace_final_seat_shares(SEXP ptr) {
    SCS_CompetitionTrace *trace = (SCS_CompetitionTrace *)get_extptr(ptr);
    if (!trace) {
        Rf_error("CompetitionTrace handle is NULL or has already been destroyed.");
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    int n_rounds = 0;
    int n_competitors = 0;
    int n_dims = 0;
    scs_check(scs_competition_trace_dims(trace, &n_rounds, &n_competitors,
                                         &n_dims, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP buffer = PROTECT(allocVector(REALSXP, n_competitors));
    scs_check(scs_competition_trace_final_seat_shares(
                  trace, REAL(buffer), n_competitors, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP result = build_named_share_vector(REAL(buffer), n_competitors);
    UNPROTECT(1);
    return result;
}

SEXP r_scs_competition_trace_strategy_kinds(SEXP ptr) {
    SCS_CompetitionTrace *trace = (SCS_CompetitionTrace *)get_extptr(ptr);
    if (!trace) {
        Rf_error("CompetitionTrace handle is NULL or has already been destroyed.");
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    int n_rounds = 0;
    int n_competitors = 0;
    int n_dims = 0;
    scs_check(scs_competition_trace_dims(trace, &n_rounds, &n_competitors,
                                         &n_dims, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(INTSXP, n_competitors));
    scs_check(scs_competition_trace_strategy_kinds(
                  trace, INTEGER(result), n_competitors, err, SCS_ERR_BUF_SIZE),
              err);

    /* Map integer enum values to human-readable strings. */
    static const char *STRATEGY_LABELS[] = {"sticker", "hunter", "aggregator",
                                            "predator", "hunter_sticker"};
    SEXP str_result = PROTECT(allocVector(STRSXP, n_competitors));
    int *raw = INTEGER(result);
    for (int i = 0; i < n_competitors; ++i) {
        int kind = raw[i];
        const char *label =
            (kind >= 0 && kind <= 4) ? STRATEGY_LABELS[kind] : "unknown";
        SET_STRING_ELT(str_result, i, mkChar(label));
    }
    UNPROTECT(2);
    return str_result;
}

SEXP r_scs_competition_run_experiment(SEXP competitor_positions,
                                      SEXP competitor_headings,
                                      SEXP strategy_kinds, SEXP voter_ideals,
                                      SEXP dist_cfg, SEXP engine_cfg,
                                      SEXP master_seed_s, SEXP num_runs_s,
                                      SEXP retain_traces_s) {
    SCS_DistanceConfig dc = build_dist_config(dist_cfg);
    SCS_CompetitionEngineConfig cfg = build_competition_engine_config(engine_cfg);
    const int n_competitors = (int)XLENGTH(strategy_kinds);
    const int n_voters = (int)(XLENGTH(voter_ideals) / dc.n_weights);
    const int n_dims = dc.n_weights;
    const double *heading_ptr =
        Rf_isNull(competitor_headings) ? NULL : REAL(competitor_headings);
    char err[SCS_ERR_BUF_SIZE] = {0};
    SCS_CompetitionExperiment *experiment = scs_competition_run_experiment(
        REAL(competitor_positions), heading_ptr, INTEGER(strategy_kinds),
        n_competitors, REAL(voter_ideals), n_voters, n_dims, &dc, &cfg,
        (uint64_t)asReal(master_seed_s), asInteger(num_runs_s),
        asLogical(retain_traces_s), err, SCS_ERR_BUF_SIZE);
    return wrap_competition_experiment(experiment, err);
}

SEXP r_scs_competition_experiment_destroy(SEXP ptr) {
    competition_experiment_finalizer(ptr);
    return R_NilValue;
}

SEXP r_scs_competition_experiment_dims(SEXP ptr) {
    SCS_CompetitionExperiment *experiment =
        (SCS_CompetitionExperiment *)get_extptr(ptr);
    if (!experiment) {
        Rf_error(
            "CompetitionExperiment handle is NULL or has already been destroyed.");
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    int num_runs = 0;
    int n_competitors = 0;
    int n_dims = 0;
    scs_check(scs_competition_experiment_dims(experiment, &num_runs,
                                              &n_competitors, &n_dims, err,
                                              SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(VECSXP, 3));
    SEXP names = PROTECT(allocVector(STRSXP, 3));
    SET_VECTOR_ELT(result, 0, ScalarInteger(num_runs));
    SET_STRING_ELT(names, 0, mkChar("n_runs"));
    SET_VECTOR_ELT(result, 1, ScalarInteger(n_competitors));
    SET_STRING_ELT(names, 1, mkChar("n_competitors"));
    SET_VECTOR_ELT(result, 2, ScalarInteger(n_dims));
    SET_STRING_ELT(names, 2, mkChar("n_dims"));
    setAttrib(result, R_NamesSymbol, names);
    UNPROTECT(2);
    return result;
}

SEXP r_scs_competition_experiment_summary(SEXP ptr) {
    SCS_CompetitionExperiment *experiment =
        (SCS_CompetitionExperiment *)get_extptr(ptr);
    if (!experiment) {
        Rf_error(
            "CompetitionExperiment handle is NULL or has already been destroyed.");
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    double mean_rounds = 0.0;
    double early_rate = 0.0;
    scs_check(scs_competition_experiment_summary(experiment, &mean_rounds,
                                                 &early_rate, err,
                                                 SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(VECSXP, 2));
    SEXP names = PROTECT(allocVector(STRSXP, 2));
    SET_VECTOR_ELT(result, 0, ScalarReal(mean_rounds));
    SET_STRING_ELT(names, 0, mkChar("mean_rounds"));
    SET_VECTOR_ELT(result, 1, ScalarReal(early_rate));
    SET_STRING_ELT(names, 1, mkChar("early_termination_rate"));
    setAttrib(result, R_NamesSymbol, names);
    UNPROTECT(2);
    return result;
}

SEXP r_scs_competition_experiment_mean_final_vote_shares(SEXP ptr) {
    SCS_CompetitionExperiment *experiment =
        (SCS_CompetitionExperiment *)get_extptr(ptr);
    if (!experiment) {
        Rf_error(
            "CompetitionExperiment handle is NULL or has already been destroyed.");
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    int num_runs = 0;
    int n_competitors = 0;
    int n_dims = 0;
    scs_check(scs_competition_experiment_dims(experiment, &num_runs,
                                              &n_competitors, &n_dims, err,
                                              SCS_ERR_BUF_SIZE),
              err);
    SEXP buffer = PROTECT(allocVector(REALSXP, n_competitors));
    scs_check(scs_competition_experiment_mean_final_vote_shares(
                  experiment, REAL(buffer), n_competitors, err,
                  SCS_ERR_BUF_SIZE),
              err);
    SEXP result = build_named_share_vector(REAL(buffer), n_competitors);
    UNPROTECT(1);
    return result;
}

SEXP r_scs_competition_experiment_mean_final_seat_shares(SEXP ptr) {
    SCS_CompetitionExperiment *experiment =
        (SCS_CompetitionExperiment *)get_extptr(ptr);
    if (!experiment) {
        Rf_error(
            "CompetitionExperiment handle is NULL or has already been destroyed.");
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    int num_runs = 0;
    int n_competitors = 0;
    int n_dims = 0;
    scs_check(scs_competition_experiment_dims(experiment, &num_runs,
                                              &n_competitors, &n_dims, err,
                                              SCS_ERR_BUF_SIZE),
              err);
    SEXP buffer = PROTECT(allocVector(REALSXP, n_competitors));
    scs_check(scs_competition_experiment_mean_final_seat_shares(
                  experiment, REAL(buffer), n_competitors, err,
                  SCS_ERR_BUF_SIZE),
              err);
    SEXP result = build_named_share_vector(REAL(buffer), n_competitors);
    UNPROTECT(1);
    return result;
}

SEXP r_scs_competition_experiment_run_round_counts(SEXP ptr) {
    SCS_CompetitionExperiment *experiment =
        (SCS_CompetitionExperiment *)get_extptr(ptr);
    if (!experiment) {
        Rf_error(
            "CompetitionExperiment handle is NULL or has already been destroyed.");
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    int num_runs = 0;
    int n_competitors = 0;
    int n_dims = 0;
    scs_check(scs_competition_experiment_dims(experiment, &num_runs,
                                              &n_competitors, &n_dims, err,
                                              SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(INTSXP, num_runs));
    scs_check(scs_competition_experiment_run_round_counts(
                  experiment, INTEGER(result), num_runs, err, SCS_ERR_BUF_SIZE),
              err);
    UNPROTECT(1);
    return result;
}

SEXP r_scs_competition_experiment_run_termination_reasons(SEXP ptr) {
    SCS_CompetitionExperiment *experiment =
        (SCS_CompetitionExperiment *)get_extptr(ptr);
    if (!experiment) {
        Rf_error(
            "CompetitionExperiment handle is NULL or has already been destroyed.");
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    int num_runs = 0;
    int n_competitors = 0;
    int n_dims = 0;
    scs_check(scs_competition_experiment_dims(experiment, &num_runs,
                                              &n_competitors, &n_dims, err,
                                              SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(INTSXP, num_runs));
    scs_check(scs_competition_experiment_run_termination_reasons(
                  experiment,
                  (SCS_CompetitionTerminationReason *)INTEGER(result), num_runs,
                  err, SCS_ERR_BUF_SIZE),
              err);
    UNPROTECT(1);
    return result;
}

SEXP r_scs_competition_experiment_run_terminated_early_flags(SEXP ptr) {
    SCS_CompetitionExperiment *experiment =
        (SCS_CompetitionExperiment *)get_extptr(ptr);
    if (!experiment) {
        Rf_error(
            "CompetitionExperiment handle is NULL or has already been destroyed.");
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    int num_runs = 0;
    int n_competitors = 0;
    int n_dims = 0;
    scs_check(scs_competition_experiment_dims(experiment, &num_runs,
                                              &n_competitors, &n_dims, err,
                                              SCS_ERR_BUF_SIZE),
              err);
    int *flags = (int *)R_alloc((size_t)num_runs, sizeof(int));
    scs_check(scs_competition_experiment_run_terminated_early_flags(
                  experiment, flags, num_runs, err, SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(LGLSXP, num_runs));
    for (int i = 0; i < num_runs; ++i) LOGICAL(result)[i] = flags[i];
    UNPROTECT(1);
    return result;
}

SEXP r_scs_competition_experiment_run_final_vote_shares(SEXP ptr) {
    SCS_CompetitionExperiment *experiment =
        (SCS_CompetitionExperiment *)get_extptr(ptr);
    if (!experiment) {
        Rf_error(
            "CompetitionExperiment handle is NULL or has already been destroyed.");
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    int num_runs = 0;
    int n_competitors = 0;
    int n_dims = 0;
    scs_check(scs_competition_experiment_dims(experiment, &num_runs,
                                              &n_competitors, &n_dims, err,
                                              SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocMatrix(REALSXP, num_runs, n_competitors));
    double *tmp = (double *)R_alloc((size_t)num_runs * n_competitors, sizeof(double));
    scs_check(scs_competition_experiment_run_final_vote_shares(
                  experiment, tmp, num_runs * n_competitors, err,
                  SCS_ERR_BUF_SIZE),
              err);
    double *dst = REAL(result);
    for (int run = 0; run < num_runs; ++run) {
        for (int c = 0; c < n_competitors; ++c) {
            dst[run + c * num_runs] = tmp[run * n_competitors + c];
        }
    }
    UNPROTECT(1);
    return result;
}

SEXP r_scs_competition_experiment_run_final_seat_shares(SEXP ptr) {
    SCS_CompetitionExperiment *experiment =
        (SCS_CompetitionExperiment *)get_extptr(ptr);
    if (!experiment) {
        Rf_error(
            "CompetitionExperiment handle is NULL or has already been destroyed.");
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    int num_runs = 0;
    int n_competitors = 0;
    int n_dims = 0;
    scs_check(scs_competition_experiment_dims(experiment, &num_runs,
                                              &n_competitors, &n_dims, err,
                                              SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocMatrix(REALSXP, num_runs, n_competitors));
    double *tmp = (double *)R_alloc((size_t)num_runs * n_competitors, sizeof(double));
    scs_check(scs_competition_experiment_run_final_seat_shares(
                  experiment, tmp, num_runs * n_competitors, err,
                  SCS_ERR_BUF_SIZE),
              err);
    double *dst = REAL(result);
    for (int run = 0; run < num_runs; ++run) {
        for (int c = 0; c < n_competitors; ++c) {
            dst[run + c * num_runs] = tmp[run * n_competitors + c];
        }
    }
    UNPROTECT(1);
    return result;
}

SEXP r_scs_competition_experiment_run_final_positions(SEXP ptr) {
    SCS_CompetitionExperiment *experiment =
        (SCS_CompetitionExperiment *)get_extptr(ptr);
    if (!experiment) {
        Rf_error(
            "CompetitionExperiment handle is NULL or has already been destroyed.");
    }
    char err[SCS_ERR_BUF_SIZE] = {0};
    int num_runs = 0;
    int n_competitors = 0;
    int n_dims = 0;
    scs_check(scs_competition_experiment_dims(experiment, &num_runs,
                                              &n_competitors, &n_dims, err,
                                              SCS_ERR_BUF_SIZE),
              err);
    SEXP result = PROTECT(allocVector(REALSXP, num_runs * n_competitors * n_dims));
    scs_check(scs_competition_experiment_run_final_positions(
                  experiment, REAL(result), num_runs * n_competitors * n_dims,
                  err, SCS_ERR_BUF_SIZE),
              err);
    SEXP dim = PROTECT(allocVector(INTSXP, 3));
    INTEGER(dim)[0] = num_runs;
    INTEGER(dim)[1] = n_competitors;
    INTEGER(dim)[2] = n_dims;
    setAttrib(result, R_DimSymbol, dim);
    UNPROTECT(2);
    return result;
}
