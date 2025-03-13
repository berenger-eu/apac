add_subdirectory(src)

add_custom_target(testMultipleDeclSplitter
COMMENT "Building DuplicateFunctions"
DEPENDS testMultipleDeclSplitter
COMMENT "Starting tests"
WORKING_DIRECTORY ${TRANSFO_PATH}/MultipleDeclSplitter/tests
COMMAND bash test.sh
COMMENT "Ending tests"
)
