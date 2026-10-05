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