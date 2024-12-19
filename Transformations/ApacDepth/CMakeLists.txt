add_subdirectory(src)

add_custom_target(testApacDepth
COMMENT "Building DuplicateFunctions"
DEPENDS testApacDepth
COMMENT "Starting tests"
WORKING_DIRECTORY ${TRANSFO_PATH}/ApacDepth/tests
COMMAND bash test.sh
COMMENT "Ending tests"
)
