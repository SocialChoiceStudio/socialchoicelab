/* init.c — DLL initialisation for the socialchoicelab R package.
 *
 * Registers all .Call() entry points and disables dynamic symbol lookup.
 * Entry points are grouped by source file and listed alphabetically within
 * each group.
 */

#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>
#include "scs_r_helpers.h"

/* ---------------------------------------------------------------------------
 * Forward declarations — B2: StreamManager, Winset, Profile lifecycle
 * --------------------------------------------------------------------------- */

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

extern SEXP r_scs_weighted_winset_2d(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);
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

/* Competition (competition.c) */
extern SEXP r_scs_competition_run(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_competition_trace_destroy(SEXP);
extern SEXP r_scs_competition_trace_dims(SEXP);
extern SEXP r_scs_competition_trace_termination(SEXP);
extern SEXP r_scs_competition_trace_round_positions(SEXP, SEXP);
extern SEXP r_scs_competition_trace_final_positions(SEXP);
extern SEXP r_scs_competition_trace_round_vote_shares(SEXP, SEXP);
extern SEXP r_scs_competition_trace_round_seat_shares(SEXP, SEXP);
extern SEXP r_scs_competition_trace_round_vote_totals(SEXP, SEXP);
extern SEXP r_scs_competition_trace_round_seat_totals(SEXP, SEXP);
extern SEXP r_scs_competition_trace_final_vote_shares(SEXP);
extern SEXP r_scs_competition_trace_final_seat_shares(SEXP);
extern SEXP r_scs_competition_run_experiment(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP,
                                             SEXP, SEXP, SEXP);
extern SEXP r_scs_competition_experiment_destroy(SEXP);
extern SEXP r_scs_competition_experiment_dims(SEXP);
extern SEXP r_scs_competition_experiment_summary(SEXP);
extern SEXP r_scs_competition_experiment_mean_final_vote_shares(SEXP);
extern SEXP r_scs_competition_experiment_mean_final_seat_shares(SEXP);
extern SEXP r_scs_competition_experiment_run_round_counts(SEXP);
extern SEXP r_scs_competition_experiment_run_termination_reasons(SEXP);
extern SEXP r_scs_competition_experiment_run_terminated_early_flags(SEXP);
extern SEXP r_scs_competition_experiment_run_final_vote_shares(SEXP);
extern SEXP r_scs_competition_experiment_run_final_seat_shares(SEXP);
extern SEXP r_scs_competition_experiment_run_final_positions(SEXP);

/* ---------------------------------------------------------------------------
 * Forward declarations — B3: function groups
 * --------------------------------------------------------------------------- */

/* B3.1–B3.3 (functions.c) */
extern SEXP r_scs_api_version(SEXP);
extern SEXP r_scs_calculate_distance(SEXP, SEXP, SEXP);
extern SEXP r_scs_convex_hull_2d(SEXP);
extern SEXP r_scs_distance_to_utility(SEXP, SEXP);
extern SEXP r_scs_level_set_1d(SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_level_set_2d(SEXP, SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_level_set_to_polygon(SEXP, SEXP);
extern SEXP r_scs_majority_prefers_2d(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_normalize_utility(SEXP, SEXP, SEXP);
extern SEXP r_scs_pairwise_matrix_2d(SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_weighted_majority_prefers_2d(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);

/* B3.5 (geometry.c) */
extern SEXP r_scs_condorcet_winner_2d(SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_copeland_scores_2d(SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_copeland_winner_2d(SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_core_2d(SEXP, SEXP, SEXP);
extern SEXP r_scs_has_condorcet_winner_2d(SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_uncovered_set_2d(SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_uncovered_set_boundary_2d(SEXP, SEXP, SEXP, SEXP);

/* B3.6 centrality (geometry.c) */
extern SEXP r_scs_centroid_2d(SEXP);
extern SEXP r_scs_geometric_mean_2d(SEXP);
extern SEXP r_scs_marginal_median_2d(SEXP);

/* B3.7 (voting_rules.c) */
extern SEXP r_scs_antiplurality_all_winners(SEXP);
extern SEXP r_scs_antiplurality_one_winner(SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_antiplurality_scores(SEXP);
extern SEXP r_scs_approval_all_winners_spatial(SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_approval_all_winners_topk(SEXP, SEXP);
extern SEXP r_scs_approval_scores_spatial(SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_approval_scores_topk(SEXP, SEXP);
extern SEXP r_scs_borda_all_winners(SEXP);
extern SEXP r_scs_borda_one_winner(SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_borda_ranking(SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_borda_scores(SEXP);
extern SEXP r_scs_plurality_all_winners(SEXP);
extern SEXP r_scs_plurality_one_winner(SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_plurality_scores(SEXP);
extern SEXP r_scs_scoring_rule_all_winners(SEXP, SEXP);
extern SEXP r_scs_scoring_rule_one_winner(SEXP, SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_scoring_rule_scores(SEXP, SEXP);

/* B3.8 (social.c) */
extern SEXP r_scs_condorcet_winner_profile(SEXP);
extern SEXP r_scs_has_condorcet_winner_profile(SEXP);
extern SEXP r_scs_is_condorcet_consistent(SEXP, SEXP);
extern SEXP r_scs_is_majority_consistent(SEXP, SEXP);
extern SEXP r_scs_is_pareto_efficient(SEXP, SEXP);
extern SEXP r_scs_pareto_set(SEXP);
extern SEXP r_scs_pairwise_ranking_from_matrix(SEXP, SEXP, SEXP, SEXP, SEXP);
extern SEXP r_scs_rank_by_scores(SEXP, SEXP, SEXP, SEXP);

/* ---------------------------------------------------------------------------
 * .Call() entry point table
 * --------------------------------------------------------------------------- */
static const R_CallMethodDef call_methods[] = {
    /* --- B2: StreamManager --- */
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
    /* --- B2: Winset --- */
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
    /* --- B2: Profile --- */
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
    /* --- B8: Competition --- */
    {"r_scs_competition_run",                         (DL_FUNC)&r_scs_competition_run,                         7},
    {"r_scs_competition_trace_destroy",               (DL_FUNC)&r_scs_competition_trace_destroy,               1},
    {"r_scs_competition_trace_dims",                  (DL_FUNC)&r_scs_competition_trace_dims,                  1},
    {"r_scs_competition_trace_termination",           (DL_FUNC)&r_scs_competition_trace_termination,           1},
    {"r_scs_competition_trace_round_positions",       (DL_FUNC)&r_scs_competition_trace_round_positions,       2},
    {"r_scs_competition_trace_final_positions",       (DL_FUNC)&r_scs_competition_trace_final_positions,       1},
    {"r_scs_competition_trace_round_vote_shares",     (DL_FUNC)&r_scs_competition_trace_round_vote_shares,     2},
    {"r_scs_competition_trace_round_seat_shares",     (DL_FUNC)&r_scs_competition_trace_round_seat_shares,     2},
    {"r_scs_competition_trace_round_vote_totals",     (DL_FUNC)&r_scs_competition_trace_round_vote_totals,     2},
    {"r_scs_competition_trace_round_seat_totals",     (DL_FUNC)&r_scs_competition_trace_round_seat_totals,     2},
    {"r_scs_competition_trace_final_vote_shares",     (DL_FUNC)&r_scs_competition_trace_final_vote_shares,     1},
    {"r_scs_competition_trace_final_seat_shares",     (DL_FUNC)&r_scs_competition_trace_final_seat_shares,     1},
    {"r_scs_competition_run_experiment",              (DL_FUNC)&r_scs_competition_run_experiment,              9},
    {"r_scs_competition_experiment_destroy",          (DL_FUNC)&r_scs_competition_experiment_destroy,          1},
    {"r_scs_competition_experiment_dims",             (DL_FUNC)&r_scs_competition_experiment_dims,             1},
    {"r_scs_competition_experiment_summary",          (DL_FUNC)&r_scs_competition_experiment_summary,          1},
    {"r_scs_competition_experiment_mean_final_vote_shares",
                                                    (DL_FUNC)&r_scs_competition_experiment_mean_final_vote_shares, 1},
    {"r_scs_competition_experiment_mean_final_seat_shares",
                                                    (DL_FUNC)&r_scs_competition_experiment_mean_final_seat_shares, 1},
    {"r_scs_competition_experiment_run_round_counts", (DL_FUNC)&r_scs_competition_experiment_run_round_counts, 1},
    {"r_scs_competition_experiment_run_termination_reasons",
                                                    (DL_FUNC)&r_scs_competition_experiment_run_termination_reasons, 1},
    {"r_scs_competition_experiment_run_terminated_early_flags",
                                                    (DL_FUNC)&r_scs_competition_experiment_run_terminated_early_flags, 1},
    {"r_scs_competition_experiment_run_final_vote_shares",
                                                    (DL_FUNC)&r_scs_competition_experiment_run_final_vote_shares, 1},
    {"r_scs_competition_experiment_run_final_seat_shares",
                                                    (DL_FUNC)&r_scs_competition_experiment_run_final_seat_shares, 1},
    {"r_scs_competition_experiment_run_final_positions",
                                                    (DL_FUNC)&r_scs_competition_experiment_run_final_positions, 1},
    /* --- B3: version, distance, level-set, convex hull, majority --- */
    {"r_scs_api_version",                    (DL_FUNC)&r_scs_api_version,                    1},
    {"r_scs_calculate_distance",             (DL_FUNC)&r_scs_calculate_distance,             3},
    {"r_scs_convex_hull_2d",                 (DL_FUNC)&r_scs_convex_hull_2d,                 1},
    {"r_scs_distance_to_utility",            (DL_FUNC)&r_scs_distance_to_utility,            2},
    {"r_scs_level_set_1d",                   (DL_FUNC)&r_scs_level_set_1d,                   4},
    {"r_scs_level_set_2d",                   (DL_FUNC)&r_scs_level_set_2d,                   5},
    {"r_scs_level_set_to_polygon",           (DL_FUNC)&r_scs_level_set_to_polygon,           2},
    {"r_scs_majority_prefers_2d",            (DL_FUNC)&r_scs_majority_prefers_2d,            7},
    {"r_scs_normalize_utility",              (DL_FUNC)&r_scs_normalize_utility,              3},
    {"r_scs_pairwise_matrix_2d",             (DL_FUNC)&r_scs_pairwise_matrix_2d,             4},
    {"r_scs_weighted_majority_prefers_2d",   (DL_FUNC)&r_scs_weighted_majority_prefers_2d,   8},
    /* --- B3: Copeland, Condorcet, core, uncovered set --- */
    {"r_scs_condorcet_winner_2d",            (DL_FUNC)&r_scs_condorcet_winner_2d,            4},
    {"r_scs_copeland_scores_2d",             (DL_FUNC)&r_scs_copeland_scores_2d,             4},
    {"r_scs_copeland_winner_2d",             (DL_FUNC)&r_scs_copeland_winner_2d,             4},
    {"r_scs_core_2d",                        (DL_FUNC)&r_scs_core_2d,                        3},
    {"r_scs_has_condorcet_winner_2d",        (DL_FUNC)&r_scs_has_condorcet_winner_2d,        4},
    {"r_scs_uncovered_set_2d",               (DL_FUNC)&r_scs_uncovered_set_2d,               4},
    {"r_scs_uncovered_set_boundary_2d",      (DL_FUNC)&r_scs_uncovered_set_boundary_2d,      4},
    /* --- B3: Centrality --- */
    {"r_scs_centroid_2d",                    (DL_FUNC)&r_scs_centroid_2d,                    1},
    {"r_scs_geometric_mean_2d",              (DL_FUNC)&r_scs_geometric_mean_2d,              1},
    {"r_scs_marginal_median_2d",             (DL_FUNC)&r_scs_marginal_median_2d,             1},
    /* --- B3: Voting rules --- */
    {"r_scs_antiplurality_all_winners",      (DL_FUNC)&r_scs_antiplurality_all_winners,      1},
    {"r_scs_antiplurality_one_winner",       (DL_FUNC)&r_scs_antiplurality_one_winner,       4},
    {"r_scs_antiplurality_scores",           (DL_FUNC)&r_scs_antiplurality_scores,           1},
    {"r_scs_approval_all_winners_spatial",   (DL_FUNC)&r_scs_approval_all_winners_spatial,   4},
    {"r_scs_approval_all_winners_topk",      (DL_FUNC)&r_scs_approval_all_winners_topk,      2},
    {"r_scs_approval_scores_spatial",        (DL_FUNC)&r_scs_approval_scores_spatial,        4},
    {"r_scs_approval_scores_topk",           (DL_FUNC)&r_scs_approval_scores_topk,           2},
    {"r_scs_borda_all_winners",              (DL_FUNC)&r_scs_borda_all_winners,              1},
    {"r_scs_borda_one_winner",               (DL_FUNC)&r_scs_borda_one_winner,               4},
    {"r_scs_borda_ranking",                  (DL_FUNC)&r_scs_borda_ranking,                  4},
    {"r_scs_borda_scores",                   (DL_FUNC)&r_scs_borda_scores,                   1},
    {"r_scs_plurality_all_winners",          (DL_FUNC)&r_scs_plurality_all_winners,          1},
    {"r_scs_plurality_one_winner",           (DL_FUNC)&r_scs_plurality_one_winner,           4},
    {"r_scs_plurality_scores",               (DL_FUNC)&r_scs_plurality_scores,               1},
    {"r_scs_scoring_rule_all_winners",       (DL_FUNC)&r_scs_scoring_rule_all_winners,       2},
    {"r_scs_scoring_rule_one_winner",        (DL_FUNC)&r_scs_scoring_rule_one_winner,        5},
    {"r_scs_scoring_rule_scores",            (DL_FUNC)&r_scs_scoring_rule_scores,            2},
    /* --- B3: Social rankings and properties --- */
    {"r_scs_condorcet_winner_profile",       (DL_FUNC)&r_scs_condorcet_winner_profile,       1},
    {"r_scs_has_condorcet_winner_profile",   (DL_FUNC)&r_scs_has_condorcet_winner_profile,   1},
    {"r_scs_is_condorcet_consistent",        (DL_FUNC)&r_scs_is_condorcet_consistent,        2},
    {"r_scs_is_majority_consistent",         (DL_FUNC)&r_scs_is_majority_consistent,         2},
    {"r_scs_is_pareto_efficient",            (DL_FUNC)&r_scs_is_pareto_efficient,            2},
    {"r_scs_pareto_set",                     (DL_FUNC)&r_scs_pareto_set,                     1},
    {"r_scs_pairwise_ranking_from_matrix",   (DL_FUNC)&r_scs_pairwise_ranking_from_matrix,   5},
    {"r_scs_rank_by_scores",                 (DL_FUNC)&r_scs_rank_by_scores,                 4},
    {NULL, NULL, 0} /* sentinel */
};

void R_init_socialchoicelab(DllInfo *dll) {
    R_registerRoutines(dll, NULL, call_methods, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}
