add_subdirectory(src)

add_custom_target(testMainParallel
COMMENT "Building MainParallel"
DEPENDS mainParallel
COMMENT "Starting tests"
WORKING_DIRECTORY ${TRANSFO_PATH}/MainParallel/tests
COMMAND ./test.sh
COMMENT "Ending tests"
)
