add_subdirectory(src)

add_custom_target(testDeclarationSplitter
COMMENT "Building DeclarationSplitter"
DEPENDS declarationSplitter
COMMENT "Starting tests"
WORKING_DIRECTORY ${TRANSFO_PATH}/DeclarationSplitter/tests
COMMAND bash test.sh
COMMENT "Ending tests"
)
