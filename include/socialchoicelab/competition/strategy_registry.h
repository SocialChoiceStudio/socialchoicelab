// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <socialchoicelab/competition/adaptation.h>
#include <socialchoicelab/competition/competition_types.h>

#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace socialchoicelab::competition {

class CompetitorStrategyService {
 public:
  virtual ~CompetitorStrategyService() = default;

  [[nodiscard]] virtual CompetitorStrategyKind kind() const noexcept = 0;
  [[nodiscard]] virtual std::string_view stable_id() const noexcept = 0;
  [[nodiscard]] virtual StrategyDecision adapt(
      const CompetitorState& competitor,
      const AdaptationContext& context) const = 0;
};

class StrategyRegistry {
 public:
  void register_strategy(const CompetitorStrategyService& service) {
    const std::string stable_id(service.stable_id());
    if (stable_id.empty()) {
      throw std::invalid_argument(
          "StrategyRegistry::register_strategy: stable_id must not be empty.");
    }
    if (by_kind_.find(service.kind()) != by_kind_.end()) {
      throw std::invalid_argument(
          "StrategyRegistry::register_strategy: duplicate strategy kind.");
    }
    if (by_id_.find(stable_id) != by_id_.end()) {
      throw std::invalid_argument(
          "StrategyRegistry::register_strategy: duplicate stable_id \"" +
          stable_id + "\".");
    }
    by_kind_[service.kind()] = &service;
    by_id_[stable_id] = &service;
  }

  [[nodiscard]] const CompetitorStrategyService* find(
      CompetitorStrategyKind kind) const noexcept {
    const auto it = by_kind_.find(kind);
    return it == by_kind_.end() ? nullptr : it->second;
  }

  [[nodiscard]] const CompetitorStrategyService* find(
      std::string_view stable_id) const noexcept {
    const auto it = by_id_.find(std::string(stable_id));
    return it == by_id_.end() ? nullptr : it->second;
  }

  [[nodiscard]] size_t size() const noexcept { return by_kind_.size(); }

  [[nodiscard]] static const StrategyRegistry& built_in() {
    class BuiltInStrategyService final : public CompetitorStrategyService {
     public:
      explicit BuiltInStrategyService(CompetitorStrategyKind kind)
          : kind_(kind), stable_id_(stable_strategy_id(kind)) {}

      [[nodiscard]] CompetitorStrategyKind kind() const noexcept override {
        return kind_;
      }

      [[nodiscard]] std::string_view stable_id() const noexcept override {
        return stable_id_;
      }

      [[nodiscard]] StrategyDecision adapt(
          const CompetitorState& competitor,
          const AdaptationContext& context) const override {
        detail::validate_adaptation_context(context);
        const int dimension = competition_dimension(context.bounds);

        switch (kind_) {
          case CompetitorStrategyKind::kSticker:
            return {PointNd::Zero(dimension)};

          case CompetitorStrategyKind::kHunter: {
            const bool has_previous =
                competitor.previous_round_metrics.has_value();
            const bool has_current =
                competitor.current_round_metrics.has_value();
            const PointNd current_heading =
                detail::normalize_or_zero(competitor.heading);

            if (!has_previous || !has_current) {
              if (current_heading.norm() != 0.0) {
                return {current_heading};
              }
              if (context.stream_manager == nullptr) {
                throw std::invalid_argument(
                    "BuiltInStrategyService::adapt: Hunter requires a "
                    "StreamManager when no usable current heading exists.");
              }
              auto& stream = context.stream_manager->get_stream(
                  kHunterAdaptationStreamName);
              return {detail::random_unit_heading(dimension, stream)};
            }

            const double previous_value = detail::objective_value(
                *competitor.previous_round_metrics, context.objective_kind);
            const double current_value = detail::objective_value(
                *competitor.current_round_metrics, context.objective_kind);

            if (current_value > previous_value &&
                current_heading.norm() != 0.0) {
              return {current_heading};
            }

            // Fowler & Laver: pick uniformly from the backward half-space
            // (Fowler & Laver 2008, §2; Laver 2012, ch.3). In 1D the
            // backward half-space is a single point so no stream is needed.
            // In nD (n >= 2) the draw is stochastic — a stream_manager is
            // required.
            if (current_heading.norm() != 0.0) {
              if (dimension == 1) {
                return {-current_heading};
              }
              if (context.stream_manager == nullptr) {
                throw std::invalid_argument(
                    "BuiltInStrategyService::adapt: Hunter requires a "
                    "StreamManager when performance drops in dimensions > 1.");
              }
              auto& stream = context.stream_manager->get_stream(
                  kHunterAdaptationStreamName);
              return {detail::random_in_backward_halfspace(
                  dimension, current_heading, stream)};
            }

            // Fallback: no usable heading — pick a random direction.
            if (context.stream_manager == nullptr) {
              throw std::invalid_argument(
                  "BuiltInStrategyService::adapt: Hunter requires a "
                  "StreamManager when no usable heading exists.");
            }
            auto& stream =
                context.stream_manager->get_stream(kHunterAdaptationStreamName);
            return {detail::random_unit_heading(dimension, stream)};
          }

          case CompetitorStrategyKind::kAggregator: {
            const int self_index =
                detail::find_competitor_index(context, competitor.id);
            const PointNd mean = detail::supporter_mean(
                context.supporter_ideal_points[self_index], dimension);
            return {detail::normalize_or_zero(mean - competitor.position)};
          }

          case CompetitorStrategyKind::kPredator: {
            const int leader = detail::leader_index(context);
            const auto& leader_competitor = context.competitors[leader];
            return {detail::normalize_or_zero(leader_competitor.position -
                                              competitor.position)};
          }
        }

        return {PointNd::Zero(dimension)};
      }

     private:
      CompetitorStrategyKind kind_;
      std::string_view stable_id_;
    };

    static const BuiltInStrategyService kSticker(
        CompetitorStrategyKind::kSticker);
    static const BuiltInStrategyService kHunter(
        CompetitorStrategyKind::kHunter);
    static const BuiltInStrategyService kAggregator(
        CompetitorStrategyKind::kAggregator);
    static const BuiltInStrategyService kPredator(
        CompetitorStrategyKind::kPredator);

    static const StrategyRegistry registry = [] {
      StrategyRegistry built_in_registry;
      built_in_registry.register_strategy(kSticker);
      built_in_registry.register_strategy(kHunter);
      built_in_registry.register_strategy(kAggregator);
      built_in_registry.register_strategy(kPredator);
      return built_in_registry;
    }();

    return registry;
  }

 private:
  std::unordered_map<CompetitorStrategyKind, const CompetitorStrategyService*>
      by_kind_;
  std::unordered_map<std::string, const CompetitorStrategyService*> by_id_;
};

}  // namespace socialchoicelab::competition
