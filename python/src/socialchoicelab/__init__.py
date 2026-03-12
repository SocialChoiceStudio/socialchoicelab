# socialchoicelab — spatial social choice analysis.

from socialchoicelab._loader import _lib, _ffi  # noqa: F401  eagerly loads libscs_api

# B4.2 — Exception hierarchy
from socialchoicelab._error import (  # noqa: F401
    SCSError,
    SCSInvalidArgumentError,
    SCSInternalError,
    SCSBufferTooSmallError,
    SCSOutOfMemoryError,
)

# B4.1 — Configuration types
from socialchoicelab._types import (  # noqa: F401
    DistanceConfig,
    LossConfig,
    make_dist_config,
    make_loss_config,
)

# B4.3 — StreamManager
from socialchoicelab._stream_manager import StreamManager  # noqa: F401

# B4.4 — Winset
from socialchoicelab._winset import (  # noqa: F401
    Winset,
    winset_2d,
    weighted_winset_2d,
    winset_with_veto_2d,
)

# B4.5 — Profile
from socialchoicelab._profile import (  # noqa: F401
    Profile,
    profile_build_spatial,
    profile_from_utility_matrix,
    profile_impartial_culture,
    profile_uniform_spatial,
    profile_gaussian_spatial,
    VoterSamplerConfig,
    make_voter_sampler,
    draw_voters,
)

# B5.1 — Version, distance, loss, level-set, geometry
from socialchoicelab._functions import (  # noqa: F401
    scs_version,
    calculate_distance,
    distance_to_utility,
    normalize_utility,
    level_set_1d,
    level_set_2d,
    level_set_to_polygon,
    convex_hull_2d,
    majority_prefers_2d,
    weighted_majority_prefers_2d,
    pairwise_matrix_2d,
)

# B5.2 — Copeland, Condorcet, core, uncovered set, centrality
from socialchoicelab._geometry import (  # noqa: F401
    copeland_scores_2d,
    copeland_winner_2d,
    has_condorcet_winner_2d,
    condorcet_winner_2d,
    core_2d,
    uncovered_set_2d,
    uncovered_set_boundary_2d,
    marginal_median_2d,
    centroid_2d,
    geometric_mean_2d,
)

# B5.3 — Voting rules
from socialchoicelab._voting_rules import (  # noqa: F401
    plurality_scores,
    plurality_all_winners,
    plurality_one_winner,
    borda_scores,
    borda_all_winners,
    borda_one_winner,
    borda_ranking,
    antiplurality_scores,
    antiplurality_all_winners,
    antiplurality_one_winner,
    scoring_rule_scores,
    scoring_rule_all_winners,
    scoring_rule_one_winner,
    approval_scores_spatial,
    approval_all_winners_spatial,
    approval_scores_topk,
    approval_all_winners_topk,
)

# B5.4 — Social rankings and properties
from socialchoicelab._social import (  # noqa: F401
    rank_by_scores,
    pairwise_ranking_from_matrix,
    pareto_set,
    is_pareto_efficient,
    has_condorcet_winner_profile,
    condorcet_winner_profile,
    is_condorcet_consistent,
    is_majority_consistent,
)

from socialchoicelab._competition import (  # noqa: F401
    CompetitionStepConfig,
    CompetitionTerminationConfig,
    CompetitionEngineConfig,
    CompetitionTrace,
    CompetitionExperiment,
    competition_run,
    competition_run_experiment,
)

# C13.B — Built-in scenarios
from socialchoicelab.scenarios import (  # noqa: F401
    list_scenarios,
    load_scenario,
)

# C13.4 — Palette and theme utilities (also accessible via socialchoicelab.plots)
from socialchoicelab.palette import (  # noqa: F401
    scl_palette,
    scl_theme_colors,
)

__version__ = "0.2.0"
