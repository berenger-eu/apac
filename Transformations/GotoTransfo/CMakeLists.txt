add_subdirectory(src)

add_custom_target(testGoto 
COMMENT "Building Heapify"
DEPENDS gotoRet
COMMENT "Starting tests"
WORKING_DIRECTORY ../test
COMMAND ./test.sh
COMMENT "Ending tests"
)
