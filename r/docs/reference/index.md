# Package index

## Configuration

Create distance and loss-function configurations.

- [`make_dist_config()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_dist_config.md)
  : Create a distance configuration
- [`make_loss_config()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_loss_config.md)
  : Create a loss function configuration

## Distance and Utility

Compute distances, convert to utility, and compute level sets.

- [`calculate_distance()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/calculate_distance.md)
  : Compute the distance between two n-dimensional points

- [`distance_to_utility()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/distance_to_utility.md)
  : Convert a distance to a utility value

- [`normalize_utility()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/normalize_utility.md)
  :

  Normalize a utility value to `[0, 1]`

- [`level_set_1d()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/level_set_1d.md)
  : Compute the 1D level set (indifference points)

- [`level_set_2d()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/level_set_2d.md)
  : Compute the exact 2D level set (indifference curve)

- [`level_set_to_polygon()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/level_set_to_polygon.md)
  : Sample a 2D level set as a closed polygon

## Convex Hull and Majority Preference

Geometry utilities and pairwise majority tests.

- [`convex_hull_2d()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/convex_hull_2d.md)
  : Compute the 2D convex hull of a set of points
- [`majority_prefers_2d()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/majority_prefers_2d.md)
  : Test whether a k-majority of voters prefer point A to point B
- [`weighted_majority_prefers_2d()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/weighted_majority_prefers_2d.md)
  : Test whether the weighted majority prefers point A to point B
- [`pairwise_matrix_2d()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/pairwise_matrix_2d.md)
  : Compute the n_alts × n_alts pairwise preference matrix

## Winset

Construct and query 2D majority winset regions.

- [`Winset`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/Winset.md)
  : Winset: a 2D majority winset region
- [`winset_2d()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/winset_2d.md)
  : Compute the 2D k-majority winset of a status quo
- [`weighted_winset_2d()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/weighted_winset_2d.md)
  : Compute the weighted-majority 2D winset of a status quo
- [`winset_with_veto_2d()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/winset_with_veto_2d.md)
  : Compute the k-majority winset constrained by veto players

## Geometry: Copeland, Condorcet, Core, Uncovered Set

Compute Copeland scores, find Condorcet winners, and approximate the
continuous uncovered set.

- [`copeland_scores_2d()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/copeland_scores_2d.md)
  : Compute Copeland scores for a set of 2D alternatives
- [`copeland_winner_2d()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/copeland_winner_2d.md)
  : Find the Copeland winner in a set of 2D alternatives
- [`has_condorcet_winner_2d()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/has_condorcet_winner_2d.md)
  : Test whether a Condorcet winner exists in a finite alternative set
- [`condorcet_winner_2d()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/condorcet_winner_2d.md)
  : Find the Condorcet winner in a finite alternative set
- [`core_2d()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/core_2d.md)
  : Compute the core in continuous 2D space
- [`uncovered_set_2d()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/uncovered_set_2d.md)
  : Compute the uncovered set over a finite alternative set
- [`uncovered_set_boundary_2d()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/uncovered_set_boundary_2d.md)
  : Approximate the boundary of the continuous uncovered set

## Preference Profiles

Create and inspect ordinal preference profiles.

- [`Profile`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/Profile.md)
  : Profile: an ordinal preference profile
- [`profile_build_spatial()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/profile_build_spatial.md)
  : Build an ordinal preference profile from a 2D spatial model
- [`profile_from_utility_matrix()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/profile_from_utility_matrix.md)
  : Build a profile from a utility matrix
- [`profile_impartial_culture()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/profile_impartial_culture.md)
  : Generate a profile under the impartial culture model
- [`profile_uniform_spatial()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/profile_uniform_spatial.md)
  : Generate a spatial profile with voters drawn from a uniform 2D
  distribution
- [`profile_gaussian_spatial()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/profile_gaussian_spatial.md)
  : Generate a spatial profile with voter ideals drawn from N(mean,
  stddev²)

## Voting Rules

Plurality, Borda, anti-plurality, scoring rules, and approval voting.

- [`plurality_scores()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plurality_scores.md)
  : Compute plurality scores (first-place vote counts)
- [`plurality_all_winners()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plurality_all_winners.md)
  : Return all plurality winners (alternatives tied for most first-place
  votes)
- [`plurality_one_winner()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plurality_one_winner.md)
  : Return a single plurality winner with tie-breaking
- [`borda_scores()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/borda_scores.md)
  : Compute Borda scores
- [`borda_all_winners()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/borda_all_winners.md)
  : Return all Borda winners
- [`borda_one_winner()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/borda_one_winner.md)
  : Return a single Borda winner with tie-breaking
- [`borda_ranking()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/borda_ranking.md)
  : Full social ordering by descending Borda score
- [`antiplurality_scores()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/antiplurality_scores.md)
  : Compute anti-plurality scores
- [`antiplurality_all_winners()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/antiplurality_all_winners.md)
  : Return all anti-plurality winners
- [`antiplurality_one_winner()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/antiplurality_one_winner.md)
  : Return a single anti-plurality winner with tie-breaking
- [`scoring_rule_scores()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/scoring_rule_scores.md)
  : Compute scores under a positional scoring rule
- [`scoring_rule_all_winners()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/scoring_rule_all_winners.md)
  : Return all winners under a positional scoring rule
- [`scoring_rule_one_winner()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/scoring_rule_one_winner.md)
  : Return a single winner under a positional scoring rule with
  tie-breaking
- [`approval_scores_spatial()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/approval_scores_spatial.md)
  : Compute approval scores under the spatial threshold model
- [`approval_all_winners_spatial()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/approval_all_winners_spatial.md)
  : Return all winners under spatial approval voting
- [`approval_scores_topk()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/approval_scores_topk.md)
  : Compute approval scores under the ordinal top-k model
- [`approval_all_winners_topk()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/approval_all_winners_topk.md)
  : Return all winners under the top-k approval rule

## Social Rankings and Properties

Aggregate scores into rankings; test Pareto efficiency and Condorcet
consistency.

- [`rank_by_scores()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/rank_by_scores.md)
  : Sort alternatives by descending score with tie-breaking
- [`pairwise_ranking_from_matrix()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/pairwise_ranking_from_matrix.md)
  : Rank alternatives by Copeland score derived from a pairwise matrix
- [`pareto_set()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/pareto_set.md)
  : Return the Pareto set of a profile
- [`is_pareto_efficient()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/is_pareto_efficient.md)
  : Test whether a single alternative is Pareto-efficient
- [`has_condorcet_winner_profile()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/has_condorcet_winner_profile.md)
  : Test whether the profile has a Condorcet winner
- [`condorcet_winner_profile()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/condorcet_winner_profile.md)
  : Return the Condorcet winner from a profile
- [`is_condorcet_consistent()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/is_condorcet_consistent.md)
  : Test whether an alternative is Condorcet-consistent
- [`is_majority_consistent()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/is_majority_consistent.md)
  : Test whether an alternative is majority-consistent

## Visualization

Interactive Plotly-based plots for spatial voting analysis. Compose
layers by passing the figure from one function into the next.

- [`plot_spatial_voting()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_spatial_voting.md)
  : Create a base 2D spatial voting plot
- [`layer_winset()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/layer_winset.md)
  : Add a winset polygon layer
- [`layer_yolk()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/layer_yolk.md)
  : Add a yolk circle layer
- [`layer_uncovered_set()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/layer_uncovered_set.md)
  : Add an uncovered set boundary layer
- [`layer_convex_hull()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/layer_convex_hull.md)
  : Add a convex hull layer
- [`layer_centroid()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/layer_centroid.md)
  : Add a centroid (mean voter position) marker layer
- [`layer_marginal_median()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/layer_marginal_median.md)
  : Add a marginal median marker layer
- [`layer_ic()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/layer_ic.md)
  : Add voter indifference curves
- [`layer_preferred_regions()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/layer_preferred_regions.md)
  : Add voter preferred-to regions
- [`centroid_2d()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/centroid_2d.md)
  : Coordinate-wise arithmetic mean (centroid) of voter ideal points
- [`marginal_median_2d()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/marginal_median_2d.md)
  : Coordinate-wise median of voter ideal points (marginal median)
- [`geometric_mean_2d()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/geometric_mean_2d.md)
  : Coordinate-wise geometric mean of voter ideal points
- [`scl_theme_colors()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/scl_theme_colors.md)
  : Return the fill and line colours for a layer type in a theme
- [`scl_palette()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/scl_palette.md)
  : Return n RGBA colour strings from a named palette
- [`.PALETTE_HEX`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/dot-PALETTE_HEX.md)
  : Approved qualitative palettes. Source: ColorBrewer 2.0 (Cynthia
  Brewer) and Okabe & Ito (2008).
- [`finalize_plot()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/finalize_plot.md)
  : Enforce the canonical layer stack order
- [`save_plot()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/save_plot.md)
  : Save a spatial voting plot to file
- [`list_scenarios()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/list_scenarios.md)
  : List available built-in scenarios
- [`load_scenario()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/load_scenario.md)
  : Load a built-in named scenario

## Candidate Competition

Run multi-candidate spatial competition simulations (Layer 7). Sticker,
Hunter, Aggregator, and Predator strategies; plurality and proportional
seat conversion; full trace recording and experiment sweeps.

- [`competition_run()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/competition_run.md)
  : Run a spatial candidate competition simulation
- [`competition_run_experiment()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/competition_run_experiment.md)
  : Run replicated competition simulations
- [`make_competition_engine_config()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_competition_engine_config.md)
  : Create a competition engine configuration
- [`make_competition_step_config()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_competition_step_config.md)
  : Create a competition step-policy configuration
- [`make_competition_termination_config()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/make_competition_termination_config.md)
  : Create a competition termination configuration
- [`CompetitionTrace`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/CompetitionTrace.md)
  : CompetitionTrace: one competition simulation trace
- [`CompetitionExperiment`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/CompetitionExperiment.md)
  : CompetitionExperiment: replicated competition runs
- [`plot_competition_trajectories()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/plot_competition_trajectories.md)
  : Plot 2D competition trajectories from a CompetitionTrace
- [`animate_competition_trajectories()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/animate_competition_trajectories.md)
  : Animate 2D competition trajectories from a CompetitionTrace

## Stream Manager

Reproducible pseudo-random number streams.

- [`StreamManager`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/StreamManager.md)
  : StreamManager: reproducible pseudo-random number streams
- [`stream_manager()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/stream_manager.md)
  : Create a StreamManager (convenience factory)

## Package

Version information.

- [`scs_version()`](https://socialchoicestudio.github.io/socialchoicelab/r/reference/scs_version.md)
  : Query the compiled C API version
