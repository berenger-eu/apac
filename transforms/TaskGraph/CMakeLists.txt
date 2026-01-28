add_subdirectory(src)

add_custom_target(testTaskGraph
COMMENT "Building TaskGraph"
DEPENDS taskGraph
COMMENT "Starting tests"
WORKING_DIRECTORY ${TRANSFO_PATH}/TaskGraph/tests
COMMAND ./test.sh
COMMENT "Ending tests"
)
