// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <limits>
#include <string>
#include <vector>

#include "socialchoicelab/core/rng/prng.h"
#include "socialchoicelab/core/rng/stream_manager.h"

using socialchoicelab::core::rng::candidates_rng;
using socialchoicelab::core::rng::k_default_master_seed;
using socialchoicelab::core::rng::PRNG;
using socialchoicelab::core::rng::set_global_stream_manager_seed;
using socialchoicelab::core::rng::StreamManager;
using socialchoicelab::core::rng::voters_rng;

class PRNGTest : public ::testing::Test {
 protected:
  // No per-test setup needed
};

TEST_F(PRNGTest, Construction) {
  PRNG rng1(k_default_master_seed);
  PRNG rng2(k_default_master_seed);
  PRNG rng3(54321);

  // Same seed should produce same sequence
  int val1 = rng1.uniform_int(0, 100);
  int val2 = rng2.uniform_int(0, 100);
  EXPECT_EQ(val1, val2);

  int val3 = rng1.uniform_int(0, 100);
  int val4 = rng2.uniform_int(0, 100);
  EXPECT_EQ(val3, val4);

  // Different seed should produce different sequence
  EXPECT_NE(rng1.uniform_int(0, 100), rng3.uniform_int(0, 100));
}

TEST_F(PRNGTest, UniformInt) {
  PRNG rng(k_default_master_seed);

  // Test range
  for (int i = 0; i < 1000; ++i) {
    int val = rng.uniform_int(10, 20);
    EXPECT_GE(val, 10);
    EXPECT_LE(val, 20);
  }

  // Test single value
  int val = rng.uniform_int(5, 5);
  EXPECT_EQ(val, 5);
}

TEST_F(PRNGTest, UniformReal) {
  PRNG rng(k_default_master_seed);

  // Test range
  for (int i = 0; i < 1000; ++i) {
    double val = rng.uniform_real(1.0, 2.0);
    EXPECT_GE(val, 1.0);
    EXPECT_LT(val, 2.0);
  }
}

TEST_F(PRNGTest, Normal) {
  PRNG rng(k_default_master_seed);

  // Test normal distribution
  double sum = 0.0;
  int count = 10000;

  for (int i = 0; i < count; ++i) {
    double val = rng.normal(0.0, 1.0);
    sum += val;
  }

  double mean = sum / count;
  EXPECT_NEAR(mean, 0.0, 0.1);  // Should be close to 0
}

TEST_F(PRNGTest, Exponential) {
  PRNG rng(k_default_master_seed);

  // Test exponential distribution
  for (int i = 0; i < 1000; ++i) {
    double val = rng.exponential(1.0);
    EXPECT_GE(val, 0.0);
  }
}

// Item 24: exponential mean = 1/lambda (statistical)
TEST_F(PRNGTest, ExponentialMean) {
  PRNG rng(k_default_master_seed);
  const double lambda = 2.0;
  double sum = 0.0;
  const int n = 50000;
  for (int i = 0; i < n; ++i) {
    sum += rng.exponential(lambda);
  }
  double mean = sum / n;
  EXPECT_NEAR(mean, 1.0 / lambda, 0.05);
}

TEST_F(PRNGTest, Bernoulli) {
  PRNG rng(k_default_master_seed);

  // Test bernoulli distribution
  int true_count = 0;
  int total = 10000;

  for (int i = 0; i < total; ++i) {
    if (rng.bernoulli(0.3)) {
      true_count++;
    }
  }

  double proportion = static_cast<double>(true_count) / total;
  EXPECT_NEAR(proportion, 0.3, 0.02);  // Should be close to 0.3
}

TEST_F(PRNGTest, DiscreteChoice) {
  PRNG rng(k_default_master_seed);

  std::vector<double> weights{1.0, 2.0, 3.0, 4.0};

  // Test discrete choice
  for (int i = 0; i < 1000; ++i) {
    size_t choice = rng.discrete_choice(weights);
    EXPECT_LT(choice, weights.size());
  }
}

TEST_F(PRNGTest, UniformChoice) {
  PRNG rng(k_default_master_seed);

  // Test uniform choice
  for (int i = 0; i < 1000; ++i) {
    size_t choice = rng.uniform_choice(5);
    EXPECT_LT(choice, 5);
  }
}

TEST_F(PRNGTest, UniformChoiceZeroSizeThrows) {
  PRNG rng(k_default_master_seed);
  EXPECT_THROW(rng.uniform_choice(0), std::invalid_argument);
}

TEST_F(PRNGTest, DiscreteChoiceEmptyWeightsThrows) {
  PRNG rng(k_default_master_seed);
  std::vector<double> empty_weights;
  EXPECT_THROW(rng.discrete_choice(empty_weights), std::invalid_argument);
}

TEST_F(PRNGTest, BetaInvalidParametersThrow) {
  PRNG rng(k_default_master_seed);
  EXPECT_THROW(rng.beta(0.0, 1.0), std::invalid_argument);
  EXPECT_THROW(rng.beta(1.0, 0.0), std::invalid_argument);
  EXPECT_THROW(rng.beta(-0.1, 1.0), std::invalid_argument);
  EXPECT_THROW(rng.beta(1.0, -0.1), std::invalid_argument);
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();
  EXPECT_THROW(rng.beta(nan, 1.0), std::invalid_argument);
  EXPECT_THROW(rng.beta(1.0, nan), std::invalid_argument);
  EXPECT_THROW(rng.beta(inf, 1.0), std::invalid_argument);
  EXPECT_THROW(rng.beta(1.0, inf), std::invalid_argument);
}

// Item 24: beta(alpha, beta) happy path — values in (0,1), mean ≈
// alpha/(alpha+beta)
TEST_F(PRNGTest, BetaHappyPath) {
  PRNG rng(k_default_master_seed);
  const double alpha = 2.0, beta_param = 5.0;
  double sum = 0.0;
  const int n = 20000;
  for (int i = 0; i < n; ++i) {
    double v = rng.beta(alpha, beta_param);
    EXPECT_GT(v, 0.0);
    EXPECT_LT(v, 1.0);
    sum += v;
  }
  double mean = sum / n;
  double expected_mean = alpha / (alpha + beta_param);
  EXPECT_NEAR(mean, expected_mean, 0.03);
}

// Item 24: gamma(alpha, beta) happy path — positive, mean = alpha * beta (shape
// * scale per std::gamma_distribution)
TEST_F(PRNGTest, GammaHappyPath) {
  PRNG rng(k_default_master_seed);
  const double alpha = 2.0, beta = 1.5;
  double sum = 0.0;
  const int n = 30000;
  for (int i = 0; i < n; ++i) {
    double v = rng.gamma(alpha, beta);
    EXPECT_GT(v, 0.0);
    sum += v;
  }
  double mean = sum / n;
  double expected_mean = alpha * beta;
  EXPECT_NEAR(mean, expected_mean, 0.2);
}

TEST_F(PRNGTest, InvalidDistributionParametersThrow) {
  PRNG rng(k_default_master_seed);
  const double inf = std::numeric_limits<double>::infinity();
  const double nan = std::numeric_limits<double>::quiet_NaN();

  EXPECT_THROW(rng.uniform_int(10, 5), std::invalid_argument);
  EXPECT_THROW(rng.uniform_real(1.0, 0.0), std::invalid_argument);
  EXPECT_THROW(rng.uniform_real(0.0, 0.0), std::invalid_argument);
  EXPECT_THROW(rng.uniform_real(nan, 1.0), std::invalid_argument);

  EXPECT_THROW(rng.normal(0.0, 0.0), std::invalid_argument);
  EXPECT_THROW(rng.normal(0.0, -1.0), std::invalid_argument);
  EXPECT_THROW(rng.normal(0.0, nan), std::invalid_argument);

  EXPECT_THROW(rng.exponential(0.0), std::invalid_argument);
  EXPECT_THROW(rng.exponential(-0.1), std::invalid_argument);
  EXPECT_THROW(rng.exponential(inf), std::invalid_argument);

  EXPECT_THROW(rng.gamma(0.0, 1.0), std::invalid_argument);
  EXPECT_THROW(rng.gamma(1.0, -0.1), std::invalid_argument);

  EXPECT_THROW(rng.bernoulli(-0.1), std::invalid_argument);
  EXPECT_THROW(rng.bernoulli(1.5), std::invalid_argument);
  EXPECT_THROW(rng.bernoulli(nan), std::invalid_argument);
}

TEST_F(PRNGTest, Reset) {
  PRNG rng(k_default_master_seed);

  // Generate some numbers
  int first = rng.uniform_int(0, 100);
  int second = rng.uniform_int(0, 100);

  // Reset with same seed
  rng.reset(k_default_master_seed);

  // Should get same sequence
  EXPECT_EQ(rng.uniform_int(0, 100), first);
  EXPECT_EQ(rng.uniform_int(0, 100), second);
}

// Item 15: Copy produces independent object with same state (same sequence from
// copy point)
TEST_F(PRNGTest, CopyProducesIdenticalSequenceFromCopyPoint) {
  PRNG original(k_default_master_seed);
  (void)original.uniform_int(0, 100);
  (void)original.uniform_int(0, 100);

  PRNG copy = original;

  for (int i = 0; i < 20; ++i) {
    EXPECT_EQ(original.uniform_int(0, 1000000), copy.uniform_int(0, 1000000))
        << "Copy should produce same sequence as original from copy point";
  }
}

TEST_F(PRNGTest, Skip) {
  PRNG rng1(k_default_master_seed);
  PRNG rng2(k_default_master_seed);

  // Item 25: Test engine position directly — draw 10 from rng1.engine(),
  // discard 10 on rng2.engine()
  for (int i = 0; i < 10; ++i) {
    (void)rng1.engine()();
  }
  rng2.engine().discard(10);

  // Next raw engine output should be equal
  EXPECT_EQ(rng1.engine()(), rng2.engine()());
}

class StreamManagerTest : public ::testing::Test {
 protected:
  // No per-test setup needed
};

TEST_F(StreamManagerTest, BasicOperations) {
  StreamManager manager(k_default_master_seed);

  // Get streams
  PRNG& voters = manager.get_stream("voters");
  PRNG& candidates = manager.get_stream("candidates");

  // Should be different streams
  EXPECT_NE(voters.uniform_int(0, 100), candidates.uniform_int(0, 100));

  // Same name should return same stream (they're the same object)
  PRNG& voters2 = manager.get_stream("voters");
  // Since they're the same object, calling one affects the other
  // Let's test that they're actually the same object by checking state
  EXPECT_EQ(&voters, &voters2);  // Same memory address
}

TEST_F(StreamManagerTest, ResetAll) {
  StreamManager manager(k_default_master_seed);

  PRNG& voters = manager.get_stream("voters");
  int first = voters.uniform_int(0, 100);

  // Reset all
  manager.reset_all(54321);

  PRNG& voters_new = manager.get_stream("voters");
  int second = voters_new.uniform_int(0, 100);

  // Should be different due to new seed
  EXPECT_NE(first, second);
}

TEST_F(StreamManagerTest, StreamNames) {
  StreamManager manager(k_default_master_seed);

  // Initially empty
  EXPECT_EQ(manager.size(), 0);

  // Create some streams
  manager.get_stream("voters");
  manager.get_stream("candidates");

  EXPECT_EQ(manager.size(), 2);

  auto names = manager.stream_names();
  EXPECT_EQ(names.size(), 2);

  // Should contain both names
  EXPECT_TRUE(std::find(names.begin(), names.end(), "voters") != names.end());
  EXPECT_TRUE(std::find(names.begin(), names.end(), "candidates") !=
              names.end());
}

// Item 23: StreamManager reset_stream, has_stream, remove_stream, clear,
// debug_info
TEST_F(StreamManagerTest, HasStream) {
  StreamManager manager(k_default_master_seed);
  EXPECT_FALSE(manager.has_stream("voters"));
  manager.get_stream("voters");
  EXPECT_TRUE(manager.has_stream("voters"));
  EXPECT_FALSE(manager.has_stream("candidates"));
}

TEST_F(StreamManagerTest, ResetStream) {
  StreamManager manager(k_default_master_seed);
  PRNG& v = manager.get_stream("voters");
  int a = v.uniform_int(0, 1000);
  manager.reset_stream("voters", 99999);
  int b = manager.get_stream("voters").uniform_int(0, 1000);
  // New seed should give different sequence (with overwhelming probability)
  EXPECT_NE(a, b);
  // reset_stream on non-existent stream creates it
  EXPECT_FALSE(manager.has_stream("newstream"));
  manager.reset_stream("newstream", 11111);
  EXPECT_TRUE(manager.has_stream("newstream"));
}

TEST_F(StreamManagerTest, RemoveStream) {
  StreamManager manager(k_default_master_seed);
  manager.get_stream("voters");
  EXPECT_TRUE(manager.has_stream("voters"));
  manager.remove_stream("voters");
  EXPECT_FALSE(manager.has_stream("voters"));
  EXPECT_EQ(manager.size(), 0);
  // Removing non-existent is no-op
  manager.remove_stream("nonexistent");
}

TEST_F(StreamManagerTest, Clear) {
  StreamManager manager(k_default_master_seed);
  manager.get_stream("voters");
  manager.get_stream("candidates");
  EXPECT_EQ(manager.size(), 2);
  manager.clear();
  EXPECT_EQ(manager.size(), 0);
  EXPECT_FALSE(manager.has_stream("voters"));
  EXPECT_FALSE(manager.has_stream("candidates"));
}

TEST_F(StreamManagerTest, DebugInfo) {
  StreamManager manager(k_default_master_seed);
  manager.get_stream("voters");
  std::string info = manager.debug_info();
  EXPECT_TRUE(info.find("StreamManager") != std::string::npos);
  EXPECT_TRUE(info.find("voters") != std::string::npos);
  EXPECT_TRUE(info.find("master_seed") != std::string::npos);
}

// Item 23: reset_for_run — same (master_seed, run_index) => same sequences;
// different index => different
TEST_F(StreamManagerTest, ResetForRunReproducibility) {
  const uint64_t master = 42;
  const uint64_t run0 = 0;

  StreamManager m1(k_default_master_seed);
  m1.reset_for_run(master, run0);
  int a1 = m1.get_stream("voters").uniform_int(0, 1000000);
  int b1 = m1.get_stream("voters").uniform_int(0, 1000000);

  StreamManager m2(k_default_master_seed);
  m2.reset_for_run(master, run0);
  int a2 = m2.get_stream("voters").uniform_int(0, 1000000);
  int b2 = m2.get_stream("voters").uniform_int(0, 1000000);

  EXPECT_EQ(a1, a2)
      << "Same (master_seed, run_index) should give same first draw";
  EXPECT_EQ(b1, b2)
      << "Same (master_seed, run_index) should give same second draw";
}

TEST_F(StreamManagerTest, ResetForRunDifferentIndicesDifferentSequences) {
  const uint64_t master = 42;

  StreamManager m0(k_default_master_seed);
  m0.reset_for_run(master, 0);
  int v0 = m0.get_stream("voters").uniform_int(0, 1000000);

  StreamManager m1(k_default_master_seed);
  m1.reset_for_run(master, 1);
  int v1 = m1.get_stream("voters").uniform_int(0, 1000000);

  StreamManager m2(k_default_master_seed);
  m2.reset_for_run(master, 2);
  int v2 = m2.get_stream("voters").uniform_int(0, 1000000);

  EXPECT_NE(v0, v1);
  EXPECT_NE(v1, v2);
  EXPECT_NE(v0, v2);
}

TEST_F(StreamManagerTest, GlobalStreamManager) {
  // Test global convenience functions
  set_global_stream_manager_seed(k_default_master_seed);

  PRNG& voters = voters_rng();
  PRNG& candidates = candidates_rng();

  // Should be different streams
  EXPECT_NE(voters.uniform_int(0, 100), candidates.uniform_int(0, 100));
}

// Item 27: set_global_stream_manager_seed second-call branch (reset existing)
TEST_F(StreamManagerTest,
       SetGlobalStreamManagerSeedSecondCallProducesDifferentSequences) {
  set_global_stream_manager_seed(11111);
  int a1 = voters_rng().uniform_int(0, 1000000);
  int b1 = voters_rng().uniform_int(0, 1000000);

  set_global_stream_manager_seed(22222);
  int a2 = voters_rng().uniform_int(0, 1000000);
  int b2 = voters_rng().uniform_int(0, 1000000);

  EXPECT_NE(a1, a2) << "Second seed should produce different sequence";
  EXPECT_NE(b1, b2);
}

// Item 30: register_streams — allowlist enforcement
TEST_F(StreamManagerTest, RegisterStreamsThenGetUnknownThrows) {
  StreamManager mgr(12345);
  mgr.register_streams({"voters", "candidates", "tiebreak"});
  EXPECT_THROW(mgr.get_stream("voter"), std::invalid_argument);  // typo
  EXPECT_THROW(mgr.get_stream("unknown"), std::invalid_argument);
}

TEST_F(StreamManagerTest, RegisterStreamsThenGetAllowedWorks) {
  StreamManager mgr(12345);
  mgr.register_streams({"voters", "candidates"});
  PRNG& v = mgr.get_stream("voters");
  PRNG& c = mgr.get_stream("candidates");
  (void)v.uniform_int(0, 1);
  (void)c.uniform_int(0, 1);
  EXPECT_TRUE(mgr.has_stream("voters"));
  EXPECT_TRUE(mgr.has_stream("candidates"));
}

TEST_F(StreamManagerTest, RegisterStreamsEmptyClearsAllowlist) {
  StreamManager mgr(12345);
  mgr.register_streams({"voters"});
  EXPECT_THROW(mgr.get_stream("other"), std::invalid_argument);
  mgr.register_streams({});
  EXPECT_NO_THROW(mgr.get_stream("other"));
  EXPECT_TRUE(mgr.has_stream("other"));
}

// Item 2: discrete_choice weight validation
TEST_F(PRNGTest, DiscreteChoiceAllZeroWeightsThrows) {
  PRNG rng(k_default_master_seed);
  std::vector<double> all_zero = {0.0, 0.0, 0.0};
  EXPECT_THROW(rng.discrete_choice(all_zero), std::invalid_argument);
}

TEST_F(PRNGTest, DiscreteChoiceNegativeWeightThrows) {
  PRNG rng(k_default_master_seed);
  std::vector<double> neg = {1.0, -0.1, 1.0};
  EXPECT_THROW(rng.discrete_choice(neg), std::invalid_argument);
}

TEST_F(PRNGTest, DiscreteChoiceNaNWeightThrows) {
  PRNG rng(k_default_master_seed);
  std::vector<double> nan_w = {1.0, std::numeric_limits<double>::quiet_NaN()};
  EXPECT_THROW(rng.discrete_choice(nan_w), std::invalid_argument);
}

TEST_F(PRNGTest, DiscreteChoiceInfWeightThrows) {
  PRNG rng(k_default_master_seed);
  std::vector<double> inf_w = {1.0, std::numeric_limits<double>::infinity()};
  EXPECT_THROW(rng.discrete_choice(inf_w), std::invalid_argument);
}

TEST_F(PRNGTest, DiscreteChoiceSinglePositiveAmongZerosReturnsItsIndex) {
  PRNG rng(k_default_master_seed);
  // Only index 2 has positive weight — must always return 2
  std::vector<double> w = {0.0, 0.0, 5.0, 0.0};
  for (int i = 0; i < 20; ++i) {
    EXPECT_EQ(rng.discrete_choice(w), size_t{2});
  }
}

TEST_F(PRNGTest, DiscreteChoiceValidWeightsWorks) {
  PRNG rng(k_default_master_seed);
  std::vector<double> w = {1.0, 2.0, 3.0};
  for (int i = 0; i < 30; ++i) {
    size_t idx = rng.discrete_choice(w);
    EXPECT_LT(idx, w.size());
  }
}
