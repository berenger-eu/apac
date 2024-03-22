add_subdirectory(src)

add_custom_target(testConstify
COMMENT "Building Constify"
DEPENDS constify
COMMENT "Starting tests"
WORKING_DIRECTORY ../tests/autotests
COMMAND ./test.sh
)
