# devScript_MAIN.R — Comprehensive walkthrough of socialchoicelab R package
# ============================================================================
# This script demonstrates user-facing functions across all major categories.
# Run in RStudio with devtools::load_all() or library(socialchoicelab).
#
# Scenario: Laing-Olmsted-Bear (A_Group_05)
#   Space:  0–200 × 0–200
#   Rule:   Simple majority (0.5)
#
# After any change to the R package source, run at the console:
   devtools::document()   # regenerate NAMESPACE + man pages
   devtools::install()    # reinstall the package
library(socialchoicelab)

# ===========================================================================
# 1. SETUP: Voter and alternative data in 2D policy space
# ===========================================================================

cat("\n=== 1. SETUP: Voter and Alternative Data (Laing-Olmsted-Bear) ===\n")

# Load the built-in scenario
scenario <- load_scenario("laing_olmsted_bear")

voters <- scenario$voters
sq     <- scenario$status_quo
alts   <- sq  # No other alternatives in this scenario

cat("Scenario:", scenario$name, "\n")
cat("Players (flat [x0, y0, x1, y1, ...]):\n", voters, "\n")
cat("Status quo:", sq, "\n")

# ===========================================================================
# 2. DISTANCE & UTILITY FUNCTIONS
# ===========================================================================

cat("\n=== 2. DISTANCE & UTILITY ===\n")

# Distance under different metrics
d_euclidean <- calculate_distance(c(0, 0), c(3, 4))
d_manhattan <- calculate_distance(c(0, 0), c(3, 4), make_dist_config("manhattan"))

cat("Distance from origin to (3,4):\n")
cat("  Euclidean:", d_euclidean, "\n")
cat("  Manhattan:", d_manhattan, "\n")

# Convert distance to utility
utility_at_2 <- distance_to_utility(2.0)
utility_at_5 <- distance_to_utility(5.0)

cat("Utility (linear loss):\n")
cat("  At distance 2:", utility_at_2, "\n")
cat("  At distance 5:", utility_at_5, "\n")

# Normalize utility to [0, 1]
utility_norm <- normalize_utility(utility_at_2, max_distance = 5)
cat("Normalized utility (max_distance=5):", utility_norm, "\n")

# ===========================================================================
# 3. MAJORITY PREFERENCE & PAIRWISE COMPARISONS
# ===========================================================================

cat("\n=== 3. MAJORITY PREFERENCE ===\n")

# Does a simple majority prefer each test point over the status quo?
# Test point A: slight shift toward Player 3 (upper-right quadrant)
# Test point B: slight shift toward Player 5 (lower-left quadrant)
a_beats_sq <- majority_prefers_2d(c(110.0, 110.0), sq, voters)
b_beats_sq <- majority_prefers_2d(c( 60.0,  60.0), sq, voters)

cat("Majority preference (5 voters):\n")
cat("  (110, 110) beats SQ:", a_beats_sq, "\n")
cat("  ( 60,  60) beats SQ:", b_beats_sq, "\n")

# Pairwise comparison matrix
pairwise_mat <- pairwise_matrix_2d(alts, voters)
cat("\nPairwise matrix (1=WIN, 0=TIE, -1=LOSS):\n")
print(pairwise_mat)

# ===========================================================================
# 4. GEOMETRY: COPELAND, CONDORCET, UNCOVERED SET
# ===========================================================================

cat("\n=== 4. GEOMETRY: COPELAND & CONDORCET ===\n")

# Copeland scores (wins minus losses in pairwise comparisons)
copeland <- copeland_scores_2d(alts, voters)
cat("Copeland scores:\n")
print(copeland)

# Copeland winner (ties broken by smallest index)
copeland_winner <- copeland_winner_2d(alts, voters)
cat("Copeland winner (1-based):", copeland_winner, "\n")

# Does a Condorcet winner exist?
has_cw <- has_condorcet_winner_2d(alts, voters)
cw <- condorcet_winner_2d(alts, voters)
cat("Has Condorcet winner:", has_cw, "\n")
cat("Condorcet winner (1-based):", cw, "\n")

# Uncovered set (Miller covering relation)
uncovered <- uncovered_set_2d(alts, voters)
cat("Uncovered set (1-based indices):", uncovered, "\n")

# ===========================================================================
# 5. WINSET: MAJORITY WINSET OF STATUS QUO
# ===========================================================================

cat("\n=== 5. WINSET ===\n")

# Create a winset: all policies that beat SQ under majority rule
winset <- winset_2d(sq, voters)

cat("Winset of SQ at (0, 0):\n")
cat("  Empty:", winset$is_empty(), "\n")

if (!winset$is_empty()) {
  bbox <- winset$bbox()
  cat("  Bounding box: [", bbox$min_x, ",", bbox$max_x, "] × [",
      bbox$min_y, ",", bbox$max_y, "]\n", sep="")

  cat("  Contains (110, 80):", winset$contains(110, 80), "\n")
  cat("  Contains (150, 150):", winset$contains(150, 150), "\n")
}

# ===========================================================================
# 6. ORDINAL PROFILES & VOTING RULES
# ===========================================================================

cat("\n=== 6. ORDINAL PROFILES & VOTING RULES ===\n")

# Build a preference profile from spatial locations
# Voters rank alternatives by distance (closest first)
profile <- profile_build_spatial(alts, voters)

cat("Profile dimensions:", profile$dims()$n_voters, "voters,",
    profile$dims()$n_alts, "alternatives\n")

cat("\nRankings (1-based indices, 1=best):\n")
print(profile$export())

# Plurality
plurality <- plurality_scores(profile)
plurality_winner <- plurality_one_winner(profile)
cat("\nPlurality scores:\n")
print(plurality)
cat("Plurality winner (1-based):", plurality_winner, "\n")

# Borda count
borda <- borda_scores(profile)
borda_winner <- borda_one_winner(profile)
borda_ranking <- borda_ranking(profile)
cat("\nBorda scores:\n")
print(borda)
cat("Borda winner (1-based):", borda_winner, "\n")
cat("Borda ranking (best first):", borda_ranking, "\n")

# Anti-plurality
antiplurality <- antiplurality_scores(profile)
antiplurality_winner <- antiplurality_one_winner(profile)
cat("\nAnti-plurality scores:\n")
print(antiplurality)
cat("Anti-plurality winner (1-based):", antiplurality_winner, "\n")

# Approval voting (top-k): k capped at n_alts so this works for any scenario
k_approval <- min(2L, profile$dims()$n_alts)
approval_topk <- approval_scores_topk(profile, k = k_approval)
approval_topk_winners <- approval_all_winners_topk(profile, k = k_approval)
cat(sprintf("\nApproval voting (top-%d):\n", k_approval))
cat("  Scores:", approval_topk, "\n")
cat("  Winners (1-based):", approval_topk_winners, "\n")

# ===========================================================================
# 7. SOCIAL RANKINGS & PROPERTIES
# ===========================================================================

cat("\n=== 7. SOCIAL RANKINGS & PROPERTIES ===\n")

# Rank by scores
scores <- c(1.5, 0.8, 2.1)
ranking <- rank_by_scores(scores)
cat("Rank by scores [1.5, 0.8, 2.1]:\n")
cat("  Best-to-worst (1-based):", ranking, "\n")

# Pareto set (Pareto-efficient alternatives)
pareto <- pareto_set(profile)
cat("\nPareto set (1-based):", pareto, "\n")

for (i in seq_len(profile$dims()$n_alts)) {
  is_pareto <- is_pareto_efficient(profile, i)
  cat("  Alt", i, "is Pareto-efficient:", is_pareto, "\n")
}

# Condorcet & majority consistency
has_cw_prof <- has_condorcet_winner_profile(profile)
cw_prof <- condorcet_winner_profile(profile)
cat("\nProfile has Condorcet winner:", has_cw_prof, "\n")
cat("Condorcet winner (profile, 1-based):", cw_prof, "\n")

if (!is.na(cw_prof)) {
  is_majority_cons <- is_majority_consistent(profile, cw_prof)
  is_condorcet_cons <- is_condorcet_consistent(profile, cw_prof)
  cat("  Is majority-consistent:", is_majority_cons, "\n")
  cat("  Is Condorcet-consistent:", is_condorcet_cons, "\n")
}

# ===========================================================================
# 8. STREAM MANAGER & REPRODUCIBILITY
# ===========================================================================

cat("\n=== 8. STREAM MANAGER & REPRODUCIBILITY ===\n")

# Create a stream manager with a fixed seed
sm <- stream_manager(master_seed = 42, stream_names = c("draws", "ties"))

# Draw random values (reproducible)
draw1 <- sm$uniform_real("draws", 0, 1)
draw2 <- sm$uniform_real("draws", 0, 1)

cat("Draws from stream 'draws':\n")
cat("  Draw 1:", draw1, "\n")
cat("  Draw 2:", draw2, "\n")

# Reset to get the same values
sm$reset_all(42)
draw1_again <- sm$uniform_real("draws", 0, 1)
draw2_again <- sm$uniform_real("draws", 0, 1)

cat("After reset (master_seed=42):\n")
cat("  Draw 1:", draw1_again, "(equal:", draw1 == draw1_again, ")\n")
cat("  Draw 2:", draw2_again, "(equal:", draw2 == draw2_again, ")\n")

# Generate a stochastic profile (impartial culture)
sm_ic <- stream_manager(master_seed = 99, stream_names = "ic")
profile_ic <- profile_impartial_culture(n_voters = 10L, n_alts = 4L,
                                        stream_manager = sm_ic, stream_name = "ic")
cat("\nImpartial culture profile (10 voters, 4 alts):\n")
cat("  Dims:", profile_ic$dims()$n_voters, "×", profile_ic$dims()$n_alts, "\n")
cat("  Voter 1 ranking:", profile_ic$get_ranking(1L), "\n")

# ===========================================================================
# 9. CONFIGURATION FUNCTIONS
# ===========================================================================

cat("\n=== 9. CONFIGURATION ===\n")

# Distance configs
dist_euc <- make_dist_config("euclidean", weights = c(1.0, 1.0))
dist_l1 <- make_dist_config("manhattan")
dist_weighted <- make_dist_config("euclidean", weights = c(2.0, 0.5))

cat("Distance configs:\n")
cat("  Euclidean (equal weights):\n")
cat("    - distance_type:", dist_euc$distance_type, "\n")
cat("    - weights:", paste(dist_euc$weights, collapse=", "), "\n")

# Loss configs
loss_linear <- make_loss_config("linear")
loss_quad <- make_loss_config("quadratic")
loss_gaussian <- make_loss_config("gaussian", max_loss = 2.0, steepness = 0.5)

cat("\n  Loss configs:\n")
cat("    - Linear:", loss_linear$loss_type, "\n")
cat("    - Quadratic:", loss_quad$loss_type, "\n")
cat("    - Gaussian (max_loss=2, steepness=0.5):", loss_gaussian$loss_type, "\n")

# ===========================================================================
# 10. API VERSION
# ===========================================================================

cat("\n=== 10. API VERSION ===\n")

version <- scs_version()
cat("C API version:", version$major, ".", version$minor, ".", version$patch, "\n")

# ===========================================================================
# 10. VISUALIZATION (requires plotly — install.packages("plotly") once)
# ===========================================================================

cat("\n=== 10. VISUALIZATION ===\n")

hull <- convex_hull_2d(voters)
ws   <- winset_2d(sq, voters)
bnd  <- uncovered_set_boundary_2d(voters, grid_resolution = 10L)

# Base plot: players + status quo only (no additional alternatives in this scenario)
fig <- plot_spatial_voting(
  voters,
  sq          = sq,
  voter_names = scenario$voter_names,
  dim_names   = scenario$dim_names,
  title       = scenario$name
)

# Add layers
fig <- layer_winset(fig, ws)
fig <- layer_convex_hull(fig, hull)
fig <- finalize_plot(fig)

# Display (opens in RStudio Viewer / browser)
print(fig)
cat("Plot displayed in Viewer. Hover over points for details.\n")

# ===========================================================================
# END OF SCRIPT
# ===========================================================================

cat("\n=== END ===\n")
cat("All examples completed successfully!\n")
cat("For more detail, see: ?function_name or vignette('spatial_voting')\n")

