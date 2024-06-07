project(APAC CXX)
cmake_minimum_required(VERSION 3.0)
enable_language(C)
# Allow the user to override the install prefix.
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++11 support
# for your compiler.
SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
add_compile_options(-std=c++17 -Wall)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_subdirectory(lib)
add_subdirectory(Transformations)

set(EXECUTABLES stackheap constify gotoRet unstack declarationSplitter)

add_custom_target(test 
COMMENT "Building project"
DEPENDS ${EXECUTABLES}
COMMENT "Starting tests"
WORKING_DIRECTORY ..
COMMAND ./test.sh
COMMENT "Ending tests"
)
