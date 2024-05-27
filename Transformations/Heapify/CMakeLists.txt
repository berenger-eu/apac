add_subdirectory(src)

add_custom_target(testHeapify 
COMMENT "Building Heapify"
DEPENDS stackheap
COMMENT "Starting tests"
WORKING_DIRECTORY ${TRANSFO_PATH}/Heapify/tests
COMMAND bash test.sh
COMMENT "Ending tests"
)
