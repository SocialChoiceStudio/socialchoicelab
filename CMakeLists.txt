cmake_minimum_required(VERSION 3.20)
project(SocialChoiceLab VERSION 1.0.0 LANGUAGES CXX C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Include directories
include_directories(include)

# Fetch dependencies
include(FetchContent)

# Fetch Google Test
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)

# Fetch Eigen
FetchContent_Declare(
  eigen
  GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
  GIT_TAG        3.4.0
)

FetchContent_MakeAvailable(googletest eigen)

# Create the core library
add_library(socialchoicelab_core
    include/socialchoicelab/core/rng/prng.h
    include/socialchoicelab/core/rng/stream_manager.h
    include/socialchoicelab/preference/distance/distance_functions.h
    include/socialchoicelab/preference/loss/loss_functions.h
    src/core/rng/stream_manager.cpp
)

# Set properties
set_target_properties(socialchoicelab_core PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Link Eigen
target_link_libraries(socialchoicelab_core Eigen3::Eigen)

# Enable testing
enable_testing()

# Add unit tests
add_executable(test_distance_functions tests/unit/test_distance_functions.cpp)
target_link_libraries(test_distance_functions
    socialchoicelab_core
    gtest_main
    gtest
)

add_executable(test_prng tests/unit/test_prng.cpp)
target_link_libraries(test_prng
    socialchoicelab_core
    gtest_main
    gtest
)

add_executable(test_distance_comparison tests/unit/test_distance_comparison.cpp)
target_link_libraries(test_distance_comparison
    socialchoicelab_core
    gtest_main
    gtest
)

add_executable(test_distance_speed tests/unit/test_distance_speed.cpp)
target_link_libraries(test_distance_speed
    socialchoicelab_core
    gtest_main
    gtest
)

add_executable(test_performance_comparison tests/unit/test_performance_comparison.cpp)
target_link_libraries(test_performance_comparison
    socialchoicelab_core
    gtest_main
    gtest
)

add_executable(test_loss_functions tests/unit/test_loss_functions.cpp)
target_link_libraries(test_loss_functions
    socialchoicelab_core
    gtest_main
    gtest
)

# Register tests with CTest
add_test(NAME DistanceFunctionsTest COMMAND test_distance_functions)
add_test(NAME PRNGTest COMMAND test_prng)
add_test(NAME DistanceComparisonTest COMMAND test_distance_comparison)
add_test(NAME DistanceSpeedTest COMMAND test_distance_speed)
add_test(NAME PerformanceComparisonTest COMMAND test_performance_comparison)
add_test(NAME LossFunctionsTest COMMAND test_loss_functions)

# Performance/benchmark tests: report timing only, no strict thresholds.
# Run only these: ctest -L benchmark. Exclude from fast runs: ctest -LE benchmark.
set_tests_properties(DistanceSpeedTest PerformanceComparisonTest
    PROPERTIES LABELS "benchmark")

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