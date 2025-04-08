


include_directories(${LLVM_INCLUDE_DIRS})
message(STATUS "Found LLVM includes ${LLVM_INCLUDE_DIRS}")
link_directories(${LLVM_LIBRARY_DIRS})
message(STATUS "Found LLVM libs ${LLVM_LIBRARY_DIRS}")
add_definitions(${LLVM_DEFINITIONS})
message(STATUS "Found LLVM libs ${LLVM_DEFINITIONS}")

# Now build our tools
set (COMMON_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../lib)

set (TRANSFO_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../Transformations)
#Could be done better
set (UNSTACK_INC ${TRANSFO_PATH}/Unstack/include)
set (DECLSPLIT_INC ${TRANSFO_PATH}/DeclarationSplitter/include)
set (TASKGRAPH_INC ${TRANSFO_PATH}/TaskGraph/include)
set (GOTO_INC ${TRANSFO_PATH}/GotoTransfo/include)
set (CONSTIFY_INC ${TRANSFO_PATH}/Constify/include)
set (STACKHEAP_INC ${TRANSFO_PATH}/Heapify/include)
set (MULTIDECL_INC ${TRANSFO_PATH}/MultipleDeclSplitter/include)

set( TRANSFOS_INC ${MULTIDECL_INC} ${UNSTACK_INC} ${DECLSPLIT_INC} ${TASKGRAPH_INC} ${GOTO_INC} ${CONSTIFY_INC} ${STACKHEAP_INC})

set(INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include ${COMMON_LIB} ${TRANSFOS_INC})
message(STATUS "Include directories: ${INC_DIR}")
set(SOURCES apac.cpp )
add_executable(apac)
 
target_sources(apac PRIVATE ${SOURCES})
target_include_directories(apac PRIVATE ${INC_DIR})


# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs x86asmparser bitreader support mc option profiledata)
message(STATUS "LLVM_LIBS: ${llvm_libs}")

target_link_libraries(apac
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

target_link_libraries(apac ${llvm_libs})

execute_process(COMMAND bash "-c" "llvm-config --cxxflags" OUTPUT_VARIABLE compile_flags OUTPUT_STRIP_TRAILING_WHITESPACE)
set_property(TARGET apac APPEND PROPERTY COMPILE_FLAGS "${compile_flags} -Wno-strict-aliasing")

install(TARGETS apac DESTINATION bin)
