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
  GIT_TAG        v1.14.0
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

# Create the core library
add_library(socialchoicelab_core
    include/socialchoicelab/core/numeric_constants.h
    include/socialchoicelab/core/rng/prng.h
    include/socialchoicelab/core/rng/stream_manager.h
    include/socialchoicelab/preference/distance/distance_functions.h
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