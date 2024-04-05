add_subdirectory(src)

add_custom_target(testUnstack 
COMMENT "Building Unstack"
DEPENDS unstack
COMMENT "Starting tests"
WORKING_DIRECTORY ../test
COMMAND ./test.sh
COMMENT "Ending tests"
)
