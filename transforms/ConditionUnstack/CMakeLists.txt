add_subdirectory(src)

add_custom_target(testConditionUnstack 
COMMENT "Building Heapify"
DEPENDS conditionUnstack
COMMENT "Starting tests"
WORKING_DIRECTORY ${TRANSFO_PATH}/ConditionUnstack/tests
COMMAND bash test.sh
COMMENT "Ending tests"
)
