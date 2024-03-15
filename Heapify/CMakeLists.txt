add_subdirectory(src)

add_custom_target(testHeapify 
COMMENT "Building Heapify"
DEPENDS stackheap
COMMENT "Starting tests"
WORKING_DIRECTORY ../test
COMMAND ./test.sh
COMMENT "Ending tests"
)
