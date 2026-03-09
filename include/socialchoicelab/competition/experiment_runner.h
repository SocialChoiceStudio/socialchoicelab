// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <socialchoicelab/competition/engine.h>
#include <socialchoicelab/core/rng/stream_manager.h>

#include <limits>
#include <numeric>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace socialchoicelab::competition {

struct CompetitionExperimentConfig {
  CompetitionEngineConfig engine_config;
  std::vector<CompetitorState> initial_competitors;
  std::vector<PointNd> voter_ideals;
  uint64_t master_seed = core::rng::k_default_master_seed;
  int num_runs = 1;
  bool retain_traces = false;
};

struct CompetitionRunSummary {
  int run_index = 0;
  uint64_t run_seed = 0;
  int rounds_completed = 0;
  TerminationReason termination_reason = TerminationReason::kMaxRounds;
  bool terminated_early = false;
  std::vector<PointNd> final_positions;
  std::vector<double> final_vote_shares;
  std::vector<double> final_seat_shares;
};

struct CompetitionExperimentSummary {
  int num_runs = 0;
  double mean_rounds_completed = 0.0;
  double early_termination_rate = 0.0;
  std::vector<double> mean_final_vote_shares;
  std::vector<double> mean_final_seat_shares;
  std::vector<int> termination_reason_counts;
};

struct CompetitionExperimentResult {
  CompetitionExperimentConfig config;
  CompetitionExperimentSummary summary;
  std::vector<CompetitionRunSummary> run_summaries;
  std::vector<CompetitionTrace> traces;
};

struct CompetitionScenario {
  std::string label;
  CompetitionExperimentConfig config;
};

struct CompetitionSweepResult {
  std::vector<CompetitionScenario> scenarios;
  std::vector<CompetitionExperimentResult> results;
};

namespace detail {

[[nodiscard]] inline uint64_t run_seed_for_index(uint64_t master_seed,
                                                 uint64_t run_index) {
  core::rng::StreamManager stream_manager(master_seed);
  stream_manager.reset_for_run(master_seed, run_index);
  return stream_manager.master_seed();
}

inline void register_competition_streams(
    core::rng::StreamManager& stream_manager) {
  stream_manager.register_streams({
    kHunterAdaptationStreamName,
    kMotionStepSizesStreamName,
  });
}

inline void validate_experiment_config(
    const CompetitionExperimentConfig& config) {
  detail::validate_engine_inputs(
      config.engine_config, config.initial_competitors, config.voter_ideals);
  if (config.num_runs < 1) {
    throw std::invalid_argument(
        "validate_experiment_config: num_runs must be at least 1.");
  }
}

[[nodiscard]] inline CompetitionRunSummary summarize_run(
    const CompetitionTrace& trace, uint64_t run_seed, int run_index) {
  CompetitionRunSummary summary;
  summary.run_index = run_index;
  summary.run_seed = run_seed;
  summary.rounds_completed = static_cast<int>(trace.rounds.size());
  summary.termination_reason = trace.termination_reason;
  summary.terminated_early = trace.terminated_early;

  summary.final_positions.reserve(trace.final_competitors.size());
  for (const auto& competitor : trace.final_competitors) {
    summary.final_positions.push_back(competitor.position);
    if (competitor.current_round_metrics.has_value()) {
      summary.final_vote_shares.push_back(
          competitor.current_round_metrics->vote_share);
      summary.final_seat_shares.push_back(
          competitor.current_round_metrics->seat_share);
    } else {
      summary.final_vote_shares.push_back(0.0);
      summary.final_seat_shares.push_back(0.0);
    }
  }
  return summary;
}

[[nodiscard]] inline CompetitionExperimentSummary summarize_experiment_runs(
    const std::vector<CompetitionRunSummary>& run_summaries) {
  CompetitionExperimentSummary summary;
  summary.num_runs = static_cast<int>(run_summaries.size());
  summary.termination_reason_counts.assign(4, 0);
  if (run_summaries.empty()) return summary;

  const size_t competitor_count =
      run_summaries.front().final_vote_shares.size();
  summary.mean_final_vote_shares.assign(competitor_count, 0.0);
  summary.mean_final_seat_shares.assign(competitor_count, 0.0);

  int early_terminations = 0;
  for (const auto& run : run_summaries) {
    summary.mean_rounds_completed += run.rounds_completed;
    if (run.terminated_early) ++early_terminations;
    ++summary
          .termination_reason_counts[static_cast<int>(run.termination_reason)];
    for (size_t i = 0; i < competitor_count; ++i) {
      summary.mean_final_vote_shares[i] += run.final_vote_shares[i];
      summary.mean_final_seat_shares[i] += run.final_seat_shares[i];
    }
  }

  summary.mean_rounds_completed /= summary.num_runs;
  summary.early_termination_rate =
      static_cast<double>(early_terminations) / summary.num_runs;
  for (size_t i = 0; i < competitor_count; ++i) {
    summary.mean_final_vote_shares[i] /= summary.num_runs;
    summary.mean_final_seat_shares[i] /= summary.num_runs;
  }

  return summary;
}

}  // namespace detail

[[nodiscard]] inline CompetitionExperimentResult run_competition_experiment(
    const CompetitionExperimentConfig& config,
    const StrategyRegistry& registry = StrategyRegistry::built_in()) {
  detail::validate_experiment_config(config);

  CompetitionExperimentResult result;
  result.config = config;
  result.run_summaries.reserve(config.num_runs);
  if (config.retain_traces) result.traces.reserve(config.num_runs);

  core::rng::StreamManager stream_manager(config.master_seed);
  detail::register_competition_streams(stream_manager);

  for (int run = 0; run < config.num_runs; ++run) {
    stream_manager.reset_for_run(config.master_seed,
                                 static_cast<uint64_t>(run));
    detail::register_competition_streams(stream_manager);
    const uint64_t run_seed = detail::run_seed_for_index(
        config.master_seed, static_cast<uint64_t>(run));
    auto trace = run_fixed_round_competition(
        config.engine_config, config.initial_competitors, config.voter_ideals,
        &stream_manager, registry);
    result.run_summaries.push_back(detail::summarize_run(trace, run_seed, run));
    if (config.retain_traces) {
      result.traces.push_back(std::move(trace));
    }
  }

  result.summary = detail::summarize_experiment_runs(result.run_summaries);
  return result;
}

[[nodiscard]] inline CompetitionSweepResult run_competition_sweep(
    const std::vector<CompetitionScenario>& scenarios,
    const StrategyRegistry& registry = StrategyRegistry::built_in()) {
  if (scenarios.empty()) {
    throw std::invalid_argument(
        "run_competition_sweep: scenarios must not be empty.");
  }

  CompetitionSweepResult sweep;
  sweep.scenarios = scenarios;
  sweep.results.reserve(scenarios.size());
  for (const auto& scenario : scenarios) {
    if (scenario.label.empty()) {
      throw std::invalid_argument(
          "run_competition_sweep: scenario labels must not be empty.");
    }
    sweep.results.push_back(
        run_competition_experiment(scenario.config, registry));
  }
  return sweep;
}

}  // namespace socialchoicelab::competition
