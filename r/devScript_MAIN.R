# devScript_MAIN.R — Comprehensive walkthrough of socialchoicelab R package
# ============================================================================
# This script demonstrates user-facing functions across all major categories.
# Run in RStudio with devtools::load_all() or library(socialchoicelab).

library(socialchoicelab)

# ===========================================================================
# 1. SETUP: Voter and alternative data in 2D policy space
# ===========================================================================

cat("\n=== 1. SETUP: Voter and Alternative Data ===\n")

# 5 voters in 2D (economic left-right × social liberal-conservative)
voters <- c(
  -1.0, -0.5,   # voter 1: left-economic, moderate-social
   0.0,  0.0,   # voter 2: centrist
   0.8,  0.6,   # voter 3: right-economic, conservative
  -0.4,  0.8,   # voter 4: centre-left, liberal
   0.5, -0.7    # voter 5: right-economic, libertarian
)

# 3 policy alternatives in 2D
alts <- c(
  0.0,  0.0,    # SQ: status quo (centroid)
  0.6,  0.4,    # A: right-of-centre reform
 -0.5,  0.3     # B: left-of-centre reform
)

cat("Voters (flat [x0, y0, x1, y1, ...]):\n", voters, "\n")
cat("Alternatives (flat [x0, y0, x1, y1, ...]):\n", alts, "\n")

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

# Does a simple majority prefer alternative A over status quo?
a_beats_sq <- majority_prefers_2d(c(0.6, 0.4), c(0.0, 0.0), voters)
b_beats_sq <- majority_prefers_2d(c(-0.5, 0.3), c(0.0, 0.0), voters)

cat("Majority preference (5 voters):\n")
cat("  A beats SQ:", a_beats_sq, "\n")
cat("  B beats SQ:", b_beats_sq, "\n")

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
winset <- winset_2d(c(0, 0), voters)

cat("Winset of SQ at (0, 0):\n")
cat("  Empty:", winset$is_empty(), "\n")

if (!winset$is_empty()) {
  bbox <- winset$bbox()
  cat("  Bounding box: [", bbox$min_x, ",", bbox$max_x, "] × [",
      bbox$min_y, ",", bbox$max_y, "]\n", sep="")
  
  cat("  Contains (0.5, 0.5):", winset$contains(0.5, 0.5), "\n")
  cat("  Contains (2.0, 2.0):", winset$contains(2.0, 2.0), "\n")
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

# Approval voting (top-k)
approval_top2 <- approval_scores_topk(profile, k = 2L)
approval_top2_winners <- approval_all_winners_topk(profile, k = 2L)
cat("\nApproval voting (top-2):\n")
cat("  Scores:", approval_top2, "\n")
cat("  Winners (1-based):", approval_top2_winners, "\n")

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
# END OF SCRIPT
# ===========================================================================

cat("\n=== END ===\n")
cat("All examples completed successfully!\n")
cat("For more detail, see: ?function_name or vignette('spatial_voting')\n")
