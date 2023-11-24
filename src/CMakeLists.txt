


include_directories(${LLVM_INCLUDE_DIRS})
message(STATUS "Found LLVM includes ${LLVM_INCLUDE_DIRS}")
link_directories(${LLVM_LIBRARY_DIRS})
message(STATUS "Found LLVM libs ${LLVM_LIBRARY_DIRS}")
add_definitions(${LLVM_DEFINITIONS})
message(STATUS "Found LLVM libs ${LLVM_DEFINITIONS}")

# Now build our tools
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
add_executable(apac ASTInitVisitor.cpp ASTConstifyVisitor.cpp ASTPrintVisitor.cpp core.cpp rewritersample.cpp)




# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs x86asmparser bitreader support mc option profiledata)
message(STATUS "LLVM_LIBS: ${llvm_libs}")

target_link_libraries(apac
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

target_link_libraries(apac ${llvm_libs})

execute_process(COMMAND bash "-c" "llvm-config --cxxflags" OUTPUT_VARIABLE compile_flags OUTPUT_STRIP_TRAILING_WHITESPACE)
set_property(TARGET apac APPEND PROPERTY COMPILE_FLAGS "${compile_flags} -Wno-strict-aliasing")

install(TARGETS apac DESTINATION bin)
