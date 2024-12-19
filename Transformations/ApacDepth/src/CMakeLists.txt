


include_directories(${LLVM_INCLUDE_DIRS})
message(STATUS "Found LLVM includes ${LLVM_INCLUDE_DIRS}")
link_directories(${LLVM_LIBRARY_DIRS})
message(STATUS "Found LLVM libs ${LLVM_LIBRARY_DIRS}")
add_definitions(${LLVM_DEFINITIONS})
message(STATUS "Found LLVM libs ${LLVM_DEFINITIONS}")

# Now build our tools
set (COMMON_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib)
set(INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include ${COMMON_LIB})
set(SOURCES apacDepth.cpp codeModification.cpp )
add_executable(apacDepth)
 
target_sources(apacDepth PRIVATE ${SOURCES})
target_include_directories(apacDepth PRIVATE ${INC_DIR})


# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs x86asmparser bitreader support mc option profiledata)
message(STATUS "LLVM_LIBS: ${llvm_libs}")

target_link_libraries(apacDepth
    clangAST 
    clangASTMatchers
    clangBasic
    clangFrontend
    clangSerialization
    clangTooling 
    commonLib
    clangDriver
    clangParse
    clangSema
    clangAnalysis
    clangRewrite
    clangEdit

    clangAST
    clangLex

    #clangFrontendTool 
    #clangRewriteFrontend
    #clangStaticAnalyzerFrontend 
    #clangStaticAnalyzerCheckers 
    #clangStaticAnalyzerCore 

    #clangIndex 
    #clangToolingCore 
    #clangFormat

    LLVMFrontendOpenMP
    clangAPINotes
    clangSupport
    LLVMWindowsManifest
    LLVMWindowsDriver
  )

target_link_libraries(apacDepth ${llvm_libs})

execute_process(COMMAND bash "-c" "llvm-config --cxxflags" OUTPUT_VARIABLE compile_flags OUTPUT_STRIP_TRAILING_WHITESPACE)
set_property(TARGET apacDepth APPEND PROPERTY COMPILE_FLAGS "${compile_flags} -Wno-strict-aliasing")

install(TARGETS apacDepth DESTINATION bin)
