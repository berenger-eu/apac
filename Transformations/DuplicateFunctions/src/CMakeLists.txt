

SET (CURRENT_LIBRARY_NAME APACDuplicateFunctionsLib)
SET (CURRENT_EXECUTABLE_NAME duplicateFunctions)

include_directories(${LLVM_INCLUDE_DIRS})
message(STATUS "Found LLVM includes ${LLVM_INCLUDE_DIRS}")
link_directories(${LLVM_LIBRARY_DIRS})
message(STATUS "Found LLVM libs ${LLVM_LIBRARY_DIRS}")
add_definitions(${LLVM_DEFINITIONS})
message(STATUS "Found LLVM libs ${LLVM_DEFINITIONS}")

# Now build our tools
set (COMMON_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib)
set(INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include ${COMMON_LIB})

# Sources for your library (replace with actual source files)
set(SOURCES duplicateFunctions.cpp ASTDuplicateVisitor.cpp ASTChangeNameVisitor.cpp )

# Lib Creation
add_library(${CURRENT_LIBRARY_NAME} ${SOURCES})  

# Add include directories to the library
target_include_directories(${CURRENT_LIBRARY_NAME} PUBLIC ${INC_DIR})

# Find the libraries that correspond to the LLVM components
llvm_map_components_to_libnames(llvm_libs x86asmparser bitreader support mc option profiledata)
message(STATUS "LLVM_LIBS: ${llvm_libs}")

# Link the LLVM libraries to the library
target_link_libraries(${CURRENT_LIBRARY_NAME}
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

# Link the additional LLVM component libraries
target_link_libraries(${CURRENT_LIBRARY_NAME} ${llvm_libs})

# Get the C++ compile flags from llvm-config
execute_process(COMMAND bash "-c" "llvm-config --cxxflags" OUTPUT_VARIABLE compile_flags OUTPUT_STRIP_TRAILING_WHITESPACE)
set_property(TARGET ${CURRENT_LIBRARY_NAME} APPEND PROPERTY COMPILE_FLAGS "${compile_flags} -Wno-strict-aliasing")

# Install the library
install(TARGETS ${CURRENT_LIBRARY_NAME} DESTINATION lib)

# Create the executable
add_executable(${CURRENT_EXECUTABLE_NAME} main.cpp)

# Link the executable with the library and the LLVM components
target_link_libraries(${CURRENT_EXECUTABLE_NAME}
    ${CURRENT_LIBRARY_NAME}  # Link the library we created
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

target_link_libraries(${CURRENT_EXECUTABLE_NAME} ${llvm_libs})

set_property(TARGET ${CURRENT_EXECUTABLE_NAME} APPEND PROPERTY COMPILE_FLAGS "${compile_flags} -Wno-strict-aliasing")

install(TARGETS ${CURRENT_EXECUTABLE_NAME} DESTINATION bin)
