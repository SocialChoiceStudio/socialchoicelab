"""cffi ABI-mode loader for libscs_api.

Locates and opens the pre-built libscs_api shared library, then parses the
C declarations from scs_api.h so that all C API functions are callable via
the module-level ``_lib`` and ``_ffi`` singletons.

Library search order
--------------------
1. ``SCS_LIB_PATH`` environment variable (directory containing the library).
2. Standard system library paths (LD_LIBRARY_PATH / DYLD_LIBRARY_PATH /
   PATH depending on platform).

Set ``SCS_LIB_PATH`` to the ``build/`` directory of the socialchoicelab
C++ project for local development, or to the directory where the pre-built
library artifact was downloaded in CI.
"""

from __future__ import annotations

import os
import sys
from pathlib import Path

import cffi

# ---------------------------------------------------------------------------
# Locate libscs_api
# ---------------------------------------------------------------------------

def _find_library() -> str:
    """Return the path to libscs_api, or raise ImportError."""
    if sys.platform == "win32":
        lib_names = ["scs_api.dll"]
    elif sys.platform == "darwin":
        lib_names = ["libscs_api.dylib"]
    else:
        lib_names = ["libscs_api.so"]

    # 1. SCS_LIB_PATH environment variable.
    env_path = os.environ.get("SCS_LIB_PATH")
    if env_path:
        for name in lib_names:
            candidate = Path(env_path) / name
            if candidate.exists():
                return str(candidate)
        raise ImportError(
            f"SCS_LIB_PATH is set to '{env_path}' but none of "
            f"{lib_names} were found there. "
            f"Check that libscs_api has been built (cmake --build build) "
            f"and that SCS_LIB_PATH points to the directory containing it."
        )

    # 2. Let cffi try to find it on the system path.
    # This covers installed distributions where the library is on LD_LIBRARY_PATH.
    for name in lib_names:
        try:
            ffi_probe = cffi.FFI()
            ffi_probe.cdef("void _probe(void);")
            lib = ffi_probe.dlopen(name)
            return name  # cffi resolved it; use the bare name
        except OSError:
            continue

    raise ImportError(
        f"Could not locate libscs_api ({lib_names}). "
        f"Set the SCS_LIB_PATH environment variable to the directory "
        f"containing the library, e.g.:\n"
        f"  export SCS_LIB_PATH=/path/to/socialchoicelab/build"
    )


# ---------------------------------------------------------------------------
# C declarations (stripped from scs_api.h for cffi ABI mode)
# ---------------------------------------------------------------------------

_DECLARATIONS = """
    /* Fixed-width integer types */
    typedef unsigned long long uint64_t;
    typedef signed int         int32_t;
    typedef signed long long   int64_t;

    /* ---------------------------------------------------------------------------
     * Pairwise result and tie-break scalars
     * ------------------------------------------------------------------------- */
    typedef int32_t SCS_PairwiseResult;
    typedef int32_t SCS_TieBreak;

    /* ---------------------------------------------------------------------------
     * Enums
     * ------------------------------------------------------------------------- */
    typedef enum {
        SCS_LOSS_LINEAR    = 0,
        SCS_LOSS_QUADRATIC = 1,
        SCS_LOSS_GAUSSIAN  = 2,
        SCS_LOSS_THRESHOLD = 3
    } SCS_LossType;

    typedef enum {
        SCS_DIST_EUCLIDEAN = 0,
        SCS_DIST_MANHATTAN = 1,
        SCS_DIST_CHEBYSHEV = 2,
        SCS_DIST_MINKOWSKI = 3
    } SCS_DistanceType;

    typedef enum {
        SCS_LEVEL_SET_CIRCLE       = 0,
        SCS_LEVEL_SET_ELLIPSE      = 1,
        SCS_LEVEL_SET_SUPERELLIPSE = 2,
        SCS_LEVEL_SET_POLYGON      = 3
    } SCS_LevelSetType;

    /* ---------------------------------------------------------------------------
     * POD structs
     * ------------------------------------------------------------------------- */
    typedef struct {
        SCS_LossType loss_type;
        double sensitivity;
        double max_loss;
        double steepness;
        double threshold;
    } SCS_LossConfig;

    typedef struct {
        SCS_DistanceType  distance_type;
        double            order_p;
        const double*     salience_weights;
        int               n_weights;
    } SCS_DistanceConfig;

    typedef struct {
        SCS_LevelSetType type;
        double center_x;
        double center_y;
        double param0;
        double param1;
        double exponent_p;
        int    n_vertices;
        double vertices[8];
    } SCS_LevelSet2d;

    typedef struct {
        double center_x;
        double center_y;
        double radius;
    } SCS_Yolk2d;

    /* ---------------------------------------------------------------------------
     * Opaque handles
     * ------------------------------------------------------------------------- */
    typedef struct SCS_StreamManagerImpl SCS_StreamManager;
    typedef struct SCS_WinsetImpl        SCS_Winset;
    typedef struct SCS_ProfileImpl       SCS_Profile;
    typedef struct SCS_CompetitionTraceImpl SCS_CompetitionTrace;
    typedef struct SCS_CompetitionExperimentImpl SCS_CompetitionExperiment;

    typedef enum {
        SCS_COMPETITION_STRATEGY_STICKER = 0,
        SCS_COMPETITION_STRATEGY_HUNTER = 1,
        SCS_COMPETITION_STRATEGY_AGGREGATOR = 2,
        SCS_COMPETITION_STRATEGY_PREDATOR = 3
    } SCS_CompetitionStrategyKind;

    typedef enum {
        SCS_COMPETITION_SEAT_RULE_PLURALITY_TOP_K = 0,
        SCS_COMPETITION_SEAT_RULE_HARE_LARGEST_REMAINDER = 1
    } SCS_CompetitionSeatRule;

    typedef enum {
        SCS_COMPETITION_STEP_FIXED = 0,
        SCS_COMPETITION_STEP_RANDOM_UNIFORM = 1,
        SCS_COMPETITION_STEP_SHARE_DELTA_PROPORTIONAL = 2
    } SCS_CompetitionStepPolicyKind;

    typedef enum {
        SCS_COMPETITION_BOUNDARY_PROJECT_TO_BOX = 0,
        SCS_COMPETITION_BOUNDARY_STAY_PUT = 1,
        SCS_COMPETITION_BOUNDARY_REFLECT = 2
    } SCS_CompetitionBoundaryPolicy;

    typedef enum {
        SCS_COMPETITION_OBJECTIVE_VOTE_SHARE = 0,
        SCS_COMPETITION_OBJECTIVE_SEAT_SHARE = 1
    } SCS_CompetitionObjectiveKind;

    typedef enum {
        SCS_COMPETITION_TERM_MAX_ROUNDS = 0,
        SCS_COMPETITION_TERM_CONVERGED = 1,
        SCS_COMPETITION_TERM_CYCLE_DETECTED = 2,
        SCS_COMPETITION_TERM_NO_IMPROVEMENT_WINDOW = 3
    } SCS_CompetitionTerminationReason;

    typedef struct {
        SCS_CompetitionStepPolicyKind kind;
        double fixed_step_size;
        double min_step_size;
        double max_step_size;
        double proportionality_constant;
        double jitter;
    } SCS_CompetitionStepConfig;

    typedef struct {
        int stop_on_convergence;
        double position_epsilon;
        int stop_on_cycle;
        int cycle_memory;
        double signature_resolution;
        int stop_on_no_improvement;
        int no_improvement_window;
        double objective_epsilon;
    } SCS_CompetitionTerminationConfig;

    typedef struct {
        int seat_count;
        SCS_CompetitionSeatRule seat_rule;
        SCS_CompetitionStepConfig step_config;
        SCS_CompetitionBoundaryPolicy boundary_policy;
        SCS_CompetitionObjectiveKind objective_kind;
        int max_rounds;
        SCS_CompetitionTerminationConfig termination;
    } SCS_CompetitionEngineConfig;

    /* ---------------------------------------------------------------------------
     * API version
     * ------------------------------------------------------------------------- */
    int scs_api_version(int* out_major, int* out_minor, int* out_patch,
                        char* err_buf, int err_buf_len);

    /* ---------------------------------------------------------------------------
     * Distance and loss functions
     * ------------------------------------------------------------------------- */
    int scs_calculate_distance(const double* ideal_point,
                               const double* alt_point, int n_dims,
                               const SCS_DistanceConfig* dist_cfg,
                               double* out, char* err_buf, int err_buf_len);

    int scs_distance_to_utility(double distance,
                                const SCS_LossConfig* loss_cfg, double* out,
                                char* err_buf, int err_buf_len);

    int scs_normalize_utility(double utility, double max_distance,
                              const SCS_LossConfig* loss_cfg, double* out,
                              char* err_buf, int err_buf_len);

    /* ---------------------------------------------------------------------------
     * Level-set functions
     * ------------------------------------------------------------------------- */
    int scs_level_set_1d(double ideal, double weight, double utility_level,
                         const SCS_LossConfig* loss_cfg, double* out_points,
                         int* out_n, char* err_buf, int err_buf_len);

    int scs_level_set_2d(double ideal_x, double ideal_y,
                         double utility_level,
                         const SCS_LossConfig* loss_cfg,
                         const SCS_DistanceConfig* dist_cfg,
                         SCS_LevelSet2d* out, char* err_buf, int err_buf_len);

    int scs_level_set_to_polygon(const SCS_LevelSet2d* level_set,
                                 int num_samples, double* out_xy,
                                 int out_capacity, int* out_n,
                                 char* err_buf, int err_buf_len);

    /* ---------------------------------------------------------------------------
     * Geometry — stateless functions
     * ------------------------------------------------------------------------- */
    int scs_convex_hull_2d(const double* points, int n_points,
                           double* out_xy, int* out_n,
                           char* err_buf, int err_buf_len);

    int scs_majority_prefers_2d(double point_a_x, double point_a_y,
                                double point_b_x, double point_b_y,
                                const double* voter_ideals_xy, int n_voters,
                                const SCS_DistanceConfig* dist_cfg, int k,
                                int* out, char* err_buf, int err_buf_len);

    int scs_pairwise_matrix_2d(const double* alt_xy, int n_alts,
                               const double* voter_ideals_xy, int n_voters,
                               const SCS_DistanceConfig* dist_cfg, int k,
                               SCS_PairwiseResult* out_matrix, int out_len,
                               char* err_buf, int err_buf_len);

    int scs_weighted_majority_prefers_2d(
        double point_a_x, double point_a_y, double point_b_x, double point_b_y,
        const double* voter_ideals_xy, int n_voters, const double* weights,
        const SCS_DistanceConfig* dist_cfg, double threshold, int* out,
        char* err_buf, int err_buf_len);

    /* ---------------------------------------------------------------------------
     * Geometry — Copeland, Condorcet, core, uncovered set
     * ------------------------------------------------------------------------- */
    int scs_copeland_scores_2d(const double* alt_xy, int n_alts,
                               const double* voter_ideals_xy, int n_voters,
                               const SCS_DistanceConfig* dist_cfg, int k,
                               int* out_scores, int out_len,
                               char* err_buf, int err_buf_len);

    int scs_copeland_winner_2d(const double* alt_xy, int n_alts,
                               const double* voter_ideals_xy, int n_voters,
                               const SCS_DistanceConfig* dist_cfg, int k,
                               int* out_winner_idx,
                               char* err_buf, int err_buf_len);

    int scs_has_condorcet_winner_2d(const double* alt_xy, int n_alts,
                                    const double* voter_ideals_xy, int n_voters,
                                    const SCS_DistanceConfig* dist_cfg, int k,
                                    int* out_found, char* err_buf,
                                    int err_buf_len);

    int scs_condorcet_winner_2d(const double* alt_xy, int n_alts,
                                const double* voter_ideals_xy, int n_voters,
                                const SCS_DistanceConfig* dist_cfg, int k,
                                int* out_found, int* out_winner_idx,
                                char* err_buf, int err_buf_len);

    int scs_core_2d(const double* voter_ideals_xy, int n_voters,
                    const SCS_DistanceConfig* dist_cfg, int k,
                    int* out_found, double* out_x, double* out_y,
                    char* err_buf, int err_buf_len);

    int scs_uncovered_set_2d(const double* alt_xy, int n_alts,
                             const double* voter_ideals_xy, int n_voters,
                             const SCS_DistanceConfig* dist_cfg, int k,
                             int* out_indices, int out_capacity, int* out_n,
                             char* err_buf, int err_buf_len);

    int scs_uncovered_set_boundary_size_2d(
        const double* voter_ideals_xy, int n_voters,
        const SCS_DistanceConfig* dist_cfg, int grid_resolution, int k,
        int* out_xy_pairs, char* err_buf, int err_buf_len);

    int scs_uncovered_set_boundary_2d(const double* voter_ideals_xy,
                                      int n_voters,
                                      const SCS_DistanceConfig* dist_cfg,
                                      int grid_resolution, int k,
                                      double* out_xy, int out_capacity,
                                      int* out_n, char* err_buf,
                                      int err_buf_len);

    /* ---------------------------------------------------------------------------
     * StreamManager lifecycle and PRNG draws
     * ------------------------------------------------------------------------- */
    SCS_StreamManager* scs_stream_manager_create(uint64_t master_seed,
                                                 char* err_buf,
                                                 int err_buf_len);

    void scs_stream_manager_destroy(SCS_StreamManager* mgr);

    int scs_register_streams(SCS_StreamManager* mgr, const char** names,
                             int n_names, char* err_buf, int err_buf_len);

    int scs_reset_all(SCS_StreamManager* mgr, uint64_t master_seed,
                      char* err_buf, int err_buf_len);

    int scs_reset_stream(SCS_StreamManager* mgr, const char* stream_name,
                         uint64_t seed, char* err_buf, int err_buf_len);

    int scs_skip(SCS_StreamManager* mgr, const char* stream_name,
                 uint64_t n, char* err_buf, int err_buf_len);

    int scs_uniform_real(SCS_StreamManager* mgr, const char* stream_name,
                         double min, double max, double* out,
                         char* err_buf, int err_buf_len);

    int scs_normal(SCS_StreamManager* mgr, const char* stream_name,
                   double mean, double stddev, double* out,
                   char* err_buf, int err_buf_len);

    int scs_bernoulli(SCS_StreamManager* mgr, const char* stream_name,
                      double probability, int* out,
                      char* err_buf, int err_buf_len);

    int scs_uniform_int(SCS_StreamManager* mgr, const char* stream_name,
                        int64_t min, int64_t max, int64_t* out,
                        char* err_buf, int err_buf_len);

    int scs_uniform_choice(SCS_StreamManager* mgr, const char* stream_name,
                           int64_t n, int64_t* out,
                           char* err_buf, int err_buf_len);

    /* ---------------------------------------------------------------------------
     * Winset factory functions
     * ------------------------------------------------------------------------- */
    SCS_Winset* scs_winset_2d(double status_quo_x, double status_quo_y,
                              const double* voter_ideals_xy, int n_voters,
                              const SCS_DistanceConfig* dist_cfg, int k,
                              int num_samples, char* err_buf, int err_buf_len);

    SCS_Winset* scs_weighted_winset_2d(
        double status_quo_x, double status_quo_y,
        const double* voter_ideals_xy, int n_voters,
        const double* weights, const SCS_DistanceConfig* dist_cfg,
        double threshold, int num_samples, char* err_buf, int err_buf_len);

    SCS_Winset* scs_winset_with_veto_2d(
        double status_quo_x, double status_quo_y,
        const double* voter_ideals_xy, int n_voters,
        const SCS_DistanceConfig* dist_cfg,
        const double* veto_ideals_xy, int n_veto,
        int k, int num_samples, char* err_buf, int err_buf_len);

    /* ---------------------------------------------------------------------------
     * Winset lifecycle, queries, boolean operations
     * ------------------------------------------------------------------------- */
    void scs_winset_destroy(SCS_Winset* ws);

    int scs_winset_is_empty(const SCS_Winset* ws, int* out_is_empty,
                            char* err_buf, int err_buf_len);

    int scs_winset_contains_point_2d(const SCS_Winset* ws, double x, double y,
                                     int* out_contains,
                                     char* err_buf, int err_buf_len);

    int scs_winset_bbox_2d(const SCS_Winset* ws, int* out_found,
                           double* out_min_x, double* out_min_y,
                           double* out_max_x, double* out_max_y,
                           char* err_buf, int err_buf_len);

    int scs_winset_boundary_size_2d(const SCS_Winset* ws, int* out_xy_pairs,
                                    int* out_n_paths,
                                    char* err_buf, int err_buf_len);

    int scs_winset_sample_boundary_2d(
        const SCS_Winset* ws,
        double* out_xy, int out_xy_capacity, int* out_xy_n,
        int* out_path_starts, int out_path_capacity, int* out_path_is_hole,
        int* out_n_paths, char* err_buf, int err_buf_len);

    SCS_Winset* scs_winset_union(const SCS_Winset* a, const SCS_Winset* b,
                                 char* err_buf, int err_buf_len);

    SCS_Winset* scs_winset_intersection(const SCS_Winset* a,
                                        const SCS_Winset* b,
                                        char* err_buf, int err_buf_len);

    SCS_Winset* scs_winset_difference(const SCS_Winset* a, const SCS_Winset* b,
                                      char* err_buf, int err_buf_len);

    SCS_Winset* scs_winset_symmetric_difference(const SCS_Winset* a,
                                                const SCS_Winset* b,
                                                char* err_buf, int err_buf_len);

    SCS_Winset* scs_winset_clone(const SCS_Winset* ws,
                                 char* err_buf, int err_buf_len);

    /* ---------------------------------------------------------------------------
     * Profile factory functions
     * ------------------------------------------------------------------------- */
    SCS_Profile* scs_profile_build_spatial(
        const double* alt_xy, int n_alts,
        const double* voter_ideals_xy, int n_voters,
        const SCS_DistanceConfig* dist_cfg,
        char* err_buf, int err_buf_len);

    SCS_Profile* scs_profile_from_utility_matrix(
        const double* utilities, int n_voters, int n_alts,
        char* err_buf, int err_buf_len);

    SCS_Profile* scs_profile_impartial_culture(
        int n_voters, int n_alts,
        SCS_StreamManager* mgr, const char* stream_name,
        char* err_buf, int err_buf_len);

    SCS_Profile* scs_profile_uniform_spatial(
        int n_voters, int n_alts, int n_dims, double lo, double hi,
        const SCS_DistanceConfig* dist_cfg,
        SCS_StreamManager* mgr, const char* stream_name,
        char* err_buf, int err_buf_len);

    SCS_Profile* scs_profile_gaussian_spatial(
        int n_voters, int n_alts, int n_dims, double mean, double stddev,
        const SCS_DistanceConfig* dist_cfg,
        SCS_StreamManager* mgr, const char* stream_name,
        char* err_buf, int err_buf_len);

    /* ---------------------------------------------------------------------------
     * Voter sampling
     * ------------------------------------------------------------------------- */
    typedef enum {
        SCS_VOTER_SAMPLER_UNIFORM  = 0,
        SCS_VOTER_SAMPLER_GAUSSIAN = 1
    } SCS_VoterSamplerKind;

    typedef struct {
        SCS_VoterSamplerKind kind;
        double param1;
        double param2;
    } SCS_VoterSamplerConfig;

    int scs_draw_voters(
        int n_voters, int n_dims,
        const SCS_VoterSamplerConfig* config,
        SCS_StreamManager* mgr, const char* stream_name,
        double* out_xy, int out_len,
        char* err_buf, int err_buf_len);

    /* ---------------------------------------------------------------------------
     * Profile lifecycle and inspection
     * ------------------------------------------------------------------------- */
    void scs_profile_destroy(SCS_Profile* p);

    int scs_profile_dims(const SCS_Profile* p,
                         int* out_n_voters, int* out_n_alts,
                         char* err_buf, int err_buf_len);

    int scs_profile_get_ranking(const SCS_Profile* p, int voter,
                                int* out_ranking, int out_len,
                                char* err_buf, int err_buf_len);

    int scs_profile_export_rankings(const SCS_Profile* p,
                                    int* out_rankings, int out_len,
                                    char* err_buf, int err_buf_len);

    SCS_Profile* scs_profile_clone(const SCS_Profile* p,
                                   char* err_buf, int err_buf_len);

    /* ---------------------------------------------------------------------------
     * Voting rules — Plurality
     * ------------------------------------------------------------------------- */
    int scs_plurality_scores(const SCS_Profile* p, int* out_scores,
                             int out_len, char* err_buf, int err_buf_len);

    int scs_plurality_all_winners(const SCS_Profile* p,
                                  int* out_winners, int out_capacity,
                                  int* out_n, char* err_buf, int err_buf_len);

    int scs_plurality_one_winner(const SCS_Profile* p, SCS_TieBreak tie_break,
                                 SCS_StreamManager* mgr,
                                 const char* stream_name, int* out_winner,
                                 char* err_buf, int err_buf_len);

    /* ---------------------------------------------------------------------------
     * Voting rules — Borda
     * ------------------------------------------------------------------------- */
    int scs_borda_scores(const SCS_Profile* p, int* out_scores,
                         int out_len, char* err_buf, int err_buf_len);

    int scs_borda_all_winners(const SCS_Profile* p,
                              int* out_winners, int out_capacity,
                              int* out_n, char* err_buf, int err_buf_len);

    int scs_borda_one_winner(const SCS_Profile* p, SCS_TieBreak tie_break,
                             SCS_StreamManager* mgr, const char* stream_name,
                             int* out_winner, char* err_buf, int err_buf_len);

    int scs_borda_ranking(const SCS_Profile* p, SCS_TieBreak tie_break,
                          SCS_StreamManager* mgr, const char* stream_name,
                          int* out_ranking, int out_len,
                          char* err_buf, int err_buf_len);

    /* ---------------------------------------------------------------------------
     * Voting rules — Anti-plurality
     * ------------------------------------------------------------------------- */
    int scs_antiplurality_scores(const SCS_Profile* p, int* out_scores,
                                 int out_len, char* err_buf, int err_buf_len);

    int scs_antiplurality_all_winners(const SCS_Profile* p,
                                      int* out_winners, int out_capacity,
                                      int* out_n, char* err_buf,
                                      int err_buf_len);

    int scs_antiplurality_one_winner(const SCS_Profile* p,
                                     SCS_TieBreak tie_break,
                                     SCS_StreamManager* mgr,
                                     const char* stream_name, int* out_winner,
                                     char* err_buf, int err_buf_len);

    /* ---------------------------------------------------------------------------
     * Voting rules — Generic scoring rule
     * ------------------------------------------------------------------------- */
    int scs_scoring_rule_scores(const SCS_Profile* p,
                                const double* score_weights, int n_weights,
                                double* out_scores, int out_len,
                                char* err_buf, int err_buf_len);

    int scs_scoring_rule_all_winners(const SCS_Profile* p,
                                     const double* score_weights, int n_weights,
                                     int* out_winners, int out_capacity,
                                     int* out_n, char* err_buf,
                                     int err_buf_len);

    int scs_scoring_rule_one_winner(const SCS_Profile* p,
                                    const double* score_weights, int n_weights,
                                    SCS_TieBreak tie_break,
                                    SCS_StreamManager* mgr,
                                    const char* stream_name, int* out_winner,
                                    char* err_buf, int err_buf_len);

    /* ---------------------------------------------------------------------------
     * Voting rules — Approval voting
     * ------------------------------------------------------------------------- */
    int scs_approval_scores_spatial(const double* alt_xy, int n_alts,
                                    const double* voter_ideals_xy, int n_voters,
                                    double threshold,
                                    const SCS_DistanceConfig* dist_cfg,
                                    int* out_scores, int out_len,
                                    char* err_buf, int err_buf_len);

    int scs_approval_all_winners_spatial(const double* alt_xy, int n_alts,
                                         const double* voter_ideals_xy,
                                         int n_voters, double threshold,
                                         const SCS_DistanceConfig* dist_cfg,
                                         int* out_winners, int out_capacity,
                                         int* out_n, char* err_buf,
                                         int err_buf_len);

    int scs_approval_scores_topk(const SCS_Profile* p, int k,
                                 int* out_scores, int out_len,
                                 char* err_buf, int err_buf_len);

    int scs_approval_all_winners_topk(const SCS_Profile* p, int k,
                                      int* out_winners, int out_capacity,
                                      int* out_n, char* err_buf,
                                      int err_buf_len);

    /* ---------------------------------------------------------------------------
     * Social rankings and properties
     * ------------------------------------------------------------------------- */
    int scs_rank_by_scores(const double* scores, int n_alts,
                           SCS_TieBreak tie_break, SCS_StreamManager* mgr,
                           const char* stream_name, int* out_ranking,
                           int out_len, char* err_buf, int err_buf_len);

    int scs_pairwise_ranking_from_matrix(
        const SCS_PairwiseResult* pairwise_matrix, int n_alts,
        SCS_TieBreak tie_break, SCS_StreamManager* mgr,
        const char* stream_name, int* out_ranking, int out_len,
        char* err_buf, int err_buf_len);

    int scs_pareto_set(const SCS_Profile* p, int* out_indices,
                       int out_capacity, int* out_n,
                       char* err_buf, int err_buf_len);

    int scs_is_pareto_efficient(const SCS_Profile* p, int alt_idx, int* out,
                                char* err_buf, int err_buf_len);

    int scs_has_condorcet_winner_profile(const SCS_Profile* p,
                                         int* out_found,
                                         char* err_buf, int err_buf_len);

    int scs_condorcet_winner_profile(const SCS_Profile* p,
                                     int* out_found, int* out_winner,
                                     char* err_buf, int err_buf_len);

    int scs_is_selected_by_condorcet_consistent_rules(const SCS_Profile* p,
                                                      int alt_idx, int* out,
                                                      char* err_buf,
                                                      int err_buf_len);

    int scs_is_selected_by_majority_consistent_rules(const SCS_Profile* p,
                                                     int alt_idx, int* out,
                                                     char* err_buf,
                                                     int err_buf_len);

    /* ---------------------------------------------------------------------------
     * Centrality measures
     * ------------------------------------------------------------------------- */
    int scs_marginal_median_2d(const double* voter_ideals_xy, int n_voters,
                               double* out_x, double* out_y,
                               char* err_buf, int err_buf_len);

    int scs_centroid_2d(const double* voter_ideals_xy, int n_voters,
                        double* out_x, double* out_y,
                        char* err_buf, int err_buf_len);

    int scs_geometric_mean_2d(const double* voter_ideals_xy, int n_voters,
                              double* out_x, double* out_y,
                              char* err_buf, int err_buf_len);

    /* ---------------------------------------------------------------------------
     * Competition
     * ------------------------------------------------------------------------- */
    SCS_CompetitionTrace* scs_competition_run(
        const double* competitor_positions, const double* competitor_headings,
        const int* strategy_kinds, int n_competitors, const double* voter_ideals,
        int n_voters, int n_dims, const SCS_DistanceConfig* dist_cfg,
        const SCS_CompetitionEngineConfig* engine_cfg, SCS_StreamManager* mgr,
        char* err_buf, int err_buf_len);

    void scs_competition_trace_destroy(SCS_CompetitionTrace* trace);

    int scs_competition_trace_dims(const SCS_CompetitionTrace* trace,
                                   int* out_n_rounds,
                                   int* out_n_competitors,
                                   int* out_n_dims, char* err_buf,
                                   int err_buf_len);

    int scs_competition_trace_termination(
        const SCS_CompetitionTrace* trace, int* out_terminated_early,
        SCS_CompetitionTerminationReason* out_reason, char* err_buf,
        int err_buf_len);

    int scs_competition_trace_round_positions(
        const SCS_CompetitionTrace* trace, int round_index, double* out_positions,
        int out_len, char* err_buf, int err_buf_len);

    int scs_competition_trace_final_positions(
        const SCS_CompetitionTrace* trace, double* out_positions, int out_len,
        char* err_buf, int err_buf_len);

    int scs_competition_trace_round_vote_shares(
        const SCS_CompetitionTrace* trace, int round_index, double* out_shares,
        int out_len, char* err_buf, int err_buf_len);

    int scs_competition_trace_round_seat_shares(
        const SCS_CompetitionTrace* trace, int round_index, double* out_shares,
        int out_len, char* err_buf, int err_buf_len);

    int scs_competition_trace_round_vote_totals(
        const SCS_CompetitionTrace* trace, int round_index, int* out_totals,
        int out_len, char* err_buf, int err_buf_len);

    int scs_competition_trace_round_seat_totals(
        const SCS_CompetitionTrace* trace, int round_index, int* out_totals,
        int out_len, char* err_buf, int err_buf_len);

    int scs_competition_trace_final_vote_shares(
        const SCS_CompetitionTrace* trace, double* out_shares, int out_len,
        char* err_buf, int err_buf_len);

    int scs_competition_trace_final_seat_shares(
        const SCS_CompetitionTrace* trace, double* out_shares, int out_len,
        char* err_buf, int err_buf_len);

    int scs_competition_trace_strategy_kinds(
        const SCS_CompetitionTrace* trace, int* out_kinds, int out_len,
        char* err_buf, int err_buf_len);

    SCS_CompetitionExperiment* scs_competition_run_experiment(
        const double* competitor_positions, const double* competitor_headings,
        const int* strategy_kinds, int n_competitors, const double* voter_ideals,
        int n_voters, int n_dims, const SCS_DistanceConfig* dist_cfg,
        const SCS_CompetitionEngineConfig* engine_cfg, uint64_t master_seed,
        int num_runs, int retain_traces, char* err_buf, int err_buf_len);

    void scs_competition_experiment_destroy(
        SCS_CompetitionExperiment* experiment);

    int scs_competition_experiment_dims(
        const SCS_CompetitionExperiment* experiment, int* out_num_runs,
        int* out_n_competitors, int* out_n_dims, char* err_buf, int err_buf_len);

    int scs_competition_experiment_summary(
        const SCS_CompetitionExperiment* experiment, double* out_mean_rounds,
        double* out_early_termination_rate, char* err_buf, int err_buf_len);

    int scs_competition_experiment_mean_final_vote_shares(
        const SCS_CompetitionExperiment* experiment, double* out_shares, int out_len,
        char* err_buf, int err_buf_len);

    int scs_competition_experiment_mean_final_seat_shares(
        const SCS_CompetitionExperiment* experiment, double* out_shares, int out_len,
        char* err_buf, int err_buf_len);

    int scs_competition_experiment_run_round_counts(
        const SCS_CompetitionExperiment* experiment, int* out_round_counts,
        int out_len, char* err_buf, int err_buf_len);

    int scs_competition_experiment_run_termination_reasons(
        const SCS_CompetitionExperiment* experiment,
        SCS_CompetitionTerminationReason* out_reasons, int out_len, char* err_buf,
        int err_buf_len);

    int scs_competition_experiment_run_terminated_early_flags(
        const SCS_CompetitionExperiment* experiment, int* out_flags, int out_len,
        char* err_buf, int err_buf_len);

    int scs_competition_experiment_run_final_vote_shares(
        const SCS_CompetitionExperiment* experiment, double* out_shares, int out_len,
        char* err_buf, int err_buf_len);

    int scs_competition_experiment_run_final_seat_shares(
        const SCS_CompetitionExperiment* experiment, double* out_shares, int out_len,
        char* err_buf, int err_buf_len);

    int scs_competition_experiment_run_final_positions(
        const SCS_CompetitionExperiment* experiment, double* out_positions,
        int out_len, char* err_buf, int err_buf_len);
"""


# ---------------------------------------------------------------------------
# Module-level singletons
# ---------------------------------------------------------------------------

_ffi = cffi.FFI()
_ffi.cdef(_DECLARATIONS)

try:
    _lib_path = _find_library()
    _lib = _ffi.dlopen(_lib_path)
except ImportError as exc:
    raise ImportError(str(exc)) from None
