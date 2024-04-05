


include_directories(${LLVM_INCLUDE_DIRS})
message(STATUS "Found LLVM includes ${LLVM_INCLUDE_DIRS}")
link_directories(${LLVM_LIBRARY_DIRS})
message(STATUS "Found LLVM libs ${LLVM_LIBRARY_DIRS}")
add_definitions(${LLVM_DEFINITIONS})
message(STATUS "Found LLVM libs ${LLVM_DEFINITIONS}")

# Now build our tools

set(INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)
set(SOURCES unstack.cpp ASTUnstackVisitor.cpp)
add_executable(unstack)
 
target_sources(unstack PRIVATE ${SOURCES})
target_include_directories(unstack PRIVATE ${INC_DIR})


# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs x86asmparser bitreader support mc option profiledata)
message(STATUS "LLVM_LIBS: ${llvm_libs}")

target_link_libraries(unstack
    clangFrontend
    clangSerialization
    clangDriver
    clangTooling
    clangParse
    clangSema
    clangAnalysis
    clangRewriteFrontend
    clangRewrite
    clangEdit
    clangAST
    clangLex
    clangBasic
    clangASTMatchers
    clangAST 
    clangFrontendTool 
    clangStaticAnalyzerFrontend 
    clangStaticAnalyzerCheckers 
    clangStaticAnalyzerCore 
    clangCrossTU 
    clangIndex 
    clangToolingCore 
    clangTooling 
    clangFormat
    LLVMFrontendOpenMP
  )

target_link_libraries(unstack ${llvm_libs})

execute_process(COMMAND bash "-c" "llvm-config --cxxflags" OUTPUT_VARIABLE compile_flags OUTPUT_STRIP_TRAILING_WHITESPACE)
set_property(TARGET unstack APPEND PROPERTY COMPILE_FLAGS "${compile_flags} -Wno-strict-aliasing")

install(TARGETS unstack DESTINATION bin)
