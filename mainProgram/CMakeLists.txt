set(APAC_PATH ../../../mainProgram)

add_subdirectory(src)

add_custom_target(testApac 
COMMENT "Building Apac"
DEPENDS apac
COMMENT "Starting tests"
WORKING_DIRECTORY ${APAC_PATH}/tests
COMMAND bash test.sh
COMMENT "Ending tests"
)
