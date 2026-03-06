// Copyright (C) 2026 The SocialChoiceLab Authors.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// test_centrality.cpp — GTest suite for centrality.h.
//
// Tests cover:
//   marginal_median_2d:
//     - Odd n: exact median coordinate
//     - Even n: average of two middle values
//     - Single voter: returns that voter's ideal point
//     - Empty input throws
//     - Works with NOMINATE-scale [-1, 1] coordinates (no positivity req.)
//   centroid_2d:
//     - Known arithmetic mean
//     - Single voter: returns that voter's ideal point
//     - Symmetric configuration: centroid at origin
//     - Empty input throws
//     - Works with NOMINATE-scale coordinates
//   geometric_mean_2d:
//     - Known geometric mean for positive coordinates
//     - Single voter: returns that voter's ideal point
//     - Empty input throws
//     - Non-positive x throws with descriptive message
//     - Non-positive y throws with descriptive message
//     - Both dims invalid: message mentions both
//     - Zero coordinate throws
//     - NOMINATE-scale mixed-sign data throws with NOMINATE suggestion
//
// Theoretical grounding:
//   For the marginal median, Black (1948) guarantees the median voter on each
//   dimension independently wins under majority rule — the coordinate-wise
//   median is the "issue-by-issue" equilibrium under separable preferences.
//   The centroid appears in Shapley-Owen power index results (Shapley & Owen
//   1989): the strong point equals the SOV-weighted average of ideal points.
//   The geometric mean is the arithmetic mean in log space.

#include <gtest/gtest.h>
#include <socialchoicelab/geometry/centrality.h>

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

using socialchoicelab::core::types::Point2d;
using socialchoicelab::geometry::centroid_2d;
using socialchoicelab::geometry::geometric_mean_2d;
using socialchoicelab::geometry::marginal_median_2d;

static constexpr double kTol = 1e-10;

// ===========================================================================
// marginal_median_2d
// ===========================================================================

TEST(MarginalMedian, OddNExactMedian) {
  // 3 voters: medians are the middle values in each dim independently.
  const std::vector<Point2d> voters{Point2d(1.0, 10.0), Point2d(3.0, 30.0),
                                    Point2d(2.0, 20.0)};
  const Point2d m = marginal_median_2d(voters);
  EXPECT_NEAR(m.x(), 2.0, kTol);
  EXPECT_NEAR(m.y(), 20.0, kTol);
}

TEST(MarginalMedian, FiveVoters) {
  // Sorted x: 1,2,3,4,5 → median = 3. Sorted y: 1,2,3,4,5 → median = 3.
  const std::vector<Point2d> voters{Point2d(3.0, 3.0), Point2d(1.0, 5.0),
                                    Point2d(5.0, 1.0), Point2d(2.0, 4.0),
                                    Point2d(4.0, 2.0)};
  const Point2d m = marginal_median_2d(voters);
  EXPECT_NEAR(m.x(), 3.0, kTol);
  EXPECT_NEAR(m.y(), 3.0, kTol);
}

TEST(MarginalMedian, EvenNAveragesMiddleTwo) {
  // 4 voters, sorted x: 1,2,3,4 → median = (2+3)/2 = 2.5
  //           sorted y: 10,20,30,40 → median = (20+30)/2 = 25.0
  const std::vector<Point2d> voters{Point2d(1.0, 10.0), Point2d(4.0, 40.0),
                                    Point2d(2.0, 20.0), Point2d(3.0, 30.0)};
  const Point2d m = marginal_median_2d(voters);
  EXPECT_NEAR(m.x(), 2.5, kTol);
  EXPECT_NEAR(m.y(), 25.0, kTol);
}

TEST(MarginalMedian, SingleVoter) {
  const std::vector<Point2d> voters{Point2d(7.5, -3.2)};
  const Point2d m = marginal_median_2d(voters);
  EXPECT_NEAR(m.x(), 7.5, kTol);
  EXPECT_NEAR(m.y(), -3.2, kTol);
}

TEST(MarginalMedian, NominateScaleNegativeCoordinates) {
  // Marginal median has no positivity requirement — works on any real coords.
  const std::vector<Point2d> voters{Point2d(-0.8, -0.5), Point2d(0.0, 0.3),
                                    Point2d(0.6, -0.1)};
  const Point2d m = marginal_median_2d(voters);
  EXPECT_NEAR(m.x(), 0.0, kTol);
  EXPECT_NEAR(m.y(), -0.1, kTol);
}

TEST(MarginalMedian, EmptyThrows) {
  EXPECT_THROW(marginal_median_2d({}), std::invalid_argument);
}

TEST(MarginalMedian, ErrorMessageMentionsFunction) {
  try {
    marginal_median_2d({});
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_NE(std::string(e.what()).find("marginal_median_2d"),
              std::string::npos);
  }
}

// ===========================================================================
// centroid_2d
// ===========================================================================

TEST(Centroid, KnownMean) {
  const std::vector<Point2d> voters{Point2d(1.0, 2.0), Point2d(3.0, 4.0),
                                    Point2d(5.0, 6.0)};
  const Point2d c = centroid_2d(voters);
  EXPECT_NEAR(c.x(), 3.0, kTol);
  EXPECT_NEAR(c.y(), 4.0, kTol);
}

TEST(Centroid, SymmetricConfigurationAtOrigin) {
  const std::vector<Point2d> voters{Point2d(1.0, 0.0), Point2d(-1.0, 0.0),
                                    Point2d(0.0, 1.0), Point2d(0.0, -1.0)};
  const Point2d c = centroid_2d(voters);
  EXPECT_NEAR(c.x(), 0.0, kTol);
  EXPECT_NEAR(c.y(), 0.0, kTol);
}

TEST(Centroid, SingleVoter) {
  const std::vector<Point2d> voters{Point2d(-0.4, 0.7)};
  const Point2d c = centroid_2d(voters);
  EXPECT_NEAR(c.x(), -0.4, kTol);
  EXPECT_NEAR(c.y(), 0.7, kTol);
}

TEST(Centroid, NominateScaleMixedSign) {
  // Centroid works on any real-valued coordinates.
  const std::vector<Point2d> voters{Point2d(-0.6, -0.4), Point2d(0.2, 0.8),
                                    Point2d(0.4, -0.2)};
  const Point2d c = centroid_2d(voters);
  EXPECT_NEAR(c.x(), 0.0, kTol);
  EXPECT_NEAR(c.y(), (-0.4 + 0.8 - 0.2) / 3.0, kTol);
}

TEST(Centroid, EmptyThrows) {
  EXPECT_THROW(centroid_2d({}), std::invalid_argument);
}

TEST(Centroid, ErrorMessageMentionsFunction) {
  try {
    centroid_2d({});
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_NE(std::string(e.what()).find("centroid_2d"), std::string::npos);
  }
}

// ===========================================================================
// geometric_mean_2d
// ===========================================================================

TEST(GeometricMean, KnownResult) {
  // geom_mean(1, 4) = sqrt(4) = 2; geom_mean(2, 8) = sqrt(16) = 4.
  const std::vector<Point2d> voters{Point2d(1.0, 2.0), Point2d(4.0, 8.0)};
  const Point2d g = geometric_mean_2d(voters);
  EXPECT_NEAR(g.x(), 2.0, kTol);
  EXPECT_NEAR(g.y(), 4.0, kTol);
}

TEST(GeometricMean, ThreeVoters) {
  // geom_mean(1, 2, 4) = (1·2·4)^(1/3) = 8^(1/3) = 2
  // geom_mean(3, 9, 27) = (729)^(1/3) = 9
  const std::vector<Point2d> voters{Point2d(1.0, 3.0), Point2d(2.0, 9.0),
                                    Point2d(4.0, 27.0)};
  const Point2d g = geometric_mean_2d(voters);
  EXPECT_NEAR(g.x(), 2.0, 1e-9);
  EXPECT_NEAR(g.y(), 9.0, 1e-9);
}

TEST(GeometricMean, SingleVoter) {
  const std::vector<Point2d> voters{Point2d(3.5, 7.2)};
  const Point2d g = geometric_mean_2d(voters);
  EXPECT_NEAR(g.x(), 3.5, kTol);
  EXPECT_NEAR(g.y(), 7.2, kTol);
}

TEST(GeometricMean, EmptyThrows) {
  EXPECT_THROW(geometric_mean_2d({}), std::invalid_argument);
}

TEST(GeometricMean, NegativeXThrows) {
  const std::vector<Point2d> voters{Point2d(-0.5, 1.0), Point2d(0.3, 2.0)};
  EXPECT_THROW(geometric_mean_2d(voters), std::invalid_argument);
}

TEST(GeometricMean, NegativeYThrows) {
  const std::vector<Point2d> voters{Point2d(1.0, -0.3), Point2d(2.0, 0.5)};
  EXPECT_THROW(geometric_mean_2d(voters), std::invalid_argument);
}

TEST(GeometricMean, ZeroXThrows) {
  const std::vector<Point2d> voters{Point2d(0.0, 1.0), Point2d(2.0, 3.0)};
  EXPECT_THROW(geometric_mean_2d(voters), std::invalid_argument);
}

TEST(GeometricMean, ZeroYThrows) {
  const std::vector<Point2d> voters{Point2d(1.0, 0.0), Point2d(2.0, 3.0)};
  EXPECT_THROW(geometric_mean_2d(voters), std::invalid_argument);
}

TEST(GeometricMean, NominateScaleThrowsWithUsefulMessage) {
  // Mixed-sign NOMINATE-scale data: both dims have non-positive values.
  const std::vector<Point2d> voters{Point2d(-0.8, -0.5), Point2d(0.2, 0.3),
                                    Point2d(0.6, -0.1)};
  try {
    geometric_mean_2d(voters);
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    const std::string msg(e.what());
    EXPECT_NE(msg.find("geometric_mean_2d"), std::string::npos)
        << "Error should name the function";
    EXPECT_NE(msg.find("-0.8"), std::string::npos)
        << "Error should include the coordinate range";
    EXPECT_NE(msg.find("NOMINATE"), std::string::npos)
        << "Error should mention NOMINATE scale";
    EXPECT_NE(msg.find("centroid_2d"), std::string::npos)
        << "Error should suggest centroid_2d as alternative";
  }
}

TEST(GeometricMean, ErrorMessageMentionsBothDimsWhenBothInvalid) {
  // Both x and y have non-positive values; message should mention both.
  const std::vector<Point2d> voters{Point2d(-1.0, -2.0), Point2d(0.5, 0.5)};
  try {
    geometric_mean_2d(voters);
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    const std::string msg(e.what());
    EXPECT_NE(msg.find("x-coordinates"), std::string::npos);
    EXPECT_NE(msg.find("y-coordinates"), std::string::npos);
  }
}

TEST(GeometricMean, ErrorMessageYOnlyWhenXIsPositive) {
  // Only y has a non-positive value; message should not mention x.
  const std::vector<Point2d> voters{Point2d(1.0, -0.5), Point2d(2.0, 0.5)};
  try {
    geometric_mean_2d(voters);
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    const std::string msg(e.what());
    EXPECT_NE(msg.find("y-coordinates"), std::string::npos);
    EXPECT_EQ(msg.find("x-coordinates"), std::string::npos)
        << "x is valid; error should not mention x-coordinates";
  }
}
