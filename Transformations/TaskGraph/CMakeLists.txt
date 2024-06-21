add_subdirectory(src)

add_custom_target(testTaskGraph
COMMENT "Building TaskGraph"
DEPENDS taskGraph
COMMENT "Starting tests"
WORKING_DIRECTORY ../test
COMMAND ./test.sh
COMMENT "Ending tests"
)
