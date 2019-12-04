cmake_minimum_required(VERSION 2.8)
project(eLoop)

option(BUILD_TESTS_FLAG "TESTS" OFF)

set(TEST_BUILD_DIR ${BUILD_DIR}/tests)

set(TESTS
    "dummy.c"
    "list.c"
    "olist_test.c"
    "scheduler.c"
    "fd.c"
    "syncer.c"
    "tasks_queue.c"
    "eloop.c"
)

set(LIBS
    ${ELOOP_LIB}
    rt
)

include_directories(${ROOT_DIR}/src)
link_directories(${BUILD_DIR})

foreach(TEST_SRC ${TESTS}) 
    string(REGEX REPLACE "^(.+).c$" "\\1_test" TEST_BIN ${TEST_SRC}) 
    add_executable(${TEST_BIN} ${TEST_SRC})
    set_target_properties(${TEST_BIN}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TEST_BUILD_DIR}
    )
    target_link_libraries(${TEST_BIN} ${LIBS})
endforeach(TEST_SRC)

