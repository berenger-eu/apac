


include_directories(${LLVM_INCLUDE_DIRS})
message(STATUS "Found LLVM includes ${LLVM_INCLUDE_DIRS}")
link_directories(${LLVM_LIBRARY_DIRS})
message(STATUS "Found LLVM libs ${LLVM_LIBRARY_DIRS}")
add_definitions(${LLVM_DEFINITIONS})
message(STATUS "Found LLVM libs ${LLVM_DEFINITIONS}")

# Now build our tools
set (COMMON_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib)
set(INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include ${COMMON_LIB})

set(SOURCES ASTInitVisitor.cpp ASTConstifyVisitor.cpp ASTPrintVisitor.cpp SymTab.cpp core.cpp rewritersample.cpp)
add_executable(constify)
 
target_sources(constify PRIVATE ${SOURCES})
target_include_directories(constify PRIVATE ${INC_DIR})


# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs x86asmparser bitreader support mc option profiledata)
message(STATUS "LLVM_LIBS: ${llvm_libs}")

target_link_libraries(constify
    commonLib
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

target_link_libraries(constify ${llvm_libs})

execute_process(COMMAND bash "-c" "llvm-config --cxxflags" OUTPUT_VARIABLE compile_flags OUTPUT_STRIP_TRAILING_WHITESPACE)
set_property(TARGET constify APPEND PROPERTY COMPILE_FLAGS "${compile_flags} -Wno-strict-aliasing")

install(TARGETS constify DESTINATION bin)
