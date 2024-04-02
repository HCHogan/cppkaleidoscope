cmake_minimum_required(VERSION 3.28.0 FATAL_ERROR)

project(cpplox LANGUAGES CXX C)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

include(std.cmake)

add_library(cpplox
  STATIC
  src/parser.cpp
  src/codegen.cpp
  src/ast.cpp
  src/KaleidoscopeJIT.cpp
)

target_include_directories(cpplox
  PUBLIC
  include
)

add_executable(cpplox-repl
  apps/repl.cpp
)

llvm_map_components_to_libnames(llvm_libs support core irreader)

target_link_libraries(cpplox PUBLIC ${llvm_libs})
target_link_libraries(cpplox-repl PUBLIC cpplox)
