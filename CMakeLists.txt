# in order to compile for synology diskstation, use
# cmake -G Ninja .. -DCMAKE_TOOLCHAIN_FILE=../toolchain-synology.cmake -DTESTS=FALSE
cmake_minimum_required(VERSION 3.12)

# set the project name
project(notibeast)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# test that filesystem header actually is there and works
try_compile(HAS_FS "${CMAKE_BINARY_DIR}/temp"
            "${CMAKE_SOURCE_DIR}/test/has_filesystem.cpp"
            CMAKE_FLAGS -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON
            LINK_LIBRARIES stdc++fs)
if(HAS_FS)
  message(STATUS "Compiler has filesystem support")
else()
  message(FATAL_ERROR "Compiler is missing filesystem capabilities.")
endif(HAS_FS)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_subdirectory(notify)
add_subdirectory(glue)
add_subdirectory(beast)

# add the executable
add_executable(notibeast
    main.cpp
    logging.cpp
    notify_event_funcs.cpp
    cl_parser.cpp
    recursive_i_notify_factory.cpp
    ${NOTIFY_SRC}
    ${GLUE_SRC}
    ${BEAST_SRC}
)

# you may want to specify it with 'cmake .. -DBOOST_ROOT=YOUR_PATH_TO_PROPER_VERSION'
if( BOOST_ROOT )
    message(STATUS "BOOST_ROOT set to ${BOOST_ROOT}")
endif()

find_package(Boost COMPONENTS log log_setup program_options json                                                                                                                                REQUIRED)
message(STATUS "Boost version: ${Boost_VERSION}")

find_package (Threads REQUIRED)
message(STATUS "Threads library: ${CMAKE_THREAD_LIBS_INIT}")

target_include_directories(notibeast PRIVATE
  ${Boost_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}
)

target_link_libraries (notibeast stdc++fs ${CMAKE_THREAD_LIBS_INIT} Boost::log Boost::log_setup Boost::program_options)

set(TESTS TRUE CACHE BOOL "Build tests")

if(TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

