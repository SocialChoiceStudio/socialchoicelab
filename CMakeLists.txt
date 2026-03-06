cmake_minimum_required(VERSION 3.20)
project(SocialChoiceLab VERSION 0.1.0 LANGUAGES CXX)

# C++ standard (applies to all targets)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate compile_commands.json for clangd/LSP and clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Fetch dependencies
include(FetchContent)

# Fetch Google Test
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.17.0
)

# Prevent Eigen from registering its tests in CTest so only our project's tests run.
# CMP0077 NEW makes our set() calls take priority over option() calls inside Eigen.
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(EIGEN_BUILD_TESTING OFF)
set(EIGEN_BUILD_DOC OFF)
set(EIGEN_BUILD_PKGCONFIG OFF)
set(BUILD_TESTING OFF)
FetchContent_Declare(
  eigen
  GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
  GIT_TAG        3.4.0
)

FetchContent_MakeAvailable(googletest eigen)

# CGAL: system install required (too large and has system-level dependencies
# that CMake FetchContent cannot satisfy).  Install via:
#   Ubuntu: sudo apt-get install -y libcgal-dev libgmp-dev libmpfr-dev
#   macOS:  brew install cgal
find_package(CGAL REQUIRED)

# Create the core library
add_library(socialchoicelab_core
    include/socialchoicelab/core/linalg.h
    include/socialchoicelab/core/numeric_constants.h
    include/socialchoicelab/core/rng/prng.h
    include/socialchoicelab/core/types.h
    include/socialchoicelab/core/rng/stream_manager.h
    include/socialchoicelab/preference/distance/distance_functions.h
    include/socialchoicelab/preference/indifference/level_set.h
    include/socialchoicelab/preference/loss/loss_functions.h
    src/core/rng/stream_manager.cpp
)
target_include_directories(socialchoicelab_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# Compiler-specific flags (Item 34): GCC/Clang use -Wall; MSVC uses /W4
if(MSVC)
  set(SocialChoiceLab_compile_options
    $<$<CONFIG:Debug>:/Zi /Od /W4>
    $<$<CONFIG:Release>:/O2 /DNDEBUG>
  )
else()
  set(SocialChoiceLab_compile_options
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
  )
endif()
target_compile_options(socialchoicelab_core PRIVATE ${SocialChoiceLab_compile_options})
target_compile_features(socialchoicelab_core PUBLIC cxx_std_20)
target_link_libraries(socialchoicelab_core PUBLIC Eigen3::Eigen)

# Enable testing
enable_testing()

# Add unit tests (gtest_main pulls in gtest)
add_executable(test_distance_functions tests/unit/test_distance_functions.cpp)
target_compile_options(test_distance_functions PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_distance_functions PRIVATE socialchoicelab_core gtest_main)

add_executable(test_prng tests/unit/test_prng.cpp)
target_compile_options(test_prng PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_prng PRIVATE socialchoicelab_core gtest_main)

add_executable(test_distance_comparison tests/unit/test_distance_comparison.cpp)
target_compile_options(test_distance_comparison PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_distance_comparison PRIVATE socialchoicelab_core gtest_main)

add_executable(test_distance_speed tests/unit/test_distance_speed.cpp)
target_compile_options(test_distance_speed PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_distance_speed PRIVATE socialchoicelab_core gtest_main)

add_executable(test_performance_comparison tests/unit/test_performance_comparison.cpp)
target_compile_options(test_performance_comparison PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_performance_comparison PRIVATE socialchoicelab_core gtest_main)

add_executable(test_loss_functions tests/unit/test_loss_functions.cpp)
target_compile_options(test_loss_functions PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_loss_functions PRIVATE socialchoicelab_core gtest_main)

add_executable(test_core_types tests/unit/test_core_types.cpp)
target_compile_options(test_core_types PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_core_types PRIVATE socialchoicelab_core gtest_main)

# Register tests with CTest
add_test(NAME DistanceFunctionsTest COMMAND test_distance_functions)
add_test(NAME PRNGTest COMMAND test_prng)
add_test(NAME DistanceComparisonTest COMMAND test_distance_comparison)
add_test(NAME DistanceSpeedTest COMMAND test_distance_speed)
add_test(NAME PerformanceComparisonTest COMMAND test_performance_comparison)
add_test(NAME LossFunctionsTest COMMAND test_loss_functions)
add_test(NAME CoreTypesTest COMMAND test_core_types)

add_executable(test_level_set tests/unit/test_level_set.cpp)
target_compile_options(test_level_set PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_level_set PRIVATE socialchoicelab_core gtest_main)

add_test(NAME LevelSetTest COMMAND test_level_set)

# Performance/benchmark tests: report timing only, no strict thresholds.
# Run only these: ctest -L benchmark. Exclude from fast runs: ctest -LE benchmark.
set_tests_properties(DistanceSpeedTest PerformanceComparisonTest
    PROPERTIES LABELS "benchmark")

# ---------------------------------------------------------------------------
# socialchoicelab_geometry: header-only exact 2D geometry layer (CGAL EPEC)
# ---------------------------------------------------------------------------
add_library(socialchoicelab_geometry INTERFACE)
target_include_directories(socialchoicelab_geometry INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(socialchoicelab_geometry INTERFACE
    socialchoicelab_core
    CGAL::CGAL)
target_compile_features(socialchoicelab_geometry INTERFACE cxx_std_20)

add_executable(test_geom2d tests/unit/test_geom2d.cpp)
target_compile_options(test_geom2d PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_geom2d PRIVATE socialchoicelab_geometry gtest_main)
add_test(NAME Geom2dTest COMMAND test_geom2d)

add_executable(test_convex_hull tests/unit/test_convex_hull.cpp)
target_compile_options(test_convex_hull PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_convex_hull PRIVATE socialchoicelab_geometry gtest_main)
add_test(NAME ConvexHullTest COMMAND test_convex_hull)

add_executable(test_majority tests/unit/test_majority.cpp)
target_compile_options(test_majority PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_majority PRIVATE socialchoicelab_geometry gtest_main)
add_test(NAME MajorityTest COMMAND test_majority)

add_executable(test_winset tests/unit/test_winset.cpp)
target_compile_options(test_winset PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_winset PRIVATE socialchoicelab_geometry gtest_main)
add_test(NAME WinsetTest COMMAND test_winset)

add_executable(test_yolk tests/unit/test_yolk.cpp)
target_compile_options(test_yolk PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_yolk PRIVATE socialchoicelab_geometry gtest_main)
add_test(NAME YolkTest COMMAND test_yolk)

add_executable(test_uncovered_set tests/unit/test_uncovered_set.cpp)
target_compile_options(test_uncovered_set PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_uncovered_set PRIVATE socialchoicelab_geometry gtest_main)
add_test(NAME UncoveredSetTest COMMAND test_uncovered_set)

# Phase F — Extended winset services
add_executable(test_winset_ops tests/unit/test_winset_ops.cpp)
target_compile_options(test_winset_ops PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_winset_ops PRIVATE socialchoicelab_geometry gtest_main)
add_test(NAME WinsetOpsTest COMMAND test_winset_ops)

add_executable(test_core tests/unit/test_core.cpp)
target_compile_options(test_core PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_core PRIVATE socialchoicelab_geometry gtest_main)
add_test(NAME CoreTest COMMAND test_core)

add_executable(test_copeland tests/unit/test_copeland.cpp)
target_compile_options(test_copeland PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_copeland PRIVATE socialchoicelab_geometry gtest_main)
add_test(NAME CopelandTest COMMAND test_copeland)

add_executable(test_heart tests/unit/test_heart.cpp)
target_compile_options(test_heart PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_heart PRIVATE socialchoicelab_geometry gtest_main)
add_test(NAME HeartTest COMMAND test_heart)

add_executable(test_centrality tests/unit/test_centrality.cpp)
target_compile_options(test_centrality PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_centrality PRIVATE socialchoicelab_geometry gtest_main)
add_test(NAME CentralityTest COMMAND test_centrality)

add_executable(test_geometry_integration tests/unit/test_geometry_integration.cpp)
target_compile_options(test_geometry_integration PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_geometry_integration PRIVATE socialchoicelab_geometry gtest_main)
add_test(NAME GeometryIntegrationTest COMMAND test_geometry_integration)

# ---------------------------------------------------------------------------
# socialchoicelab_aggregation: header-only profiles & voting rules layer
# Depends on socialchoicelab_geometry (for DistConfig, voter_distance,
# pairwise_matrix, copeland_scores).
# ---------------------------------------------------------------------------
add_library(socialchoicelab_aggregation INTERFACE)
target_include_directories(socialchoicelab_aggregation INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(socialchoicelab_aggregation INTERFACE
    socialchoicelab_geometry)
target_compile_features(socialchoicelab_aggregation INTERFACE cxx_std_20)

add_executable(test_profile tests/unit/test_profile.cpp)
target_compile_options(test_profile PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_profile PRIVATE socialchoicelab_aggregation gtest_main)
add_test(NAME ProfileTest COMMAND test_profile)

add_executable(test_voting_rules tests/unit/test_voting_rules.cpp)
target_compile_options(test_voting_rules PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_voting_rules PRIVATE socialchoicelab_aggregation gtest_main)
add_test(NAME VotingRulesTest COMMAND test_voting_rules)

add_executable(test_aggregation_properties tests/unit/test_aggregation_properties.cpp)
target_compile_options(test_aggregation_properties PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_aggregation_properties PRIVATE socialchoicelab_aggregation gtest_main)
add_test(NAME AggregationPropertiesTest COMMAND test_aggregation_properties)

add_executable(test_aggregation_integration tests/unit/test_aggregation_integration.cpp)
target_compile_options(test_aggregation_integration PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_aggregation_integration PRIVATE socialchoicelab_aggregation gtest_main)
add_test(NAME AggregationIntegrationTest COMMAND test_aggregation_integration)

# ---------------------------------------------------------------------------
# scs_api: stable C API shared library (wraps socialchoicelab_core)
# ---------------------------------------------------------------------------
add_library(scs_api SHARED
    include/scs_api.h
    src/c_api/scs_api.cpp
)
target_include_directories(scs_api
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/socialchoicelab
)
target_compile_options(scs_api PRIVATE ${SocialChoiceLab_compile_options})
target_compile_features(scs_api PUBLIC cxx_std_20)
target_link_libraries(scs_api PRIVATE socialchoicelab_core socialchoicelab_geometry)

add_executable(test_c_api tests/unit/test_c_api.cpp)
target_compile_options(test_c_api PRIVATE ${SocialChoiceLab_compile_options})
target_link_libraries(test_c_api PRIVATE scs_api gtest_main)
add_test(NAME CApiTest COMMAND test_c_api)

# ---------------------------------------------------------------------------
# Scratch: dump_winset — temporary visualisation helper (source is gitignored).
# Only built when scratch/dump_winset.cpp is present locally.
# ---------------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/scratch/dump_winset.cpp")
  add_executable(dump_winset scratch/dump_winset.cpp)
  target_compile_options(dump_winset PRIVATE ${SocialChoiceLab_compile_options})
  target_compile_features(dump_winset PUBLIC cxx_std_20)
  target_link_libraries(dump_winset PRIVATE socialchoicelab_geometry)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/scratch/dump_heart.cpp")
  add_executable(dump_heart scratch/dump_heart.cpp)
  target_compile_options(dump_heart PRIVATE ${SocialChoiceLab_compile_options})
  target_compile_features(dump_heart PUBLIC cxx_std_20)
  target_link_libraries(dump_heart PRIVATE socialchoicelab_geometry)
endif()

# Format and lint targets (run from source dir; require clang-format and cpplint)
add_custom_target(format
    COMMAND ${CMAKE_SOURCE_DIR}/lint.sh format
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Format C++ code with clang-format"
)
add_custom_target(lint
    COMMAND ${CMAKE_SOURCE_DIR}/lint.sh lint
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Lint C++ code with cpplint (permissive)"
)

# Install targets
include(GNUInstallDirs)
install(TARGETS socialchoicelab_core
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/socialchoicelab
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)