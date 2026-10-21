cmake_minimum_required(VERSION 3.14)
project(regex-dfa CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(regex-dfa src/regex-to-dfa.cpp)
target_include_directories(regex-dfa PUBLIC src)
target_compile_options(regex-dfa PRIVATE -Wall -Wextra)

enable_testing()
add_executable(test-regex-dfa tests/test-regex-dfa.cpp)
target_link_libraries(test-regex-dfa PRIVATE regex-dfa)
add_test(NAME test-regex-dfa COMMAND test-regex-dfa)

add_executable(usage examples/usage.cpp)
target_link_libraries(usage PRIVATE regex-dfa)

find_library(HS_LIB hs PATHS /usr/local/lib64 /usr/local/lib REQUIRED)
find_path(HS_INCLUDE hs/hs.h PATHS /usr/local/include REQUIRED)

add_executable(benchmark
    tests/bench/bench-main.cpp
    tests/bench/bench-followpos.cpp
    tests/bench/bench-llama.cpp
    tests/bench/bench-vectorscan.cpp)
target_include_directories(benchmark PRIVATE ${HS_INCLUDE})
target_link_libraries(benchmark PRIVATE regex-dfa ${HS_LIB})