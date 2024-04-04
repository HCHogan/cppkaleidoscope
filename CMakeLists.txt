cmake_minimum_required(VERSION 3.28.0 FATAL_ERROR)

project(cppkaleidoscope LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set up std modules
include(std.cmake)

# set up llvm
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})
llvm_map_components_to_libnames(llvm_libs passes support core nativecodegen OrcJIT native)

add_library(cppkaleidoscope
  STATIC
  src/parser.cpp
  src/codegen.cpp
  src/ast.cpp
  src/KaleidoscopeJIT.cpp
  src/libraryfunctions.cpp
)

target_include_directories(cppkaleidoscope
  PUBLIC
  include
)

add_executable(cppkaleidoscope-repl
  apps/repl.cpp
)

add_executable(kc
  apps/kc.cpp
)


target_link_libraries(cppkaleidoscope PUBLIC ${llvm_libs})

target_link_libraries(cppkaleidoscope-repl PUBLIC cppkaleidoscope)
target_link_libraries(kc PUBLIC cppkaleidoscope)
