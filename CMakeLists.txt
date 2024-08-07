cmake_minimum_required(VERSION 3.25)
project(learncompiler)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(contrib)


add_executable(chibicc "${PROJECT_SOURCE_DIR}/src/main.cc")
target_link_libraries(chibicc fmtlib)

set(learncompiler_test_dir "${CMAKE_SOURCE_DIR}/test")

add_custom_target(test
  ${learncompiler_test_dir}/test.sh)
