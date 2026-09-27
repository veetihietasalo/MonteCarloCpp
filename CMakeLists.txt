cmake_minimum_required(VERSION 3.20)
project(MonteCarloEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(include)

# Options: make CUDA optional and enable tests
option(BUILD_CUDA "Enable CUDA support for Monte Carlo" OFF)
option(BUILD_TESTS "Build test executables" ON)

# Main executable sources
set(MC_SOURCES
    src/main.cpp
    src/Random.cpp
    src/MonteCarlo.cpp
)

if (BUILD_CUDA)
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    list(APPEND MC_SOURCES src/MonteCarlo.cu)
endif()

add_executable(MonteCarloEngine ${MC_SOURCES})

add_library(MonteCarloAPI SHARED
    src/api.cpp
    src/Random.cpp
    src/MonteCarlo.cpp
)
target_compile_definitions(MonteCarloAPI PRIVATE MONTECARLO_API_EXPORTS)
target_include_directories(MonteCarloAPI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Testing (optional for now, but good to have ready)
enable_testing()

# Optional test executable: barrier test
if (BUILD_TESTS)
    add_executable(test_barrier src/test_barrier.cpp src/Random.cpp)
    add_test(NAME barrier_test COMMAND test_barrier)

    add_executable(test_payoffs src/test_payoffs.cpp src/Random.cpp)
    add_test(NAME payoffs_test COMMAND test_payoffs)
endif()

# Provide a helpful message about CUDA
if (BUILD_CUDA)
    message(STATUS "Building with CUDA support (MonteCarlo.cu included)")
else()
    message(STATUS "Building without CUDA support; MonteCarlo.cu excluded")
endif()
