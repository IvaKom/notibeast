find_package(Catch2 REQUIRED)

add_subdirectory(../notify/tests ${CMAKE_CURRENT_BINARY_DIR}/notify_tests)
add_subdirectory(../beast/tests ${CMAKE_CURRENT_BINARY_DIR}/beast_tests)
#message(STATUS "beast_tests: ${BEAST_TEST_SRC}")
list(TRANSFORM NOTIFY_SRC PREPEND "../")
list(TRANSFORM BEAST_SRC PREPEND "../")
list(TRANSFORM BEAST_TEST_SRC PREPEND "../")

add_executable(tests
  ${NOTIFY_SRC}
  ${NOTIFY_TEST_SRC}
  ${BEAST_TEST_SRC}
  ${BEAST_SRC}
  helper.cpp
)
target_include_directories(tests PRIVATE
  ${PROJECT_SOURCE_DIR}/test
  ${PROJECT_SOURCE_DIR}/notify
  ${PROJECT_SOURCE_DIR}
)
target_link_libraries(tests PRIVATE
    stdc++fs
    Catch2::Catch2WithMain
    ${CMAKE_THREAD_LIBS_INIT}
    Boost::log Boost::log_setup Boost::json
)

include(CTest)
include(Catch)
catch_discover_tests(tests)
