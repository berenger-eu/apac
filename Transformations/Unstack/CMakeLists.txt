add_subdirectory(src)

add_custom_target(testUnstack 
COMMENT "Building Unstack"
DEPENDS unstack
COMMENT "Starting tests"
WORKING_DIRECTORY ${TRANSFO_PATH}/Unstack/tests
COMMAND bash test.sh
COMMENT "Ending tests"
)
