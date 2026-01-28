


include_directories(${LLVM_INCLUDE_DIRS})
message(STATUS "Found LLVM includes ${LLVM_INCLUDE_DIRS}")
link_directories(${LLVM_LIBRARY_DIRS})
message(STATUS "Found LLVM libs ${LLVM_LIBRARY_DIRS}")
add_definitions(${LLVM_DEFINITIONS})
message(STATUS "Found LLVM libs ${LLVM_DEFINITIONS}")

# Now build our tools
set (COMMON_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../../core) 
set(COMMON_LIB_SRC common.cpp)
add_library(commonLib ${COMMON_LIB_SRC})

