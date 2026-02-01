add_subdirectory(src)

add_custom_target(testGotoTransfo 
COMMENT "Building Heapify"
DEPENDS gotoRet
COMMENT "Starting tests"
WORKING_DIRECTORY ${TRANSFO_PATH}/GotoTransfo/tests
COMMAND bash test.sh
COMMENT "Ending tests"
)
