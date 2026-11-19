# Builds upstream's own gtest suite (test/CMakeLists.txt) offline: upstream's
# top-level CMakeLists downloads googletest at configure time unless the
# `gtest` and `gmock_main` targets already exist, so this wrapper adds the
# googletest sources shipped by Debian's `googletest` package first.
cmake_minimum_required(VERSION 3.13)
project(cpu_features_mayhem_tests C CXX)
add_subdirectory(/usr/src/googletest googletest-build EXCLUDE_FROM_ALL)
set(BUILD_TESTING ON CACHE BOOL "Enable test rule" FORCE)
enable_testing()
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. cpu_features)
