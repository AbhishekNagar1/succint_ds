cmake_minimum_required(VERSION 3.14)
project(SuccinctRankSelect LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra")

include_directories(include)

set(COMMON_SOURCES
    src/primitives.cpp
    src/bit_vector.cpp
)

add_executable(main_demo src/main_demo.cpp ${COMMON_SOURCES})
add_executable(correctness_tests src/correctness_tests.cpp ${COMMON_SOURCES})
add_executable(benchmark_runner src/benchmark_runner.cpp ${COMMON_SOURCES})
add_executable(text_index_demo src/text_index_demo.cpp ${COMMON_SOURCES})
