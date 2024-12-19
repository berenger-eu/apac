add_subdirectory(src)

add_custom_target(testDuplicateFunctions
COMMENT "Building DuplicateFunctions"
DEPENDS duplicateFunctions
COMMENT "Starting tests"
WORKING_DIRECTORY ${TRANSFO_PATH}/DuplicateFunctions/tests
COMMAND bash test.sh
COMMENT "Ending tests"
)
