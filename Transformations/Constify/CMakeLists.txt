add_subdirectory(src)

add_custom_target(testConstify
COMMENT "Building Constify"
DEPENDS constify
COMMENT "Starting tests"
WORKING_DIRECTORY ${TRANSFO_PATH}/Constify/tests/autotests
COMMAND bash ../test.sh
)
